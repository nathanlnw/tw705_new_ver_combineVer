#ifndef _H_RS485
#define _H_RS485

#include "camera.h"

#ifndef false
#define false	0
#endif
#ifndef true
#define true	1
#endif

typedef u8 bool;

///定义RS485串口相关宏 START
#define RS485_UART		 			USART2
#define RS485_UART_IRQ		 		USART2_IRQn
#define RS485_GPIO_AF_UART		 	GPIO_AF_USART2
#define RS485_RCC_APBxPeriph_UART	RCC_APB1Periph_USART2

#define RS485_RCC_APBPeriphGPIO		RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC

#define RS485_TX_GPIO				GPIOA
#define RS485_TX_PIN				GPIO_Pin_2
#define RS485_TX_PIN_SOURCE			GPIO_PinSource2

#define RS485_RX_GPIO				GPIOA
#define RS485_RX_PIN				GPIO_Pin_3
#define RS485_RX_PIN_SOURCE			GPIO_PinSource3

#define RS485_PWR_PORT				GPIOB
#define RS485_PWR_PIN				GPIO_Pin_8

#define RS485_RXTX_PORT				GPIOC
#define RS485_RXTX_PIN				GPIO_Pin_4
///定义RS485串口相关宏 END

#define  RX_485const         GPIO_ResetBits(RS485_RXTX_PORT,RS485_RXTX_PIN)
#define  TX_485const         GPIO_SetBits(RS485_RXTX_PORT,RS485_RXTX_PIN)
#define  Power_485CH1_ON     GPIO_SetBits(RS485_PWR_PORT,RS485_PWR_PIN)  // 第一路485的电	       上电工作
#define  Power_485CH1_OFF      //GPIO_ResetBits(RS485_PWR_PORT,RS485_PWR_PIN)

/*串口接收缓存区定义*/

extern rt_size_t RS485_write( uint8_t *p, uint8_t len );
extern bool RS485_read_char(u8 *c);
extern void RS485_init( void );

#endif
