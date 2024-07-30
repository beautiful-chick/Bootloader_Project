/*
 * spi3_flash.c
 *
 *  Created on: Jun 18, 2024
 *      Author: lizhao
 */


#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include "spi3_flash.h"
#define Dummy_Byte 0xff
#define SPI_TIMEOUT 1000 // 设置超时为 1000 毫秒




uint8_t SPI3_FLASH_SendByte(uint8_t byte)
{
	uint8_t 					r_data = 0;
	HAL_StatusTypeDef 			status;
	if(  (status = HAL_SPI_TransmitReceive(&hspi3, &byte, &r_data, 1, SPI_TIMEOUT)) != HAL_OK )
	{
		if (status == HAL_ERROR)
		        {
		            printf("SPI transmission error!\n");
		        }
		        else if (status == HAL_TIMEOUT)
		        {
		            printf("SPI transmission timeout!\n");
		        }
		        else if (status == HAL_BUSY)
		        {
		            printf("SPI is busy!\n");
		        }
		return 0;
	}
	return r_data;
}


uint8_t SPI3_FLASH_ReadByte(void)
{
	/*uint8_t t_data,r_data;
	r_data = Dummy_Byte;

	if( HAL_SPI_TransmitReceive(&hspi3, &t_data, &r_data, 1, 0xFFFFFF) != HAL_OK )
	{
		r_data = 0xff;
	}
	return r_data;*/

	uint8_t t_data = Dummy_Byte; // 发送一个 Dummy_Byte
	uint8_t r_data = 0;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi3, &t_data, &r_data, 1, SPI_TIMEOUT);
	printf("Sent: 0x%02X, Received: 0x%02X, Status: %d\n", t_data, r_data, status);
	return r_data;

}


