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

// ---------------- 定义锁----------------
struct spinlock {
    int locked;       // 1=已上锁, 0=未上锁
    char *name;       // 锁的名字(调试用)
};

// 锁的相关函数
void spin_init(struct spinlock *lk, char *name);
void spin_lock(struct spinlock *lk);
void spin_unlock(struct spinlock *lk);

#endif