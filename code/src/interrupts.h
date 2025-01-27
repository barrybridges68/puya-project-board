#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

extern volatile uint8_t g_button_press;

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
void NVIC_Config(void);

void NVIC_Disable(void);

#endif // INTERRUPTS_H