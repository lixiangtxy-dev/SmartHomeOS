#ifndef __HAL_GPIO_H
#define __HAL_GPIO_H

#include "ch32v30x.h" // 引入底层芯片头文件

// ==========================================
// 1. 定义 GPIO 硬件配置结构体 (这就是你的“对象”)
// ==========================================
typedef struct {
    uint32_t         rcc_gpio_clock; // GPIO 时钟 (如 RCC_APB2Periph_GPIOC)
    GPIO_TypeDef* gpio_port;      // GPIO 端口 (如 GPIOC)
    uint16_t         gpio_pin;       // GPIO 引脚 (如 GPIO_Pin_3)
    GPIOMode_TypeDef gpio_mode;      // GPIO 模式 (如 GPIO_Mode_Out_PP)
    uint8_t          active_level;   // 重点！激活电平：1 表示高电平触发，0 表示低电平触发
} gpio_hw_config_t;

// 定义通用的开关状态语义
#define OS_GPIO_OFF  0
#define OS_GPIO_ON   1

// ==========================================
// 2. 通用操作接口
// ==========================================

/**
 * @brief  初始化指定的 GPIO 引脚，并默认将其置为关闭状态
 * @param  config : 指向 GPIO 硬件配置表对象的指针
 */
void os_hal_gpio_init(const gpio_hw_config_t *config);

/**
 * @brief  设置 GPIO 的逻辑状态 (底层会自动处理高低电平反转)
 * @param  config : 指向被操作的 GPIO 对象的指针
 * @param  state  : OS_GPIO_ON 或 OS_GPIO_OFF
 */
void os_hal_gpio_set(const gpio_hw_config_t *config, uint8_t state);

/**
 * @brief  翻转 GPIO 的状态
 */
void os_hal_gpio_toggle(const gpio_hw_config_t *config);

#endif // __HAL_GPIO_H