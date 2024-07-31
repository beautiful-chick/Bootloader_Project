/*********************************************************************************
 *      Copyright:  (C) 2024 Avnet. All Rights Reserved.
 *         Author:  Zhao Li <15927192685@163.com>
 *
 *    Description:  This is a bootloader program to load FreeRTOS app on STM32.
 *
 ********************************************************************************/
#include <stdio.h>
#include "spi_flash.h"
#include "elf.h"
#include "dump.h"
#include "littlefs_port.h"
#include "lfs.h"
/* LittleFS partition start address and size */
#define LFS_PART_OFSET		0x500000
#define LFS_PART_SIZE		0x500000

#define DEF_APPS_DIR		"/"

#define PAGE_SIZE			256
#define MAX_PROGRAMS 		4
#define RXBUF_SIZE  		1

extern lfs_t 					lfs;
extern lfs_file_t 				file;
extern uint8_t            	 	g_xymodem_rxbuf[RXBUF_SIZE];
extern struct ring_buffer  		g_xymodem_rb;

typedef struct
{
    char name[32];
    char file[64];
} Program;

Program programs[MAX_PROGRAMS] = {
    {"program1.elf", "classa.elf"},
    {"cprogram2.elf", "classb.elf"},
    {"program3.elf", "classc.elf"},
    {"Default Program", "SPI_Project.elf"}
};

int get_user_choice();

int parser_bootfile(char *elf, int size);

int boot_elf(char *elf);

int do_load_elf()
{
	int				rv;
	char            elf_fname[32];
	int				choice = -1;

	const struct lfs_config cfg = {
		.read 				= lfs_read,
		.prog 				= lfs_write,
		.erase 				= lfs_SectorErase,
		.sync				= lfs_sync,

		.read_size 			= 256,
		.prog_size 			= 256,
		.block_size 		= 65536,
		.block_count 		= 64,
		.block_cycles 		= 100,
		.cache_size 		= 256,

	};

	printf ("\r\nAU15P Board SPI Flash Bootloader v1.0 Build on %s\r\n", __DATE__);

	/* Displays the menu. */
	choice = get_user_choice();
	switch (choice) {
	        case 1:
	            strncpy(elf_fname, "classa.elf", sizeof(elf_fname));
	            break;
	        case 2:
	            strncpy(elf_fname, "classb.elf", sizeof(elf_fname));
	            break;
	        case 3:
	            strncpy(elf_fname, "classc.elf", sizeof(elf_fname));
	            break;
	        case 4:
	        default:
	            strncpy(elf_fname, "SPI_Project.elf", sizeof(elf_fname));
	            printf("Invalid choice. Loading default program.\n");
	            break;
	    }

	/* Sets the selected program file name. */

/*
	rv = parser_bootfile(elf_fname, sizeof(elf_fname));
	if( rv < 0 )
	{
		printf("ERROR: Parser boot configure file failure,the rv's value is %d\r\n", rv);
		goto cleanup;
	}
*/
BOOT_ELF:
	printf("Boot application %s\r\n", elf_fname);
	boot_elf(elf_fname);

cleanup:
	lfs_unmount(&lfs);



	return 0;
}

int parser_bootfile( char *elf, int size)
{
	lfs_file_t 		file;
	char            buf[64];
	char           *ptr;
	int             i, rv = 0;

	memset(buf, 0, sizeof(buf));
	if( lfs_file_read(&lfs, &file, buf, sizeof(buf)) <= 0 )
	{
		rv = -3;
		goto cleanup;
	}

	/* parser boot application */
	ptr = strstr(buf, "BOOT=");
	if( !ptr )
	{
		rv = -4;
		goto cleanup;
	}
	ptr += strlen("BOOT=");

	/* get rid the end blank char \r\n */
	for(i=0; i<size; i++)
	{
		if( isblank(ptr[i]) )
			break;

		elf[i] = ptr[i];
	}

cleanup:
	lfs_file_close(&lfs, &file);
	return rv;
}

int valid_elf_image(void *addr);

void load_elf_image(Elf32_Ehdr *ehdr);

int boot_elf(char *elf)
{

	Elf32_Ehdr	    ehdr; /* Elf header structure pointer */
	int				rv = 0;
	char            fpath[64];

	/* ELF images is in apps/ folder */
	strcpy(fpath, DEF_APPS_DIR);
	strncat(fpath, elf, sizeof(fpath)-strlen(DEF_APPS_DIR));
	printf("Attempting to open file: %s\n", fpath);

	/* open and read boot configure file */
	if( lfs_file_open(&lfs, &file, fpath, LFS_O_RDONLY) < 0)
	{
		printf("ERROR: Open ELF image %s failure\r\n", elf);
		lfs_unmount(&lfs);
		return -2;
	}

	/* Read the ELF image header littlefs rootfs */
	if( lfs_file_read(&lfs, &file, &ehdr, sizeof(ehdr)) <= 0 )
	{
		rv = -3;
		goto cleanup;
	}

    /* check it's valid ELF image or not */
    if ( !valid_elf_image(&ehdr) )
    {
		printf("## %s is not a 32-bit ELF image\r\n", elf);
		goto cleanup;
    }

    /* Load ELF application from SPI Flash to DDR */
    load_elf_image(&ehdr);

    /* Jump to run ELF application */
    printf("the program address 0x%02X\r\n", ehdr.e_entry);
	((void (*)(void))(ehdr.e_entry))();
    return 0;

    /* should never come here */
cleanup:
	lfs_file_close(&lfs, &file);
	return rv;
}

int valid_elf_image(void *addr)
{
    Elf32_Ehdr *ehdr; /* Elf header structure pointer */

    ehdr = (Elf32_Ehdr *)addr;

    if (!IS_ELF(*ehdr) || ehdr->e_type != ET_EXEC)
    {
        return 0;
    }

    return 1;
}

void load_elf_image( Elf32_Ehdr *ehdr)
{
	uint8_t			buf[PAGE_SIZE];
	Elf32_Phdr 	   *phdr; /* Program header structure pointer */
	int				ofset;
	int				i;

	/* Load each program header */
	for(i=0; i<ehdr->e_phnum; ++i)
	{
		ofset = ehdr->e_phoff+i*sizeof(*phdr);
		lfs_file_seek(&lfs, &file , ofset, SEEK_SET);
		lfs_file_read(&lfs, &file, buf, sizeof(*phdr));

		phdr = (Elf32_Phdr *)buf;
		if( phdr->p_type == PT_LOAD )
		{
			//xil_printf("Loading phdr[%d] %d bytes to RAM addr@0x%x\r\n", i, phdr->p_memsz, phdr->p_paddr);

			lfs_file_seek(&lfs, &file , phdr->p_offset, SEEK_SET);
			lfs_file_read(&lfs, &file, (uint8_t *)phdr->p_paddr, phdr->p_memsz);
			printf(".");
		}
	}
	printf("\r\n");
}

int get_user_choice()
{
    int 	choice = -1;
    char 	input[10];
    printf("Please select a program to boot:\n");
    printf("--------------------------------\n");
    for (int i = 0; i < MAX_PROGRAMS; i++)
    {
        printf("%d. %s\n", i + 1, programs[i].name);
    }
    //memset(input, 0, sizeof(input));
    printf("--------------------------------\n");
    printf("Enter the number of the program to load (1-%d): ", MAX_PROGRAMS);
    printf("g_xymodem_rxbuf:%d\r\n", g_xymodem_rxbuf[0]);
    input[0] = g_xymodem_rxbuf[0];
    input[1] = '\0';
    if (sscanf(input, "%d", &choice) == 1)
    {
    	printf("choice:%d\r\n", choice);
        if (choice < 1 || choice > MAX_PROGRAMS)
        {
        	printf("Invalid choice. Loading default program.\n");
            choice = MAX_PROGRAMS;
        }
        }
    else
        {
            printf("Invalid input. Loading default program.\n");
            choice = MAX_PROGRAMS;
        }

        return choice - 1;
    }

void print_bootloader_header() {
    printf("\n");
    printf("*********************************************\n");
    printf("*                                           *\n");
    printf("*       ISK Board SPI Flash Bootloader      *\n");
    printf("*           Version 1.0                     *\n");
    printf("*           Build Date: %s                  *\n", __DATE__);
    printf("*                                           *\n");
    printf("*********************************************\n");
    printf("\n");
}

