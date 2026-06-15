#include "debug.h"
#include "task.h"
#include "mempool.h"
// 定义两个任务控制块
task_t task1;
task_t task2;
task_t task3;

// 我们需要一个指针来保存主函数(main)的栈，以便第一次切出去
uint32_t *main_sp;

// 硬件初始化解耦
void LED_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 默认熄灭
    GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_SET);
    GPIO_WriteBit(GPIOC, GPIO_Pin_2, Bit_SET);
}

// 任务 1 的执行内容
void task1_entry(void) {
    while(1) {
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET); 
        Delay_Ms(500);                               
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_SET);   
        Delay_Ms(500);    
        
        // 主动让出 CPU，切换到任务 2
        cpu_switch(&task1.sp, &task2.sp); 
    }
}

void task2_entry(void) {
    while(1) {
        GPIO_WriteBit(GPIOC, GPIO_Pin_2, Bit_RESET); 
        Delay_Ms(500);                               
        GPIO_WriteBit(GPIOC, GPIO_Pin_2, Bit_SET);   
        Delay_Ms(500);   
        
        // 主动让出 CPU，切换回任务 1
        cpu_switch(&task2.sp, &task3.sp); 
    }
}

// 任务 3：系统监视器 (纯软件逻辑)
void task3_entry(void) {
    uint32_t loop_count = 0;
    while(1) {
        loop_count++;
        // 打印系统运行日志
        printf("[Monitor] System is running normally. Loop count: %d\r\n", loop_count);
        
        // 这里可以加一点小延时避免打印过快，也可以不加
        Delay_Ms(200);
        
        // 执行完当前周期，将 CPU 交回给任务 1，形成闭环！
        cpu_switch(&task3.sp, &task1.sp); 
    }
}
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	SystemCoreClockUpdate();
	Delay_Init();
	USART_Printf_Init(115200);	
	printf("SystemClk:%d\r\n",SystemCoreClock);
	printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );
	printf("This is printf example\r\n");

	LED_Init();
// 1. 初始化内存池
    stack_pool_init();

    // 2. 动态分配栈并初始化任务 1
    uint32_t *stack1_ptr = (uint32_t*)stack_pool_alloc();
    task_init(&task1, task1_entry, stack1_ptr, TASK_STACK_SIZE_BYTES / 4);

    // 3. 动态分配栈并初始化任务 2
    uint32_t *stack2_ptr = (uint32_t*)stack_pool_alloc();
    task_init(&task2, task2_entry, stack2_ptr, TASK_STACK_SIZE_BYTES / 4);

    // 4. 动态分配栈并初始化任务 3
    uint32_t *stack3_ptr = (uint32_t*)stack_pool_alloc();
    task_init(&task3, task3_entry, stack3_ptr, TASK_STACK_SIZE_BYTES / 4);

    printf("3 Tasks created. Starting scheduler...\r\n");

    // 5. 启动系统，切入任务 1
    cpu_switch(&main_sp, &task1.sp);
    while(1) {}
}

