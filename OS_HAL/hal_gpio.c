#include "hal_gpio.h"

void os_hal_gpio_init(const gpio_hw_config_t *config) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    // 1. 开启对应的时钟
    RCC_APB2PeriphClockCmd(config->rcc_gpio_clock, ENABLE);

    // 2. 配置引脚和模式
    GPIO_InitStructure.GPIO_Pin = config->gpio_pin;
    GPIO_InitStructure.GPIO_Mode = config->gpio_mode; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(config->gpio_port, &GPIO_InitStructure);

    // 3. 极其重要：初始化后，默认使其处于 OFF (关闭) 状态
    os_hal_gpio_set(config, OS_GPIO_OFF);
}

void os_hal_gpio_set(const gpio_hw_config_t *config, uint8_t state) {
    // 【内核级屏蔽】：判断物理层到底该输出高电平还是低电平
    // 如果想要打开 (state==1)，且硬件是高电平触发 (active_level==1) -> 输出高
    // 如果想要打开 (state==1)，但硬件是低电平触发 (active_level==0) -> 输出低
    if (state == config->active_level) {
        // 物理输出高电平
        GPIO_SetBits(config->gpio_port, config->gpio_pin);
    } else {
        // 物理输出低电平
        GPIO_ResetBits(config->gpio_port, config->gpio_pin);
    }
}

void os_hal_gpio_toggle(const gpio_hw_config_t *config) {
    // 读取当前输出寄存器的状态并翻转
    uint8_t current_val = GPIO_ReadOutputDataBit(config->gpio_port, config->gpio_pin);
    if (current_val == Bit_SET) {
        GPIO_ResetBits(config->gpio_port, config->gpio_pin);
    } else {
        GPIO_SetBits(config->gpio_port, config->gpio_pin);
    }
}