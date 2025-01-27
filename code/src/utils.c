/* System includes */   
#include <stdio.h>
#include <string.h>
#include "py32f0xx.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_ll_cortex.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_adc.h"
#include "py32f0xx_hal.h"

#include "globals.h"
#include "utils.h"

uint16_t crc16(uint8_t *addr, uint8_t num, uint32_t crc);

void ResetOptionBytesToDefault(void)
{
    // Unlock the Flash memory interface
    HAL_FLASH_Unlock();

    // Unlock the option bytes
    HAL_FLASH_OB_Unlock();

    // Configure the option bytes to their default values
    FLASH_OBProgramInitTypeDef OBInit;
    // Get the current option bytes configuration
    HAL_FLASH_OBGetConfig(&OBInit);

    /* Only set if it hasn't been alaready - Will reset device if changed */
    if (!(OBInit.USERConfig & FLASH_OPTR_NRST_MODE))
    {
        OBInit.OptionType = OPTIONBYTE_USER;
        OBInit.USERType = OB_USER_NRST_MODE;
        OBInit.USERConfig |= FLASH_OPTR_NRST_MODE;
        OBInit.USERConfig &= ~FLASH_OPTR_BOR_EN;

        // Program the option bytes
        if (HAL_FLASH_OBProgram(&OBInit) != HAL_OK)
        {
            // Error occurred while programming option bytes
            while(1);
        }

        // Launch the option byte loading
        if (HAL_FLASH_OB_Launch() != HAL_OK)
        {
            // Error occurred while launching option bytes
            while(1);
        }
    }

    // Lock the option bytes
    HAL_FLASH_OB_Lock();

    // Lock the Flash memory interface
    HAL_FLASH_Lock();
}
#define poly 0x1021
uint16_t crc16(uint8_t *addr, uint8_t num, uint32_t crc)
{
	uint16_t i;

	for (; num > 0; num--) /* Step through bytes in memory */
	{
		crc = crc ^ (*addr++ << 8); /* Fetch byte from memory, XOR into CRC top byte*/
		for (i = 0; i < 8; i++)		/* Prepare to rotate 8 bits */
		{
			crc = crc << 1;					 /* rotate */
			if (crc & 0x10000)				 /* bit 15 was set (now bit 16)... */
				crc = (crc ^ poly) & 0xFFFF; /* XOR with XMODEM polynomic */
											 /* and ensure CRC remains 16-bit value */
		}									 /* Loop for 8 bits */
	}										 /* Loop until num=0 */
	return (crc);							 /* Return updated CRC */
}

/**
 * @brief Gets the processors unique ID.
 *
 */
uint16_t Read_UID(void)
{
    uint32_t uid[3];
    uid[0] = *(uint32_t *)(UID_BASE_ADDRESS);
    uid[1] = *(uint32_t *)(UID_BASE_ADDRESS + 4);
    uid[2] = *(uint32_t *)(UID_BASE_ADDRESS + 8);

    return crc16((uint8_t*)&uid[0], 12, 0);
}
