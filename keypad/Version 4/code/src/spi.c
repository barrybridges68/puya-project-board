/* System includes */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "py32f0xx.h"
#include "py32f0xx_hal.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_hal_spi.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_spi.h"
#include "py32f0xx_ll_bus.h"

/* Project includes */
#include "globals.h"
#include "utils.h"


/* Variables */
SPI_HandleTypeDef spi1_handle;

/* Functions */
void APP_SPIConfig(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // Configure SPI1 SCK (PA1), MISO (PA0), and MOSI (PA7) pins
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_1, LL_GPIO_AF_0);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_1, LL_GPIO_PULL_NO);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_0, LL_GPIO_AF_10);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_UP);

    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_7, LL_GPIO_AF_0);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_7, LL_GPIO_PULL_NO);
    
	__HAL_RCC_SPI1_CLK_ENABLE();
    // Configure SPI1
	HAL_SPI_DeInit(&spi1_handle);
    spi1_handle.Instance = SPI1;
    spi1_handle.Init.Mode = SPI_MODE_MASTER;
    spi1_handle.Init.Direction = SPI_DIRECTION_2LINES;
    spi1_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    spi1_handle.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi1_handle.Init.CLKPhase = SPI_PHASE_1EDGE;
    spi1_handle.Init.NSS = SPI_NSS_SOFT;
    spi1_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    spi1_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;

    if (HAL_SPI_Init(&spi1_handle) != HAL_OK)
    {
        // Initialization Error
        __NOP();
    }
    __HAL_SPI_ENABLE(&spi1_handle);
}

uint8_t spi_transfer(uint8_t data)
{
	uint8_t result;
	HAL_SPI_TransmitReceive(&spi1_handle, &data, &result,1 ,10);
	return result;
}