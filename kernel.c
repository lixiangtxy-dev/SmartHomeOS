#include "os.h"

// ---------------- 硬件定义 ----------------
#define UART0_DR   ((volatile unsigned char *)0x10000000)
#define CLINT_MTIMECMP (volatile uint64_t *)0x2004000
#define CLINT_MTIME    (volatile uint64_t *)0x200bff8
// 全局锁
struct spinlock uart_lock;

// ---------------- 辅助函数 ----------------
void uart_putc(char c) { *UART0_DR = c; }
void uart_puts(char *s) { while (*s) uart_putc(*s++); }
void delay(volatile int count) { while (count--) {} }

void enable_interrupts() {
    uint32_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r" (mstatus));
    mstatus |= 0x8; 
    asm volatile("csrw mstatus, %0" : : "r" (mstatus));
}

// 安全打印函数
void slow_safe_print(char *s) {
    spin_lock(&uart_lock);
    while (*s) {
        uart_putc(*s++);
        delay(1000000); 
    }
    uart_putc('\n');
    spin_unlock(&uart_lock);
}

// ---------------- 任务函数 ----------------
void task_a_entry() {
    enable_interrupts();
    while (1) {
        slow_safe_print("Task A: Managing Smart Home...");
        delay(5000000);
    }
}

void task_b_entry() {
    enable_interrupts();
    while (1) {
        slow_safe_print("Task B: Reading Sensors...");
        delay(5000000);
    }
}

void task_c_entry() {
    enable_interrupts();
    while (1) {
        slow_safe_print("Task C: Connecting to Cloud...");
        delay(5000000);
    }
}

void timer_init() {
    *CLINT_MTIMECMP = *CLINT_MTIME + 100000;
}

// ---------------- Main ----------------
void main() {
    extern void spin_init(struct spinlock *lk, char *name);
    spin_init(&uart_lock, "uart");

    uart_puts("OS: Booting...\n");
    
    // 1. 初始化任务系统
    task_os_init();

    // 2. 创建 3 个任务
    task_create(task_a_entry, "TaskA");
    task_create(task_b_entry, "TaskB");
    task_create(task_c_entry, "TaskC");

    // 3. 硬件初始化
    timer_init();
    extern void trap_entry();
    asm volatile("csrw mtvec, %0" : : "r" ((uint32_t)trap_entry));
    
    uint32_t mie;
    asm volatile("csrr %0, mie" : "=r" (mie));
    mie |= 0x80;
    asm volatile("csrw mie, %0" : : "r" (mie));

    enable_interrupts();

    uart_puts("OS: Scheduler Initialized. Starting...\n");

    // 启动调度器！
    current_task = &tasks[0];
    tasks[0].state = TASK_RUNNING;
    
    uint32_t *tmp_sp;
    switch_to(&tmp_sp, tasks[0].sp);

    while(1) {} // Should not reach here
}