#ifndef __OS_H__
#define __OS_H__

#include <stdint.h>

// ---------------- 结构体声明 ----------------
struct task {
    uint32_t *sp;
    char name[16];
};

// ---------------- 外部变量声明  ----------------
extern struct task *current_task;
extern struct task *next_task;

// ---------------- 函数声明 ----------------
void uart_puts(char *s);
void delay(volatile int count);
extern void switch_to(uint32_t **old_sp, uint32_t *new_sp);

#endif