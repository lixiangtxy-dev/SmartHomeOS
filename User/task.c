#include "task.h"
#include <stddef.h>

// --- 全局调度器数据结构 ---
static task_t *task_list[MAX_TASKS];  // 任务就绪名单
static uint8_t task_count = 0;        // 已经注册的任务数量
static uint8_t current_task_idx = 0;  // 当前正在运行的任务索引

static task_t *current_task = NULL;   // 指向当前正在运行的任务
static uint32_t *main_sp;             // 隐藏在系统内部，保存 main 函数的废弃栈

// 初始化任务 (原有逻辑不变)
void task_init(task_t *task, void (*entry)(void), uint32_t *stack, uint32_t stack_size) {
    task->sp = stack + stack_size;
    task->sp = (uint32_t*)((uint32_t)task->sp & ~0xF); // 16字节对齐
    task->sp -= 16;
    for(int i = 0; i < 16; i++) {
        task->sp[i] = 0;
    }
    task->sp[0] = (uint32_t)entry;
}

// 注册任务到调度器
void task_register(task_t *task) {
    if (task_count < MAX_TASKS) {
        task_list[task_count] = task;
        task_count++;
    }
}

// 核心调度函数：让出 CPU，自动切换到下一个任务
void task_yield(void) {
    if (task_count == 0) return;

    // 1. 记录交出 CPU 的是谁
    task_t *prev_task = current_task;

    // 2. 轮询寻找下一个任务 (Round-Robin)
    current_task_idx = (current_task_idx + 1) % task_count;
    task_t *next_task = task_list[current_task_idx];
    
    // 3. 更新当前系统状态
    current_task = next_task;

    // 4. 执行底层汇编切换
    cpu_switch(&(prev_task->sp), &(next_task->sp));
}

// 启动操作系统调度
void os_start(void) {
    if (task_count == 0) return;
    
    current_task_idx = 0;
    current_task = task_list[0];
    
    // 第一次切入，从 main 切换到就绪名单里的第一个任务
    cpu_switch(&main_sp, &(current_task->sp));
}