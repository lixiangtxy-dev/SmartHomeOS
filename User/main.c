#include "debug.h"
#include "task.h"
#include "mempool.h"
#include "device.h"        // 引入 OS 设备管理中枢
#include "drv_smarthome.h" // 引入底层驱动初始化入口
#include "msgqueue.h"
#include "shadow.h"
#include "timer.h"         // 引入软件定时器机制

// 任务控制块
task_t task1;
task_t task2;
task_t task3;

// 声明消息队列（留作后续扩展使用）
os_mq_t temp_msg_queue;
float temp_queue_buffer[5];

// ==========================================================
// 【核心新增】：软件定时器控制块与回调函数
// ==========================================================
os_timer_t led_auto_off_timer;

// 定时器到期后的回调函数（由 TIM2 硬件中断在后台自动调用）
void led_auto_off_callback(void *parameter) {
    os_device_t *led_dev = (os_device_t *)parameter;
    uint8_t cmd_off = 0;
    
    if (led_dev != NULL) {
        os_device_write(led_dev, &cmd_off, sizeof(uint8_t)); // 物理关灯
        os_shadow_update_led(0);                             // 同步影子状态
        printf(">>> [定时器异步事件] 3秒时间到！已自动关闭临时警报灯。 <<<\r\n");
    }
}
// ==========================================================


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
        
        // 【巨变】：使用 OS 级无阻塞延时，主动交出 CPU！
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
            printf("[Task 2] 光敏传感电压=%d mV\r\n", voltage_mv);
        }
        
        os_delay(500); // 无阻塞延时
    }
}
// --- 任务 3：报警联动控制 (引入状态锁机制) ---
void task3_entry(void) {
    os_device_t *led_dev = os_device_open("alarm_led");
    smarthome_shadow_t local_shadow; 

    // 【核心新增】：报警状态锁 (State Machine Flag)
    // 0 = 正常监控待命
    // 1 = 已触发过报警，正在静音或等待温度回落
    uint8_t alarm_lock = 0; 

    // 初始化软件定时器：单次触发，3000ms
    os_timer_init(&led_auto_off_timer, "led_auto_off", led_auto_off_callback, (void*)led_dev, 3000, OS_TIMER_FLAG_ONE_SHOT);

    while(1) {
        os_shadow_get_snapshot(&local_shadow);
        float current_temp = local_shadow.room_temp;

        // 【状态 1】：检测到超温，且当前没有“上锁”
        if (current_temp > 28.0f && alarm_lock == 0) {
            uint8_t cmd_on = 1;
            if (led_dev) os_device_write(led_dev, &cmd_on, sizeof(uint8_t)); 
            os_shadow_update_led(1);
            
            printf(">>> [Task 3 决策] 检测到超温！立刻开启警报，3秒后自动静音... <<<\r\n");
            
            // 启动定时器（3秒后在中断里自动关灯）
            os_timer_start(&led_auto_off_timer);
            
            // 【关键】：立刻上锁！只要温度降不下来，这把锁就永远不解开，彻底杜绝重复报警！
            alarm_lock = 1;
        }
        // 【状态 2】：温度终于降回 28 度及以下，且系统处于“上锁”状态
        else if (current_temp <= 28.0f && alarm_lock == 1) {
            
            // （防御性编程）如果温度降得太快，3秒时间还没到，灯还没被定时器关掉，我们就顺手把它关了
            if (local_shadow.alarm_led_state == 1) {
                uint8_t cmd_off = 0;
                if (led_dev) os_device_write(led_dev, &cmd_off, sizeof(uint8_t)); 
                os_shadow_update_led(0);
                
                // 停止可能还在后台运行的定时器（防止其乱触发）
                os_timer_stop(&led_auto_off_timer); 
            }

            // 【关键】：开锁！系统恢复到待命状态，准备迎接下一次真正的高温
            alarm_lock = 0;
            printf(">>> [Task 3 决策] 温度已彻底恢复正常，警报系统重置待命。 <<<\r\n");
        }

        os_delay(200); 
    }
}

int main(void)
{
    // 基础时钟与外设初始化
    SystemCoreClockUpdate();
    Delay_Init(); // 原厂延时只在开机初始化时使用
    USART_Printf_Init(115200);      
    printf("\r\n================================\r\n");
    printf("     SmartHomeOS 启动中...      \r\n");
    printf("================================\r\n");

    os_shadow_init();
    drv_smarthome_init();
    printf("硬件驱动与设备节点挂载完成。\r\n");
    
    // 开启 TIM2 提供 1ms 的 OS 硬件心跳
    os_tick_hardware_init();
    
    stack_pool_init();
    os_mq_init(&temp_msg_queue, temp_queue_buffer, sizeof(float), 5);

    uint32_t *sp1 = (uint32_t*)stack_pool_alloc();
    task_init(&task1, task1_entry, sp1, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *sp2 = (uint32_t*)stack_pool_alloc();
    task_init(&task2, task2_entry, sp2, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *sp3 = (uint32_t*)stack_pool_alloc();
    task_init(&task3, task3_entry, sp3, TASK_STACK_SIZE_BYTES / 4);

    task_register(&task1);
    task_register(&task2);
    task_register(&task3);
    
    printf("系统任务创建完毕，接管 CPU 控制权...\r\n");
    os_start();
    
    while(1) {}
}