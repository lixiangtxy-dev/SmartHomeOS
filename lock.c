#include "os.h"

// ---------------- 控制中断 ----------------
void push_off(void) {
    int old_int_enable;
    uint32_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r" (mstatus));
    old_int_enable = (mstatus & 0x8) != 0;
    asm volatile("csrc mstatus, 0x8");//关中断
    struct cpu *c = mycpu();
    if(c->noff == 0) {
        c->int_ena = old_int_enable;
    }
    c->noff += 1;
}

void pop_off(void) {
    struct cpu *c = mycpu();
    // 如果中断已经开着，说明 pop_off 调用多了，或者逻辑乱了
    uint32_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r" (mstatus));
    if(mstatus & 0x8) {
        uart_puts("PANIC: pop_off - interrupts already enabled\n");
        while(1);
    }
    // 检查嵌套计数
    if(c->noff < 1) {
        uart_puts("PANIC: pop_off - count underflow\n");
        while(1);
    }
    // 1. 减少嵌套深度
    c->noff -= 1;
    // 2. 只有当嵌套深度归零，且之前中断是开启的，才真正开中断
    if(c->noff == 0 && c->int_ena) {
        asm volatile("csrs mstatus, 0x8");
    }
}

// ---------------- 自旋锁 ----------------
void spin_init(struct spinlock *lk, char *name) {
    lk->locked = 0;
    lk->name = name;
    lk->cpu = 0;
}

void spin_lock(struct spinlock *lk) {
    push_off(); 
    if(lk->locked) {
        uart_puts("PANIC: Recursive lock or Deadlock on: ");
        uart_puts(lk->name);
        uart_puts("\n");
    }
    int count = 0;
    // 如果返回 1，说明锁被占用，继续循环。
    while(__sync_lock_test_and_set(&lk->locked, 1) != 0) {
        // 可视化等待：每 10000 次循环打印一个点
        if (++count % 1000000 == 0) {
            uart_putc('.'); 
        }
    }
    lk->cpu = mycpu();
    __sync_synchronize();
}

void spin_unlock(struct spinlock *lk) {
// 1. 内存屏障
    __sync_synchronize();

    // 2. 清空持有者信息
    lk->cpu = 0;

    // 3. 原子释放锁
    __sync_lock_release(&lk->locked);

    // 4. 恢复中断状态
    pop_off();
}