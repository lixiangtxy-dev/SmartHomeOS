#include "sys_config.h"
#include "fs_port.h" // 引入 LittleFS 挂载实例 lfs
#include "cJSON.h"   // 引入 cJSON 库 (需要你手动添加到工程中)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 实例化全局配置变量
sys_config_t g_sys_config = {0};

// 出厂默认的 JSON 配置字符串 (首次开机自动写入 Flash)
static const char* default_config_json = 
    "{\n"
    "  \"wifi_ssid\": \"FAST_CD1A\",\n"
    "  \"wifi_pwd\": \"123456789\",\n"
    "  \"mqtt_server\": \"broker.emqx.io\"\n"
    "}";

void sys_config_init(void) {
    lfs_file_t file;
    char json_buf[256] = {0}; // 用于存放从 Flash 读出的 JSON 文本

    printf("\r\n[Config] 正在从 LittleFS 加载系统配置...\r\n");

    // 1. 尝试从 Flash 打开 config.json
    int err = lfs_file_open(&lfs, &file, "config.json", LFS_O_RDWR);
    
    if (err < 0) {
        // 2. 如果文件不存在，说明是第一次开机，自动创建出厂配置并写入
        printf("[Config] 配置文件不存在，正在生成出厂默认配置...\r\n");
        lfs_file_open(&lfs, &file, "config.json", LFS_O_RDWR | LFS_O_CREAT);
        lfs_file_write(&lfs, &file, default_config_json, strlen(default_config_json));
        lfs_file_close(&lfs, &file);
        
        // 把默认字符串拷贝到 buffer 中准备给下面解析
        strcpy(json_buf, default_config_json);
    } else {
        // 3. 如果文件存在，读取文件内容到内存
        lfs_size_t read_len = lfs_file_read(&lfs, &file, json_buf, sizeof(json_buf) - 1);
        json_buf[read_len] = '\0'; // 确保字符串结尾
        lfs_file_close(&lfs, &file);
        printf("[Config] 成功读取到配置文件 (%d 字节)\r\n", read_len);
    }

    // 4. 使用 cJSON 解析提取字段
    cJSON *root = cJSON_Parse(json_buf);
    if (root != NULL) {
        cJSON *ssid = cJSON_GetObjectItem(root, "wifi_ssid");
        cJSON *pwd  = cJSON_GetObjectItem(root, "wifi_pwd");
        cJSON *mqtt = cJSON_GetObjectItem(root, "mqtt_server");

        // 安全地将解析出的数据存入系统全局配置结构体
        if (ssid && cJSON_IsString(ssid)) strncpy(g_sys_config.wifi_ssid, ssid->valuestring, 31);
        if (pwd  && cJSON_IsString(pwd))  strncpy(g_sys_config.wifi_pwd,  pwd->valuestring,  31);
        if (mqtt && cJSON_IsString(mqtt)) strncpy(g_sys_config.mqtt_server, mqtt->valuestring, 63);

        // ?? 极其重要：解析完必须删除 JSON 树，释放内存，否则必死机！
        cJSON_Delete(root);

        // 打印最终生效的配置
        printf("[Config] 系统配置加载成功！\r\n");
        printf("  > SSID: %s\r\n", g_sys_config.wifi_ssid);
        printf("  > PWD:  %s\r\n", g_sys_config.wifi_pwd);
        printf("  > MQTT: %s\r\n", g_sys_config.mqtt_server);
    } else {
        printf("[Config] ? JSON 解析失败！请检查系统 Heap 大小。\r\n");
        
        // 兜底策略：如果解析失败，直接使用硬编码防止系统瘫痪
        strcpy(g_sys_config.wifi_ssid, "FAST_CD1A");
        strcpy(g_sys_config.wifi_pwd, "123456789");
        strcpy(g_sys_config.mqtt_server, "broker.emqx.io");
    }
}