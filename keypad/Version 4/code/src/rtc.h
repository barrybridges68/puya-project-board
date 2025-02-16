#ifndef RTC_H
#define RTC_H

#define RTC_CONTROL_1_NORMAL    0x00U
#define RTC_CONTROL_1_STOP      (1U<<5U)

#define RTC_CONTROL_2_INACTIVE  0x00U
#define RTC_CONTROL_2_TI_TP     (1U<<4U)
#define RTC_CONTROL_2_TIE       (1U<<0U)

#define RTC_VL           0x80U
#define RTC_SECONDS      0x7FU
#define RTC_CENTURY      0x80U
#define RTC_MONTHS       0x1FU

#define RTC_CLOCKOUT_INHIBITED    0x00U

#define RTC_TIMER_CONTROL_TE    (1U<<7U)
#define RTC_TIMER_TD_64         (1U<<0U)

typedef enum
{
    Reg_Control_status_1 = 0x00,      //TEST1 N STOP N TESTC N N N
    Reg_Control_status_2 = 0x01,      //N N N TI_TP AF TF AIE TIE
    Reg_VL_seconds       = 0x02,      //VL SECONDS (0 to 59)
    Reg_Minutes          = 0x03,      //x MINUTES (0 to 59)
    Reg_Hours            = 0x04,      //x x HOURS (0 to 23)
    Reg_Days             = 0x05,      //x x DAYS (1 to 31)
    Reg_Weekdays         = 0x06,      //x x x x x WEEKDAYS (0 to 6)
    Reg_Century_months   = 0x07,      //C x x MONTHS (1 to 12)
    Reg_Years            = 0x08,      //YEARS (0 to 99)PCF8563 All information provided in this document is subject to legal disclaimers. © NXP Semiconductors N.V. 2015. All rights reserved.
    Reg_Minute_alarm     = 0x09,      //AE_M MINUTE_ALARM (0 to 59)
    Reg_Hour_alarm       = 0x0A,      //AE_H x HOUR_ALARM (0 to 23)
    Reg_Day_alarm        = 0x0B,      //AE_D x DAY_ALARM (1 to 31)
    Reg_Weekday_alarm    = 0x0C,      //AE_W x x x x WEEKDAY_ALARM (0 to 6)
    Reg_CLKOUT_control   = 0x0D,      //FE x x x x x FD[1:0]
    Reg_Timer_control    = 0x0E,      //TE x x x x x TD[1:0]
    Reg_Timer            = 0x0F,      //TIMER[7:0]
} pcf8563_reg_t;


/**
 * @brief  RTC Time and Date registers
 *
 * Registers 02h..08h
 *
 * All values are BCD.
 */
typedef struct
{
        uint8_t VL_seconds;
        uint8_t minutes;
        uint8_t hours;
        uint8_t days;
        uint8_t weekdays;
        uint8_t century_months;
        uint8_t years;
}rtc_date_time_t;

/**
 * @brief System Time
 *
 * All times are stored as UTC in seconds (Unix Epoch) without time zone correction.
 * The time zone offset is sstored so that the UTC can be converted to a local time as/when required.
 *
 */
typedef struct
{
    uint64_t utc_s;            ///< Number of seconds since 1970-01-01 00:00:00, Linux Epoch time, UTC
    int32_t  zone_offset_s;    ///< Number of seconds offset from UTC
} rtc_time_t;
extern rtc_time_t rtc_time;

void i2c_init(void);
void rtc_init(void);

/**
 * @fn bool rtc_update_time(uint8_t hour, uint8_t min, uint8_t sec)
 * @brief Updates time only
 *
 * @param [in]  hour bcd format
 * @param [in]  min  bcd format
 * @param [in]  sec  bcd format
 * @return      true if successful
 */
bool rtc_update_time(uint8_t hour, uint8_t min, uint8_t sec);

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
rtc_date_time_t rtc_sec_to_date_time(uint64_t seconds);

/**
 * @fn rtc_date_time_t rtc_sec_to_date_time_str(uint64_t seconds, char* date_time_str)
 * @brief Convert unix epoch time in seconds to date time into string format
 *
 *
 * @param [in]  seconds - epoch seconds
 * @param [in]  date_time_str - char buffer for the formated string.
 * @return      srtc_date_time_t
 */
rtc_date_time_t rtc_sec_to_date_time_str(uint64_t seconds, char* date_time_str);

/* @fn bool rtc_set_time(rtc_time_t* rtc_time)
 * @brief Set the RTC time
 *
 * @param[in]   rtc_time_t*
 * @return      true if success, false otherwise
 */
bool rtc_set_time(rtc_time_t* rtc_time);

/**
 * @fn rtc_time_t rtc_date_time_to_sec(rtc_date_time_t date_time_in)
 * @brief Convert date time in BCD format to unix epoch time in seconds
 *
 * Code from fsl_rtc.c RTC_ConvertDatetimeToSeconds(). Modified to be 64 bit.
 *
 * @param [in]  uint64_t
 * @return      seconds
 */
uint64_t rtc_date_time_to_sec(rtc_date_time_t date_time_in);

uint32_t rtc_get_seconds(void);

/**
 * @fn bool rtc_update_date(uint8_t year, uint8_t month, uint8_t day)
 * @brief Updates date only
 *
 * @param [in]  year  bcd format
 * @param [in]  month bcd format
 * @param [in]  day   bcd format
 * @return      true if successful
 */
bool rtc_update_date(uint8_t year, uint8_t month, uint8_t day);

#endif /* RTC_H */