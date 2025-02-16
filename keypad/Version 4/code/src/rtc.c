#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "rtc.h"
#include "py32f0xx.h"
#include "py32f0xx_hal.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_hal_spi.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_i2c.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_exti.h"

I2C_HandleTypeDef I2cHandle;
rtc_time_t rtc_time;
rtc_date_time_t rtc_date_time;
#define PCF8563T_MASTER_ADDRESS 0xA2

#define RTC_INT_PORT  GPIOA
#define RTC_INT_PIN   LL_GPIO_PIN_4

/**
 * @brief Basic definitions
 *
 */
#define SECONDS_IN_A_DAY    (86400U)
#define SECONDS_IN_A_HOUR   (3600U)
#define SECONDS_IN_A_MINUTE (60U)
#define DAYS_IN_A_YEAR      (365U)
#define YEAR_RANGE_START    (1970U)

uint32_t rtc_get_seconds(void)
{
    return rtc_time.utc_s;
}

void i2c_init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

	__HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* PA4 as input */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Triggerred by falling edge */
    LL_EXTI_InitTypeDef EXTI_InitStruct;
    EXTI_InitStruct.Line = LL_EXTI_LINE_4;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    // PF1 SCL
    GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_12;
    LL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    // PF0 SDA
    GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_12;
    LL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_I2C1);
    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_I2C1);

    I2cHandle.Instance             = I2C1;
    I2cHandle.Init.ClockSpeed      = 100000;        // 100KHz ~ 400KHz
    I2cHandle.Init.DutyCycle       = I2C_DUTYCYCLE_16_9;
    I2cHandle.Init.OwnAddress1     = 0;
    I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    I2cHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&I2cHandle);
}

void rtc_init(void)
{
    volatile HAL_StatusTypeDef result;

    uint8_t data[40];

    i2c_init();
    HAL_Delay(100);

    data[0] = Reg_Control_status_1;
    data[1] = RTC_CONTROL_1_NORMAL;
    result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    data[0] = Reg_Control_status_2;
    data[1] = RTC_CONTROL_2_INACTIVE;
    result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    data[0] = Reg_CLKOUT_control;
    data[1] = RTC_CLOCKOUT_INHIBITED;
    result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    rtc_time.utc_s = 0;

    data[0] = Reg_VL_seconds;  
    result = HAL_I2C_Mem_Read(&I2cHandle, PCF8563T_MASTER_ADDRESS, Reg_VL_seconds, 1, (uint8_t*)&rtc_date_time, sizeof(rtc_date_time), 100);

    rtc_date_time.VL_seconds = 50;

    memcpy(&data[1], &rtc_date_time, sizeof(rtc_date_time));
    result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], sizeof(rtc_date_time)+1, 1000);

    //HAL_Delay(2000);
    result = HAL_I2C_Mem_Read(&I2cHandle, PCF8563T_MASTER_ADDRESS, Reg_VL_seconds, 1, (uint8_t*)&rtc_date_time, sizeof(rtc_date_time), 100);

    /* Setup the 1 second timer */
    data[0] = Reg_Timer;
    data[1] = 0x40;    
    result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    data[0] = Reg_Timer_control;
    data[1] = RTC_TIMER_CONTROL_TE | RTC_TIMER_TD_64;
    result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    /* Enable the interrupt */
    data[0] = Reg_Control_status_2;
    data[1] = RTC_CONTROL_2_TI_TP | RTC_CONTROL_2_TIE ;
    result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    /* Clear the interrupt */
    data[0] = Reg_Control_status_2;
    data[1] = 0 ;

    result = HAL_I2C_Mem_Read(&I2cHandle, PCF8563T_MASTER_ADDRESS, Reg_Control_status_2, 1, &data[1], 1, 100); 
    data[0] &= 0x04;
    result = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], 2, 1000);

    /* To get rid of warnings */
    if(result != HAL_OK)
    {
        result = HAL_ERROR;
    }
}

/**
 * @fn bool rtc_update_time(uint8_t hour, uint8_t min, uint8_t sec)
 * @brief Updates time only
 *
 * @param [in]  hour bcd format
 * @param [in]  min  bcd format
 * @param [in]  sec  bcd format
 * @return      true if successful
 */
bool rtc_update_time(uint8_t hour, uint8_t min, uint8_t sec)
{
	/* Get the current epoch time */
	bool success;
	uint64_t epoch_sec = rtc_get_seconds();
	rtc_date_time_t rtc_date_time = rtc_sec_to_date_time(epoch_sec);

	/* Update the values */
	rtc_date_time.hours      = hour;
	rtc_date_time.minutes    = min;
	rtc_date_time.VL_seconds = sec;

	/* Write the changes back to the RTC */
    rtc_time.utc_s = rtc_date_time_to_sec(rtc_date_time);
	success = rtc_set_time((rtc_time_t*)&rtc_time);
	return success;
}

/**
 * @fn bool rtc_update_date(uint8_t year, uint8_t month, uint8_t day)
 * @brief Updates date only
 *
 * @param [in]  year  bcd format
 * @param [in]  month bcd format
 * @param [in]  day   bcd format
 * @return      true if successful
 */
bool rtc_update_date(uint8_t year, uint8_t month, uint8_t day)
{
	/* Get the current epoch time */
	bool success;
	uint64_t epoch_sec = rtc_get_seconds();
	rtc_date_time_t rtc_date_time = rtc_sec_to_date_time(epoch_sec);

	/* Update the values */
	rtc_date_time.years = year;
	rtc_date_time.century_months = month;
	rtc_date_time.days = day;

	/* Write the changes back to the RTC */
	rtc_time.utc_s = rtc_date_time_to_sec(rtc_date_time);
	success = rtc_set_time((rtc_time_t*)&rtc_time);
	return success;
}

/**
 * @fn rtc_date_time_t rtc_sec_to_date_time(rtc_time_t seconds)
 * @brief Convert unix epoch time in seconds to date time in BCD format
 *
 * Code from fsl_rtc.c RTC_ConvertSecondsToDatetime(). Modified to be 64 bit.
 *
 * Not efficient but only used when setting a new time date in the RTC,
 * so not time critical.
 *
 * @param [in]  seconds
 * @return      srtc_date_time_t
 */
rtc_date_time_t rtc_sec_to_date_time(uint64_t seconds)
{
    rtc_date_time_t date_time_out = {0};

    struct datetime_t
    {
        uint16_t year;  /*!< Range from 1970 to 2099.*/
        uint8_t month;  /*!< Range from 1 to 12.*/
        uint8_t day;    /*!< Range from 1 to 31 (depending on month).*/
        uint8_t hour;   /*!< Range from 0 to 23.*/
        uint8_t minute; /*!< Range from 0 to 59.*/
        uint8_t second; /*!< Range from 0 to 59.*/
    } datetime;

    /*** FSL SDK code ***/
    uint8_t i;
    uint16_t daysInYear;
    uint64_t secondsRemaining; //*** Changed 32->64 bits ***
    uint32_t days;
    /* Table of days in a month for a non leap year. First entry in the table is not used,
     * valid months start from 1
     */
    uint8_t daysPerMonth[] = {0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};

    /* Start with the seconds value that is passed in to be converted to date time format */
    secondsRemaining = seconds;

    /* Calcuate the number of days, we add 1 for the current day which is represented in the
     * hours and seconds field
     */
    days = secondsRemaining / SECONDS_IN_A_DAY + 1U;

    /* Update seconds left*/
    secondsRemaining = secondsRemaining % SECONDS_IN_A_DAY;

    /* Calculate the datetime hour, minute and second fields */
    datetime.hour   = (uint8_t)(secondsRemaining / SECONDS_IN_A_HOUR);
    secondsRemaining = secondsRemaining % SECONDS_IN_A_HOUR;
    datetime.minute = (uint8_t)(secondsRemaining / 60U);
    datetime.second = (uint8_t)(secondsRemaining % SECONDS_IN_A_MINUTE);

    /* Calculate year */
    daysInYear     = DAYS_IN_A_YEAR;
    datetime.year = YEAR_RANGE_START;
    while (days > daysInYear)
    {
        /* Decrease day count by a year and increment year by 1 */
        days -= daysInYear;
        datetime.year++;

        /* Adjust the number of days for a leap year */
        if ((datetime.year & 3U) != 0x00U)
        {
            daysInYear = DAYS_IN_A_YEAR;
        }
        else
        {
            daysInYear = DAYS_IN_A_YEAR + 1U;
        }
    }

    /* Adjust the days in February for a leap year */
    if ((datetime.year & 3U) == 0x00U)
    {
        daysPerMonth[2] = 29U;
    }

    for (i = 1U; i <= 12U; i++)
    {
        if (days <= daysPerMonth[i])
        {
            datetime.month = i;
            break;
        }
        else
        {
            days -= daysPerMonth[i];
        }
    }

    datetime.day = (uint8_t)days;

   /* End FSL SDK code */

    datetime.year -= (datetime.year >= 2000U) ? 2000U : 1900;  //using 2 digits for year

    /* Convert to BCD */
    date_time_out.VL_seconds = ((datetime.second / 10U) << 4U) + (datetime.second % 10U);
    date_time_out.minutes = ((datetime.minute / 10U) << 4U) + (datetime.minute % 10U);
    date_time_out.hours = ((datetime.hour / 10U) << 4U) + (datetime.hour % 10U);
    date_time_out.days = ((datetime.day / 10U) << 4U) + (datetime.day % 10U);
    date_time_out.century_months = ((datetime.month / 10U) << 4U) + (datetime.month % 10U);
    date_time_out.years = ((datetime.year / 10U) << 4U) + (datetime.year % 10U);
    date_time_out.weekdays = 0U; //don't use weekdays

    return (date_time_out);

}
/**
 * @fn rtc_date_time_t rtc_sec_to_date_time_str(uint64_t seconds, char* date_time_str)
 * @brief Convert unix epoch time in seconds to date time into string format
 *
 *
 * @param [in]  seconds - epoch seconds
 * @param [in]  date_time_str - char buffer for the formated string.
 * @return      srtc_date_time_t
 */
rtc_date_time_t rtc_sec_to_date_time_str(uint64_t seconds, char* date_time_str)
{
	static char *months[12] = { "Jan", "Feb", "Mar", "Apr",	"May", "Jun", "Jul", "Aug",	"Sep", "Oct", "Nov", "Dec" };

	/* Convert the epoch time in seconds into a date time structure */
	rtc_date_time_t rtc_date_time = rtc_sec_to_date_time(seconds);
	sprintf(date_time_str,
			"%02X:%02X:%02X %02X %s %02X",
			rtc_date_time.hours,
			rtc_date_time.minutes,
			(rtc_date_time.VL_seconds & 0x7F),
			rtc_date_time.days,
			months[rtc_date_time.century_months-1],
		    (rtc_date_time.years));

	return rtc_date_time;
}

/* @fn bool rtc_set_time(rtc_time_t* rtc_time)
 * @brief Set the RTC time
 *
 * @NOTE global interrupts disabled in order to protect access to 64 bit value.
 *
 * Converts the time to BDC and saves in the external RTC device.
 *
 * @param[in]   rtc_time_t*
 * @return      true if success, false otherwise
 */
bool rtc_set_time(rtc_time_t* new_rtc_time)
{
    rtc_date_time_t rtc_date_time;
    bool success = false;
    uint8_t data[40];

#if RTC_DEBUG
	sprintf(rtc_debug_buff, "\n\rRTC - rtc_set_time utc %ld\n ", rtc_time->utc_s);
	debug_out_str(rtc_debug_buff);
#endif

    rtc_date_time = rtc_sec_to_date_time(new_rtc_time->utc_s);

    /* Remove unwanted bits from RTC fields */
    rtc_date_time.century_months &= 0x1F;
    rtc_date_time.days &= 0x3F;
    rtc_date_time.hours &= 0x3F;
    rtc_date_time.minutes &= 0x7F;
    rtc_date_time.VL_seconds &= 0x7F;

#if RTC_DEBUG
	uint8_t day =((rtc_date_time.days >> 4U) * 10U + (rtc_date_time.days & 0x0f));
	uint8_t month = ((rtc_date_time.century_months & RTC_MONTHS) >> 4U) * 10U + (rtc_date_time.century_months & 0x0f);
	uint8_t year = (rtc_date_time.years >> 4U) * 10U + (rtc_date_time.years & 0x0f);
	uint8_t hour = (rtc_date_time.hours >> 4U) * 10U + (rtc_date_time.hours & 0x0f);
	uint8_t minute = (rtc_date_time.minutes >> 4U) * 10U + (rtc_date_time.minutes & 0x0f);
	uint8_t second = ((rtc_date_time.VL_seconds & RTC_SECONDS) >> 4U) * 10U + (rtc_date_time.VL_seconds & 0x0f);

	sprintf(rtc_debug_buff, "\n\rRTC - rtc_set_time Date/Time read as %d-%02d-%0d  %02d:%02d:%02d\n\r", day,month,year,hour,minute,second);
	debug_out_str(rtc_debug_buff);
#endif

    /* Set the date time registers */
	rtc_date_time = rtc_sec_to_date_time(new_rtc_time->utc_s);
	//success = rtc_write(Reg_VL_seconds, sizeof(rtc_date_time), (uint8_t*)&rtc_date_time);

    /* Enable the interrupt */
    data[0] = Reg_VL_seconds;
    memcpy( &data[1], &rtc_date_time, sizeof(rtc_date_time));
    success = HAL_I2C_Master_Transmit(&I2cHandle, PCF8563T_MASTER_ADDRESS, &data[0], sizeof(rtc_date_time)+1, 1000);    

	return (success);
}

/**
 * @fn rtc_time_t rtc_date_time_to_sec(rtc_date_time_t date_time_in)
 * @brief Convert date time in BCD format to unix epoch time in seconds
 *
 * Code from fsl_rtc.c RTC_ConvertDatetimeToSeconds(). Modified to be 64 bit.
 *
 * @param [in]  uint64_t
 * @return      seconds
 */
uint64_t rtc_date_time_to_sec(rtc_date_time_t date_time_in)
{
    struct datetime_t
    {
        uint16_t year;  /*!< Range from 1970 to 2099.*/
        uint8_t month;  /*!< Range from 1 to 12.*/
        uint8_t day;    /*!< Range from 1 to 31 (depending on month).*/
        uint8_t hour;   /*!< Range from 0 to 23.*/
        uint8_t minute; /*!< Range from 0 to 59.*/
        uint8_t second; /*!< Range from 0 to 59.*/
    } datetime;

    /* Remove unwanted bits from RTC fields */
    date_time_in.century_months &= 0x1F;
    date_time_in.days &= 0x3F;
    date_time_in.hours &= 0x3F;
    date_time_in.minutes &= 0x7F;
    date_time_in.VL_seconds &= 0x7F;

    /* Convert from BCD */
    datetime.year = 2000U + (date_time_in.years >> 4U) * 10U + (date_time_in.years & 0x0f);
    datetime.month = ((date_time_in.century_months & RTC_MONTHS) >> 4U) * 10U + (date_time_in.century_months & 0x0f);
    datetime.day = (date_time_in.days >> 4U) * 10U + (date_time_in.days & 0x0f);
    datetime.hour = (date_time_in.hours >> 4U) * 10U + (date_time_in.hours & 0x0f);
    datetime.minute = (date_time_in.minutes >> 4U) * 10U + (date_time_in.minutes & 0x0f);
    datetime.second = ((date_time_in.VL_seconds & RTC_SECONDS) >> 4U) * 10U + (date_time_in.VL_seconds & 0x0f);

    /*** FSL SDK code ***/

    /* Number of days from begin of the non Leap-year*/
    uint16_t monthDays[] = {0U, 0U, 31U, 59U, 90U, 120U, 151U, 181U, 212U, 243U, 273U, 304U, 334U};
    uint64_t seconds; //*** Changed 32->64 bits ***

    /* Compute number of days from 1970 till given year*/
    seconds = ((uint64_t)datetime.year - 1970U) * DAYS_IN_A_YEAR;
    /* Add leap year days */
    seconds += (((uint64_t)datetime.year / 4U) - (1970U / 4U));
    /* Add number of days till given month*/
    seconds += (uint64_t)monthDays[datetime.month];
    /* Add days in given month. We subtract the current day as it is
     * represented in the hours, minutes and seconds field*/
    seconds += ((uint64_t)datetime.day - 1U);
    /* For leap year if month less than or equal to Febraury, decrement day counter*/
    if (((datetime.year & 3U) == 0x00U) && (datetime.month <= 2U))
    {
        seconds--;
    }

    seconds = (seconds * SECONDS_IN_A_DAY) + ((uint64_t)datetime.hour * SECONDS_IN_A_HOUR) +
              ((uint64_t)datetime.minute * SECONDS_IN_A_MINUTE) + (uint64_t)datetime.second;

    /* End FSL SDK code */

    return (seconds);
}