#include "os.h"

// ---------------- 硬件定义 ----------------
#define UART0_DR   ((volatile unsigned char *)0x10000000)
#define CLINT_MTIMECMP (volatile uint64_t *)0x2004000
#define CLINT_MTIME    (volatile uint64_t *)0x200bff8

// ---------------- 变量实体定义 ----------------
#define STACK_SIZE 1024
struct task task_a = {0, "Task A"};
struct task task_b = {0, "Task B"};
uint32_t stack_a[STACK_SIZE];
uint32_t stack_b[STACK_SIZE];

struct task *current_task = &task_a;
struct task *next_task = &task_b;

// ---------------- 辅助函数 ----------------
void uart_putc(char c) { *UART0_DR = c; }
void uart_puts(char *s) { while (*s) uart_putc(*s++); }
void delay(volatile int count) { while (count--) {} }

// 开启机器模式全局中断
void enable_interrupts() {
    uint32_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r" (mstatus));
    mstatus |= 0x8; 
    asm volatile("csrw mstatus, %0" : : "r" (mstatus));
}

// ---------------- 任务初始化 ----------------
void task_init(struct task *t, void (*entry)(), uint32_t *stack) {
    t->sp = &stack[STACK_SIZE];
    t->sp -= 14; 
    t->sp[13] = (uint32_t)entry; 
}

// ---------------- 任务逻辑 ----------------
void task_a_entry() {
    enable_interrupts(); // 关键修正：新任务启动时要开中断
    while (1) {
        uart_puts("A");
        delay(10000000);
    }
}

void task_b_entry() {
    enable_interrupts(); // 关键修正
    while (1) {
        uart_puts("B");
        delay(10000000);
    }
}

// ---------------- 硬件初始化 ----------------
void timer_init() {
    *CLINT_MTIMECMP = *CLINT_MTIME + 1000000; // 改小一点，切换快一点 (0.1秒)
}

void main() {
    uart_puts("OS: Booting...\n");

    task_init(&task_a, task_a_entry, stack_a);
    task_init(&task_b, task_b_entry, stack_b);

    extern void trap_entry();
    asm volatile("csrw mtvec, %0" : : "r" ((uint32_t)trap_entry));

    timer_init();
    
    // 开启定时器中断位 (MTIE)
    uint32_t mie;
    asm volatile("csrr %0, mie" : "=r" (mie));
    mie |= 0x80;
    asm volatile("csrw mie, %0" : : "r" (mie));

    // main 函数自己也要开全局中断，否则第一个任务A启动前中断就被封死了
    enable_interrupts();

    uart_puts("OS: Interrupts Enabled. Starting Multitasking...\n");

    uint32_t tmp_sp;
    switch_to(&tmp_sp, task_a.sp); 

    while(1) {} 
}