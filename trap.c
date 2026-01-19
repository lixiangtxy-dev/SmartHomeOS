#include "os.h"

#define CLINT_MTIMECMP (volatile uint64_t *)0x2004000
#define CLINT_MTIME    (volatile uint64_t *)0x200bff8
#define CAUSE_MACHINE_TIMER_INTERRUPT 0x80000007

void timer_handler() {
    *CLINT_MTIMECMP = *CLINT_MTIME + 100000; // 0.01s
}

void trap_handler(uintptr_t mcause, uintptr_t mepc) {
    if (mcause == CAUSE_MACHINE_TIMER_INTERRUPT) {
        timer_handler();
        
        task_yield();
    }
}