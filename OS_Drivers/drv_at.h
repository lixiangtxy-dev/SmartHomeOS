#ifndef __DRV_AT_H
#define __DRV_AT_H

#include "ch32v30x.h"
#include <stdint.h>

// 初始化 AT 串口与环形缓冲区
void drv_at_init(void);

// 核心函数：发送指令并等待期望的响应，带超时机制
int at_send_cmd_wait(const char* cmd, const char* expect, uint32_t timeout_ms);

// 【新增】：从底层的环形缓冲区安全取出一个字节的数据给上层业务使用
// 返回 1 表示成功取到数据，0 表示缓冲区为空
int at_get_char(char *c);

#endif