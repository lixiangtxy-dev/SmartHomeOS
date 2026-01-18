#include "os.h"

#define CLINT_MTIMECMP (volatile uint64_t *)0x2004000
#define CLINT_MTIME    (volatile uint64_t *)0x200bff8
#define CAUSE_MACHINE_TIMER_INTERRUPT 0x80000007

void timer_handler() {
    // 重设闹钟，下一次中断在 1 秒后
    *CLINT_MTIMECMP = *CLINT_MTIME + 10000000;
}

void trap_handler(uintptr_t mcause, uintptr_t mepc) {
    if (mcause == CAUSE_MACHINE_TIMER_INTERRUPT) {
        timer_handler();
        struct task *prev = current_task;
        struct task *next = next_task;
        current_task = next;
        next_task = prev;
        uart_puts("\n[!] Timer Interrupt: Switching Task...\n"); 

        switch_to(&prev->sp, next->sp);
    }
}