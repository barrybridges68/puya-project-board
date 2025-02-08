#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "py32f0xx.h"
#include "py32f0xx_hal.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_hal_spi.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_i2c.h"
#include "py32f0xx_ll_bus.h"

I2C_HandleTypeDef I2cHandle;
#define HDC1080_MASTER_ADDRESS 0x80

void i2c_init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

    // PF1 SCL
    GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_12;
    LL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // PF0 SDA
    GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_12;
    LL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_I2C1);
    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_I2C1);


    I2cHandle.Instance             = I2C1;
    I2cHandle.Init.ClockSpeed      = 100000;        // 100KHz ~ 400KHz
    I2cHandle.Init.DutyCycle       = I2C_DUTYCYCLE_16_9;
    I2cHandle.Init.OwnAddress1     = 0;
    I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    I2cHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&I2cHandle);
}

bool hdc1080_init(void)
{

    volatile HAL_StatusTypeDef result;

    uint8_t data[8];

    i2c_init();
    HAL_Delay(1);

    data[0] = 0x02;
    data[1] = 0x10;
    data[1] = 0x00;
    result = HAL_I2C_Master_Transmit(&I2cHandle, HDC1080_MASTER_ADDRESS, &data[0], 3, 1000);

    if (result != HAL_OK)
    {
        return false;
    }
    return true;

    // data[0] = Reg_Control_status_2;
    // data[1] = RTC_CONTROL_2_INACTIVE;
    // result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    // data[0] = Reg_CLKOUT_control;
    // data[1] = RTC_CLOCKOUT_INHIBITED;
    // result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    // rtc_time.utc_s = 0;

    // data[0] = Reg_VL_seconds;  
    // result = HAL_I2C_Mem_Read(&I2cHandle, PCF8563T_MASTER_ADDRESS, Reg_VL_seconds, 1, (uint8_t*)&rtc_date_time, sizeof(rtc_date_time), 100);


    // rtc_date_time.VL_seconds = 50;

    // memcpy(&data[1], &rtc_date_time, sizeof(rtc_date_time));
    // result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], sizeof(rtc_date_time)+1, 1000);

    // //HAL_Delay(2000);
    // result = HAL_I2C_Mem_Read(&I2cHandle, PCF8563T_MASTER_ADDRESS, Reg_VL_seconds, 1, (uint8_t*)&rtc_date_time, sizeof(rtc_date_time), 100);

    // /* Setup the 1 second timer */
    // data[0] = Reg_Timer;
    // data[1] = 0x40;    
    // result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    // data[0] = Reg_Timer_control;
    // data[1] = RTC_TIMER_CONTROL_TE | RTC_TIMER_TD_64;
    // result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    // /* Enable the interrupt */
    // data[0] = Reg_Control_status_2;
    // data[1] = RTC_CONTROL_2_TI_TP | RTC_CONTROL_2_TIE ;
    // result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    // /* Clear the interrupt */
    // data[0] = Reg_Control_status_2;
    // data[1] = 0 ;

    // result = HAL_I2C_Mem_Read(&I2cHandle, PCF8563T_MASTER_ADDRESS, Reg_Control_status_2, 1, &data[1], 1, 100); 
    // data[0] &= 0x04;
    // result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);
}

hdc1080_read(uint8_t *humidity, uint8_t *temperature)
{
    uint8_t data[40];
    HAL_StatusTypeDef result;

    data[0] = 0x00;
    result = HAL_I2C_Master_Transmit(&I2cHandle, HDC1080_MASTER_ADDRESS, &data[0], 1, 1000);

    result = HAL_I2C_Master_Receive(&I2cHandle, HDC1080_MASTER_ADDRESS, data, 4, 1000);

    *humidity = data[0];
    *humidity = data[1];
    *temperature = data[2];
    *temperature = data[3];
}