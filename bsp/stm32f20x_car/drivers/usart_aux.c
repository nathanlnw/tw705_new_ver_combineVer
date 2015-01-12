/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/


/*ͬ����ͨ�ţ���ǰ֧��CAN,2xUART��
   ����
   <0x7e><chn><type><info><0x7e>

   ��ʹ���̣߳���ʱ�����յ����ݼ���
 */
#include <rtthread.h>

///����RS232������غ� START
#define RS232_UART		 			USART3
#define RS232_UART_IRQ		 		USART3_IRQn
#define RS232_GPIO_AF_UART		 	GPIO_AF_USART3
#define RS232_RCC_APBxPeriph_UART	RCC_APB1Periph_USART3

#define RS232_RCC_APBPeriphGPIO		RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOE

#define RS232_TX_GPIO				GPIOB
#define RS232_TX_PIN				GPIO_Pin_10
#define RS232_TX_PIN_SOURCE			GPIO_PinSource10

#define RS232_RX_GPIO				GPIOB
#define RS232_RX_PIN				GPIO_Pin_11
#define RS232_RX_PIN_SOURCE			GPIO_PinSource11

#define RS232_PWR_PORT				GPIOE
#define RS232_PWR_PIN				GPIO_Pin_7
///����RS232������غ� END


static struct timer tmr_aux;

#define UART_RS232_RXBUF_SIZE 128
static uint8_t rxbuf[UART_RS232_RXBUF_SIZE];
static uint8_t rxbuf_wr = 0;

struct rt_ringbuffer rb_rs232_rx;


static rt_tick_t last_tick = 0;


/**����3�жϷ������**/
void UART3_IRQHandler( void )
{
    rt_interrupt_enter( );
    if( USART_GetITStatus( RS232_UART, USART_IT_RXNE ) != RESET )
    {
        rt_ringbuffer_putchar(rb_rs232_rx, USART_ReceiveData( RS232_UART ));
        USART_ClearITPendingBit( RS232_UART, USART_IT_RXNE );
        last_tick = rt_tick_get( );
    }
    rt_interrupt_leave( );
}

ALIGN( RT_ALIGN_SIZE )
static char thread_aux_stack [256];
struct rt_thread thread_aux;

/*�������ݴ�������жϽ���,��ʱ����0x7e*/
static void rt_thread_entry_aux( void *parameter )
{
    uint8_t ch;
    static uint8_t rx[256];
    static uint8_t bytestuff = 0;
    uint16_t rx_wr = 0;
    while(1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND / 20);
        if(rt_tick_get() - last_tick < 10) continue;
        while(rt_ringbuffer_getchar(rb_rs232_rx, &ch) == 1)
        {
            if(ch == 0x7e)
            {
                if(rx_wr) break;
                else continue;
            }
            if(ch == 0x7d)
            {
                bytestuff = 1;
                continue;
            }
            if(bytestuff)
            {
                ch += 0x7c;
            }
            rx[rx_wr++] = ch;
        }


    }

}


/*����ӿڳ�ʼ��*/
void usart_aux_init( void )
{
    USART_InitTypeDef	USART_InitStructure;
    GPIO_InitTypeDef	GPIO_InitStructure;
    NVIC_InitTypeDef	NVIC_InitStructure;

    /*�����������ʱ��*/
    RCC_AHB1PeriphClockCmd( RS232_RCC_APBPeriphGPIO, ENABLE );
    RCC_APB1PeriphClockCmd( RS232_RCC_APBxPeriph_UART, ENABLE );

    /*���IO������*/
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

    ///5V3��Դ�ڳ�ʼ��
    GPIO_InitStructure.GPIO_Pin = RS232_PWR_PIN;
    GPIO_Init( RS232_PWR_PORT, &GPIO_InitStructure );
    GPIO_ResetBits( RS232_PWR_PORT, RS232_PWR_PIN );

    /*uartX �ܽ�����*/
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin		= RS232_TX_PIN;
    GPIO_Init( RS232_TX_GPIO, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin		= RS232_RX_PIN;
    GPIO_Init( RS232_RX_GPIO, &GPIO_InitStructure );

    GPIO_PinAFConfig( RS232_TX_GPIO, RS232_TX_PIN_SOURCE, RS232_GPIO_AF_UART );
    GPIO_PinAFConfig( RS232_RX_GPIO, RS232_RX_PIN_SOURCE, RS232_GPIO_AF_UART );

    //NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel						= RS232_UART_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    USART_InitStructure.USART_BaudRate				= 115200;
    USART_InitStructure.USART_WordLength			= USART_WordLength_8b;
    USART_InitStructure.USART_StopBits				= USART_StopBits_1;
    USART_InitStructure.USART_Parity				= USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode					= USART_Mode_Rx | USART_Mode_Tx;
    USART_Init( RS232_UART, &USART_InitStructure );
    /// Enable USART
    USART_Cmd( RS232_UART, ENABLE );
    /// ENABLE RX interrupts
    USART_ITConfig( RS232_UART, USART_IT_RXNE, ENABLE );


    rt_thread_init( &thread_aux,
                    "uart_aux",
                    rt_thread_entry_aux,
                    RT_NULL,
                    &thread_aux_stack [0],
                    sizeof( thread_aux_stack ), 16, 5 );
    rt_thread_startup( &thread_aux );

}

/************************************** The End Of File **************************************/
