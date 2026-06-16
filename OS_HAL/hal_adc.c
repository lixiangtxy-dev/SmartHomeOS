#include "hal_adc.h"
#include "debug.h"    // 沁恒提供的头文件，包含原厂库函数和 Delay_Ms
#include "mutex.h"    // 引入 OS 的互斥锁机制

// 把互斥锁定义为 static，隐藏在文件内部
// 外部文件无法直接访问它，只能通过 os_hal_adc_read 间接使用，这保证了绝对的安全。
static mutex_t adc_mutex;

void os_hal_adc_core_init(void)
{
    ADC_InitTypeDef ADC_InitStructure={0};

    mutex_init(&adc_mutex);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);

    // 5. ADC 硬件零点校准
    ADC_BufferCmd(ADC1, DISABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    ADC_BufferCmd(ADC1, ENABLE);
    
    printf("[HAL] ADC Core and Mutex Initialized.\r\n");
}

void os_hal_adc_channel_init(const adc_hw_config_t *config) {
    GPIO_InitTypeDef GPIO_InitStructure={0};

    // 机器根据传入的“配方”干活
    RCC_APB2PeriphClockCmd(config->rcc_gpio_clock, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = config->gpio_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(config->gpio_port, &GPIO_InitStructure);
}

uint16_t os_hal_adc_read(uint8_t channel)
{
    uint32_t temp_val = 0;
    
    // ================= 临界区开始 =================
    // 获取硬件独占锁。如果其他任务正在读 ADC，当前任务会自动 yield 让出 CPU
    mutex_lock(&adc_mutex);

    // 连续采样 10 次求平均值 (软件滤波)
    for(uint8_t t = 0; t < 10; t++)
    {
        // 配置当前要采集的通道
        ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_239Cycles5 );
        
        // 触发转换
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);

        // 等待转换完成
        while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));

        // 累加数值
        temp_val += ADC_GetConversionValue(ADC1);
        
        // 短暂延时，让 ADC 采样电容稳定
        Delay_Ms(5); 
    }
    
    // 释放硬件独占锁
    mutex_unlock(&adc_mutex);
    // ================= 临界区结束 =================

    // 返回平均值
    return (uint16_t)(temp_val / 10);
}