
#include "py32f0xx.h"
#include "uart.h"
#include "py32f0xx_hal_gpio.h"
#include "py32f0xx_hal_def.h"

UART_HandleTypeDef UartHandle;
char aRxBuffer[12] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/**
 * @brief Setup the UART port pins from the GPIO persecptive
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    /*  USART1
        PA2 ------> USART1_TX
        PA3 ------> USART1_RX
    */
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
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
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    UartHandle.Instance = USART1;
    UartHandle.Init.BaudRate = 230400;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits = UART_STOPBITS_1;
    UartHandle.Init.Parity = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode = UART_MODE_TX_RX;
    if (HAL_UART_DeInit(&UartHandle) != HAL_OK)
    {
    }
    if (HAL_UART_Init(&UartHandle) != HAL_OK)
    {
    }
}
/**
 * @fn void uart_send_string(char* data)
 * @brief Sends Null terminated string to the uart
 *
 */
void uart_send_string(char *data)
{
    while (*data)
    {
        HAL_UART_Transmit(&UartHandle, (void *)data++, 1, 100);
    }
}

/**
 * @fn void debug_send_usb(uint8_t* data, uint16_t* len)
 * @brief Sends data to the USB port
 *
 */
void uart_send(uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        HAL_UART_Transmit(&UartHandle, &data[i], 1, 100);
    }
}