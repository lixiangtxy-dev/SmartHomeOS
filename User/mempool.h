#ifndef __MEMPOOL_H
#define __MEMPOOL_H

#include <stdint.h>
#include <stddef.h>

// 定义栈的参数
#define TASK_STACK_SIZE_BYTES  1024  // 每个任务栈 1024 字节 (256字)
#define MAX_TASK_COUNT         8     // 最多支持 8 个任务

// 内存池初始化
void stack_pool_init(void);

// 申请一个任务栈 (返回栈底指针)
void* stack_pool_alloc(void);

// 释放一个任务栈
void stack_pool_free(void* stack_ptr);

#endif