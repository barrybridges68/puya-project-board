#ifndef __UART_H__
#define __UART_H__
#include "py32f0xx_hal_uart.h"

extern UART_HandleTypeDef UartHandle;
/**
  * @brief Setup the UART port pins from the GPIO persecptive 
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart);
void APP_USARTConfig(void);


/**
 * @fn void uart_send_string(char* data)
 * @brief Sends Null terminated string to the uart
 *
 */
void uart_send_string(char *data);

/**
 * @fn void debug_send_usb(uint8_t* data, uint16_t* len)
 * @brief Sends data to the USB port
 *
 */
void uart_send(uint8_t *data, uint32_t len);

#endif