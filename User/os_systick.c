#include "os_systick.h"

volatile uint32_t os_tick_count = 0;

void OS_Tick_Init(void) {
    uint32_t ticks = SystemCoreClock / OS_TICK_RATE_HZ - 1;

    SysTick->SR = 0;
    SysTick->CMP = ticks;
    SysTick->CNT = 0;
    SysTick->CTLR = 0xF;

    NVIC_SetPriority(SysTicK_IRQn, 0);
    NVIC_EnableIRQ(SysTicK_IRQn);
    
    // 【终极排雷】：强制使用寄存器传递参数，绕过青稞内核的立即数 Bug！
    __asm volatile("csrs mstatus, %0" :: "r"(8)); 
}

uint32_t OS_Get_Tick(void) {
    return os_tick_count;
}

void OS_Delay(uint32_t ms) {
    uint32_t target_tick = os_tick_count + ms;
    while (os_tick_count < target_tick) {
        __asm volatile("wfi"); 
    }
}