#ifndef __HAL_SPI_H
#define __HAL_SPI_H

#include "ch32v30x.h"

// 初始化 SPI1 外设及引脚
void os_hal_spi_init(void);

// 片选信号控制 (0=选中, 1=释放)
void os_hal_spi_cs(uint8_t state);

// SPI 底层收发一个字节
uint8_t os_hal_spi_rw_byte(uint8_t tx_data);

#endif