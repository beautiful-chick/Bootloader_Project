/*
 * spi_flash.c
 *
 *  Created on: Jun 16, 2024
 *      Author: lizhao
 */
#include "spi_flash.h"
#include "spi.h"
#include <stdint.h>
#include "usart.h"
#define Dummy_Byte 0xff
#define SPI_TIMEOUT 1000

#define Page_Size	256
#define Sector_Size 4096
#define Block_Size	65536
#define Flash_Size (32 * 1024 * 1024)



#define CS_PIN GPIO_PIN_4
#define CS_PORT GPIOA

#define cs_low()  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET)
#define cs_high() HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET)



static void SPI1_FLASH_WriteEnable(void)
{
	cs_low();
	SPI_FLASH_SendByte( 0x06 );
	HAL_Delay(1);
}

void SPI_FLASH_WriteDisable(void)
{
	SPI_FLASH_SendByte(0x04);
	HAL_Delay(10);
}

/* Description:  Wait flash program/erase finished by read Status Register for BUSY bit
 * Reference  :  P15, 7.1 Status Registers
 */
static void SPI1_FLASH_WaitEnd(void)
{
	uint8_t	state = 0;
	do
	{
		cs_low();
		SPI_FLASH_SendByte(0x05);
		state = SPI_FLASH_SendByte(0x00);
		cs_high();
	}
	while( (state & 0x01) == SET );
}

int SPI_Flash_ChipErase(void) {

	int			rv;
	printf("Start to ChipErase\n");
    // Enable write operations
    SPI1_FLASH_WriteEnable();
    // Begin the chip erase sequence
    cs_low();
    SPI_FLASH_SendByte(0xC7);// 0xC7 for Chip Erase
    HAL_Delay(500);
    cs_high();
    SPI1_FLASH_WaitEnd();
    printf("Chip erase done\n");

    // Verification of erase
    rv = SPI_FLASH_VerifyErase( 0x00000, Page_Size );
    if (rv == -1 )
    {
    	printf("ChipErase failed\n");
    	return -1;
    }
    else if( rv == -2 )
    {
    	printf("The address is not legal.\n");
    	return -2;
    }
    printf("ChipErase okey\n");
    return 0;
}

/* Description:  Erase the block which is between first and last. */
int SPI_Flash_BlockErase(uint32_t addr, uint32_t size) {
    uint32_t block, first, last;
    uint32_t address;
    uint8_t buf[4];
    int bytes = 0;

    /* Find the first and last block that need to be erased */
    first = addr / Block_Size;
    last = (addr + size - 1) / Block_Size;
   printf("Norflash Erase %ld Bytes Block@0x%lx Begin...\r\n", size, addr);
    for (block = first; block <= last; block++) {
        address = block * Block_Size;
       printf("Norflash Erase Block@%lx ...\r\n", address);

        SPI1_FLASH_WriteEnable();
        cs_low();
        buf[bytes++] = 0x20;  // Block Erase Instruction
        buf[bytes++] = (address & 0xFF0000) >> 16;
        buf[bytes++] = (address & 0xFF00) >> 8;
        buf[bytes++] = (address & 0xFF);
        SPI_FLASH_Xfer(buf, NULL, bytes);
        cs_high();
        SPI1_FLASH_WaitEnd();
        HAL_Delay(100);

    }

    printf("Norflash EraseBlock@0x%lx done.\r\n", addr);
   int rv = SPI_FLASH_VerifyErase( 0x00000, Page_Size );
        if (rv == -1 )
        {
        	printf("BlockErase failed\n");
        	return -1;
        }
        else if( rv == -2 )
        {
        	printf("The address is not legal.\n");
        	return -2;
        }
        printf("BlockErase okey\n");
        return 0;

    return 0;
}

int SPI_FLASH_VerifyErase(uint32_t addr, uint32_t length)
{
    uint8_t buffer[256];
    uint32_t bytesRead;
    uint32_t toRead;

    if(addr + length > Flash_Size )
    {
    	return -2;
    }
    for (bytesRead = 0; bytesRead < length; bytesRead += sizeof(buffer))
    {
        toRead = sizeof(buffer);
        if (bytesRead + toRead > length)
        {
            toRead = length - bytesRead;
        }
        New_SPI_FLASH_BufferRead(addr + bytesRead, buffer, toRead);

        for (uint32_t i = 0; i < toRead; i++)
        {
            if (buffer[i] != 0xFF)
            {
                return -1;
            }
        }
    }
    return 0;
}

uint8_t SPI_FLASH_ReadStatusRegister(void)
{
	uint8_t			cmd;
	uint8_t			status;
	cmd = 0x05;
	cs_low();
	SPI_FLASH_SendByte( cmd );
	status = SPI_FLASH_SendByte(0xFF);
	cs_high();
	return status;
}

void SPI_FLASH_WaitUntilNotBusy(void)
{
	uint8_t			status;
	do
	{
		status = SPI_FLASH_ReadStatusRegister();
	}while(status & 0x01);
}

/* Descriptions : Erase the Sectors which is between first and last */
int New_SPI_FLASH_SectorErase(uint32_t addr, uint32_t size)
{
    uint32_t sector, first, last;
    uint32_t address;
    int rv;

    first = addr / Sector_Size;
    last = (addr + size - 1) / Sector_Size;

    /* Start to erase all sectors */
    for (sector = first; sector <= last; sector++)
    {
        address = sector * Sector_Size;
        //printf("Norflash Erase Sector@%lx ...\r\n", address);

        SPI1_FLASH_WaitEnd();
        SPI1_FLASH_WriteEnable();
        cs_low();
        SPI_FLASH_SendByte(0x20);

        SPI_FLASH_SendByte((address & 0xFF0000) >> 16);
        SPI_FLASH_SendByte((address & 0xFF00) >> 8);
        SPI_FLASH_SendByte(address & 0xFF);
        cs_high();

        SPI1_FLASH_WaitEnd();
        HAL_Delay(10);
    }

    rv = SPI_FLASH_VerifyErase(addr, size);
    if (rv != 0)
    {
        printf("Erase the sector error\n");
        return -1;
    }
    else
    {
        printf("Erase ok\n");
    }

    return 0;
}

/* Write only one byte */
uint8_t SPI_FLASH_SendByte(uint8_t byte)
{
	uint8_t 						r_data = 0;
	HAL_StatusTypeDef 				status;
	if(  (status = HAL_SPI_TransmitReceive(&hspi1, &byte, &r_data, 1, SPI_TIMEOUT)) != HAL_OK )
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

	  }

	 return r_data;
}

void SPI_FLASH_Xfer(uint8_t *snd_buf, uint8_t *recv_buf, int bytes)
{
	int 		i;
	uint8_t		send_data, recv_data;
	for(i = 0; i < bytes; i ++)
	{
		send_data = (snd_buf != NULL) ? snd_buf[i] : 0xFF;

		recv_data = SPI_FLASH_SendByte(send_data);

		if( recv_buf != NULL )
		{
			recv_buf[i] = recv_data;
		}
	//	printf("Sending: 0x%02X, Received: 0x%02X\n", send_data, recv_data);


	}

	HAL_Delay(1);


}

/* page program */
int New_SPI_FLASH_PageWrite( uint8_t *data, uint32_t addr, uint32_t size)
{
	uint32_t			first, last, page;
	uint32_t			address, ofset, len;
	uint8_t				buf[Page_Size+5];
	int					bytes = 0;

	if( addr + size > 0x400000 )
		return -1;


	/* find the fist and the last page */
	first = addr / Page_Size;
	last = ( addr + size - 1 ) / Page_Size;
	//printf("Norflash Write %ld Bytes to addr@0x%06X Begin...\r\n", size, addr );

	/*Initial  address in page and offset in buffer */
	address = addr;
	ofset = 0;

	/* Start to write to all pages */
	for( page = first; page <= last; page ++)
	{
		len = Page_Size - ( address % Page_Size );
		len = len > size ? size : len;
		bytes = 0;
		//printf("Norflash write addr@0x%lx, %lu bytes,and the data is %s \r\n", addr, len, data);


		buf[bytes++] = 0x02;
		buf[bytes++] = (addr & 0xFF0000) >> 16 ;
		buf[bytes++] = (addr & 0xFF00) >> 8 ;
		buf[bytes++] = (addr & 0xFF);

		/* send command and data */
		memcpy(&buf[bytes], data+ofset, len);
		bytes += len;
		SPI1_FLASH_WriteEnable();
		cs_low();

		SPI_FLASH_Xfer(buf, NULL, bytes);

		cs_high();

		SPI1_FLASH_WaitEnd();
		addr  += len;
		ofset += len;
		size  -= len;

	}

	HAL_Delay(10);
	printf("Norflash WriteByte@0x%06X done.\r\n", addr + size );
	return 0;

}

/* Read flash'ID */
uint32_t SPI_FLASH_ReadId(void)
{
	 uint32_t 			id = 0;
	 uint8_t 			idBytes[2] = {0};

	    cs_low(); // 拉低片选
	    printf("1\n");
	    SPI_FLASH_SendByte(0x90); // 发送读取ID命令
	    printf("2\n");

	    // 读取2个字节的数据
	    SPI_FLASH_SendByte(Dummy_Byte);
	    SPI_FLASH_SendByte(Dummy_Byte);
	    SPI_FLASH_SendByte(0x00);
	    idBytes[0] = SPI_FLASH_SendByte(Dummy_Byte);
	    idBytes[1] = SPI_FLASH_SendByte(Dummy_Byte);

	    cs_high(); // 拉高片选

	    id = (idBytes[0] << 8) | idBytes[1];
	    printf("ID Bytes: %02X %02X\n", idBytes[0], idBytes[1]);
	    printf("Combined ID: 0x%04X\n", id);
	    return id;
}

void New_SPI_FLASH_BufferRead(uint32_t addr, uint8_t *buffer, uint32_t size)
{
    uint8_t cmd[4];

    cmd[0] = 0x03; // Read Data command
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    cs_low();
    SPI_FLASH_Xfer(cmd, NULL, 4);
    SPI_FLASH_Xfer(NULL, buffer, size);
    cs_high();

}

void test( void )
{
	uint8_t 				Rx[100] = {0};
	uint8_t 				Tx[] = "Hello!", n;
	int						rv;
	/* Test read and write functions */
	n= sizeof(Tx) -1;
	rv = New_SPI_FLASH_PageWrite(Tx, 0x00000 ,n);
	if( rv != 0 )
	{
		printf("Write failed\n");
	  	return -1;
	}
	printf("write ok\n");

	New_SPI_FLASH_BufferRead(0x00000 ,Rx ,n);
	printf("the data is : ");
	for(int i = 0; i < n; i ++)
	{
		printf("0x%02x", Rx[i]);
	}

	/* Testing sector erasure */
	New_SPI_FLASH_SectorErase(0x00000, 257);
	/* Test block erase */
	New_SPI_FLASH_PageWrite(Tx, 0x00000 ,n);
	SPI_Flash_BlockErase(0x00000, Block_Size);
	/* Test the entire chip erase */
	New_SPI_FLASH_PageWrite(Tx, 0x00000 ,n);
	SPI_Flash_ChipErase();
	printf("test okey\n");
}
