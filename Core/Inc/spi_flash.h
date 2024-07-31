/*
 * spi_flash.h
 *
 *  Created on: Jun 17, 2024
 *      Author: lizhao
 */

#ifndef INC_SPI_FLASH_H_
#define INC_SPI_FLASH_H_


#include <stdint.h>
#include "stm32l4xx_hal.h"

uint8_t SPI_FLASH_SendByte(uint8_t byte);
uint8_t SPI1_FLASH_ReadByte(void);
static void SPI1_FLASH_WriteEnable(void);
void SPI_FLASH_WriteDisable(void);
static void SPI1_FLASH_WaitEnd(void);
uint32_t SPI_FLASH_ReadId(void);
uint32_t SPI_FLASH_ReadId(void);
int SPI_FLASH_SectorErase(uint32_t addr);
int New_SPI_FLASH_SectorErase(uint32_t addr, uint32_t size);
int SPI_FLASH_PageWrite(uint32_t addr, uint8_t *pBuffer, uint8_t size);
int New_SPI_FLASH_PageWrite( uint8_t *data, uint32_t addr, uint32_t size);
void SPI_FLASH_BufferRead(uint8_t *buffer, uint32_t addr, uint32_t size);
void New_SPI_FLASH_BufferRead(uint32_t addr, uint8_t *buffer, uint32_t size);
int SPI_FLASH_VerifyErase(uint32_t addr, uint32_t length);

int SPI_FLASH_NoCheck(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
int SPI_FLASH_Differ_PageWrite(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_FLASH_Xfer(uint8_t *snd_buf, uint8_t *recv_buf, int bytes);
uint8_t SPI_FLASH_ReadStatusRegister(void);
void SPI_FLASH_WaitUntilNotBusy(void);
int SPI_Flash_BlockErase(uint32_t addr, uint32_t size);
void test(void);
#endif /* INC_SPI_FLASH_H_ */
