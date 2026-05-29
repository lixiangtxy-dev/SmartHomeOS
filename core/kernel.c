#include "os.h"
#include "sensor.h" // 告诉编译器 current_env 和 sensor_init 在这里
#include "string.h" // 告诉编译器 strcat, itoa, strlen 在这里
// ---------------- 硬件定义 ----------------
#define UART0_DR   ((volatile unsigned char *)0x10000000)
#define CLINT_MTIMECMP (volatile uint64_t *)0x2004000
#define CLINT_MTIME    (volatile uint64_t *)0x200bff8
// 全局锁
struct spinlock uart_lock;
struct cpu cpu_single;//单核 CPU 实例

struct cpu* mycpu() {
    return &cpu_single;
}

// ---------------- 辅助函数 ----------------
void uart_putc(char c) { *UART0_DR = c; }
void uart_puts(char *s) { while (*s) uart_putc(*s++); }
void delay(volatile int count) { while (count--) {} }

void enable_interrupts() {
    uint32_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r" (mstatus));
    mstatus |= 0x8; 
    asm volatile("csrw mstatus, %0" : : "r" (mstatus));
}

// 安全打印函数
void slow_safe_print(char *s) {
    spin_lock(&uart_lock);
    while (*s) {
        uart_putc(*s++);
        delay(1000000); 
    }
    uart_putc('\n');
    spin_unlock(&uart_lock);
}

// ---------------- 任务函数 ----------------
void task_a_entry() {
    enable_interrupts();
    while (1) {
        // 分析全局传感器数据
        if (current_env.smoke_alarm) {
            slow_safe_print(">>> ALARM: Smoke Detected! Turn on exhaust fan! <<<");
        } else if (current_env.temperature > 30) {
            slow_safe_print(">>> INFO: Temp > 30C, Turn on AC. <<<");
        }
        
        delay(3000000); // 检查频率较高
    }
}

void task_b_entry() {
    enable_interrupts();
    char log_buf[64];
    char temp_str[16];
    
    while (1) {
        // 1. 读取传感器 (更新随机数)
        sensor_read_all();
        
        // 2. 纯 C 语言拼接字符串 (模拟 sprintf)
        // 目标格式: "T:25 H:50 S:0\n"
        log_buf[0] = '\0'; // 清空字符串
        strcat(log_buf, "T:");
        itoa(current_env.temperature, temp_str);
        strcat(log_buf, temp_str);
        
        strcat(log_buf, " H:");
        itoa(current_env.humidity, temp_str);
        strcat(log_buf, temp_str);
        
        strcat(log_buf, " S:");
        itoa(current_env.smoke_alarm, temp_str);
        strcat(log_buf, temp_str);
        strcat(log_buf, "\n"); // 换行符很重要
        
        // 3. 打印并存入 LittleFS
        //slow_safe_print("Task B: Saved sensor data to Flash.");
        os_file_append("env.log", log_buf, strlen(log_buf));
        
        delay(4000000); // 采集频率较慢
    }
}

void task_c_entry() {
    enable_interrupts();
    char read_buf[256]; // 一次性读大一点
    
    while (1) {
        delay(4000000); // 很久才上传一次云端
        
        //slow_safe_print("Task C: Connecting to Cloud...");
        
        // 一次性读取历史日志
        int len = os_file_read("env.log", read_buf, 255);
        if (len > 0) {
            read_buf[len] = '\0'; 
            //slow_safe_print("---- Cloud Upload Start ----");
            //slow_safe_print(read_buf);
            //slow_safe_print("---- Cloud Upload Done ----");
        }
    }
}

// ================= 任务 D: TUI 实时高级数据大屏 =================
void task_ui_entry() {
    enable_interrupts();
    char num_buf[16];
    
    // 隐藏终端的光标 (\033[?25l)，并做一次彻底清屏 (\033[2J)
    uart_puts("\033[?25l\033[2J"); 

    while (1) {
        spin_lock(&uart_lock); 
        
        // 【防闪烁魔法】 \033[H 将光标移回左上角 (0,0) 进行覆盖刷新，而不是清屏
        uart_puts("\033[H");
        
        // ---------------- 面板头部 (青色加粗) ----------------
        uart_puts("\033[1;36m"); 
        uart_puts("╔══════════════════════════════════════════════════╗\n");
        uart_puts("║            SmartHomeOS Edge Dashboard            ║\n");
        uart_puts("╚══════════════════════════════════════════════════╝\n");
        uart_puts("\033[0m\n"); // 恢复默认
        
        // ---------------- 1. 温度模块 ----------------
        uart_puts("\033[1m [ 🌡️ Temp ] \033[0m"); 
        itoa(current_env.temperature, num_buf);
        
        if (current_env.temperature > 30) {
            uart_puts("\033[1;31m"); // 超温：红色文字
        } else {
            uart_puts("\033[1;32m"); // 正常：绿色文字
        }
        
        // 对齐排版（个位数补空格）
        if(current_env.temperature < 10) uart_puts(" "); 
        uart_puts(num_buf);
        uart_puts(" C  \033[0m| ");
        
        // 温度色块进度条 (假设满格 40 度，40个字符宽)
        for(int i = 0; i < 40; i++) {
            if (i < current_env.temperature) {
                if (current_env.temperature > 30) 
                    uart_puts("\033[41m \033[0m"); // 红底空格(色块)
                else 
                    uart_puts("\033[42m \033[0m"); // 绿底空格(色块)
            } else {
                uart_puts("\033[47m \033[0m");     // 灰白底(背景槽)
            }
        }
        uart_puts("\n\n");

        // ---------------- 2. 湿度模块 ----------------
        uart_puts("\033[1m [ 💧 Hum  ] \033[0m");
        itoa(current_env.humidity, num_buf);
        
        uart_puts("\033[1;34m"); // 蓝色文字
        uart_puts(num_buf);
        uart_puts(" %  \033[0m| ");
        
        // 湿度色块进度条 (假设满格 100%，换算成 40 格，即 humidity * 40 / 100)
        int hum_blocks = (current_env.humidity * 4) / 10;
        for(int i = 0; i < 40; i++) {
            if (i < hum_blocks) {
                uart_puts("\033[44m \033[0m"); // 蓝底空格(色块)
            } else {
                uart_puts("\033[47m \033[0m"); // 灰白底(背景槽)
            }
        }
        uart_puts("\n\n");

        // ---------------- 3. 烟雾警报模块 ----------------
        uart_puts("\033[1m [ 🔥 Fire ] \033[0m");
        
        if (current_env.smoke_alarm) {
            // 警报高亮：红底白字加粗
            uart_puts("\033[1;37;41m   !!! ALARM !!! EVACUATE IMMEDIATELY !!!   \033[0m\n");
        } else {
            // 正常状态：绿字
            uart_puts("\033[1;32m   All Systems Normal. Safe.                \033[0m\n");
        }
        uart_puts("\n");
        
        // 底部边框 (深灰色)
        uart_puts("\033[1;90m────────────────────────────────────────────────────\033[0m\n");
        
        spin_unlock(&uart_lock); 
        
        delay(10000000); // UI 刷新频率控制
    }
}

void timer_init() {
    *CLINT_MTIMECMP = *CLINT_MTIME + 100000;
}

// ---------------- Main ----------------
void main() {
    extern void spin_init(struct spinlock *lk, char *name);
    spin_init(&uart_lock, "uart");

    cpu_single.noff = 0;
    cpu_single.int_ena = 0;

    uart_puts("OS: Booting...\n");
    
    kinit(); 
    uart_puts("OS: Memory Initialized.\n");
    
    // 简单的 kalloc 测试
    void *p = kalloc();
    if(p) {
        uart_puts("OS: kalloc test passed (Allocated Page)\n");
        kfree(p);
    } else {
        uart_puts("PANIC: kalloc failed!\n");
        while(1);
    }
    // 1. 初始化任务系统
    task_os_init();
    os_fs_init();
    sensor_init();

    // 2. 创建 3 个任务
    task_create(task_a_entry, "TaskA");
    task_create(task_b_entry, "TaskB");
    task_create(task_c_entry, "TaskC");
    task_create(task_ui_entry, "TaskUI");

    // 3. 硬件初始化
    timer_init();
    extern void trap_entry();
    asm volatile("csrw mtvec, %0" : : "r" ((uint32_t)trap_entry));

    enable_interrupts();

    uart_puts("OS: Scheduler Initialized. Starting...\n");

    // 启动调度器！
    current_task = &tasks[0];
    tasks[0].state = TASK_RUNNING;
    
    uint32_t mie;
    asm volatile("csrr %0, mie" : "=r" (mie));
    mie |= 0x80;
    asm volatile("csrw mie, %0" : : "r" (mie));

    uint32_t *tmp_sp;
    switch_to(&tmp_sp, tasks[0].sp);

    while(1) {} // Should not reach here
}