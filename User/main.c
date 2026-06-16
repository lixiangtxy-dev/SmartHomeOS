#include "debug.h"
#include "task.h"
#include "mempool.h"
#include "hal_adc.h"   // 引入 ADC 硬件抽象层
#include "hal_gpio.h"  // 引入 GPIO 硬件抽象层
#include <math.h>

// ==========================================
// BSP (Board Support Package) 硬件配方表
// ==========================================

// 定义光敏传感器的硬件信息 (PC1)
const adc_hw_config_t light_sensor_hw = {
    .rcc_gpio_clock = RCC_APB2Periph_GPIOC,
    .gpio_port      = GPIOC,
    .gpio_pin       = GPIO_Pin_1,
    .adc_channel    = 11
};

// 定义热敏传感器的硬件信息 (PC2)
const adc_hw_config_t temp_sensor_hw = {
    .rcc_gpio_clock = RCC_APB2Periph_GPIOC,
    .gpio_port      = GPIOC,
    .gpio_pin       = GPIO_Pin_2,
    .adc_channel    = 12
};

// ==========================================
// BSP 硬件配方表新增：报警 LED
// ==========================================
const gpio_hw_config_t alarm_led_hw = {
    .rcc_gpio_clock = RCC_APB2Periph_GPIOC,
    .gpio_port      = GPIOC,
    .gpio_pin       = GPIO_Pin_3,
    .gpio_mode      = GPIO_Mode_Out_PP,
    .active_level   = 0   // 【重点】告诉底层：我的开发板是低电平点亮！
};
// ==========================================

// NTC 热敏电阻的物理参数
#define NTC_R25       10000.0f  
#define NTC_B_VALUE   3950.0f   
#define SERIES_R      10000.0f  

// 任务控制块
task_t task1;
task_t task2;
task_t task3;

// 全局温度共享变量
volatile float g_current_temp = 0.0f;

/**
 * @brief 将 ADC 刻度值转换为摄氏度
 */
float Convert_ADC_To_Celsius(uint16_t adc_raw) {
    if (adc_raw == 0) return -273.15f; 
    if (adc_raw == 4095) return 999.0f; 

    float current_R = SERIES_R * ((float)adc_raw / (4095.0f - (float)adc_raw));
    float temp_kelvin = 1.0f / ( (1.0f / 298.15f) + (1.0f / NTC_B_VALUE) * log(current_R / NTC_R25) );
    return temp_kelvin - 273.15f;
}

// --- 任务 1：热敏传感器采集 ---
void task1_entry(void) {
    u16 adc_value;
    float current_temp_c;

    while(1) {
        // 【巨变1】：直接调 HAL 接口读取数据，底层自动排队，无需手动管理互斥锁！
        adc_value = os_hal_adc_read(temp_sensor_hw.adc_channel); 
        
        current_temp_c = Convert_ADC_To_Celsius(adc_value);
        g_current_temp = current_temp_c; 
        
        int temp_int = (int)current_temp_c; 
        int temp_frac = (int)((current_temp_c - temp_int) * 100); 
        if (temp_frac < 0) temp_frac = -temp_frac; 
        
        printf("[Task 1] 室内温度(PC2): %d.%02d ℃ (ADC: %04d)\r\n", temp_int, temp_frac, adc_value);
        
        Delay_Ms(500); 
        task_yield(); 
    }
}

// --- 任务 2：光敏传感器采集 ---
void task2_entry(void) {
    u16 adc_value;
    u32 voltage_mv;

    while(1) {
        // 【巨变2】：无脑读取即可，不怕和任务1抢夺硬件
        adc_value = os_hal_adc_read(light_sensor_hw.adc_channel); 
        voltage_mv = (adc_value * 3300) / 4096;
        
        printf("[Task 2] 光敏传感(PC1): ADC=%04d, 电压=%d mV\r\n", adc_value, voltage_mv);
        
        Delay_Ms(500); 
        task_yield(); 
    }
}

// --- 任务 3：温度报警控制任务 ---
void task3_entry(void) {
    static uint8_t led_is_on = 0; 

    while(1) {
        float temp = g_current_temp;

        if (temp > 28.0f && led_is_on == 0) {
            // 【终极进化】：直接传对象指针和语义指令！底层会自动翻译成 ResetBits
            os_hal_gpio_set(&alarm_led_hw, OS_GPIO_ON); 
            led_is_on = 1;
            printf(">>> [Task 3] 警告: 温度达到 %d ℃, 报警 LED 已点亮！ <<<\r\n", (int)temp);
        }
        else if (temp <= 28.0f && led_is_on == 1) {
            os_hal_gpio_set(&alarm_led_hw, OS_GPIO_OFF); 
            led_is_on = 0;
            printf(">>> [Task 3] 恢复: 温度降至 %d ℃, 报警 LED 已关闭。 <<<\r\n", (int)temp);
        }

        Delay_Ms(200); 
        task_yield(); 
    }
}

int main(void)
{
    // 系统级硬件初始化
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);      
    printf("系统启动中...\r\n");

    // ===================================================
    // 1. 初始化 OS HAL 层
    // ===================================================
    os_hal_adc_core_init();                           // 1.1 开启 ADC 核心与时钟 (内部包含了 mutex 初始化)
    os_hal_adc_channel_init(&light_sensor_hw);        // 1.2 挂载光敏引脚
    os_hal_adc_channel_init(&temp_sensor_hw);         // 1.3 挂载热敏引脚
    os_hal_gpio_init(&alarm_led_hw);                  // 1.4 初始化报警指示灯

    // 2. 初始化内存池并分配任务
    stack_pool_init();
    
    uint32_t *stack1_ptr = (uint32_t*)stack_pool_alloc();
    task_init(&task1, task1_entry, stack1_ptr, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *stack2_ptr = (uint32_t*)stack_pool_alloc();
    task_init(&task2, task2_entry, stack2_ptr, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *stack3_ptr = (uint32_t*)stack_pool_alloc();
    task_init(&task3, task3_entry, stack3_ptr, TASK_STACK_SIZE_BYTES / 4);

    task_register(&task1);
    task_register(&task2);
    task_register(&task3);
    
    printf("任务创建完毕，启动调度器...\r\n");
    
    // 3. 启动系统
    os_start();
    
    while(1) {}
}