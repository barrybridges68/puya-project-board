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
#include "py32f0xx_hal.h"

/**
 * @brief SysTick interrupt handler.
 *
 * This function is called when the SysTick timer reaches zero. It is used to
 * handle periodic tasks that need to be executed at a regular interval.
 *
 * @note This function is typically called from an interrupt context, so it
 * should be kept as short and efficient as possible to avoid delaying other
 * interrupts.
 */
void SysTick_Handler()
{
    HAL_IncTick();
}


#define UID_BASE_ADDRESS 0x1FFFF7AC // Adjust this address based on the specific PY32 processor documentation

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
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}












void EnterLowPowerMode(void)
{
    /* Stop the system tick*/
    HAL_SuspendTick();     

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
    SystemCoreClockUpdate();
    /* Enter stop mode */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

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






    //HAL_Init();
    HAL_Delay(5000);



    /* Configure the IO */
    APP_GPIO_Config();


    /* Main loop */
    while(1)
    {
        /* Place the device into sleep mode after suspending the timer tick, as these causes interrupts */
        EnterLowPowerMode();

        /* Small debouce delay */
        HAL_Delay(5000);

        /* Check all pins are low */
        while ( (GPIOF->IDR & (GPIO_IDR_ID0 | GPIO_IDR_ID1 | GPIO_IDR_ID4)) || 
                (GPIOB->IDR & (GPIO_IDR_ID2 | GPIO_IDR_ID3)) || 
                (GPIOA->IDR & (GPIO_IDR_ID12)));
        {
            /* Wait */
        }    
    }
}
