#ifndef __DRV_W25QXX_H
#define __DRV_W25QXX_H

#include <stdint.h>

// 初始化 Flash，返回芯片 ID (如 0xEF13 代表 W25Q80)
uint16_t drv_w25qxx_init(void);

// 连续读取数据
void drv_w25qxx_read(uint8_t *pBuffer, uint32_t ReadAddr, uint16_t size);

// 页编程 (写入前该页必须已被擦除，size 最大 256 字节)
void drv_w25qxx_write_page(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t size);

// 擦除指定的一个扇区 (4KB 大小)
// 注意参数是扇区号，不是实际地址！(如擦除扇区1，传入 1，实际地址是 0x1000)
void drv_w25qxx_erase_sector(uint32_t sector_num);

#endif