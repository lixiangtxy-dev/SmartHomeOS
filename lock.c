#include "os.h"

// ---------------- 辅助函数：控制中断 ----------------
void push_off(void) {
    uint32_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r" (mstatus));
    asm volatile("csrc mstatus, 0x8");
}

// 开启中断 (Set Bit 3 of mstatus)
void pop_off(void) {
    asm volatile("csrs mstatus, 0x8"); 
}

// ---------------- 锁的实现 ----------------
void spin_init(struct spinlock *lk, char *name) {
    lk->locked = 0;
    lk->name = name;
}

void spin_lock(struct spinlock *lk) {
    push_off(); 
    if (lk->locked) {
        uart_puts("PANIC: Deadlock detected!\n");
        while(1);
    }
    lk->locked = 1;
}

void spin_unlock(struct spinlock *lk) {
    lk->locked = 0;
    pop_off();
}