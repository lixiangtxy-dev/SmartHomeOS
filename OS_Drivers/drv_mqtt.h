#ifndef __DRV_MQTT_H
#define __DRV_MQTT_H

#include <stdint.h>

// 初始化 MQTT 连接 (发送 CONNECT 报文)
// client_id: 你的设备唯一标识符
// 返回值: 0=成功, -1=失败
int drv_mqtt_connect(const char* client_id);

// 发送遥测数据 (发送 PUBLISH 报文, QoS=0)
// topic: 目标主题
// payload: JSON 字符串
// 返回值: 0=成功, -1=失败
int drv_mqtt_publish(const char* topic, const char* payload);

// 发送心跳包 (发送 PINGREQ 报文)
// 建议每 30~50 秒调用一次，维持 TCP 链路存活
int drv_mqtt_ping(void);

#endif // __DRV_MQTT_H