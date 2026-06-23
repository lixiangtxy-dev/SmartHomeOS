#include "app_tasks.h"
#include "task.h"
#include "mempool.h"
#include "device.h"        
#include "msgqueue.h"
#include "shadow.h"
#include "timer.h"         
#include "drv_at.h"
#include "sys_config.h" // 引入配置解耦
#include "drv_mqtt.h"   // 引入手搓 MQTT 驱动
#include <stdio.h> 
#include <string.h>

// 静态定义任务控制块，不污染全局命名空间
static task_t task_sensor;  // 对应原 Task 1 (温度) & Task 2 (光照可以合并，这里保持原样)
static task_t task_light;   // 对应原 Task 2
static task_t task_alarm;   // 对应原 Task 3
static task_t task_oled;    // 对应原 Task 4
static task_t task_net;     // 对应原 Task 5

static os_mq_t temp_msg_queue;
static float temp_queue_buffer[5];
static os_timer_t led_auto_off_timer;
// 顶部定义一个静态接收缓冲区
static char mqtt_rx_buf[256];
static int mqtt_rx_idx = 0;
// ==========================================
// 定时器回调
// ==========================================
static void led_auto_off_callback(void *parameter) {
    os_device_t *led_dev = (os_device_t *)parameter;
    uint8_t cmd_off = 0;
    if (led_dev != NULL) {
        os_device_write(led_dev, &cmd_off, sizeof(uint8_t)); 
        os_shadow_update_led(0);                             
        printf(">>> [定时器异步事件] 3秒时间到！已自动关闭临时警报灯。 <<<\r\n");
    }
}
// ==========================================
// 任务 1：温度采集
// ==========================================
static void task1_entry(void) {
    os_device_t *temp_dev = os_device_open("sensor_temp");
    float current_temp = 0.0f;
    while(1) {
        if (temp_dev != NULL) {
            os_device_read(temp_dev, &current_temp, sizeof(float));
            os_shadow_update_temp(current_temp); 
            int temp_int = (int)current_temp; 
            int temp_frac = (int)((current_temp - temp_int) * 100); 
            if (temp_frac < 0) temp_frac = -temp_frac;
            printf("[Task 1] 物理温度更新至影子: %d.%02d ℃\r\n", temp_int, temp_frac);
        }
        os_delay(500); 
    }
}

// ==========================================
// 任务 2：光照采集
// ==========================================
static void task2_entry(void) {
    os_device_t *light_dev = os_device_open("sensor_light");
    uint32_t voltage_mv = 0;
    while(1) {
        if (light_dev != NULL) {
            os_device_read(light_dev, &voltage_mv, sizeof(uint32_t));
        }
        os_delay(500); 
    }
}

// ==========================================
// 任务 3：报警联动控制
// ==========================================
static void task3_entry(void) {
    os_device_t *led_dev = os_device_open("alarm_led");
    smarthome_shadow_t local_shadow; 
    uint8_t alarm_lock = 0; 
    
    // 初始化一次性定时器
    os_timer_init(&led_auto_off_timer, "led_auto_off", led_auto_off_callback, (void*)led_dev, 3000, OS_TIMER_FLAG_ONE_SHOT);

    while(1) {
        os_shadow_get_snapshot(&local_shadow);
        float current_temp = local_shadow.room_temp;

        if (current_temp > 28.0f && alarm_lock == 0) {
            uint8_t cmd_on = 1;
            if (led_dev) os_device_write(led_dev, &cmd_on, sizeof(uint8_t)); 
            os_shadow_update_led(1);
            printf(">>> [Task 3 决策] 检测到超温！立刻开启警报，3秒后自动静音... <<<\r\n");
            os_timer_start(&led_auto_off_timer);
            alarm_lock = 1;
        }
        else if (current_temp <= 28.0f && alarm_lock == 1) {
            if (local_shadow.alarm_led_state == 1) {
                uint8_t cmd_off = 0;
                if (led_dev) os_device_write(led_dev, &cmd_off, sizeof(uint8_t)); 
                os_shadow_update_led(0);
                os_timer_stop(&led_auto_off_timer); 
            }
            alarm_lock = 0;
            printf(">>> [Task 3 决策] 温度已彻底恢复正常，警报系统重置待命。 <<<\r\n");
        }
        os_delay(200); 
    }
}

// ==========================================
// 任务 4：OLED 仪表盘刷新
// ==========================================
static void task4_entry(void) {
    os_device_t *oled_dev = os_device_open("display_oled");
    os_device_t *light_dev = os_device_open("sensor_light"); 
    smarthome_shadow_t local_shadow; 
    char display_buf[128]; 

    while(1) {
        if (oled_dev != NULL) {
            os_shadow_get_snapshot(&local_shadow);
            int temp_int = (int)local_shadow.room_temp;
            int temp_frac = (int)((local_shadow.room_temp - temp_int) * 10); 
            if(temp_frac < 0) temp_frac = -temp_frac;
            
            uint32_t light_mv = 0;
            if (light_dev) os_device_read(light_dev, &light_mv, sizeof(uint32_t));
            
            sprintf(display_buf, 
                    "--- SmartHome ---\n"
                    "Temp : %d.%d C\n"
                    "Light: %d mV\n"
                    "Alarm: %s\n", 
                    temp_int, temp_frac, light_mv,
                    local_shadow.alarm_led_state ? "ON !!!" : "OFF");
            
            os_device_write(oled_dev, display_buf, sizeof(display_buf));
        }
        os_delay(500); 
    }
}

// ==========================================
// 任务 5：网络与 MQTT 上云 (配置解耦 + 温光双路数据)
// ==========================================
static void task_network_entry(void) {
    char cmd_buf[128];
    smarthome_shadow_t local_shadow;
    uint32_t heartbeat_cnt = 0;
    os_device_t *light_dev = os_device_open("sensor_light");
    os_device_t *led_dev   = os_device_open("alarm_led");
    uint32_t light_mv = 0;
    uint32_t loop_cnt = 0;
    
    printf("\r\n>>> 网络与云端服务启动 <<<\r\n");
    
    at_send_cmd_wait("AT+RST\r\n", "OK", 2000);
    os_delay(3500); 
    at_send_cmd_wait("AT\r\n", "OK", 1000);
    at_send_cmd_wait("AT+CWMODE=1\r\n", "OK", 1000);

    // 读取 Flash 中的 JSON 配置进行联网
    printf("[网络] 准备连接 Wi-Fi: %s\r\n", g_sys_config.wifi_ssid);
    sprintf(cmd_buf, "AT+CWJAP=\"%s\",\"%s\"\r\n", g_sys_config.wifi_ssid, g_sys_config.wifi_pwd);
    
    if (at_send_cmd_wait(cmd_buf, "WIFI GOT IP", 15000) != 0) {
        printf("[网络] ? Wi-Fi 连接失败挂起。\r\n");
        while(1) os_delay(1000); 
    }
    printf("[网络] ? Wi-Fi 已就绪！\r\n");
    os_delay(2000);

    at_send_cmd_wait("AT+CIPMUX=0\r\n", "OK", 1000);

    // 连接动态配置的 MQTT 服务器
    printf("[网络] 正在连接与 %s 的 TCP 通道...\r\n", g_sys_config.mqtt_server);
    sprintf(cmd_buf, "AT+CIPSTART=\"TCP\",\"%s\",1883\r\n", g_sys_config.mqtt_server);
    if (at_send_cmd_wait(cmd_buf, "CONNECT", 10000) != 0) {
        printf("[网络] ? TCP 连接建立失败！\r\n");
        while(1) os_delay(1000);
    }
    os_delay(500);

    // 一键登录 API
    if (drv_mqtt_connect("CH32V307_Device_001") != 0) {
        while(1) os_delay(1000);
    }
    os_delay(1000);

    // 【新增】2. 订阅控制下发主题
    drv_mqtt_subscribe("smarthome/cmd");
    os_delay(500);

    // 清空接收缓冲区准备就绪
    mqtt_rx_idx = 0;
    memset(mqtt_rx_buf, 0, sizeof(mqtt_rx_buf));
while(1) {
        // --- A. 极速轮询接收云端数据 (+IPD) ---
        char c;
        while(at_get_char(&c)) {
            // 【核心修复】：把 0x00 替换成空格，防止 C 语言字符串被截断！
            if (c == '\0') {
                c = ' '; 
            }
            
            if (mqtt_rx_idx < 255) {
                mqtt_rx_buf[mqtt_rx_idx++] = c;
                mqtt_rx_buf[mqtt_rx_idx] = '\0';
            }
        }

        // --- B. 简易命令拦截器 (暴力匹配 JSON) ---
        // 电脑端发送 {"led":1} 时，截获命令并开灯
        if (strstr(mqtt_rx_buf, "\"led\":1") != NULL) {
            printf("\r\n[云端下发] ? 收到强制【开灯】指令！\r\n");
            uint8_t cmd_on = 1;
            if(led_dev) os_device_write(led_dev, &cmd_on, 1);
            os_shadow_update_led(1);
            // 处理完毕，清空缓存
            mqtt_rx_idx = 0; memset(mqtt_rx_buf, 0, sizeof(mqtt_rx_buf));
        }
        // 电脑端发送 {"led":0} 时，截获命令并关灯
        else if (strstr(mqtt_rx_buf, "\"led\":0") != NULL) {
            printf("\r\n[云端下发] ? 收到强制【关灯】指令！\r\n");
            uint8_t cmd_off = 0;
            if(led_dev) os_device_write(led_dev, &cmd_off, 1);
            os_shadow_update_led(0);
            mqtt_rx_idx = 0; memset(mqtt_rx_buf, 0, sizeof(mqtt_rx_buf));
        }
        // 防止长期的无用数据撑爆缓冲区
        else if (mqtt_rx_idx > 200) {
            mqtt_rx_idx = 0; memset(mqtt_rx_buf, 0, sizeof(mqtt_rx_buf));
        }

        // --- C. 维持异步上报逻辑 ---
        loop_cnt++;
        if (loop_cnt >= 500) { // 500 * 10ms = 5秒
            os_shadow_get_snapshot(&local_shadow);
            int temp_int = (int)local_shadow.room_temp;
            int temp_frac = (int)((local_shadow.room_temp - temp_int) * 10); 
            if(temp_frac < 0) temp_frac = -temp_frac;
            if (light_dev) os_device_read(light_dev, &light_mv, sizeof(uint32_t));

            char json_payload[128];
            sprintf(json_payload, "{\"temperature\":%d.%d, \"alarm\":%d, \"light\":%u}", 
                    temp_int, temp_frac, local_shadow.alarm_led_state, light_mv);
            
            drv_mqtt_publish("smarthome/telemetry", json_payload);
            
            heartbeat_cnt++;
            if (heartbeat_cnt >= 6) { 
                drv_mqtt_ping();
                heartbeat_cnt = 0;
            }
            loop_cnt = 0; // 重新计时
        }

        // 微睡眠交出 CPU 控制权 (10ms 即可，保证串口数据不漏接)
        os_delay(10); 
    }
}



// ==========================================
// 统一注册接口：供 main.c 调用
// ==========================================
void app_tasks_init(void) {
    // 初始化消息队列
    os_mq_init(&temp_msg_queue, temp_queue_buffer, sizeof(float), 5);

    // 分配栈并初始化任务
    uint32_t *sp1 = (uint32_t*)stack_pool_alloc();
    task_init(&task_sensor, task1_entry, sp1, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *sp2 = (uint32_t*)stack_pool_alloc();
    task_init(&task_light, task2_entry, sp2, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *sp3 = (uint32_t*)stack_pool_alloc();
    task_init(&task_alarm, task3_entry, sp3, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *sp4 = (uint32_t*)stack_pool_alloc();
    task_init(&task_oled, task4_entry, sp4, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *sp5 = (uint32_t*)stack_pool_alloc();
    task_init(&task_net, task_network_entry, sp5, TASK_STACK_SIZE_BYTES / 4);
    
    // 注册任务至内核
    task_register(&task_sensor);
    task_register(&task_light);
    task_register(&task_alarm);
    task_register(&task_oled);
    task_register(&task_net);

    printf("应用层任务组创建并注册完毕。\r\n");
}