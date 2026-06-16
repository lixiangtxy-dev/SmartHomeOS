#include "debug.h"
#include "task.h"
#include "mempool.h"
#include "mutex.h"
#include <math.h>

// NTC 热敏电阻的物理参数 (请根据你的传感器手册修改)
#define NTC_R25       10000.0f  // 25℃ 时的标准阻值 (10K)
#define NTC_B_VALUE   3950.0f   // NTC 的 B 值常数
#define SERIES_R      10000.0f  // 模块上串联的分压电阻阻值 (10K)
mutex_t adc_mutex;
// 定义两个任务控制块
task_t task1;
task_t task2;
task_t task3;

volatile float g_current_temp = 0.0f;

// 我们需要一个指针来保存主函数(main)的栈，以便第一次切出去
uint32_t *main_sp;

/*********************************************************************
 * @fn      ADC_Hardware_Init
 * @brief   初始化 PC1 和 PC2 为 ADC 采集引脚
 */
void ADC_Hardware_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure={0};
    GPIO_InitTypeDef GPIO_InitStructure={0};

    // 1. 开启 GPIOC 和 ADC1 的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_ADC1, ENABLE );
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    // 2. 配置 PC1 和 PC2 为模拟输入模式 (AIN)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 3. 初始化 ADC1 参数
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;       // 关闭扫描模式，我们将手动切换通道
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // 单次转换模式
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);

    // 4. ADC 硬件校准 (滤除硬件零点误差)
    ADC_BufferCmd(ADC1, DISABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    ADC_BufferCmd(ADC1, ENABLE);
}

/*********************************************************************
 * @fn      LED_Hardware_Init
 * @brief   初始化 PC3 为推挽输出，用于驱动 LED 指示灯
 */
void LED_Hardware_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    // 【修复 Bug】：因为你用的是 PC3，所以这里必须开启 GPIOC 的时钟，而不是 GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    // 配置 PC3 为推挽输出模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 默认关闭 LED (输出低电平)
    GPIO_SetBits(GPIOC, GPIO_Pin_3); 
}

/**
 * @brief 将 ADC 刻度值转换为摄氏度
 * @param adc_raw 单片机读到的原始 ADC 值 (0 ~ 4095)
 * @return float 摄氏度
 */
float Convert_ADC_To_Celsius(uint16_t adc_raw) {
    // 1. 防止除零错误 (ADC 不能为 0 或 4095)
    if (adc_raw == 0) return -273.15f; 
    if (adc_raw == 4095) return 999.0f; // 传感器可能开路或短路

    float current_R = SERIES_R * ((float)adc_raw / (4095.0f - (float)adc_raw));

    // 3. 使用 B 值公式计算温度 (开尔文)
    float temp_kelvin = 1.0f / ( (1.0f / 298.15f) + (1.0f / NTC_B_VALUE) * log(current_R / NTC_R25) );

    // 4. 将开尔文转换为摄氏度
    float temp_celsius = temp_kelvin - 273.15f;

    return temp_celsius;
}

/*********************************************************************
 * @fn      Get_ADC_Val
 * @brief   获取单次 ADC 转换结果
 */
u16 Get_ADC_Val(u8 ch)
{
    u16 val;
    // 配置当前要采集的通道
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    // 等待转换完成
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));

    val = ADC_GetConversionValue(ADC1);
    return val;
}

/*********************************************************************
 * @fn      Get_ADC_Average
 * @brief   多次采样求平均值 (软件滤波)
 */
u16 Get_ADC_Average(u8 ch, u8 times)
{
    u32 temp_val=0;
    for(u8 t=0; t<times; t++)
    {
        temp_val += Get_ADC_Val(ch);
        Delay_Ms(5);
    }
    return temp_val / times;
}


// --- 任务 1：热敏传感器采集 ---
void task1_entry(void) {
    u16 adc_value;
    float current_temp_c;

    while(1) {
        mutex_lock(&adc_mutex);
        adc_value = Get_ADC_Average(ADC_Channel_12, 10);
        current_temp_c = Convert_ADC_To_Celsius(adc_value);
        
        // 【新增】：将局部温度同步到全局变量，供其他任务使用
        g_current_temp = current_temp_c; 
        
        int temp_int = (int)current_temp_c; 
        int temp_frac = (int)((current_temp_c - temp_int) * 100); 
        if (temp_frac < 0) temp_frac = -temp_frac; 
        
        printf("[Task 1] 室内温度(PC2): %d.%02d ℃ (ADC: %04d)\r\n", temp_int, temp_frac, adc_value);
        mutex_unlock(&adc_mutex); 
        
        Delay_Ms(500); 
        task_yield(); 
    }
}

// --- 任务 2：光敏传感器采集 (PC1 -> Channel 11) ---
void task2_entry(void) {
    u16 adc_value;
    u32 voltage_mv;

    while(1) {
        // 轻松拿锁
        mutex_lock(&adc_mutex);
        
        adc_value = Get_ADC_Average(ADC_Channel_11, 10);
        voltage_mv = (adc_value * 3300) / 4096;
        
        printf("[Task 2] 光敏传感(PC1): ADC=%04d, 电压=%d mV\r\n", adc_value, voltage_mv);
        
        mutex_unlock(&adc_mutex);
        
        Delay_Ms(500); 
        
        // 优雅地交出 CPU
        task_yield(); 
    }
}

// --- 任务 3：温度报警 LED 控制任务 (PC3) ---
void task3_entry(void) {
    // 静态变量记录 LED 当前状态：0=关，1=开
    static uint8_t led_is_on = 0; 

    while(1) {
        // 读取由 Task 1 采集并共享的全局温度
        float temp = g_current_temp;

        // 如果温度大于 28 度，且 LED 目前是关着的
        if (temp > 28.0f && led_is_on == 0) {
            GPIO_ResetBits(GPIOC, GPIO_Pin_3); // PC3 输出高电平，点亮报警 LED
            led_is_on = 1;
            
            int t_int = (int)temp; 
            printf(">>> [Task 3] 警告: 温度达到 %d ℃, 报警 LED 已点亮！ <<<\r\n", t_int);
        }
        // 如果温度降到 28 度及以下，且 LED 目前是开着的
        else if (temp <= 28.0f && led_is_on == 1) {
            GPIO_SetBits(GPIOC, GPIO_Pin_3); // PC3 输出低电平，熄灭报警 LED
            led_is_on = 0;
            
            int t_int = (int)temp; 
            printf(">>> [Task 3] 恢复: 温度降至 %d ℃, 报警 LED 已关闭。 <<<\r\n", t_int);
        }

        // 检查周期可以稍微短一点，保证响应迅速
        Delay_Ms(200); 
        
        // 判定完毕，让出 CPU 给其他任务
        task_yield(); 
    }
}

int main(void)
{
    // 系统级硬件初始化
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);      
    
    printf("系统启动中...\r\n");

    // 1. 初始化 ADC 硬件引脚
    ADC_Hardware_Init();
    LED_Hardware_Init();
    // 2. 初始化互斥锁
    mutex_init(&adc_mutex);
    
    // 3. 初始化内存池并分配任务
    stack_pool_init();
    
    uint32_t *stack1_ptr = (uint32_t*)stack_pool_alloc();
    task_init(&task1, task1_entry, stack1_ptr, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *stack2_ptr = (uint32_t*)stack_pool_alloc();
    task_init(&task2, task2_entry, stack2_ptr, TASK_STACK_SIZE_BYTES / 4);

    uint32_t *stack3_ptr = (uint32_t*)stack_pool_alloc();
    task_init(&task3, task3_entry, stack3_ptr, TASK_STACK_SIZE_BYTES / 4);

	task_register(&task1);
    task_register(&task2);
	task_register(&task3);
    printf("任务创建完毕，启动调度器...\r\n");
    
    // 4. 启动系统
    os_start();
    
    // 永远不会执行到这里
    while(1) {}
}