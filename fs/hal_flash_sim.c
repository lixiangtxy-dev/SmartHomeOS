#include "hal_flash.h"
#include "os.h"

// 模拟一块 2MB 的 SPI Flash (如 W25Q16)
#define SIM_FLASH_SIZE (2 * 1024 * 1024)
#define SECTOR_SIZE 4096

// 在 BSS 段静态分配一块大内存当假硬盘 (QEMU 专属特权)
static uint8_t sim_flash[SIM_FLASH_SIZE];

int hal_flash_init(void) {
    uart_puts("HAL: Simulated SPI Flash Initialized (2MB).\n");
    // 如果想要每次开机都是空盘，可以在这里用 memset 填满 0xFF
    return 0;
}

int hal_flash_read(uint32_t addr, uint32_t size, uint8_t *buffer) {
    if (addr + size > SIM_FLASH_SIZE) return -1; // 防越界
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = sim_flash[addr + i];
    }
    return 0;
}

int hal_flash_prog(uint32_t addr, uint32_t size, const uint8_t *buffer) {
    if (addr + size > SIM_FLASH_SIZE) return -1;
    // SPI Flash 写入特性：只能把 1 变成 0，这里模拟这个特性
    for (uint32_t i = 0; i < size; i++) {
        sim_flash[addr + i] &= buffer[i]; 
    }
    return 0;
}

int hal_flash_erase(uint32_t addr, uint32_t size) {
    if (addr + size > SIM_FLASH_SIZE) return -1;
    // SPI Flash 擦除后数据全为 0xFF
    for (uint32_t i = 0; i < size; i++) {
        sim_flash[addr + i] = 0xFF;
    }
    return 0;
}

int hal_flash_sync(void) {
    return 0; // 内存模拟不需要同步等待
}