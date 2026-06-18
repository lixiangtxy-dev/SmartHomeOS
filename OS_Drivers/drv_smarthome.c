#include "drv_smarthome.h"
#include "device.h"     // 引入设备中枢
#include "hal_adc.h"    // 引入 ADC 底层
#include "hal_gpio.h"   // 引入 GPIO 底层
#include <math.h>

// ==========================================
// 1. 硬件配方表 (BSP)
// ==========================================
static const adc_hw_config_t light_sensor_hw = { RCC_APB2Periph_GPIOC, GPIOC, GPIO_Pin_1, 11 };
static const adc_hw_config_t temp_sensor_hw  = { RCC_APB2Periph_GPIOC, GPIOC, GPIO_Pin_2, 12 };
static const gpio_hw_config_t alarm_led_hw   = { RCC_APB2Periph_GPIOC, GPIOC, GPIO_Pin_3, GPIO_Mode_Out_PP, 0 }; // 0=低电平亮

// NTC 物理参数
#define NTC_R25       10000.0f  
#define NTC_B_VALUE   3950.0f   
#define SERIES_R      10000.0f  

// ==========================================
// 2. 真实设备的 Read / Write 实现
// ==========================================

// [设备1] 热敏读取逻辑
static int temp_read(os_device_t *dev, void *buffer, uint32_t size) {
    if (size != sizeof(float)) return -1;
    adc_hw_config_t *hw = (adc_hw_config_t *)dev->user_data;
    uint16_t adc_raw = os_hal_adc_read(hw->adc_channel);
    
    if (adc_raw == 0) *(float*)buffer = -273.15f; 
    else if (adc_raw == 4095) *(float*)buffer = 999.0f; 
    else {
        float current_R = SERIES_R * ((float)adc_raw / (4095.0f - (float)adc_raw));
        float temp_k = 1.0f / ((1.0f/298.15f) + (1.0f/NTC_B_VALUE) * log(current_R/NTC_R25));
        *(float*)buffer = temp_k - 273.15f;
    }
    return 0; // 成功
}

// [设备2] 光敏读取逻辑
static int light_read(os_device_t *dev, void *buffer, uint32_t size) {
    if (size != sizeof(uint32_t)) return -1;
    adc_hw_config_t *hw = (adc_hw_config_t *)dev->user_data;
    uint16_t adc_raw = os_hal_adc_read(hw->adc_channel);
    *(uint32_t*)buffer = (adc_raw * 3300) / 4096; // 毫伏
    return 0;
}

// [设备3] 报警 LED 写入逻辑
static int led_write(os_device_t *dev, const void *buffer, uint32_t size) {
    if (size != sizeof(uint8_t)) return -1;
    gpio_hw_config_t *hw = (gpio_hw_config_t *)dev->user_data;
    uint8_t state = *(const uint8_t*)buffer; // 获取上层传来的 1 或 0
    os_hal_gpio_set(hw, state);
    return 0;
}

// ==========================================
// 3. 将方法打包成设备对象
// ==========================================
static const os_device_ops_t temp_ops  = { .read = temp_read };
static const os_device_ops_t light_ops = { .read = light_read };
static const os_device_ops_t led_ops   = { .write = led_write };

static os_device_t dev_temp  = { .name = "sensor_temp",  .ops = &temp_ops,  .user_data = (void*)&temp_sensor_hw };
static os_device_t dev_light = { .name = "sensor_light", .ops = &light_ops, .user_data = (void*)&light_sensor_hw };
static os_device_t dev_led   = { .name = "alarm_led",    .ops = &led_ops,   .user_data = (void*)&alarm_led_hw };

// ==========================================
// 4. 初始化与注册入口
// ==========================================
void drv_smarthome_init(void) {
    // 1. 初始化底层硬件机制
    os_hal_adc_core_init();                           
    os_hal_adc_channel_init(&temp_sensor_hw);         
    os_hal_adc_channel_init(&light_sensor_hw);        
    os_hal_gpio_init(&alarm_led_hw);

    // 2. 向 OS 核心注册这三个设备节点！
    os_device_register(&dev_temp);
    os_device_register(&dev_light);
    os_device_register(&dev_led);
}