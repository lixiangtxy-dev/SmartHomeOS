#ifndef __SYS_CONFIG_H
#define __SYS_CONFIG_H

#include <stdint.h>

// 定义系统全局配置结构体
typedef struct {
    char wifi_ssid[32];   // Wi-Fi 名称
    char wifi_pwd[32];    // Wi-Fi 密码
    char mqtt_server[64]; // MQTT 服务器地址
} sys_config_t;

// 暴露给全系统使用的全局配置变量
extern sys_config_t g_sys_config;

// 初始化配置：从 LittleFS 读取 config.json，如果不存在则自动创建默认配置
void sys_config_init(void);

#endif // __SYS_CONFIG_H