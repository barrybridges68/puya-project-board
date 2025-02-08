#ifndef __UART_H__
#define __UART_H__


extern UART_HandleTypeDef UartHandle;
/**
  * @brief Setup the UART port pins from the GPIO persecptive 
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart);
void APP_USARTConfig(void);

#endif