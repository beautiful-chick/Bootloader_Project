/*
 * littlefs_port.h
 *
 *  Created on: 2024年7月10日
 *      Author: lizhao
 */

#include "lfs.h"
#include "lfs_util.h"

#ifndef INC_LITTLEFS_PORT_H_
#define INC_LITTLEFS_PORT_H_

// 定义一个结构体来跟踪内存块
typedef struct MemBlock
{
    void *ptr;
    size_t size;
    struct MemBlock *next;
} MemBlock;



int lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int lfs_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int lfs_SectorErase(const struct lfs_config *c, lfs_block_t block);
void lfs_test();
void verify_flash_erased();
int lfs_sync(const struct lfs_config *c);
void initialize_filesystem(void);
#endif /* INC_LITTLEFS_PORT_H_ */
