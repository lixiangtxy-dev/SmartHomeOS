#include "debug.h"
#include "task.h"
#include "mempool.h"
#include "device.h"        
#include "drv_smarthome.h" 
#include "msgqueue.h"
#include "shadow.h"
#include "timer.h"         
#include "drv_w25q80.h"
#include "fs_port.h"
#include <stdio.h> 

task_t task1;
task_t task2;
task_t task3;
task_t task4; 

extern void drv_oled_register(void);

os_mq_t temp_msg_queue;
float temp_queue_buffer[5];
os_timer_t led_auto_off_timer;

void led_auto_off_callback(void *parameter) {
    os_device_t *led_dev = (os_device_t *)parameter;
    uint8_t cmd_off = 0;
    if (led_dev != NULL) {
        os_device_write(led_dev, &cmd_off, sizeof(uint8_t)); 
        os_shadow_update_led(0);                             
        printf(">>> [定时器异步事件] 3秒时间到！已自动关闭临时警报灯。 <<<\r\n");
    }
}

// --- 任务 1：温度采集 ---
void task1_entry(void) {
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

// --- 任务 2：光照采集 ---
void task2_entry(void) {
    os_device_t *light_dev = os_device_open("sensor_light");
    uint32_t voltage_mv = 0;
    while(1) {
        if (light_dev != NULL) {
            os_device_read(light_dev, &voltage_mv, sizeof(uint32_t));
        }
        os_delay(500); 
    }
}

// --- 任务 3：报警联动控制 ---
void task3_entry(void) {
    os_device_t *led_dev = os_device_open("alarm_led");
    smarthome_shadow_t local_shadow; 
    uint8_t alarm_lock = 0; 
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

// --- 任务 4：OLED 仪表盘刷新任务 (UI渲染中枢) ---
void task4_entry(void) {
    os_device_t *oled_dev = os_device_open("display_oled");
    os_device_t *light_dev = os_device_open("sensor_light"); // 获取光照设备句柄
    smarthome_shadow_t local_shadow; 
    char display_buf[128]; 

    while(1) {
        if (oled_dev != NULL) {
            // 1. 获取影子数据 (温度、警报状态)
            os_shadow_get_snapshot(&local_shadow);
            int temp_int = (int)local_shadow.room_temp;
            int temp_frac = (int)((local_shadow.room_temp - temp_int) * 10); 
            if(temp_frac < 0) temp_frac = -temp_frac;
            
            // 2. 获取光敏电压
            uint32_t light_mv = 0;
            if (light_dev) {
                os_device_read(light_dev, &light_mv, sizeof(uint32_t));
            }
            
            // 3. 组装成 4 行字符串 (通过 \n 换行)
            sprintf(display_buf, 
                    "--- SmartHome ---\n"
                    "Temp : %d.%d C\n"
                    "Light: %d mV\n"
                    "Alarm: %s\n", 
                    temp_int, temp_frac,
                    light_mv,
                    local_shadow.alarm_led_state ? "ON !!!" : "OFF");
            
            // 4. 将字符串写入设备，底层会自动解析 \n 进行无缝刷新
            os_device_write(oled_dev, display_buf, sizeof(display_buf));
        }
        
        os_delay(500); 
    }
}

// 测试函数：开机计数器
void test_littlefs_boot_counter(void) {
    lfs_file_t file;
    uint32_t boot_count = 0;
    
    printf("\r\n--- 文件系统持久化测试开始 ---\r\n");

    // 打开文件，不存在则创建
    int err = lfs_file_open(&lfs, &file, "boot_count.txt", LFS_O_RDWR | LFS_O_CREAT);
    if (err < 0) {
        printf("[FS 测试] 打开文件失败，错误码：%d\r\n", err);
        return;
    }

    // 读取旧值并加 1
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));
    boot_count += 1; 
    
    // 指针移回开头并覆盖写入
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));
    
    // 关闭文件真正刷入 Flash
    lfs_file_close(&lfs, &file);
    
    printf("================================\r\n");
    printf(" ? SmartHomeOS 历史启动总次数: %d \r\n", boot_count);
    printf("================================\r\n\r\n");
}

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init(); 
    USART_Printf_Init(115200);   
    printf("\r\n================================\r\n");
    printf("     SmartHomeOS 启动中...      \r\n");
    printf("================================\r\n");

    uint16_t flash_id = drv_w25qxx_init();
    printf("探测到 SPI Flash ID: 0x%04X\r\n", flash_id);
    os_shadow_init();
    drv_smarthome_init();
    drv_oled_register();
    printf("硬件驱动与设备节点挂载完成。\r\n");
    
    drv_fs_init();
    test_littlefs_boot_counter();

    os_tick_hardware_init();
    stack_pool_init();
    os_mq_init(&temp_msg_queue, temp_queue_buffer, sizeof(float), 5);

    uint32_t *sp1 = (uint32_t*)stack_pool_alloc();
    task_init(&task1, task1_entry, sp1, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *sp2 = (uint32_t*)stack_pool_alloc();
    task_init(&task2, task2_entry, sp2, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *sp3 = (uint32_t*)stack_pool_alloc();
    task_init(&task3, task3_entry, sp3, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *sp4 = (uint32_t*)stack_pool_alloc();
    task_init(&task4, task4_entry, sp4, TASK_STACK_SIZE_BYTES / 4);
    
    task_register(&task1);
    task_register(&task2);
    task_register(&task3);
    task_register(&task4);

    printf("系统任务创建完毕，接管 CPU 控制权...\r\n");
    os_start();
    
    while(1) {}
}