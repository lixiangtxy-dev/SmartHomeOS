#include "debug.h"


/* Global typedef */

/* Global define */

/* Global Variable */
// 基于SYSTICK的毫秒级延时函数

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

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	SystemCoreClockUpdate();
	Delay_Init();
	USART_Printf_Init(115200);	
	printf("SystemClk:%d\r\n",SystemCoreClock);
	printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );
	printf("This is printf example\r\n");
	
	// 2. 硬件外设初始化
    LED_Init();

    // 4. 业务逻辑 (此时由纯正的 OS 节拍驱动)
    while(1) {
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET); // 点亮 PC1
        delay_Ms(500);                               
        
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_SET);   // 熄灭 PC1
        delay_Ms(500);      
		
		GPIO_WriteBit(GPIOC, GPIO_Pin_2, Bit_RESET); // 点亮 PC1
        delay_Ms(500);                               
        
        GPIO_WriteBit(GPIOC, GPIO_Pin_2, Bit_SET);   // 熄灭 PC1
        delay_Ms(500);                                
    }
}

