#ifndef __HAL_FLASH_H__
#define __HAL_FLASH_H__

#include <stdint.h>

// 标准的 Flash 操作接口
int hal_flash_init(void);
int hal_flash_read(uint32_t addr, uint32_t size, uint8_t *buffer);
int hal_flash_prog(uint32_t addr, uint32_t size, const uint8_t *buffer);
int hal_flash_erase(uint32_t addr, uint32_t size);
int hal_flash_sync(void);

#endif