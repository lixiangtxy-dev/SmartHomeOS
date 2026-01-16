#include <stdint.h>

#define UARTO_DR ((volatile unsigned char *)0x10000000)

void uart_putc(char c){
    *UARTO_DR=c;
}

void uart_puts(char *s){
    while(*s){
        uart_putc(*s++);
    }
}

void delay(volatile int count){
    while(count--){

    }
}

//定义栈的大小(1kb)
#define STACK_SIZE 1024

struct task{
    uint32_t *sp; //栈指针
    char name[16];//任务名
};

struct task task_a;
struct task task_b;
uint32_t stack_a[STACK_SIZE];
uint32_t stack_b[STACK_SIZE];

//声明汇编函数
extern void switch_to(uint32_t **old_sp,uint32_t *new_sp);

void task_a_entry() {
    while (1) {
        uart_puts("Task A: I am controlling the LIGHT\n");
        delay(10000000); // 模拟做一些工作
        
        // 主动让出 CPU 给任务 B
        uart_puts("Task A: Yielding...\n");
        switch_to(&task_a.sp, task_b.sp);
    }
}

void task_b_entry() {
    while (1) {
        uart_puts("Task B: I am checking SENSORS\n");
        delay(10000000);

        // 主动让出 CPU 给任务 A
        uart_puts("Task B: Yielding...\n");
        switch_to(&task_b.sp, task_a.sp);
    }
}

void task_init(struct task *t,void (*entry)(),uint32_t *stack){
    t->sp=&stack[STACK_SIZE];
    t->sp-=14;
    t->sp[13]=(uint32_t)entry;
}

void main() {
    uart_puts("OS: Initializing Tasks...\n");

    task_init(&task_a, task_a_entry, stack_a);
    task_init(&task_b, task_b_entry, stack_b);

    uart_puts("OS: Starting Multitasking...\n");

    // --- 修改开始 ---
    // 定义一个临时的变量，专门用来存 main 函数的废弃现场
    uint32_t tmp_sp; 
    switch_to(&tmp_sp, task_b.sp); 
    // --- 修改结束 ---

    // 现在，task_a.sp 依然保持着纯洁的初始状态
    // 当 Task B 切换给 task_a.sp 时，就会真的跳进 task_a_entry 了！

    while(1) {}
}