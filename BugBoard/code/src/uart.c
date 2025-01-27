
#include "py32f0xx.h"
#include "uart.h"

UART_HandleTypeDef UartHandle;
char aRxBuffer[12] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


/**
  * @brief Setup the UART port pins from the GPIO persecptive 
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    /*  USART1
        PA4 ------> USART2_TX
        PA3 ------> USART2_RX
    */
    GPIO_InitTypeDef  GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);    
}

/**
  * @brief  USART配置函数
  * @param  无
  * @retval 无
  */
void APP_USARTConfig(void) 
{
    /* USART1初始化 */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    UartHandle.Instance          = USART2;
    UartHandle.Init.BaudRate     = 115200;
    UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits     = UART_STOPBITS_1;
    UartHandle.Init.Parity       = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode         = UART_MODE_TX_RX;
    if (HAL_UART_DeInit(&UartHandle) != HAL_OK)
    {
    }
    if (HAL_UART_Init(&UartHandle) != HAL_OK)
    {
    }
}

/**
  * @brief  USART错误回调执行函数，输出错误代码
  * @param  无
  * @retval 无
  */
 void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
}

/**
  * @brief  USART发送回调执行函数
  * @param  无
  * @retval 无
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
   
    if (HAL_UART_Receive_IT(UartHandle, (uint8_t *)aRxBuffer, 12) != HAL_OK)
    {

    }
}
/**
  * @brief  USART接收回调执行函数
  * @param  无
  * @retval 无
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /*通过中断方式接收数据*/
  if (HAL_UART_Transmit_IT(UartHandle, (uint8_t *)aRxBuffer, 12) != HAL_OK)
  {
  }
}