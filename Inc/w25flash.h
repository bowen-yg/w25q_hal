#pragma once
#include"spi.h"
#include"stm32f1xx_hal.h"
#define CS_PORT GPIOA
#define CS_PIN GPIO_PIN_4
#define SPI_PORT hspi1
#define __Select_Flash() HAL_GPIO_WritePin(CS_PORT,CS_PIN,0);
#define __Deselect_Flash() HAL_GPIO_WritePin(CS_PORT,CS_PIN,1);
#define FLASH_PAGE_SIZE_F1 256
#define FLASH_SECTOR_SIZE 4096
#define FLASH_SECTOR_COUNT 4096
uint16_t W25Q_ReadID(void);
uint8_t W25Q_ReceiveByte();
HAL_StatusTypeDef W25Q_ReceiveBytes(uint8_t* pData,uint16_t size);
uint8_t W25Q_ReadSR1(void);
uint8_t W25Q_ReadSR2(void);
uint32_t W25Q_Wait_Time(void);
HAL_StatusTypeDef W25Q_WriteEn(void);
void W25Q_WriteDMA(uint32_t addr,uint8_t* PageWrite,uint16_t size);
void W25Q_ReadDMA(uint32_t addr,uint8_t *PageRead,uint16_t size);