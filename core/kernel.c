#include "os.h"

// ---------------- 硬件定义 ----------------
#define UART0_DR   ((volatile unsigned char *)0x10000000)
#define CLINT_MTIMECMP (volatile uint64_t *)0x2004000
#define CLINT_MTIME    (volatile uint64_t *)0x200bff8
// 全局锁
struct spinlock uart_lock;
struct cpu cpu_single;//单核 CPU 实例

struct cpu* mycpu() {
    return &cpu_single;
}

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
        // 模拟生成日志字符串 (裸机没有 sprintf，需要手动拼)
        char log_msg[32] = "Temp: 25C\n"; 
        // 写入文件系统 (追加模式)
        os_file_append("sensor.log", log_msg, 10);
        
        delay(10000000); // 延时一会
    }
}

void task_c_entry() {
    // 【关键修复】先把变量定义在最前面！
    char read_buf[64]; 
    
    enable_interrupts();
    
    while (1) {
        delay(30000000); // 等 Task B 写入几次后，C 再去读
        
        slow_safe_print("Task C: Uploading Cloud...");
        
        // 读取日志文件
        int len = os_file_read("sensor.log", read_buf, 60);
        if (len > 0) {
            read_buf[len] = '\0'; // 添加字符串结束符
            slow_safe_print("---- Cloud Got Data ----");
            slow_safe_print(read_buf);
        }
    }
}

void timer_init() {
    *CLINT_MTIMECMP = *CLINT_MTIME + 100000;
}

// ---------------- Main ----------------
void main() {
    extern void spin_init(struct spinlock *lk, char *name);
    spin_init(&uart_lock, "uart");

    cpu_single.noff = 0;
    cpu_single.int_ena = 0;

    uart_puts("OS: Booting...\n");
    
    kinit(); 
    uart_puts("OS: Memory Initialized.\n");
    
    // 简单的 kalloc 测试
    void *p = kalloc();
    if(p) {
        uart_puts("OS: kalloc test passed (Allocated Page)\n");
        kfree(p);
    } else {
        uart_puts("PANIC: kalloc failed!\n");
        while(1);
    }
    // 1. 初始化任务系统
    task_os_init();

    // 2. 创建 3 个任务
    task_create(task_a_entry, "TaskA");
    task_create(task_b_entry, "TaskB");
    task_create(task_c_entry, "TaskC");

    // 初始化文件系统
    os_fs_init();

    // 3. 硬件初始化
    timer_init();
    extern void trap_entry();
    asm volatile("csrw mtvec, %0" : : "r" ((uint32_t)trap_entry));

    enable_interrupts();

    uart_puts("OS: Scheduler Initialized. Starting...\n");

    // 启动调度器！
    current_task = &tasks[0];
    tasks[0].state = TASK_RUNNING;
    
    uint32_t mie;
    asm volatile("csrr %0, mie" : "=r" (mie));
    mie |= 0x80;
    asm volatile("csrw mie, %0" : : "r" (mie));

    uint32_t *tmp_sp;
    switch_to(&tmp_sp, tasks[0].sp);

    while(1) {} // Should not reach here
}