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
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "serial.h"
#include "include.h"
#include <finsh.h>
#include "jt808.h"
#include "jt808_param.h"
#include "jt808_gps.h"
#include "SLE4442.h"


#define POWER_5V3_GPIO_PIN			GPIO_Pin_7  // PB11
#define POWER_5V3_GPIO_PORT			GPIOE

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

#define  Power_5V3_ON     GPIO_SetBits(POWER_5V3_GPIO_PORT,POWER_5V3_GPIO_PIN)  		///5V3��ѹ��
#define  Power_5V3_OFF    GPIO_ResetBits(POWER_5V3_GPIO_PORT,POWER_5V3_GPIO_PIN)		///5V3��ѹ��


#define UART_RS232_RXBUF_SIZE 256
static uint8_t uart_rs232_rxbuf[UART_RS232_RXBUF_SIZE];
static uint16_t uart_rs232_rxbuf_wr = 0;
static uint16_t uart_rs232_rxbuf_rd = 0;

static rt_tick_t last_tick = 0;
rt_tick_t 	last_tick_oil = 0;		///���һ���յ��ͺ����ݵ�ʱ�̣���λΪtick
uint32_t 	oil_value = 0;			///������������λΪ1/10��
uint16_t 	oil_high = 0;			///���������߶�ʵʱֵ����λΪ1/10mm
uint16_t 	oil_high_average = 0;	///���������߶�ƽ��ʵʱֵ����λΪ1/10mm

static uint8_t	oil_buf_rx[128];
static uint8_t	oil_buf_rx_len = 0;

static uint8_t	iccard_buf_rx[256];
static uint8_t	iccard_buf_rx_len = 0;

static uint8_t	ic_card_trans_step = 0;


/**����3�жϷ������**/
void USART3_IRQHandler( void )
{
    rt_interrupt_enter( );
    /*
    if( USART_GetITStatus( RS232_UART, USART_IT_RXNE ) != RESET )
    {
    	rt_ringbuffer_putchar(&rb_rs232_rx,USART_ReceiveData( RS232_UART ));
    	USART_ClearITPendingBit( RS232_UART, USART_IT_RXNE );
    	last_tick = rt_tick_get( );
    }
    */
    if( USART_GetITStatus( RS232_UART, USART_IT_RXNE ) != RESET )
    {
        uart_rs232_rxbuf[uart_rs232_rxbuf_wr++]	= USART_ReceiveData( RS232_UART );
        uart_rs232_rxbuf_wr					%= UART_RS232_RXBUF_SIZE;
        USART_ClearITPendingBit( RS232_UART, USART_IT_RXNE );
        last_tick = rt_tick_get( );
    }
    rt_interrupt_leave( );
}

static rt_size_t RS232_write( const uint8_t *buff, rt_size_t count )
{
    rt_size_t	len = count;
    uint8_t		*p	= (uint8_t *)buff;

    while( len )
    {
        USART_SendData( RS232_UART, *p++ );
        while( USART_GetFlagStatus( RS232_UART, USART_FLAG_TC ) == RESET )
        {
        }
        len--;
    }
    return RT_EOK;
}
FINSH_FUNCTION_EXPORT( RS232_write, write to RS232 );


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
        while( uart_rs232_rxbuf_rd != uart_rs232_rxbuf_wr )   /*������ʱ����������*/
        {
            ch				= uart_rs232_rxbuf[uart_rs232_rxbuf_rd++];
            uart_rs232_rxbuf_rd	%= UART_RS232_RXBUF_SIZE;
            //while(rt_ringbuffer_getchar(&rb_rs232_rx,&ch)==1)
            //{
            rt_kprintf("%c", ch);
            rx[0] = ch;
            rx[1] = '.';
            rx[2] = 0;
            RS232_write(rx, 2);
            /*
            	if(ch==0x7e)
            	{
            		if(rx_wr) break;
            		else continue;
            	}
            	if(ch==0x7d)
            	{
            		bytestuff=1;
            		continue;
            	}
            	if(bytestuff)
            	{
            		ch+=0x7c;
            	}
            	rx[rx_wr++]=ch;
            	*/
        }
    }
}




/*����ӿڳ�ʼ��*/
void rs232_init( void )
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

    USART_InitStructure.USART_BaudRate				= 9600;
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
    Power_5V3_ON;
    /*
    	//rt_ringbuffer_init(&rb_rs232_rx,uart_rs232_rxbuf,sizeof(uart_rs232_rxbuf));
    	rt_thread_init( &thread_aux,
    	                "uart_aux",
    	                rt_thread_entry_aux,
    	                RT_NULL,
    	                &thread_aux_stack [0],
    	                sizeof( thread_aux_stack ), 16, 5 );
    	rt_thread_startup( &thread_aux );
    	*/

}


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t RS232_read_char( u8 *c )
{
    if( uart_rs232_rxbuf_wr != uart_rs232_rxbuf_rd )
    {
        *c = uart_rs232_rxbuf[uart_rs232_rxbuf_rd++];
        if( uart_rs232_rxbuf_rd >= sizeof( uart_rs232_rxbuf ) )
        {
            uart_rs232_rxbuf_rd = 0;
        }
        return 1;
    }
    return 0;
}


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t process_YH( uint8_t *pinfo )
{
    //�������������,ִ������ת��
    uint8_t		i;
    uint8_t		buf[32];
    uint8_t		commacount	= 0, count = 0;
    uint8_t		*psrc		= pinfo + 7; //ָ��ʼλ��
    uint32_t    temp_u32data = 0;
    uint8_t		crc = 0, crc2;
    for(i = 0; i < 52; i++)
    {
        crc += pinfo[i];
    }
    //rt_kprintf("\n   CRC1    =0x%2X",crc);
    while( *psrc++ )
    {
        if(( *psrc != ',' ) && (*psrc != '#'))
        {
            buf[count++]	= *psrc;
            buf[count]		= 0;
            if(count + 1 >= sizeof(buf))
                return 0;
            continue;
        }
        commacount++;
        switch( commacount )
        {
        case 1: /*Э��������ݣ�4�ֽ�*/
            break;

        case 2: /*Ӳ����ذ汾�ȣ�3�ֽ�*/
            break;

        case 3: /*�豸����ʱ����ǧʱ����ʱ��ʮʱ����ʱ��ʮ�֣����֣�6�ֽ�*/
            break;

        case 4: /*ƽ��������Һλֵ��5�ֽ�*/
            if(count < 5)
                return 0;
            oil_high_average = AssicBufToUL(buf, 5);
            rt_kprintf("\n Һλƽ��ֵ=%d", oil_high_average);
            break;
        case 5: /*�����˲��ȼ���1�ֽ�*/
            break;
        case 6: /*�������к�ֹͣ״̬��2�ֽ�*/
            break;
        case 7: /*��ǰʵʱҺλ����1cm��Χ�ڵ�Һλ������2�ֽ�*/
            break;
        case 8: /*�����ź������ȣ���85����2�ֽ�*/
            break;
        case 9: /*�ź�ǿ�ȣ����99��2�ֽ�*/
            break;
        case 10: /*ʵʱҺλ������0.1mm��5�ֽ�*/
            if(count < 5)
                return 0;
            oil_high = AssicBufToUL(buf, 5);
            rt_kprintf(" ʵʱֵ=%d", oil_high);
            break;
        case 11: /*�����̣�Ĭ��Ϊ800��5�ֽ�*/
            if(count < 5)
                return 0;
            temp_u32data = AssicBufToUL(buf, 5);
            //rt_kprintf(" ������=%d",temp_u32data);
            break;
        case 12: /*��"*"��ʼǰ��52���ַ��ĺͣ�2�ֽ�*/
            Ascii_To_Hex(&crc2, buf, 1);
            //rt_kprintf("\n   CRC2    =0x%2X",i);
            if((crc2 == crc) && (jt808_param.id_0xF04B))
            {
                temp_u32data = oil_high_average;
                temp_u32data = temp_u32data * jt808_param.id_0xF04B / 10000;
                if(last_tick_oil == 0)
                {
                    debug_write("�ͺ�ģ��ͨ������");
                }
                if((oil_value != temp_u32data) || (last_tick_oil == 0))
                {
                    oil_value = temp_u32data;
                    sprintf(buf, "��hv=%d,h=%d,%d", oil_high_average / 10, oil_high / 10, oil_value);
                    debug_write(buf);
                }
                last_tick_oil = rt_tick_get();
                rt_kprintf(" ʣ����=%d.%d��", oil_value / 10, oil_value % 10);
                return 1;
            }
            break;
        }
        count	= 0;
        buf[0]	= 0;
    }
    return 0;
}

/*********************************************************************************
  *��������:void rs232_oil_buf_init(void)
  *��������:��ʼ���ͺ�����
  *��	��	:	none
  *��	��	:	none
  *�� �� ֵ	:	void
  *��	��:������
  *��������:2014-10-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void rs232_oil_buf_init(void)
{
    memset(oil_buf_rx, 0, sizeof(oil_buf_rx));
    oil_buf_rx_len = 0;
}

/*********************************************************************************
  *��������:void rs232_oil(void)
  *��������:�ͺĴ����������������Ҫ��ʱ����
  *��	��	:	none
  *��	��	:	none
  *�� �� ֵ:	void
  *��	��:������
  *��������:2014-10-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void rs232_oil(void)
{
    if(jt808_param.id_0xF04B == 0)
    {
        return;
    }
    if( rt_tick_get() - last_tick_oil > RT_TICK_PER_SECOND * 60 )
    {
        if(oil_value)
            debug_write("�ͺ�ģ���쳣!");
        oil_value = 0;
    }
    else
    {
        ;//oil_pack_proc();
    }
}


/*********************************************************************************
  *��������:void rs232_oil_rx(uint8_t ch)
  *��������:����RS232�ӿ����ͺ�ģ��Ľ��ղ����������������RS232�յ�ÿ���ֽڵĴ�������
  *��	��	:	ch	:���յ��ĵ����ֽ�
  *��	��	:	none
  *�� �� ֵ	:	0:��ʾû����ȷ����1����ʾ���յ��˺Ϸ�����
  *��	��:������
  *��������:2014-10-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t rs232_oil_rx(uint8_t ch)
{
    uint8_t 	ret = 0;
    uint16_t 	i;
    char		 *psrc;

    if(jt808_param.id_0xF04B == 0)
    {
        return 0;
    }
    if(ch > 0x1F)
    {
        oil_buf_rx[oil_buf_rx_len++] = ch;
        oil_buf_rx_len	%= sizeof(oil_buf_rx);
        oil_buf_rx[oil_buf_rx_len] = 0;
    }
    else
    {
        if(oil_buf_rx_len)
        {
            rt_kprintf("\n%d>232_RX:%s", rt_tick_get(), oil_buf_rx);
            if (strncmp(oil_buf_rx, "*YH", 3) == 0)
            {
                //ȥ������Ĳ��ɼ��ַ�
                for(i = oil_buf_rx_len - 1; i > 0; i--)
                {
                    if(oil_buf_rx[i] > 0x1F)
                        break;
                    else
                    {
                        oil_buf_rx[i] = 0;
                    }
                }
                ret = process_YH( (uint8_t *)oil_buf_rx );
            }
        }
        oil_buf_rx_len = 0;
        oil_buf_rx[oil_buf_rx_len] = 0;
    }
    return ret;
}

/*********************************************************************************
  *��������:uint8_t iccard_rx_proc(void)
  *��������:����IC�����յ����ݰ���ĺ���
  *��	��	:	none
  *��	��	:	none
  *�� �� ֵ	:	0:��ʾû����ȷ����1����ʾ���յ��˺Ϸ�����
  *��	��:������
  *��������:2014-10-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void  iccard_send_data(u8 CMD_TYPE, u8 *Srcstr, u16 inlen )
{
    u16 		len = 0, wr = 0, i = 0;
    u8			Reg[100];
    char		buf_send[2] = {0x7E, 0x00};
    char		buf_send1[2] = {0x7D, 0x01};
    char		buf_send2[2] = {0x7D, 0x02};

    //  stuff orginal info
    Reg[0]	= 0;                        // ����У��
    Reg[1]	= 0x00;                     // �汾���
    Reg[2]	= 0x01;
    Reg[3]	= 0x01;                     // ����ID
    Reg[4]	= 0x00;
    Reg[5]	= 0x0B;                 	// ��������
    Reg[6]	= CMD_TYPE;                 // ��������
    memcpy( Reg + 7, Srcstr, inlen );   //  ��Ϣ����
    len = wr = 7 + inlen;

    // caculate  add fcs
    for( i = 3; i < wr; i++ )
    {
        Reg[0] += Reg[i];
    }

    rt_kprintf("\n%d>TX_CARD:7E", rt_tick_get());
    printf_hex_data( Reg, wr );
    rt_kprintf("7E");
    //�������ݵ�ICCARD
    RS232_write(buf_send, 1 );
    for( i = 0; i < wr; i++ )
    {
        if( Reg[i] == 0x7E )
        {
            RS232_write( buf_send2, 2 );
        }
        else if( Reg[i] == 0x7D )
        {
            RS232_write(buf_send1, 2 );
        }
        else
        {
            RS232_write(&Reg[i], 1 );
        }
    }
    RS232_write( buf_send, 1 );
}
FINSH_FUNCTION_EXPORT_ALIAS( iccard_send_data, icsend, iccard_send_data );



/*********************************************************************************
  *��������:uint8_t iccard_rx_proc(void)
  *��������:����IC�����յ����ݰ���ĺ���
  *��	��	:	none
  *��	��	:	none
  *�� �� ֵ	:	0:��ʾû����ȷ����1����ʾ���յ��˺Ϸ�����
  *��	��:������
  *��������:2014-10-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t iccard_rx_proc(void)
{
    uint8_t 	ret = 1;
    uint16_t 	i, j, len;
    uint8_t		tempbuf[80];
    switch( iccard_buf_rx[6] )
    {
    case 0x40:
    {
        switch(iccard_buf_rx[7])
        {
        case 0x00:
        {
            if(iccard_buf_rx_len < 72)
                return 0;
            debug_write("IC �������ɹ�");
            if(device_unlock)
                sock_proc(iccard_socket, SOCK_CONNECT);
            ic_card_para.IC_Card_Checked = 2;
            ic_card_para.card_change_mytime = mytime_now;
            ic_card_para.card_state = 1;
            //if( iccard_socket->state == CONNECTED )
            {
                //  Online	   Trans  64 Data  to  Centre  , wait for 1 or 25 byte result,
                memset( tempbuf, 0, sizeof( tempbuf ) );
                tempbuf[0] = 0x0B;
                memcpy( tempbuf + 1, iccard_buf_rx + 8, 64 ); //��ȡ64���ֽڵĿ�Ƭ��Ϣ
                ic_card_trans_step = 1;					   	///��ʼ�����һ��
                jt808_tx_ack_ex(iccard_socket->linkno, 0x0900, tempbuf, 65);
                debug_write("IC�ϴ�0900����");
                return 1;
            }
            /*
            else
            {													   //Off line
            	tempbuf[0] = 0x01;
            	iccard_send_data( 0x40, tempbuf, 1 );
            	debug_write("��δ����IC������");
            	return 1;
            }
            */
            break;
        }
        case 0x01:  /*�ȴ�20���ӣ�ʹ��0x43��������������ȡ*/
        {
            debug_write( "IC��δ����" );
            ic_card_para.card_change_mytime = mytime_now;
            if(ic_card_para.IC_Card_Checked == 2)
            {
                ic_card_para.IC_Card_Checked = 0;
                ic_card_para.card_state = 3;
                ///����0702���ݵ�IC��������
                IC_CARD_jt808_0x0702(mast_socket->linkno, 0);
            }
            //  send back
            tempbuf[0] = 0x03;
            iccard_send_data(0x40, tempbuf, 1);
            break;
        }
        case 0x02:
        {
            debug_write( "IC����ȡʧ��" );
            //  send back
            tempbuf[0] = 0x03;
            iccard_send_data(0x40, tempbuf, 1);
            tts_write("IC����ȡʧ��", 12);
            break;
        }
        case 0x03:
        {
            debug_write("�Ǵ�ҵ�ʸ�֤��");
            //  send back
            tempbuf[0] = 0x03;
            iccard_send_data(0x40, tempbuf, 1);
            tts_write("�Ǵ�ҵ�ʸ�֤��", 14);
            break;
        }
        case 0x04:
        {
            debug_write( "IC��������" );
            //  send back
            tempbuf[0] = 0x03;
            iccard_send_data(0x40, tempbuf, 1);
            ic_card_para.card_state = 2;
            IC_CARD_jt808_0x0702(mast_socket->linkno, 0);
            tts_write("IC��������", 10);
            break;
        }
        }
        break;
    }
    case 0x41:
    {
        /*
        7E 55 00 01 01 00 0B 41 00
        06 C2 DE B3 A4 C0 D6
        36 32 30 31 32 33 31 39 37 33 30 35 30 33 39 31 31 32 00 00
        14 C0 BC D6 DD CA D0 B9 AB C2 B7 D4 CB CA E4 B9 DC C0 ED B4 A6
        20 15 05 01
        7E
        */
        if( iccard_buf_rx[7] == 0x00 )				   // ��ȡ��ʻ�����Ϣ����
        {
            rt_kprintf( "\n��ȡ��ʻԱ�����Ϣ�ɹ�" );
            ic_card_para.card_state	= 0;
            memset(jt808_param.id_0xF008, 0, sizeof(jt808_param.id_0xF008));
            memset(jt808_param.id_0xF009, 0, sizeof(jt808_param.id_0xF009));
            memset(jt808_param.id_0xF00B, 0, sizeof(jt808_param.id_0xF00B));
            memset(jt808_param.id_0xF00C, 0, sizeof(jt808_param.id_0xF00C));
            i = *(iccard_buf_rx + 8);  /*��������*/
            strncpy(jt808_param.id_0xF008, iccard_buf_rx + 9, i);

            strncpy(jt808_param.id_0xF00B, iccard_buf_rx + 9 + i, 20 );
            j = *(iccard_buf_rx + 29 + i); /*��֤���س���*/
            strncpy(jt808_param.id_0xF00C, iccard_buf_rx + 30 + i, j);

            memcpy(ic_card_para.IC_Card_valitidy, iccard_buf_rx + 30 + i + j, 4);

            rt_kprintf("\n-����:%s", jt808_param.id_0xF008);
            rt_kprintf("\n-֤����:%s", jt808_param.id_0xF00B);
            rt_kprintf("\n-��֤����:%s", jt808_param.id_0xF00C);
            rt_kprintf("\n-��Ч����:");
            printf_hex_data(ic_card_para.IC_Card_valitidy, 4);
            IC_CARD_jt808_0x0702(mast_socket->linkno, 0);
        }
        else
        {
            rt_kprintf( "\n IC����ʻԱ��Ϣ��ȡʧ��" );
            tts_write("IC����ʻԱ��Ϣ��ȡʧ��", 22);
        }
        //  send back
        iccard_send_data( 0x41, NULL, 0 );
        break;
    }
    case 0x42:
    {
        rt_kprintf( "\n IC���γ�0x42" );
        ic_card_para.card_change_mytime = mytime_now;
        if(ic_card_para.IC_Card_Checked == 2)
        {
            ic_card_para.IC_Card_Checked = 0;
            ic_card_para.card_state = 0;
            ///����0702���ݵ�IC��������
            IC_CARD_jt808_0x0702(mast_socket->linkno, 0);
        }
        // send back
        iccard_send_data( 0x42, NULL, 0 );
        break;
    }
    default :
    {
        break;
    }
    }
    return ret;
}




/*********************************************************************************
  *��������:void rs232_iccard_buf_init(void)
  *��������:��ʼ��IC������
  *��	��	:	none
  *��	��	:	none
  *�� �� ֵ	:	void
  *��	��:������
  *��������:2014-10-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void rs232_iccard_buf_init(void)
{
    memset(iccard_buf_rx, 0, sizeof(iccard_buf_rx));
    iccard_buf_rx_len = 0;
}

/*********************************************************************************
  *��������:void rs232_iccard(void)
  *��������:IC�������������������Ҫ��ʱ����
  *��	��	:	none
  *��	��	:	none
  *�� �� ֵ	:	void
  *��	��:������
  *��������:2014-10-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void rs232_iccard(void)
{
    return;
}



/*********************************************************************************
  *��������:void rs232_iccard_rx(uint8_t ch)
  *��������:����RS232�ӿ���IC����д���Ľ��ղ����������������RS232�յ�ÿ���ֽڵĴ�������
  *��	��	:	ch	:���յ��ĵ����ֽ�
  *��	��	:	none
  *�� �� ֵ	:	0:��ʾû����ȷ����1����ʾ���յ��˺Ϸ�����
  *��	��:������
  *��������:2014-10-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t rs232_iccard_rx(uint8_t ch)
{
    uint8_t 	ret = 0;
    uint8_t 	fcs = 0;
    uint16_t 	i;
    static uint8_t	fstuff = 0;		///7D 7Eת��ʹ�ø�ֵ

    if(ch != 0x7E)
    {
        if(ch == 0x7D)
        {
            fstuff = 0x7C;
        }
        else
        {
            iccard_buf_rx[iccard_buf_rx_len++] = ch + fstuff;
            iccard_buf_rx_len	%= sizeof(iccard_buf_rx);
            fstuff = 0x00;
        }
    }
    else if(iccard_buf_rx_len)
    {
        fstuff = 0;
        rt_kprintf("\n%d>RX_CARD:7E", rt_tick_get());
        printf_hex_data(iccard_buf_rx, iccard_buf_rx_len);
        rt_kprintf("7E");
        if(iccard_buf_rx_len >= 7)		///С��7���ֽڲ������������ݰ�Ҫ�󣬰�ͷ����Ϊ7�ֽ�
        {
            for(i = 3; i < iccard_buf_rx_len; i++)
            {
                fcs += iccard_buf_rx[i];
            }
            if((fcs == iccard_buf_rx[0]) && (0x0B == iccard_buf_rx[5]))
            {
                ret = iccard_rx_proc();
            }
        }
        rs232_iccard_buf_init();
    }
    return ret;
}

/*********************************************************************************
  *��������:void rs232_proc(void)
  *��������:����RS232�ӿ��ϵ����������շ�����
  *��	��:	para	:���մ���ṹ��
   pic_id	:ͼƬID
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2014-10-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void rs232_proc(void)
{
    u8			ch;

    if(device_control.off_counter)
    {
        Power_5V3_OFF;
        return;
    }
    rs232_oil();
    rs232_iccard();
    /*�����Ƿ��յ�����*/
    while( RS232_read_char( &ch ) )
    {
        if(rs232_oil_rx(ch))		///����ͺ����ݴ���OK�������������
        {
            rs232_iccard_buf_init();
        }
        //else
        if(rs232_iccard_rx(ch))	///���IC�����ݴ���OK�������������
        {
            rs232_oil_buf_init();
        }
    }
}

void power_5v3(uint8_t on)
{
    if(on)
    {
        Power_5V3_ON;
    }
    else
    {
        Power_5V3_OFF;
    }
}
FINSH_FUNCTION_EXPORT( power_5v3, power_5v3 on 1 off 2 );
/************************************** The End Of File **************************************/
