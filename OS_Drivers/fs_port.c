#include "fs_port.h"
#include "drv_w25q80.h"
#include <stdio.h>

lfs_t lfs;

// 声明刚才在驱动里新增的解锁函数
extern void drv_w25qxx_unprotect(void);

static int lfs_w25qxx_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    drv_w25qxx_read((uint8_t *)buffer, block * c->block_size + off, size);
    return 0; 
}

static int lfs_w25qxx_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = block * c->block_size + off;
    uint8_t *buf = (uint8_t *)buffer;
    while (size > 0) {
        uint16_t write_len = (size > 256) ? 256 : size;
        drv_w25qxx_write_page(buf, addr, write_len);
        addr += write_len; buf += write_len; size -= write_len;
    }
    return 0;
}

static int lfs_w25qxx_erase(const struct lfs_config *c, lfs_block_t block) {
    drv_w25qxx_erase_sector(block);
    return 0;
}

static int lfs_w25qxx_sync(const struct lfs_config *c) { return 0; }

const struct lfs_config cfg = {
    .read  = lfs_w25qxx_read,
    .prog  = lfs_w25qxx_prog,
    .erase = lfs_w25qxx_erase,
    .sync  = lfs_w25qxx_sync,
    .read_size = 1,          
    .prog_size = 256,        
    .block_size = 4096,      
    .block_count = 256,      
    .cache_size = 256,       
    .lookahead_size = 16,    
    .block_cycles = 500,     
};

void drv_fs_init(void) {
    uint16_t flash_id = drv_w25qxx_init();
    printf("[FS] 探测到 SPI Flash ID: 0x%04X\r\n", flash_id);
    
    // 【拦截器 1】强行解锁芯片保护
    drv_w25qxx_unprotect();
    
    // 【拦截器 2】在挂载文件系统前，执行最底层的暴力读写测试！
    printf("[FS] 正在执行 Flash 裸机物理层自检...\r\n");
    uint8_t test_write[4] = {0x12, 0x34, 0x56, 0x78};
    uint8_t test_read[4]  = {0};
    
    drv_w25qxx_erase_sector(0); 
    drv_w25qxx_write_page(test_write, 0, 4); 
    drv_w25qxx_read(test_read, 0, 4); 
    
    if (test_read[0] != 0x12 || test_read[3] != 0x78) {
        printf("[FS] ? 致命错误：Flash 物理写失败！读出: %02X %02X %02X %02X\r\n", test_read[0], test_read[1], test_read[2], test_read[3]);
        printf(">>>>>>>> 救援指南 <<<<<<<<\r\n");
        printf("芯片读写被锁死了！请务必检查模块的 WP 和 HOLD 引脚是否接到了 3.3V？绝不能悬空！\r\n");
        return; // 物理层没过，坚决不挂载文件系统，防止报错刷屏
    }
    printf("[FS] ? 物理层自检通过，Flash 擦写功能完全正常！\r\n");

    // 自检通过，正式挂载 LittleFS
    printf("[FS] 正在挂载 LittleFS 文件系统...\r\n");
    int err = lfs_mount(&lfs, &cfg);

    if (err) {
        printf("[FS] 这是全新芯片，正在执行全盘格式化...\r\n");
        err = lfs_format(&lfs, &cfg);
        if (err) {
            printf("[FS] 致命错误：格式化失败！\r\n"); return;
        }
        err = lfs_mount(&lfs, &cfg);
        if (err) {
            printf("[FS] 致命错误：格式化后挂载失败！\r\n"); return;
        }
    }
    printf("[FS] LittleFS 挂载成功！系统已具备掉电保存能力。\r\n");
}