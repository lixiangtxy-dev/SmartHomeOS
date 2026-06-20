#include "drv_w25q80.h"
#include "hal_spi.h"

#define W25X_WriteEnable         0x06
#define W25X_WriteStatusReg      0x01   // 【新增】写状态寄存器指令
#define W25X_ReadStatusReg       0x05
#define W25X_ReadData            0x03
#define W25X_PageProgram         0x02
#define W25X_SectorErase         0x20
#define W25X_ManufactDeviceID    0x90

static void SPI_Flash_Wait_Busy(void) {
    uint8_t status;
    do {
        os_hal_spi_cs(0);
        os_hal_spi_rw_byte(W25X_ReadStatusReg); 
        status = os_hal_spi_rw_byte(0xFF); 
        os_hal_spi_cs(1);
    } while((status & 0x01) == 0x01); 
}

static void SPI_FLASH_Write_Enable(void) {
    os_hal_spi_cs(0);
    os_hal_spi_rw_byte(W25X_WriteEnable); 
    os_hal_spi_cs(1);
}

// ==========================================
// 【核心修复】：强行解除出厂的区块写保护
// ==========================================
void drv_w25qxx_unprotect(void) {
    SPI_FLASH_Write_Enable();
    os_hal_spi_cs(0);
    os_hal_spi_rw_byte(W25X_WriteStatusReg); 
    os_hal_spi_rw_byte(0x00); // 写入 0x00，彻底清除 BP0/BP1/BP2 等保护位
    os_hal_spi_cs(1);
    SPI_Flash_Wait_Busy();
}

uint16_t drv_w25qxx_init(void) {
    uint16_t Temp = 0;
    os_hal_spi_init(); 

    os_hal_spi_cs(0);
    os_hal_spi_rw_byte(W25X_ManufactDeviceID); 
    os_hal_spi_rw_byte(0x00);
    os_hal_spi_rw_byte(0x00);
    os_hal_spi_rw_byte(0x00); 
    Temp |= os_hal_spi_rw_byte(0xFF) << 8; 
    Temp |= os_hal_spi_rw_byte(0xFF); 
    os_hal_spi_cs(1);

    return Temp;
}

void drv_w25qxx_read(uint8_t *pBuffer, uint32_t ReadAddr, uint16_t size) {
    uint16_t i;
    os_hal_spi_cs(0);
    os_hal_spi_rw_byte(W25X_ReadData); 
    os_hal_spi_rw_byte((uint8_t)((ReadAddr) >> 16)); 
    os_hal_spi_rw_byte((uint8_t)((ReadAddr) >> 8)); 
    os_hal_spi_rw_byte((uint8_t)ReadAddr); 
    for(i = 0; i < size; i++) pBuffer[i] = os_hal_spi_rw_byte(0XFF); 
    os_hal_spi_cs(1);
}

void drv_w25qxx_write_page(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t size) {
    uint16_t i;
    SPI_FLASH_Write_Enable(); 
    os_hal_spi_cs(0);
    os_hal_spi_rw_byte(W25X_PageProgram); 
    os_hal_spi_rw_byte((uint8_t)((WriteAddr) >> 16)); 
    os_hal_spi_rw_byte((uint8_t)((WriteAddr) >> 8)); 
    os_hal_spi_rw_byte((uint8_t)WriteAddr); 
    for(i = 0; i < size; i++) os_hal_spi_rw_byte(pBuffer[i]); 
    os_hal_spi_cs(1);
    SPI_Flash_Wait_Busy(); 
}

void drv_w25qxx_erase_sector(uint32_t sector_num) {
    uint32_t Addr = sector_num * 4096; 
    SPI_FLASH_Write_Enable(); 
    os_hal_spi_cs(0);
    os_hal_spi_rw_byte(W25X_SectorErase); 
    os_hal_spi_rw_byte((uint8_t)((Addr) >> 16)); 
    os_hal_spi_rw_byte((uint8_t)((Addr) >> 8)); 
    os_hal_spi_rw_byte((uint8_t)Addr); 
    os_hal_spi_cs(1);
    SPI_Flash_Wait_Busy(); 
}