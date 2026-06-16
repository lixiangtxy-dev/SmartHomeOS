#ifndef __TASK_H
#define __TASK_H

#include <stdint.h>

#define MAX_TASKS 8  // 系统最多支持的任务数量

// 任务控制块 (TCB)
typedef struct {
    uint32_t *sp;  // 栈顶指针。必须是结构体的第一个成员！
} task_t;

// 汇编中定义的切换函数
extern void cpu_switch(uint32_t **current_sp, uint32_t **next_sp);

// 任务 API
void task_init(task_t *task, void (*entry)(void), uint32_t *stack, uint32_t stack_size);
void task_register(task_t *task);  // 新增：把任务加入调度名单
void task_yield(void);             // 新增：主动让出 CPU
void os_start(void);               // 新增：启动操作系统

#endif