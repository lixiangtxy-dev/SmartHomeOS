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

// 用于读取 mstatus 寄存器，检查中断位 (MIE)
static uint32_t r_mstatus() {
    uint32_t x;
    asm volatile("csrr %0, mstatus" : "=r" (x));
    return x;
}

void test_spinlock_logic() {
    uart_puts("\n=== STARTING SPINLOCK TEST ===\n");

    struct cpu *c = mycpu();
    struct spinlock lk1, lk2;
    spin_init(&lk1, "test_lock_1");
    spin_init(&lk2, "test_lock_2");

    // ---------------------------------------------------------
    // 测试 1: 验证中断嵌套计数 (Nesting Count)
    // ---------------------------------------------------------
    uart_puts("[TEST 1] Interrupt Nesting:\n");

    // 初始状态：中断应该是开启的 (在 main 中 enable_interrupts 之后)
    if ((r_mstatus() & 0x8) == 0) uart_puts("  ERROR: Interrupts initially disabled!\n");
    else uart_puts("  Step 0: Interrupts ON (Expected)\n");

    // 第一层关中断
    spin_lock(&lk1); // 内部调用 push_off
    uart_puts("  Step 1 (Lock 1): ");
    if (c->noff == 1 && (r_mstatus() & 0x8) == 0) uart_puts("OK (noff=1, MIE=0)\n");
    else uart_puts("FAIL!\n");

    // 第二层关中断 (模拟嵌套调用)
    spin_lock(&lk2); // 再次调用 push_off
    uart_puts("  Step 2 (Lock 2): ");
    if (c->noff == 2 && (r_mstatus() & 0x8) == 0) uart_puts("OK (noff=2, MIE=0)\n");
    else uart_puts("FAIL!\n");

    // 第一层释放
    spin_unlock(&lk2); // 调用 pop_off
    uart_puts("  Step 3 (Unlock 2): ");
    // 关键点：noff 应该变回 1，且中断必须依然是【关闭】的！
    // 如果没有计数器，这里中断可能就被错误打开了。
    if (c->noff == 1 && (r_mstatus() & 0x8) == 0) uart_puts("OK (noff=1, MIE=0) -> VITAL CHECK PASSED!\n");
    else uart_puts("FAIL! Interrupts opened too early!\n");

    // 第二层释放
    spin_unlock(&lk1); // 再次调用 pop_off
    uart_puts("  Step 4 (Unlock 1): ");
    // 此时 noff 归零，中断应该恢复开启
    if (c->noff == 0 && (r_mstatus() & 0x8) != 0) uart_puts("OK (noff=0, MIE=1)\n");
    else uart_puts("FAIL! Interrupts did not restore.\n");


    // ---------------------------------------------------------
    // 测试 2: 验证死锁可视化 (Visualization)
    // 注意：这将导致系统挂起并无限打印点号，请以此作为测试结束。
    // ---------------------------------------------------------
    uart_puts("\n[TEST 2] Deadlock Visualization:\n");
    uart_puts("  I will now artificially lock 'test_lock_1' and try to acquire it again.\n");
    uart_puts("  Expected Result: You should see endless dots '......' appearing.\n");
    uart_puts("  Starting in 3 seconds...\n");
    
    // 倒计时
    uart_puts("  3..."); delay(10000000);
    uart_puts("2..."); delay(10000000);
    uart_puts("1...\n");

    // 人为制造死锁：
    // 1. 手动将锁状态设为 1 (模拟被其他 CPU 或任务占用)
    lk1.locked = 1; 

    // 2. 尝试获取锁 (这将进入 while 循环)
    uart_puts("  Attempting spin_lock... (Watch for dots!)\n");
    spin_lock(&lk1);

    // 如果代码能走到这里，说明测试失败了（锁逻辑有漏洞）
    uart_puts("  ERROR: spin_lock returned! (Should hang)\n");
}

// ---------------- Main ----------------
void main() {
    extern void spin_init(struct spinlock *lk, char *name);
    spin_init(&uart_lock, "uart");

    cpu_single.noff = 0;
    cpu_single.int_ena = 0;

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

    enable_interrupts();

    uart_puts("OS: Scheduler Initialized. Starting...\n");

    test_spinlock_logic();

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