#include "device.h"
#include "oled.h"    
#include <stddef.h>  

// ========================================================
// OS 设备写方法：带 \n 解析与无闪烁 (Flicker-Free) 渲染
// ========================================================
static int oled_write(os_device_t *dev, const void *buffer, uint32_t size) {
    if (buffer == NULL) return -1;
    char *str = (char *)buffer;
    uint8_t y = 0;       // 行坐标 (0, 2, 4, 6 对应 1~4 行)
    char line[17];       // 每行最多 16 个字符 + 1 个结束符
    int line_idx = 0;
    
    // 遍历传入的字符串
    while(*str != '\0' && y < 8) {
        if (*str == '\n') {
            // 遇到换行符，用空格将本行剩余位置填满（精髓：覆盖旧数据，免去清屏）
            while(line_idx < 16) line[line_idx++] = ' ';
            line[16] = '\0';
            OLED_ShowString(0, y, (uint8_t *)line, 16);
            y += 2;      // 16 号字体占用 2 个 Page
            line_idx = 0;
        } else {
            if (line_idx < 16) line[line_idx++] = *str;
        }
        str++;
    }
    
    // 将最后一行，以及屏幕底部剩余的空行全部用空格刷干净
    while (y < 8) {
        while(line_idx < 16) line[line_idx++] = ' ';
        line[16] = '\0';
        OLED_ShowString(0, y, (uint8_t *)line, 16);
        y += 2;
        line_idx = 0; 
    }
    
    return 0;
}

// 实例化设备节点
static const os_device_ops_t oled_ops = { .write = oled_write };
static os_device_t dev_oled = { .name = "display_oled", .ops = &oled_ops, .user_data = NULL };

// 注册入口
void drv_oled_register(void) {
    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    OLED_Init();
    OLED_Clear(); // 仅在开机时清屏一次
    
    OLED_ShowString(0, 0, (uint8_t *)"OS Booting...", 16);
    os_device_register(&dev_oled);
}