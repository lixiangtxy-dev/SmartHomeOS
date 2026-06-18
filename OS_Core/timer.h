#ifndef __OS_TIMER_H
#define __OS_TIMER_H

#include "ch32v30x.h"
#include <stdint.h>
#include <stddef.h>

#define OS_TIMER_FLAG_ONE_SHOT   0x00  
#define OS_TIMER_FLAG_PERIODIC   0x01  

#define OS_TIMER_STATE_STOPPED   0x00  
#define OS_TIMER_STATE_RUNNING   0x01  

typedef void (*os_timer_cb_t)(void *parameter);

// 软件定时器控制块
typedef struct os_timer {
    const char       *name;          
    os_timer_cb_t     timeout_func;  
    void             *parameter;     
    uint32_t          init_tick;     
    uint32_t          timeout_tick;  
    uint8_t           flag;          
    uint8_t           state;         
    struct os_timer  *next;          
} os_timer_t;

// --- 硬件心跳初始化 ---
void os_tick_hardware_init(void);

// --- 软件定时器 API ---
void os_timer_init(os_timer_t *timer, const char *name, os_timer_cb_t timeout_func, void *parameter, uint32_t time_ms, uint8_t flag);
void os_timer_start(os_timer_t *timer);
void os_timer_stop(os_timer_t *timer);

// 供硬件中断调用的心跳驱动函数
void os_timer_ticks_update(void);

// OS 级无阻塞延时函数
void os_delay(uint32_t ms);

#endif // __OS_TIMER_H