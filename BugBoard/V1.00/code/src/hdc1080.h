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

bool hdc1080_init(void);