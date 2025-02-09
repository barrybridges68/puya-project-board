#ifndef __SPI_H__
#define __SPI_H__
#include "py32f0xx.h"
#include "py32f0xx_hal.h"
#include "py32f0xx_ll_spi.h"
extern SPI_HandleTypeDef spi1_handle;
void APP_SPIConfig(void);
uint8_t spi_transfer(uint8_t data);
#endif