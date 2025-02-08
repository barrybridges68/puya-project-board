#include <stdio.h>
#include <string.h>
#include "py32f0xx.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_ll_cortex.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_adc.h"
#include "py32f0xx_hal.h"

// ADC handle declaration
ADC_HandleTypeDef hadc;

// Function to configure the ADC
void APP_ADCConfig(void)
{
    // Enable the ADC clock
    //__HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_ADC_CLK_ENABLE();

    // Configure the ADC
    hadc.Instance = ADC1;
    hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc.Init.Resolution = ADC_RESOLUTION_12B;
    hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc.Init.LowPowerAutoWait = DISABLE;
    //hadc.Init.LowPowerAutoPowerOff = DISABLE;
    hadc.Init.ContinuousConvMode = DISABLE;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc.Init.DMAContinuousRequests = DISABLE;
    hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;

    if (HAL_ADC_Init(&hadc) != HAL_OK)
    {
        // Initialization Error
        //Error_Handler();
    }

    // Configure the ADC channel to read the internal reference voltage (VREFINT)
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_VREFINT;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;

    if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
    {
        // Channel Configuration Error
        //Error_Handler();
    }
}

// Function to measure the band gap voltage
uint32_t MeasureBandGapVoltage(void)
{
    // Start the ADC conversion
    if (HAL_ADC_Start(&hadc) != HAL_OK)
    {
        // Start Conversion Error
        //Error_Handler();
    }

    // Wait for the conversion to complete
    if (HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY) != HAL_OK)
    {
        // End Of Conversion flag not set on time
        //Error_Handler();
    }

    // Get the ADC conversion result
    uint32_t adcValue = HAL_ADC_GetValue(&hadc);

    // Stop the ADC conversion
    if (HAL_ADC_Stop(&hadc) != HAL_OK)
    {
        // Stop Conversion Error
        //Error_Handler();
    }
    return __LL_ADC_CALC_VREFANALOG_VOLTAGE(adcValue, ADC_RESOLUTION_12B);;
}
