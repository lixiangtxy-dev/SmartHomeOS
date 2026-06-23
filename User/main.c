#include "debug.h"
#include "ch32v30x.h"
#include "task.h"
#include "mempool.h"
#include "shadow.h"
#include "drv_smarthome.h" 
#include "drv_w25q80.h"  // 这里修正了头文件名，通常是 drv_w25qxx.h
#include "drv_at.h"
#include "fs_port.h"
#include "sys_config.h" 
#include "app_tasks.h" // 引入应用层任务
#include <stdio.h> 

extern void drv_oled_register(void);

// 测试函数：开机计数器
void test_littlefs_boot_counter(void) {
    lfs_file_t file;
    uint32_t boot_count = 0;
    
    printf("\r\n--- 文件系统持久化测试开始 ---\r\n");
    int err = lfs_file_open(&lfs, &file, "boot_count.txt", LFS_O_RDWR | LFS_O_CREAT);
    if (err < 0) {
        printf("[FS 测试] 打开文件失败，错误码：%d\r\n", err);
        return;
    }

    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));
    boot_count += 1; 
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));
    lfs_file_close(&lfs, &file);
    
    printf("================================\r\n");
    printf(" ? SmartHomeOS 历史启动总次数: %d \r\n", boot_count);
    printf("================================\r\n\r\n");
}

int main(void)
{
    // 1. 基础时钟与终端串口初始化
    SystemCoreClockUpdate();
    Delay_Init(); 
    USART_Printf_Init(115200);   
    printf("\r\n================================\r\n");
    printf("     SmartHomeOS 启动中...      \r\n");
    printf("================================\r\n");

    // 2. 硬件层探测与初始化
    uint16_t flash_id = drv_w25qxx_init();
    printf("探测到 SPI Flash ID: 0x%04X\r\n", flash_id);
    
    os_shadow_init();
    drv_smarthome_init();
    drv_oled_register();
    drv_at_init();
    printf("硬件驱动与设备节点挂载完成。\r\n");
    
    // 3. 文件系统与配置初始化
    drv_fs_init();
    test_littlefs_boot_counter();
    sys_config_init(); // 读取 Wi-Fi 和 MQTT 配置

    // 4. 操作系统核心组件初始化
    os_tick_hardware_init();
    stack_pool_init();

    // 5. 应用层业务逻辑注册
    app_tasks_init();

    // 6. 接管 CPU，移交调度器
    printf("系统初始化完毕，启动 OS 内核...\r\n");
    os_start();
    
    while(1) {}
}