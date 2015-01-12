#ifndef _H_INCLUDE
#define _H_INCLUDE

#ifdef STM32F4XX
#include "stm32f4xx.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#endif

#ifdef STM32F2XX
#define  FONT_IS_OUT_FLASH
#include "stm32f2xx.h"
#include "stm32f2xx_rtc.h"
#include "stm32f2xx_rcc.h"
#include "stm32f2xx_pwr.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_usart.h"
#endif

#endif
