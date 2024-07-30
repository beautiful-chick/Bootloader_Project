/*
 * spi3_flash.h
 *
 *  Created on: Jun 18, 2024
 *      Author: lizhao
 */

#ifndef INC_SPI3_FLASH_H_
#define INC_SPI3_FLASH_H_

uint8_t SPI3_FLASH_SendByte(uint8_t byte);
uint8_t SPI3_FLASH_ReadByte(void);
int _write(int file, char *ptr, int len);

#endif /* INC_SPI3_FLASH_H_ */
