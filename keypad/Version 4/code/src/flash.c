/**
 * @file flash.c
 * @brief SPI Flash releated functions
 *
 * @author Your Name
 * @date Date of creation
 */


/* System includes */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "py32f0xx_ll_gpio.h"

/* Project includes */
#include "flash.h"
#include "spi.h"

/* Definitions */
#define FLASH_CE_LOW 0
#define FLASH_CE_HIGH 1


#define FLASH_CE_PORT  GPIOA
#define FLASH_CE_PIN   LL_GPIO_PIN_12

#define FLASH_TIMEOUT 1000000

#define FLASH_CMD_READ 0x03
#define FLASH_CMD_WRITE 0x02
#define FLASH_CMD_ERASE_SECTOR 0x20
#define FLASH_CMD_ERASE_CHIP 0x60
#define FLASH_CMD_READ_STATUS 0x05
#define FLASH_CMD_WRITE_ENABLE 0x06
#define FLASH_CMD_WRITE_DISABLE 0x04
#define FLASH_CMD_POWER_DOWN 0xb9
#define FLASH_CMD_POWER_UP 0xab

/* Prototypes */
static flash_status_t flash_wait_for_ready(uint32_t timeout);

/* Variables */

/* Functions */
static void flash_ce(uint8_t level)
{
	if (level == FLASH_CE_HIGH)
    {
		LL_GPIO_SetOutputPin(FLASH_CE_PORT, FLASH_CE_PIN);
    }
	else
    {
		LL_GPIO_ResetOutputPin(FLASH_CE_PORT, FLASH_CE_PIN);
    }
}

/**
 * @brief [Brief description of the function]
 *
 * [Detailed description of the function]
 *
 * @param [param1] [Description of the first parameter]
 * @param [param2] [Description of the second parameter]
 * @return [Description of the return value]
 */
static flash_status_t flash_wait_for_ready(uint32_t timeout)
{
    uint8_t status;
    uint32_t retry_count = 0;

    do
    {
        status = flash_read_status();
        if (!(status & 0x01))
        {
            return E_SUCCESS;
        }
        retry_count++;
    }
    while (retry_count < timeout);

    return E_FAIL;
}

/**
 * @brief [Brief description of the function]
 *
 * [Detailed description of the function]
 *
 * @param [param1] [Description of the first parameter]
 * @param [param2] [Description of the second parameter]
 * @return [Description of the return value]
 */
uint8_t flash_read_status(void)
{
    uint8_t status;
    uint8_t cmd = FLASH_CMD_READ_STATUS;

    flash_ce(FLASH_CE_LOW);
    spi_transfer(cmd);
    status = spi_transfer(0xff);
    flash_ce(FLASH_CE_HIGH);

    return status;
}

/**
 * @brief [Brief description of the function]
 *
 * [Detailed description of the function]
 *
 * @param [param1] [Description of the first parameter]
 * @param [param2] [Description of the second parameter]
 * @return [Description of the return value]
 */
flash_status_t flash_write_enable(void)
{
    uint8_t cmd = FLASH_CMD_WRITE_ENABLE;

    flash_ce(FLASH_CE_LOW);
    spi_transfer(cmd);
    flash_ce(FLASH_CE_HIGH);

    return E_SUCCESS;
}

/**
 * @brief [Brief description of the function]
 *
 * [Detailed description of the function]
 *
 * @param [param1] [Description of the first parameter]
 * @param [param2] [Description of the second parameter]
 * @return [Description of the return value]
 */
flash_status_t flash_write_disable(void)
{
    uint8_t cmd = FLASH_CMD_WRITE_DISABLE;

    flash_ce(FLASH_CE_LOW);
    spi_transfer(cmd);
    flash_ce(FLASH_CE_HIGH);

    return E_SUCCESS;
}

/**
 * @brief [Brief description of the function]
 *
 * [Detailed description of the function]
 *
 * @param [param1] [Description of the first parameter]
 * @param [param2] [Description of the second parameter]
 * @return [Description of the return value]
 */
flash_status_t flash_read(uint32_t address, uint8_t *data, uint32_t len)
{
    uint8_t cmd = FLASH_CMD_READ;
    uint8_t addr[3];
    uint32_t i;

    addr[0] = (address >> 16) & 0xff;
    addr[1] = (address >> 8) & 0xff;
    addr[2] = address & 0xff;

    flash_ce(FLASH_CE_LOW);
    spi_transfer(cmd);
    for (i = 0; i < 3; i++)
    {
        spi_transfer(addr[i]);
    }
    for (i = 0; i < len; i++)
    {
        data[i] = spi_transfer(0xff);
    }
    flash_ce(FLASH_CE_HIGH);

    return E_SUCCESS;
}

/**
 * @brief Write data to flash memory
 *
 * This function writes data to the flash memory. It handles page boundaries
 * by splitting the write operation into multiple page writes if necessary.
 *
 * @param address The starting address to write to
 * @param data Pointer to the data to be written
 * @param len Length of the data to be written
 * @return flash_status_t Status of the write operation
 */
flash_status_t flash_write(uint32_t address, uint8_t *data, uint32_t len)
{
    uint8_t cmd = FLASH_CMD_WRITE;
    uint8_t addr[3];
    uint32_t i, chunk_len;

    while (len > 0)
    {
        chunk_len = SPI_FLASH_PAGE_SIZE - (address % SPI_FLASH_PAGE_SIZE);
        if (chunk_len > len)
        {
            chunk_len = len;
        }

        addr[0] = (address >> 16) & 0xff;
        addr[1] = (address >> 8) & 0xff;
        addr[2] = address & 0xff;

        flash_write_enable();

        flash_ce(FLASH_CE_LOW);
        spi_transfer(cmd);
        for (i = 0; i < 3; i++)
        {
            spi_transfer(addr[i]);
        }
        for (i = 0; i < chunk_len; i++)
        {
            spi_transfer(data[i]);
        }
        flash_ce(FLASH_CE_HIGH);

        flash_write_disable();

        address += chunk_len;
        data += chunk_len;
        len -= chunk_len;

        // Wait for the flash to be ready before the next write
        if (flash_wait_for_ready(FLASH_TIMEOUT) != E_SUCCESS)
        {
            return E_FAIL;
        }
    }

    return E_SUCCESS;
}

/**
 * @brief flash_status_t flash_erase_sector(uint32_t address)
 *
 * This function erases a sector of the flash memory.
 *
 * @param address The address of the sector to erase
 * @return flash_status_t Status of the erase operation
 */
flash_status_t flash_erase_sector(uint32_t address)
{
    uint8_t cmd = FLASH_CMD_ERASE_SECTOR;
    uint8_t addr[3];
    uint8_t i;

    addr[0] = (address >> 16) & 0xff;
    addr[1] = (address >> 8) & 0xff;
    addr[2] = address & 0xff;

    flash_write_enable();

    flash_ce(FLASH_CE_LOW);
    spi_transfer(cmd);
    for (i = 0; i < 3; i++)
    {
        spi_transfer(addr[i]);
    }
    flash_ce(FLASH_CE_HIGH);

    flash_write_disable();

    /* Wait for the flash to be ready before the next write */
    if (flash_wait_for_ready(FLASH_TIMEOUT) != E_SUCCESS)
    {
        return E_FAIL;
    }    

    return E_SUCCESS;
}

/**
 * @brief flash_status_t flash_erase_chip(void)
 *
 * This function erases the entire flash memory.
 *
 * @return Always E_SUCCESS
 */
flash_status_t flash_erase_chip(void)
{
    uint8_t cmd = FLASH_CMD_ERASE_CHIP;

    flash_write_enable();

    flash_ce(FLASH_CE_LOW);
    spi_transfer(cmd);
    flash_ce(FLASH_CE_HIGH);

    flash_write_disable();

    /* Wait for the flash to be ready before the next write */
    if (flash_wait_for_ready(FLASH_TIMEOUT) != E_SUCCESS)
    {
        return E_FAIL;
    }    

    return E_SUCCESS;
}

/**
 * @brief flash_status_t flash_init(void)
 *
 * This function initializes the flash memory.
 *
 * @return Always E_SUCCESS
 */
flash_status_t flash_init(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
    // Configure CE pin
    LL_GPIO_SetPinMode(FLASH_CE_PORT, FLASH_CE_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(FLASH_CE_PORT, FLASH_CE_PIN, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetPinOutputType(FLASH_CE_PORT, FLASH_CE_PIN, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinPull(FLASH_CE_PORT, FLASH_CE_PIN, LL_GPIO_PULL_NO);

    // Set CE high
    LL_GPIO_SetOutputPin(FLASH_CE_PORT, FLASH_CE_PIN);
 
    return E_SUCCESS;
}

/**
 * @brief flash_status_t flash_power_down(void)
 *
 * This function powers down the flash memory to deep sleep.
 *
 * @return Always E_SUCCESS
 */
flash_status_t flash_power_down(void)
{
    uint8_t cmd = FLASH_CMD_POWER_DOWN;

    flash_ce(FLASH_CE_LOW);
    spi_transfer(cmd);
    flash_ce(FLASH_CE_HIGH);

    return E_SUCCESS;
}

/**
 * @brief flash_status_t flash_power_up(void)
 *
 * This function powers up the flash memory from deep sleep.
 *
 * @return Always E_SUCCESS
 */
flash_status_t flash_power_up(void)
{
    uint8_t cmd = FLASH_CMD_POWER_UP;

    flash_ce(FLASH_CE_LOW);
    spi_transfer(cmd);
    flash_ce(FLASH_CE_HIGH);

    /* Wait for the flash to be ready before the next write */
    if (flash_wait_for_ready(FLASH_TIMEOUT) != E_SUCCESS)
    {
        return E_FAIL;
    }        

    return E_SUCCESS;
}

int flash_sync(void)
{
    return E_SUCCESS;
}