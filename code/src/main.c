/**
    ******************************************************************************
    * @file    main.c
    * @author  Your Name
    * @brief   Main program body
    ******************************************************************************
    * @attention
    *
    * <h2><center>&copy; Copyright (c) 2023 Your Company.
    * All rights reserved.</center></h2>
    *
    * This software component is licensed by Your Company under BSD 3-Clause license,
    * the "License"; You may not use this file except in compliance with the License.
    * You may obtain a copy of the License at:
    *                        opensource.org/licenses/BSD-3-Clause
    *
    ******************************************************************************
    */

/* System includes */   
#include <stdio.h>
#include <string.h>
#include "py32f0xx.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_ll_cortex.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_hal.h"

/* Project includes */
#if INCLUDE_SERIAL    
#include "uart.h"
#endif
#include "interrupts.h"
#include "RF24.h"

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



    OBInit.OptionType = OPTIONBYTE_USER;
    OBInit.USERType = OB_USER_NRST_MODE;
    OBInit.USERConfig |= FLASH_OPTR_NRST_MODE;
    //OBInit.USERConfig &= ~FLASH_OPTR_BOR_EN;

    // // Program the option bytes
    // if (HAL_FLASH_OBProgram(&OBInit) != HAL_OK)
    // {
    //     // Error occurred while programming option bytes
    //     //Error_Handler();
    // }

    // // Launch the option byte loading
    // if (HAL_FLASH_OB_Launch() != HAL_OK)
    // {
    //     // Error occurred while launching option bytes
    //   //  Error_Handler();
    // }

    // Lock the option bytes
    HAL_FLASH_OB_Lock();

    // Lock the Flash memory interface
    HAL_FLASH_Lock();
}

/**
 * @brief Gets the processors unique ID.
 *
 */
void Read_UID(uint32_t *uid)
{
    uid[0] = *(uint32_t *)(UID_BASE_ADDRESS);
    uid[1] = *(uint32_t *)(UID_BASE_ADDRESS + 4);
    uid[2] = *(uint32_t *)(UID_BASE_ADDRESS + 8);
}

/**
 * @brief Configures the GPIO pins for the application.
 *
 * This function sets up the necessary GPIO pins for the application,
 * initializing them to their required states and configurations.
 * It is typically called during the initialization phase of the application.
 *
 */
static void APP_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin   = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief Enters the microcontroller into low power mode.
 *
 * This function configures the microcontroller to enter a low power mode
 * by suspending the system tick, disabling the SPI peripheral, and configuring
 * GPIO pins to minimize power consumption. It then enters the Stop mode with
 * the low-power regulator on.
 *
 * The function also ensures that the microcontroller can be woken up by
 * configured external interrupts.
 *
 * @note This function should be called when the system needs to enter a low
 *       power state to conserve energy.
 *
 * @retval None
 */
void EnterLowPowerMode(void)
{
    /* Stop the system tick*/
    HAL_SuspendTick();

    /* Disable the SPI */
    HAL_SPI_DeInit(&spi1_handle);
    __HAL_RCC_SPI1_CLK_ENABLE();

    /* Configure the GPIO pins */
    GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 ;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA1 CLK
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA7 MOSI
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;    
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure the LSI */   
    RCC->CSR |= RCC_CSR_LSION;
    while(!RCC->CSR & RCC_CSR_LSIRDY);
    __HAL_RCC_PWR_CLK_ENABLE();
    RCC->CFGR &= ~RCC_CFGR_SW_Msk;
    RCC->CFGR |= 0b11;
    RCC->CR = 0;

    /* Configure the stop parameters */
    SET_BIT(PWR->CR1, PWR_CR1_LPR);
    SET_BIT(PWR->CR1, PWR_CR1_VOS);
    CLEAR_BIT(PWR->CR1, PWR_CR1_SRAM_RETV_0);
    SET_BIT(PWR->CR1, PWR_SRAM_RETENTION_VOLT_0p9);
    SET_BIT(PWR->CR1, PWR_BIAS_CURRENTS_FROM_BIAS_CR);
    __HAL_RCC_PWR_CLK_DISABLE();

    /* Update the clocks */
    //SystemCoreClockUpdate();
    /* Enter stop mode */
    //HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* Re-enable the power clock after exiting stop mode */
    __HAL_RCC_PWR_CLK_ENABLE();        

    // Re-enable HSI clock
    RCC->CR |= RCC_CR_HSION;
    while(!(RCC->CR & RCC_CR_HSIRDY));

    // Select HSI as system clock source
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SWS_HSI;
    RCC->CFGR = 0;
    __HAL_RCC_PWR_CLK_DISABLE();

    /* Resume the timer tick after waking*/
    HAL_ResumeTick();

    /* Reconfigure the SPI */
    APP_SPIConfig();    
}

/**
 * @brief Entry point of the program.
 *
 * This function is the main entry point of the program. It initializes
 * the necessary hardware and software components and starts the main
 * execution loop.
 *
 * @return int Returns 0 upon successful execution.
 */
int main (void)
{
    SystemInit();
    /* Basicaly sets up the system tic */
    HAL_Init();
    /* Configure the system clock */
    SystemCoreClockUpdate();

    ///ResetOptionBytesToDefault();

    /* Configure the IO */
    APP_GPIO_Config();

    /* Configure the I2C and RTC */
    //i2c_init();
    //rtc_init();

    /* Set up the IRQs */
    NVIC_Config();

#if INCLUDE_SERIAL    
    APP_USARTConfig();
#endif
    /* Configure the SPI for the radio */
    APP_SPIConfig();
    HAL_Delay(1000);

    /* Configure the radio */
    nrf_init();

    /* Powerup message */
    nrf_send(2);
    
    /* Main loop */
    while(1)
    {
#if INCLUDE_SERIAL   
        HAL_UART_Transmit(&UartHandle,&a,1,100);
        status = HAL_UART_Receive(&UartHandle, &a, 1, 0);
#endif
        /* Place the device into sleep mode */
        //EnterLowPowerMode();

        /* Send the required data */
        /* Small debouce delay */
        if (g_button_press!=0)
        { 
            HAL_Delay(600);  
            nrf_send(g_button_press);
            g_button_press = 0;
            EXTI->PR |= EXTI_PR_PR1;
        }
        
        /* Clear the button press */
        //g_button_press = 0;
 
        /* Check all pins are low */
        while ( (GPIOF->IDR & (GPIO_IDR_ID0 | GPIO_IDR_ID1 | GPIO_IDR_ID4)) || 
                (GPIOB->IDR & (GPIO_IDR_ID2 | GPIO_IDR_ID3)) || (GPIOA->IDR & (GPIO_IDR_ID12)));
        {
            /* Wait */
        }

        NVIC_EnableIRQ(EXTI0_1_IRQn);  
    }
}
