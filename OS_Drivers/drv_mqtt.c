#include "drv_mqtt.h"
#include "drv_at.h"
#include "ch32v30x.h"
#include <string.h>
#include <stdio.h>

extern void os_delay(uint32_t ms);

// 内部静态函数：向底层的 ESP-01 发送二进制流
static void uart4_send_data(const uint8_t* data, uint32_t len) {
    for(uint32_t i = 0; i < len; i++) {
        while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
        USART_SendData(UART4, data[i]);
    }
}

// ==========================================
// 1. MQTT CONNECT 报文 (动态组包)
// ==========================================
int drv_mqtt_connect(const char* client_id) {
    char cmd_buf[32];
    uint16_t id_len = strlen(client_id);
    
    // MQTT 协议规范: Remaining Length = 可变报头(10字节) + Payload(2字节长度 + ID长度)
    uint8_t remain_len = 10 + 2 + id_len; 
    uint8_t total_len = 2 + remain_len; // 加上固定报头的 2 字节

    sprintf(cmd_buf, "AT+CIPSEND=%d\r\n", total_len);
    if (at_send_cmd_wait(cmd_buf, "OK", 2000) != 0) {
        printf("[MQTT] 发送 CONNECT 请求失败\r\n");
        return -1;
    }
    os_delay(100); // 闭眼等待 '>' 吐出

    // --- 开始组装并发送报文 ---
    // 1. 固定报头: 0x10 (CONNECT), 剩余长度
    uint8_t header[2] = {0x10, remain_len};
    uart4_send_data(header, 2);

    // 2. 可变报头: 协议名 "MQTT", 版本 4, 标志位 0x02, KeepAlive 60秒
    uint8_t var_header[10] = {
        0x00, 0x04, 'M', 'Q', 'T', 'T', 
        0x04,       // MQTT v3.1.1
        0x02,       // Clean Session = 1
        0x00, 0x3C  // Keep Alive = 60s
    };
    uart4_send_data(var_header, 10);

    // 3. Payload: Client ID
    uint8_t id_len_buf[2] = { (uint8_t)(id_len >> 8), (uint8_t)(id_len & 0xFF) };
    uart4_send_data(id_len_buf, 2);
    uart4_send_data((const uint8_t*)client_id, id_len);

    printf("[MQTT] CONNECT 握手包已发送 (ClientID: %s)\r\n", client_id);
    return 0;
}

// ==========================================
// 2. MQTT PUBLISH 报文 (QoS 0)
// ==========================================
int drv_mqtt_publish(const char* topic, const char* payload) {
    char cmd_buf[32];
    uint16_t topic_len = strlen(topic);
    uint16_t payload_len = strlen(payload);
    
    // 注意: 此简易协议栈假设总报文长度 < 127 字节
    uint8_t remain_len = 2 + topic_len + payload_len;
    uint8_t total_len = 2 + remain_len; 

    sprintf(cmd_buf, "AT+CIPSEND=%d\r\n", total_len);
    if (at_send_cmd_wait(cmd_buf, "OK", 2000) != 0) {
        printf("[MQTT] 发送 PUBLISH 请求超时\r\n");
        return -1;
    }
    os_delay(100);

    // 1. 固定报头: 0x30 (PUBLISH), 剩余长度
    uint8_t header[2] = {0x30, remain_len};
    uart4_send_data(header, 2);

    // 2. Topic 长度与内容
    uint8_t t_len[2] = { (uint8_t)(topic_len >> 8), (uint8_t)(topic_len & 0xFF) };
    uart4_send_data(t_len, 2);
    uart4_send_data((const uint8_t*)topic, topic_len);

    // 3. Payload 内容
    uart4_send_data((const uint8_t*)payload, payload_len);

    printf("[MQTT] 成功发布至 %s\r\n", topic);
    return 0;
}

// ==========================================
// 3. MQTT PINGREQ 报文 (心跳包)
// ==========================================
int drv_mqtt_ping(void) {
    // PINGREQ 固定为 2 个字节: 0xC0, 0x00
    if (at_send_cmd_wait("AT+CIPSEND=2\r\n", "OK", 2000) == 0) {
        os_delay(100);
        uint8_t ping_pkt[2] = {0xC0, 0x00};
        uart4_send_data(ping_pkt, 2);
        printf("[MQTT] PING 心跳已发送\r\n");
        return 0;
    }
    return -1;
}

// ==========================================
// 4. MQTT SUBSCRIBE 报文 (订阅主题)
// ==========================================
int drv_mqtt_subscribe(const char* topic) {
    char cmd_buf[32];
    uint16_t topic_len = strlen(topic);
    
    // MQTT SUBSCRIBE 剩余长度 = 报文标识符(2字节) + Topic长度(2字节) + Topic字符串 + QoS(1字节)
    uint8_t remain_len = 2 + 2 + topic_len + 1; 
    uint8_t total_len = 2 + remain_len; 

    sprintf(cmd_buf, "AT+CIPSEND=%d\r\n", total_len);
    if (at_send_cmd_wait(cmd_buf, "OK", 2000) != 0) {
        printf("[MQTT] 发送 SUBSCRIBE 请求超时\r\n");
        return -1;
    }
    os_delay(100);

    // 1. 固定报头: 0x82 (SUBSCRIBE, QoS=1), 剩余长度
    uint8_t header[2] = {0x82, remain_len};
    uart4_send_data(header, 2);

    // 2. 报文标识符 (Packet ID = 1)
    uint8_t packet_id[2] = {0x00, 0x01};
    uart4_send_data(packet_id, 2);

    // 3. Topic 长度与内容
    uint8_t t_len[2] = { (uint8_t)(topic_len >> 8), (uint8_t)(topic_len & 0xFF) };
    uart4_send_data(t_len, 2);
    uart4_send_data((const uint8_t*)topic, topic_len);

    // 4. 请求的服务质量 (QoS = 0)
    uint8_t qos = 0x00;
    uart4_send_data(&qos, 1);

    printf("[MQTT] 已成功订阅控制主题: %s\r\n", topic);
    return 0;
}