#ifndef __TASK_H
#define __TASK_H

#include <stdint.h>

// 任务控制块 (TCB)
typedef struct {
    uint32_t *sp;  // 栈顶指针。必须是结构体的第一个成员！
} task_t;

// 汇编中定义的切换函数
extern void cpu_switch(uint32_t **current_sp, uint32_t **next_sp);

// 任务初始化函数
void task_init(task_t *task, void (*entry)(void), uint32_t *stack, uint32_t stack_size);

#endif