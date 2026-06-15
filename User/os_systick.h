#ifndef __OS_SYSTICK_H
#define __OS_SYSTICK_H

#include "ch32v30x.h"

#define OS_TICK_RATE_HZ 1000 // 设定 OS 节拍为 1000Hz (即 1ms 一次心跳)

void OS_Tick_Init(void);
uint32_t OS_Get_Tick(void);
void OS_Delay(uint32_t ms);

#endif