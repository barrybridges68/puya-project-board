/*
 * interrupts.c
 *
 * Created on: [Date]
 * Author: [Your Name]
 *
 * Description: This file contains the interrupt service routines for the project.
 */

#include "interrupts.h"
#include <stdint.h>
#include <stdbool.h>
#include "py32f0xx.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_hal_spi.h"
#include "py32f0xx_ll_cortex.h"

volatile uint8_t g_button_press = 0;

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


/**
 * @brief Configures the Nested Vectored Interrupt Controller (NVIC).
 *
 * This function sets up the NVIC to handle interrupts for the system.
 * It configures the priority grouping, enables the required IRQs, and
 * sets their priorities.
 *
 * @param None
 * @retval None
 */
void NVIC_Config(void)
{
    // Set priority
    NVIC_SetPriority(EXTI0_1_IRQn, 0);
    NVIC_SetPriority(EXTI2_3_IRQn, 1);
    NVIC_SetPriority(EXTI4_15_IRQn, 2);  
    
    // Enable IRQ
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_EnableIRQ(EXTI2_3_IRQn);   
    NVIC_EnableIRQ(EXTI4_15_IRQn);

    /* Clear all exti interrupts */
    EXTI->PR = EXTI_PR_PR0 | EXTI_PR_PR1 | EXTI_PR_PR2 | EXTI_PR_PR3 | EXTI_PR_PR4 | EXTI_PR_PR12;
}

/**
 * @brief EXTI0_1 Interrupt Handler.
 *
 * This function handles the interrupt for EXTI lines 0 and 1.
 * It is triggered when an external interrupt occurs on these lines.
 * The specific actions taken by this handler depend on the application
 * and the configuration of the EXTI lines.
 *
 * @note Ensure that the EXTI lines 0 and 1 are properly configured
 *       before enabling this interrupt.
 */
void EXTI0_1_IRQHandler(void)
{
    volatile uint32_t irqs = EXTI->PR;
    
    NVIC_DisableIRQ(EXTI0_1_IRQn);

    // Check if the interrupt was triggered by PF0
    if (irqs & EXTI_PR_PR0)
    {
        // Clear the interrupt flag
        EXTI->PR |= EXTI_PR_PR0;
        
        // Handle the interrupt (toggle an LED, for example)
        g_button_press = 0x01;
        return;
    }

    if (irqs & EXTI_PR_PR1)
    {
        // Clear the interrupt flag
        EXTI->PR |= EXTI_PR_PR1;
        
        // Handle the interrupt (toggle an LED, for example)
        g_button_press = 0x02;

        return;
    }
}

/**
 * @brief EXTI line 2 and 3 interrupt handler.
 *
 * This function handles the interrupt for external interrupt lines 2 and 3.
 * It is typically used to handle GPIO pin interrupts for pins associated with
 * EXTI lines 2 and 3.
 *
 * @note Ensure that the EXTI lines 2 and 3 are properly configured and enabled
 *       before using this handler.
 */
void EXTI2_3_IRQHandler(void)
{
    volatile uint32_t irqs = EXTI->PR;
    if (irqs & EXTI_PR_PR2)
    {
        // Clear the interrupt flag
        EXTI->PR |= EXTI_PR_PR2;
        
        g_button_press = 0x04;
        return;
    }

    if (irqs & EXTI_PR_PR3)
    {
        // Clear the interrupt flag
        EXTI->PR |= EXTI_PR_PR3;
        
        g_button_press = 0x08;
        return;
    }
}

/**
 * @brief EXTI line[4:15] interrupts handler.
 *
 * This function handles the external interrupt requests for lines 4 to 15.
 * It is typically used to manage GPIO interrupts for these lines.
 *
 * The specific implementation of this handler should include the necessary
 * logic to determine which line triggered the interrupt and to clear the
 * interrupt flag for that line. Additionally, it should execute any
 * application-specific interrupt handling code.
 *
 * Note: Ensure that the NVIC (Nested Vectored Interrupt Controller) is
 * properly configured to enable EXTI line[4:15] interrupts.
 */
void EXTI4_15_IRQHandler(void)
{
    volatile uint32_t irqs = EXTI->PR;
    if (irqs & EXTI_PR_PR4)
    {
        // Clear the interrupt flag
        EXTI->PR |= EXTI_PR_PR4;
        
        g_button_press = 0x10;
        return;
    }

    if (irqs & EXTI_PR_PR12)
    {
        // Clear the interrupt flag
        EXTI->PR |= EXTI_PR_PR12;
        
        g_button_press = 0x20;
        return;
    }    
}

// Add your interrupt service routines here