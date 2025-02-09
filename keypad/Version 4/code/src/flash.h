/**
 * @file flash.h
 * @brief SPI Flash interface
 * 
 * Detailed description of the file's purpose and contents.
 * 
 * @author Your Name
 * @date Date of creation
 */
#ifndef __FLASH_H__
#define __FLASH_H__
#include <stdint.h>

/* Definitions */
#define SPI_FLASH_PAGE_SIZE 256
#define SPI_FLASH_SECTOR_SIZE 4096
#define SPI_FLASH_CHIP_SIZE 0x1000000

typedef enum {
    E_FAIL = 1,
    E_SUCCESS = 0,
    E_BUSY = 2
} flash_status_t;

uint8_t flash_read_status(void);
flash_status_t flash_write_enable(void);
flash_status_t flash_write_disable(void);
flash_status_t flash_read(uint32_t address, uint8_t *data, uint32_t len);
flash_status_t flash_write(uint32_t address, uint8_t *data, uint32_t len);
flash_status_t flash_erase_sector(uint32_t address);
flash_status_t flash_erase_chip(void);
flash_status_t flash_init(void);
flash_status_t flash_power_down(void);
flash_status_t flash_power_up(void);
int flash_sync(void);
#endif // __FLASH_H__

