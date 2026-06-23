#include "drv_at.h"
#include <string.h>
#include <stdio.h>

extern void os_delay(uint32_t ms);

// ==========================================
// 1. 环形缓冲区 (RingBuffer) 实现
// ==========================================
#define RING_BUF_SIZE 1024
typedef struct {
    char buffer[RING_BUF_SIZE];
    volatile uint16_t head; 
    volatile uint16_t tail; 
} ring_buffer_t;

static ring_buffer_t at_rx_buf = {0};

static void ringbuffer_push(char data) {
    uint16_t next_head = (at_rx_buf.head + 1) % RING_BUF_SIZE;
    if (next_head != at_rx_buf.tail) { 
        at_rx_buf.buffer[at_rx_buf.head] = data;
        at_rx_buf.head = next_head;
    }
}

static int ringbuffer_readline(char *line_buf) {
    if (at_rx_buf.head == at_rx_buf.tail) return 0; 
    
    uint16_t temp_tail = at_rx_buf.tail;
    int idx = 0;
    int found_newline = 0;

    while (temp_tail != at_rx_buf.head) {
        char c = at_rx_buf.buffer[temp_tail];
        temp_tail = (temp_tail + 1) % RING_BUF_SIZE;
        if (c == '\n') {
            found_newline = 1;
            break;
        }
    }

    if (!found_newline) return 0; 

    while (at_rx_buf.tail != at_rx_buf.head) {
        char c = at_rx_buf.buffer[at_rx_buf.tail];
        at_rx_buf.tail = (at_rx_buf.tail + 1) % RING_BUF_SIZE;
        
        if (c != '\r' && c != '\n') { 
            line_buf[idx++] = c;
        }
        
        if (c == '\n') {
            line_buf[idx] = '\0'; 
            return 1;
        }
    }
    return 0;
}

// ==========================================
// 2. 硬件串口 4 (UART4) 初始化与中断处理
// 绝对安全的引脚：PC10 = TX, PC11 = RX
// ==========================================
void drv_at_init(void) {
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    NVIC_InitTypeDef  NVIC_InitStructure = {0};

    // 1. 使能 GPIOC 和 UART4 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

    // 2. 配置 PC10 (TX) 为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 3. 配置 PC11 (RX) 为上拉输入 (防中断风暴)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 4. 配置串口参数
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(UART4, &USART_InitStructure);

    // 5. 配置并使能串口接收中断
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
    USART_Cmd(UART4, ENABLE);
}

// RISC-V 中断声明 (对应 UART4)
void UART4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void UART4_IRQHandler(void) {
    if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET) {
        char rx_data = USART_ReceiveData(UART4);
        ringbuffer_push(rx_data); 
        USART_ClearITPendingBit(UART4, USART_IT_RXNE);
    }
}

// 基础串口发送字符串函数
static void uart4_send_str(const char* str) {
    while(*str) {
        while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
        USART_SendData(UART4, *str);
        str++;
    }
}

// ==========================================
// 3. 操作系统层 AT 指令解析核心逻辑
// ==========================================
int at_send_cmd_wait(const char* cmd, const char* expect, uint32_t timeout_ms) {
    char line_buf[256];
    // at_rx_buf.tail = at_rx_buf.head; 

    uart4_send_str(cmd);
    
    uint32_t check_times = timeout_ms / 10; 
    if (check_times == 0) check_times = 1;

    for (uint32_t i = 0; i < check_times; i++) {
        if (ringbuffer_readline(line_buf)) {
            printf("[AT-RX] %s\r\n", line_buf); 

            if (expect != NULL && strstr(line_buf, expect) != NULL) {
                return 0; 
            }
            if (strstr(line_buf, "ERROR") != NULL) {
                return -1; 
            }
        }
        os_delay(10); 
    }
    
    printf("[AT-ERR] 指令超时无响应: %s", cmd);
    return -2; 
}

// 【新增】：安全地提取一个字符，供反向控制拦截器使用
int at_get_char(char *c) {
    // 检查缓冲区是否为空
    if (at_rx_buf.head == at_rx_buf.tail) return 0; 
    
    // 取出尾部数据
    *c = at_rx_buf.buffer[at_rx_buf.tail];
    at_rx_buf.tail = (at_rx_buf.tail + 1) % RING_BUF_SIZE;
    return 1;
}
