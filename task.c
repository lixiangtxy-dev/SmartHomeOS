#include "os.h"

// 任务池：静态分配 10 个任务的内存空间
struct task tasks[MAX_TASKS];
struct task *current_task = 0; 

// 系统启动时的初始化
void task_os_init() {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
    }
}

// 动态创建任务
int task_create(void (*entry)(), char *name) {
    int i;
    // 寻找空闲槽位
    for (i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            break;
        }
    }
    
    // 如果任务池满了
    if (i == MAX_TASKS) return -1;

    struct task *t = &tasks[i];
    t->sp = &t->stack[STACK_SIZE]; 
    t->sp -= 14; 
    t->sp[13] = (uint32_t)entry;

    // 3. 设置任务信息
    int j = 0;
    while(name[j] && j < 15) { t->name[j] = name[j]; j++; }
    t->name[j] = '\0';

    t->state = TASK_RUNNABLE; // 设为就绪态，等待被 schedule 选中

    return i; // 返回任务 ID
}

// 调度器 (Round Robin)
void schedule() {
    struct task *prev = current_task;
    struct task *next = 0;
    
    int current_idx = -1;

    for (int i = 0; i < MAX_TASKS; i++) {
        if (&tasks[i] == current_task) {
            current_idx = i;
            break;
        }
    }

    // 从当前任务的下一个开始找，转一圈
    int i = (current_idx + 1) % MAX_TASKS;
    while (i != current_idx) {
        if (tasks[i].state == TASK_RUNNABLE) {
            next = &tasks[i];
            break;
        }
        i = (i + 1) % MAX_TASKS;
    }

    // 如果没找到别的任务，且当前任务是就绪的，那就继续跑当前任务
    if (!next && prev->state == TASK_RUNNABLE) {
        next = prev;
    }

    if (next) {
        // 更新状态
        next->state = TASK_RUNNING;
        current_task = next;
        switch_to(&prev->sp, next->sp);
    }
}

// 主动让出 CPU (被 Timer 中断调用)
void task_yield() {
    if (current_task->state == TASK_RUNNING) {
        current_task->state = TASK_RUNNABLE;
    }
    // 呼叫调度器
    schedule();
}