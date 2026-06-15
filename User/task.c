#include "task.h"

void task_init(task_t *task, void (*entry)(void), uint32_t *stack, uint32_t stack_size) {
    // 栈顶指向数组末尾
    task->sp = stack + stack_size;
    
    // 强制16字节对齐（RISC-V要求）
    task->sp = (uint32_t*)((uint32_t)task->sp & ~0xF);
    
    // 预留64字节上下文空间（和汇编addi sp, sp, -64对应）
    task->sp -= 16;
    
    // 初始化所有寄存器为0
    for(int i = 0; i < 16; i++) {
        task->sp[i] = 0;
    }
    
    // 设置返回地址为任务入口
    task->sp[0] = (uint32_t)entry;
}