#ifndef __HAL_ADC_H
#define __HAL_ADC_H

#include "ch32v30x.h" // 引入底层芯片头文件

// 1. 定义 ADC 通道的硬件配置结构体 
typedef struct {
    uint32_t      rcc_gpio_clock; // GPIO 时钟 (如 RCC_APB2Periph_GPIOC)
    GPIO_TypeDef* gpio_port;      // GPIO 端口 (如 GPIOC)
    uint16_t      gpio_pin;       // GPIO 引脚 (如 GPIO_Pin_1)
    uint8_t       adc_channel;    // ADC 通道号 (如 ADC_Channel_11)
} adc_hw_config_t;

// 2. 提供完全通用的接口
void os_hal_adc_core_init(void); // 只初始化 ADC1 核心内核
void os_hal_adc_channel_init(const adc_hw_config_t *config); // 根据配方初始化引脚
uint16_t os_hal_adc_read(uint8_t channel);

#endif