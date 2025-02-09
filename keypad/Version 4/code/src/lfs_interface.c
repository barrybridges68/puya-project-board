/**
 * @file lfs_interface.c
 * @brief Interface to the LittleFS filesystem
 *
 * @author Your Name
 * @date Date of creation
 */

/* System includes */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

/* Project includes */
#include "flash.h"
#include "lfs.h"
#include "lfs_interface.h"
#include "lfs_util.h"

/* Definitions */
lfs_t lfs;
lfs_file_t file;

/* Prototype */
int little_fs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int little_fs_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int little_fs_erase(const struct lfs_config *c, lfs_block_t block);
int little_fs_sync(const struct lfs_config *c);

// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {
    // block device operations
    .read  = little_fs_read,
    .prog  = little_fs_write,
    .erase = little_fs_erase,
    .sync  = little_fs_sync, /* Dummy */

    // block device configuration
    .read_size = 16,
    .prog_size = 16,
    .block_size = 4096,
    .block_count = 128,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};

int little_fs_init(void)
{
    flash_init();
    /* Get the flash ready */
    flash_power_up();
    
    /* Mount the filesystem */
    int err = lfs_mount(&lfs, &cfg);

    /* Reformat if we can't mount the filesystem
     * this should only happen on the first boot
     */
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }
    return E_SUCCESS;
}


int little_fs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    return flash_read(((block * c->block_size) + off), buffer, size);
}

int little_fs_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    return flash_write(((block * c->block_size) + off), buffer, size);
}

int little_fs_erase(const struct lfs_config *c, lfs_block_t block)
{
    return flash_erase_sector(block * c->block_size);
}

int little_fs_sync(const struct lfs_config *c)
{
    return E_SUCCESS;
}