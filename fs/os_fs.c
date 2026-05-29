#include "lfs.h"
#include "hal_flash.h"
#include "os.h"

// 全局变量
lfs_t lfs;
struct spinlock fs_lock; // 文件系统操作专属锁

// ---------------- 1. 桥接 LittleFS 与 HAL ----------------
static int lfs_hal_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    uint32_t addr = block * c->block_size + off;
    return hal_flash_read(addr, size, buffer);
}

static int lfs_hal_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = block * c->block_size + off;
    return hal_flash_prog(addr, size, buffer);
}

static int lfs_hal_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = block * c->block_size;
    return hal_flash_erase(addr, c->block_size);
}

static int lfs_hal_sync(const struct lfs_config *c) {
    return hal_flash_sync();
}

static uint8_t lfs_read_buf[64];
static uint8_t lfs_prog_buf[64];
static uint8_t lfs_lookahead_buf[16];

// LittleFS 极其克制的内存配置参数 (完美适配 CH32V307 的 64KB 内存)
const struct lfs_config lfs_cfg = {
    .read  = lfs_hal_read,
    .prog  = lfs_hal_prog,
    .erase = lfs_hal_erase,
    .sync  = lfs_hal_sync,
    
    .read_size = 16,
    .prog_size = 16,
    .block_size = 4096, // 4KB 扇区
    .block_count = 512, // 512 * 4KB = 2MB 模拟存储
    .cache_size = 64,   // 仅用 64 字节缓存！极度省内存
    .lookahead_size = 16,
    .block_cycles = 500,

    // --- 新增：把静态数组绑定给 LittleFS ---
    .read_buffer = lfs_read_buf,
    .prog_buffer = lfs_prog_buf,
    .lookahead_buffer = lfs_lookahead_buf,
};

static uint8_t file_work_buf[64]; 

static struct lfs_file_config file_cfg = {
    .buffer = file_work_buf,
    .attrs = 0,
    .attr_count = 0,
};

// ---------------- 2. 操作系统提供的 API ----------------

void os_fs_init() {
    spin_init(&fs_lock, "fs_lock");
    hal_flash_init();

    // 挂载文件系统
    int err = lfs_mount(&lfs, &lfs_cfg);
    
    // 如果挂载失败 (说明是第一次启动，没有文件系统)，则格式化
    if (err) {
        uart_puts("OS FS: Formatting Flash...\n");
        lfs_format(&lfs, &lfs_cfg);
        lfs_mount(&lfs, &lfs_cfg);
    }
    uart_puts("OS FS: LittleFS Mounted Successfully!\n");
}

// 线程安全的追加写文件接口
int os_file_append(const char *path, const char *data, int len) {
    lfs_file_t file;
    
    spin_lock(&fs_lock); 
    
    // 【修改这里】使用 lfs_file_opencfg，并把 file_cfg 传进去
    int err = lfs_file_opencfg(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND, &file_cfg);
    if (err >= 0) {
        lfs_file_write(&lfs, &file, data, len);
        lfs_file_close(&lfs, &file);
    }
    
    spin_unlock(&fs_lock);
    return err;
}

// 修改 os_file_read
int os_file_read(const char *path, char *buffer, int max_len) {
    lfs_file_t file;
    int read_len = -1;

    spin_lock(&fs_lock);
    
    // 【修改这里】同样使用 lfs_file_opencfg
    int err = lfs_file_opencfg(&lfs, &file, path, LFS_O_RDONLY, &file_cfg);
    if (err >= 0) {
        read_len = lfs_file_read(&lfs, &file, buffer, max_len);
        lfs_file_close(&lfs, &file);
    }
    
    spin_unlock(&fs_lock);
    return read_len;
}