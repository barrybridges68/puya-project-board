/*****************************************************************************
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
*****************************************************************************/

/* System includes */   
#include <stdio.h>
#include <string.h>
#include "py32f0xx.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_ll_cortex.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_adc.h"
#include "py32f0xx_hal.h"

/* Project includes */
#include "globals.h"
#include "interrupts.h"
#include "RF24.h"
#include "bandgap.h"
#include "hdc1080.h"

#if INCLUDE_SERIAL    
    #include "uart.h"
#endif

#define DEBOUNCE_TIME_MS 20 // May not be 20ms at given clock rate




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

    // __HAL_RCC_GPIOF_CLK_ENABLE();
    // GPIO_InitStruct.Pin   = GPIO_PIN_1;// | GPIO_PIN_0 | GPIO_PIN_4;
    // GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
    // GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // __HAL_RCC_GPIOB_CLK_ENABLE();
    // GPIO_InitStruct.Pin   = IO_3 | IO_4 | IO_1;
    // GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
    // GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    // HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
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
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1| GPIO_PIN_5 | GPIO_PIN_6);

    __HAL_RCC_SPI1_CLK_DISABLE();

    /* Configure the LSI */   
    RCC->CSR |= RCC_CSR_LSION;
    while(!(RCC->CSR & RCC_CSR_LSIRDY));
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
    SystemCoreClockUpdate();

    /* Re-enable the interrupts */
    NVIC_Config();

    /* Make sure we don't re-enter sleep */
    HAL_PWR_DisableSleepOnExit();
    
    /* Enter stop mode */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    NVIC_Disable();

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

    /* Resume the timer tick after waking */
    HAL_ResumeTick();

    /* Reconfigure the SPI */
    APP_SPIConfig();  
}

/**
 * @brief Debounce IO
 *
 * This function debouces the IO and returns which one is active.
 *
 * @return uint8_t bitfiled of active gpio.
 */
uint8_t debounce_buttons(void)
{
    uint8_t button_on_count = 0;
    uint8_t cycle_count = 0;
    //g_button_press = 2;
    while(TRUE)
    {
        HAL_Delay(DEBOUNCE_TIME_MS);  
        if ((GPIOB->IDR & (IO_3 | IO_4 | IO_1)) )
        {
            button_on_count++;
            if (button_on_count > 6)
            {
                return g_button_press;
            }
        }
        else
        {
            button_on_count = 0;
        }
        cycle_count++;

        if (cycle_count >= 30)
        {
            g_button_press = 0;
            return 0;
        }
    }
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
    /* Initialize the system */
    SystemInit();

    /* Basicaly sets up the system tic */
    HAL_Init();

    /* Configure the system clock */
    SystemCoreClockUpdate();

    /* Set the option to treat the RESET line as GPIO */
    ResetOptionBytesToDefault();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Configure the IO */
    APP_GPIO_Config();

    // Configure the ADC
    APP_ADCConfig();

    /* Configure the I2C and RTC */
    hdc1080_init();
    //rtc_init();

#if INCLUDE_SERIAL    
    APP_USARTConfig();
#endif

    /* Configure the SPI for the radio */
    APP_SPIConfig();

    /* Configure the radio */
    nrf_init();

    /* Small start-up delay */
    HAL_Delay(100);
    uint8_t buttons;
    /* Main loop */
    while(1)
    {
#if INCLUDE_SERIAL   
        HAL_UART_Transmit(&UartHandle,&a,1,100);
        status = HAL_UART_Receive(&UartHandle, &a, 1, 0);
#endif
        /* Place the device into sleep mode */
        //EnterLowPowerMode();
        HAL_Delay(2000);

        /* Small debouce delay */
        if ((buttons = debounce_buttons()) != 0)
        { 
            /* Send the required data */
            nrf_send(buttons);
            
            EXTI->PR |= EXTI_PR_PR1;
            g_button_press = 0;
        }

        /* Re-enable the IRQ's. 
         * This is done to prevent the IRQ from being triggered multiple times.
         * Note: Delays seem to be needed, but not sure why. Proably best 
         * not to remove them.
         */
        HAL_Delay(10);
        NVIC_Config();
        HAL_Delay(100);
    }
}
