#include "timer.h"
#include "task.h" 

volatile uint32_t g_os_tick = 0;
static os_timer_t *timer_list_head = NULL;

void os_tick_hardware_init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitStructure.TIM_Prescaler = (SystemCoreClock / 1000000) - 1;
    TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;           
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;    
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;       
    NVIC_Init(&NVIC_InitStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);  
}

void os_timer_init(os_timer_t *timer, const char *name, os_timer_cb_t timeout_func, void *parameter, uint32_t time_ms, uint8_t flag) {
    if (!timer || !timeout_func) return;
    timer->name = name;
    timer->timeout_func = timeout_func;
    timer->parameter = parameter;
    timer->init_tick = time_ms;
    timer->timeout_tick = 0;
    timer->flag = flag;
    timer->state = OS_TIMER_STATE_STOPPED;
    timer->next = NULL;
}

void os_timer_start(os_timer_t *timer) {
    if (!timer || timer->state == OS_TIMER_STATE_RUNNING) return;
    
    timer->timeout_tick = g_os_tick + timer->init_tick;
    
    // 【内核致命 Bug 修复】：防止同一个定时器重复加入链表，造成自己指向自己的“死环”！
    os_timer_t *curr = timer_list_head;
    uint8_t is_in_list = 0;
    while (curr) {
        if (curr == timer) {
            is_in_list = 1;
            break;
        }
        curr = curr->next;
    }
    
    // 只有它不在链表中时，才执行头插法
    if (!is_in_list) {
        timer->next = timer_list_head;
        timer_list_head = timer;
    }

    timer->state = OS_TIMER_STATE_RUNNING;
}

void os_timer_stop(os_timer_t *timer) {
    if (!timer || timer->state == OS_TIMER_STATE_STOPPED) return;
    timer->state = OS_TIMER_STATE_STOPPED;
    
    // 【优化】：为了系统的极致稳定性，我们不再从链表中强行剔除节点。
    // 让它安静地躺在链表里，只要状态是 STOPPED，就不会被触发，安全可靠。
}

void os_timer_ticks_update(void) {
    g_os_tick++;
    os_timer_t *current = timer_list_head;
    while (current) {
        if (current->state == OS_TIMER_STATE_RUNNING) {
            if ((int32_t)(g_os_tick - current->timeout_tick) >= 0) {
                
                // 【细节狂魔】：必须先修改状态，再去执行回调函数！
                // 防止回调函数内部尝试重启定时器时，状态被错误地覆盖掉。
                if (current->flag == OS_TIMER_FLAG_ONE_SHOT) {
                    current->state = OS_TIMER_STATE_STOPPED;
                } else {
                    current->timeout_tick = g_os_tick + current->init_tick;
                }
                
                // 执行异步回调
                current->timeout_func(current->parameter);
            }
        }
        current = current->next;
    }
}

void os_delay(uint32_t ms) {
    uint32_t target_tick = g_os_tick + ms;
    while ((int32_t)(g_os_tick - target_tick) < 0) {
        task_yield(); 
    }
}