/*********************************************************************************
 *      Copyright:  (C) 2024 Avnet. All Rights Reserved.
 *         Author:  Zhao Li<15927192685@163.com>
 *
 *    Description:  This file is W25Q32JVSSIQ SPI Norflash driver
 *
 ********************************************************************************/
#ifndef __ELF_H_
#define __ELF_H_

#include <stdint.h>
#include "spi_flash.h"

/* These typedefs need to be handled better */
typedef uint32_t    	Elf32_Addr; /* Unsigned program address */
typedef uint32_t    	Elf32_Off;  /* Unsigned file offset */
typedef int32_t     	Elf32_Sword;/* Signed large integer */
typedef uint32_t    	Elf32_Word; /* Unsigned large integer */
typedef uint16_t    	Elf32_Half; /* Unsigned medium integer */


/* e_ident[] identification indexes */
#define EI_MAG0     	0       /* file ID */
#define EI_MAG1     	1       /* file ID */
#define EI_MAG2     	2       /* file ID */
#define EI_MAG3     	3       /* file ID */
#define EI_CLASS    	4       /* file class */
#define EI_DATA     	5       /* data encoding */
#define EI_VERSION  	6       /* ELF header version */
#define EI_OSABI    	7       /* OS/ABI specific ELF extensions */
#define EI_ABIVERSION   8       /* ABI target version */
#define EI_PAD      	9       /* start of pad bytes */
#define EI_NIDENT   	16      /* Size of e_ident[] */

/* e_ident[] magic number */
#define ELFMAG0     	0x7f    /* e_ident[EI_MAG0] */
#define ELFMAG1     	'E'     /* e_ident[EI_MAG1] */
#define ELFMAG2     	'L'     /* e_ident[EI_MAG2] */
#define ELFMAG3     	'F'     /* e_ident[EI_MAG3] */
#define ELFMAG      	"\177ELF"	/* magic */
#define SELFMAG     	4       /* size of magic */

/* e_ident[] file class */
#define ELFCLASSNONE    0       /* invalid */
#define ELFCLASS32  	1       /* 32-bit objs */
#define ELFCLASS64  	2       /* 64-bit objs */
#define ELFCLASSNUM 	3       /* number of classes */

/* e_ident[] data encoding */
#define ELFDATANONE 	0       /* invalid */
#define ELFDATA2LSB 	1       /* Little-Endian */
#define ELFDATA2MSB 	2       /* Big-Endian */
#define ELFDATANUM  	3       /* number of data encode defines */

/* e_ident */
#define IS_ELF(ehdr) ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
              (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
              (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
              (ehdr).e_ident[EI_MAG3] == ELFMAG3)

/* e_type */
#define ET_NONE     0       /* No file type */
#define ET_REL      1       /* relocatable file */
#define ET_EXEC     2       /* executable file */
#define ET_DYN      3       /* shared object file */
#define ET_CORE     4       /* core file */
#define ET_NUM      5       /* number of types */
#define ET_LOOS     0xfe00      /* reserved range for operating */
#define ET_HIOS     0xfeff      /* system specific e_type */
#define ET_LOPROC   0xff00      /* reserved range for processor */
#define ET_HIPROC   0xffff      /* specific e_type */

/* ELF Header */
typedef struct {
    unsigned char   e_ident[EI_NIDENT]; /* ELF Identification */
    Elf32_Half  	e_type;     /* object file type */
    Elf32_Half  	e_machine;  /* machine */
    Elf32_Word  	e_version;  /* object file version */
    Elf32_Addr  	e_entry;    /* virtual entry point */
    Elf32_Off   	e_phoff;    /* program header table offset */
    Elf32_Off   	e_shoff;    /* section header table offset */
    Elf32_Word  	e_flags;    /* processor-specific flags */
    Elf32_Half  	e_ehsize;   /* ELF header size */
    Elf32_Half  	e_phentsize;/* program header entry size */
    Elf32_Half  	e_phnum;    /* number of program header entries */
    Elf32_Half  	e_shentsize;/* section header entry size */
    Elf32_Half  	e_shnum;    /* number of section header entries */
    Elf32_Half  	e_shstrndx; /* section header table's "section header string table" entry offset */
} Elf32_Ehdr;


/* Segment types - p_type */
#define PT_NULL     0       /* unused */
#define PT_LOAD     1       /* loadable segment */
#define PT_DYNAMIC  2       /* dynamic linking section */
#define PT_INTERP   3       /* the RTLD */
#define PT_NOTE     4       /* auxiliary information */
#define PT_SHLIB    5       /* reserved - purpose undefined */
#define PT_PHDR     6       /* program header */
#define PT_TLS      7       /* Thread local storage template */
#define PT_NUM      8       /* Number of segment types */
#define PT_LOOS     0x60000000  /* reserved range for operating */
#define PT_HIOS     0x6fffffff  /* system specific segment types */
#define PT_LOPROC   0x70000000  /* reserved range for processor */
#define PT_HIPROC   0x7fffffff  /* specific segment types */

/* Segment flags - p_flags */
#define PF_X        0x1     /* Executable */
#define PF_W        0x2     /* Writable */
#define PF_R        0x4     /* Readable */
#define PF_MASKOS   0x0ff00000  /* OS specific segment flags */
#define PF_MASKPROC 0xf0000000  /* reserved bits for processor specific segment flags */

/* Program Header */
typedef struct {
    Elf32_Word  	p_type;     /* segment type */
    Elf32_Off   	p_offset;   /* segment offset */
    Elf32_Addr  	p_vaddr;    /* virtual address of segment */
    Elf32_Addr  	p_paddr;    /* physical address of segment */
    Elf32_Word  	p_filesz;   /* number of bytes in file for seg */
    Elf32_Word  	p_memsz;    /* number of bytes in mem. for seg */
    Elf32_Word  	p_flags;    /* flags */
    Elf32_Word  	p_align;    /* memory alignment */
} Elf32_Phdr;

#endif
