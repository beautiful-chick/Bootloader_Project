/*
 * littlefs_port.c
 *
 *  Created on: 2024年7月9日
 *      Author: lizhao
 */


#include"spi_flash.h"
#include"lfs.h"
#include <stdio.h>
#include <stdlib.h>
#include "littlefs_port.h"
#include "usart.h"

//#define CONFIG_LITTLEFS_DEBUG

#ifdef CONFIG_LITTLEFS_DEBUG

#define littlefs_print(format,args...) printf(format, ##args)
#else
#define littlefs_print(format,args...) do{} while(0)
#endif


extern	lfs_t 						lfs;
extern	lfs_file_t 					file;
extern  lfs_dir_t 					dir;

struct lfs_config cfg;

 int lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
	uint32_t addr = block * c->block_size + off;
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Reading from address: 0x%08X, size: %d\n", addr, size);
#endif
	New_SPI_FLASH_BufferRead( addr, buffer, size);
	return LFS_ERR_OK;
}

int lfs_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	uint32_t addr = block * c->block_size + off;
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Writing to address: 0x%06X, size: %d\n", addr, size);
#endif
	New_SPI_FLASH_PageWrite((uint8_t *)buffer, addr, size);
	HAL_Delay(100);
	return LFS_ERR_OK;

}

int lfs_SectorErase(const struct lfs_config *c, lfs_block_t block)
{
	uint32_t address = block * c->block_size;
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Erasing block at address: 0x%06X\n", address);
#endif
	 SPI_Flash_BlockErase(address, c->block_size);
	 return LFS_ERR_OK;
}

int lfs_sync(const struct lfs_config *c)
{
	 return LFS_ERR_OK;
}

void init_lfs_config ()
{


	cfg.context 			= NULL;
	cfg.read 				= lfs_read;
	cfg.prog 				= lfs_write;
	cfg.erase 				= lfs_SectorErase;
	cfg.sync				= lfs_sync;
	cfg.read_size 			= 256;
	cfg.prog_size 			= 256;
	cfg.block_size 			= 65536;
	cfg.block_count 		= 64;
	cfg.block_cycles 		= 100;
	cfg.cache_size 			= 256;
	cfg.lookahead_size 		= 16;
	cfg.read_buffer 		= NULL;
	cfg.prog_buffer 		= NULL;
	cfg.lookahead_buffer 	= NULL;
	cfg.name_max 			= 255;
	cfg.file_max 			= 0;
	cfg.attr_max 			= 0;

}

void lfs_test()
{
	lfs_t 						lfs;
	lfs_file_t 					file;

	const char					*message = "Hello, LittleFS!";
	char 						buffer[100];
	lfs_ssize_t 				read_size;
	int							err;
	uint32_t					boot_count = 0;
	/* Configure the little fs file system */
	init_lfs_config();
	HAL_Delay(10);
	/* Mount the little fs file system */

	printf("Starting lfs_test\n");
	HAL_Delay(10);
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Mounting file system...\n");
#endif
	err = lfs_mount(&lfs, &cfg);
	if( err )
	{
		SPI_Flash_ChipErase();
		printf("start to fromat...\r\n");
		err = lfs_format( &lfs, &cfg );
		if( err )
		{
            printf("lfs_format error: %d\r\n", err);
            return ;
		}
		printf("format successful\n");
		err = lfs_mount(&lfs, &cfg);
		if (err)
		{
			printf("lfs_mount error: %d\r\n", err);
		    return;
		}
	}
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("the first period okey\r\n");
#endif
	/* read current count */
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Opening file...\n");
#endif
	err = lfs_file_open( &lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
	if ( err )
	{
	    printf("lfs_file_open error: %d\r\n", err);
	    lfs_unmount(&lfs);
	    return;
	}
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("File opened successfully\r\n");
#endif
	/* Read and start automatic counting */
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Reading boot count...\r\n");
#endif
	read_size = lfs_file_read( &lfs, &file, &boot_count, sizeof(boot_count) );
	if( read_size < 0 )
	{
		printf("lfs_file_read error :%d\r\n", read_size);
		lfs_file_close(&lfs, &file);
		lfs_unmount(&lfs);
		return;
	}
	else if( read_size == 0 )
	{
		boot_count = 0;
	}
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Boot count read successfully: %d\r\n", boot_count);
#endif
	boot_count += 1;
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Rewinding file...\r\n");
#endif
	err = lfs_file_rewind( &lfs, &file);
	if (err)
	{
		printf("lfs_file_rewind error: %d\r\n", err);
		lfs_file_close(&lfs, &file);
		lfs_unmount(&lfs);
		return;
	}
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("File rewinded successfully\r\n");
	littlefs_print("Writing new boot count...\r\n");
#endif
	err = lfs_file_write( &lfs, &file, &boot_count, sizeof(boot_count) );
	if (err < 0 )
	{
		printf("lfs_file_write error: %d\r\n", err);
		lfs_file_close(&lfs, &file);
		lfs_unmount(&lfs);
		return;
	}
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Closing file...\r\n");
#endif
	err = lfs_file_close(&lfs, &file);
	if (err < 0)
	{
		printf("lfs_file_write error: %d\r\n", err);
		return;
	}
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("File closed successfully\r\n");
	littlefs_print("Unmounting file system...\r\n");
#endif
	lfs_unmount(&lfs);
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("File system unmounted successfully.\r\n");
#endif
	printf("boot_count:%d\r\n", boot_count);
	HAL_Delay(100);

}

void initialize_filesystem(void)
{
	int							err;
init_lfs_config();
#ifdef CONFIG_LITTLEFS_DEBUG
	littlefs_print("Mounting file system...\n");
#endif
	err = lfs_mount(&lfs, &cfg);
	if( err )
	{
		SPI_Flash_ChipErase();
		printf("start to fromat...\r\n");
		err = lfs_format( &lfs, &cfg );
		if( err )
		{
            printf("lfs_format error: %d\r\n", err);
            return ;
		}
		printf("format successful\n");
		err = lfs_mount(&lfs, &cfg);
		if (err)
		{
			printf("lfs_mount error: %d\r\n", err);
		    return;
		}
	}


}

void print_all_files()
{
    struct lfs_info 	info;
    int					res;
    printf("Listing all files and their sizes:\n");

    while (true)
    {
        res = lfs_dir_read(&lfs, &dir, &info);
        if (res <= 0)
        {
            break;
        }

        if (info.type == LFS_TYPE_REG)
        {
        	lfs_file_open(&lfs, &file, info.name, LFS_O_RDONLY);
            int size = lfs_file_size(&lfs, &file);
            lfs_file_close(&lfs, &file);
            printf("File: %s, Size: %d bytes\n", info.name, size);
        }
    }
    lfs_dir_close(&lfs, &dir);
}



