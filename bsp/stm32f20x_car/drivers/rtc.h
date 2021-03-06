#ifndef __RTC_H__
#define __RTC_H__

//#include <time.h>
/*
#include "stm32f2xx_rtc.h"
#include "stm32f2xx_rcc.h"
#include "stm32f2xx_pwr.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_usart.h"
*/
#include "include.h"


#include <time.h>


rt_err_t rtc_init(void);
void time_set(uint8_t hour, uint8_t min, uint8_t sec);
void date_set(uint8_t year, uint8_t month, uint8_t day);
void datetime( void ); /*返回当前的时间戳*/

#endif
