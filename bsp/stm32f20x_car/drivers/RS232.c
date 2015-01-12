/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// 文件名
 * Author:			// 作者
 * Date:			// 日期
 * Description:		// 模块描述
 * Version:			// 版本信息
 * Function List:	// 主要函数及其功能
 *     1. -------
 * History:			// 历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/


/*同外设通信，当前支持CAN,2xUART，
   采用
   <0x7e><chn><type><info><0x7e>

   不使用线程，定时检查接收的数据即可
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

///定义RS232串口相关宏 START
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
///定义RS232串口相关宏 END

#define  Power_5V3_ON     GPIO_SetBits(POWER_5V3_GPIO_PORT,POWER_5V3_GPIO_PIN)  		///5V3电压开
#define  Power_5V3_OFF    GPIO_ResetBits(POWER_5V3_GPIO_PORT,POWER_5V3_GPIO_PIN)		///5V3电压关


#define UART_RS232_RXBUF_SIZE 256
static uint8_t uart_rs232_rxbuf[UART_RS232_RXBUF_SIZE];
static uint16_t uart_rs232_rxbuf_wr = 0;
static uint16_t uart_rs232_rxbuf_rd = 0;

static rt_tick_t last_tick = 0;
rt_tick_t 	last_tick_oil = 0;		///最后一次收到油耗数据的时刻，单位为tick
uint32_t 	oil_value = 0;			///邮箱油量，单位为1/10升
uint16_t 	oil_high = 0;			///邮箱油量高度实时值，单位为1/10mm
uint16_t 	oil_high_average = 0;	///邮箱油量高度平均实时值，单位为1/10mm

static uint8_t	oil_buf_rx[128];
static uint8_t	oil_buf_rx_len = 0;

static uint8_t	iccard_buf_rx[256];
static uint8_t	iccard_buf_rx_len = 0;

static uint8_t	ic_card_trans_step = 0;


/**串口3中断服务程序**/
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

/*接收数据处理，如何判断结束,超时还是0x7e*/
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
        while( uart_rs232_rxbuf_rd != uart_rs232_rxbuf_wr )   /*有数据时，保存数据*/
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




/*外设接口初始化*/
void rs232_init( void )
{
    USART_InitTypeDef	USART_InitStructure;
    GPIO_InitTypeDef	GPIO_InitStructure;
    NVIC_InitTypeDef	NVIC_InitStructure;

    /*开启相关外设时钟*/
    RCC_AHB1PeriphClockCmd( RS232_RCC_APBPeriphGPIO, ENABLE );
    RCC_APB1PeriphClockCmd( RS232_RCC_APBxPeriph_UART, ENABLE );

    /*相关IO口设置*/
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

    ///5V3电源口初始化
    GPIO_InitStructure.GPIO_Pin = RS232_PWR_PIN;
    GPIO_Init( RS232_PWR_PORT, &GPIO_InitStructure );
    GPIO_ResetBits( RS232_PWR_PORT, RS232_PWR_PIN );

    /*uartX 管脚设置*/
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

    //NVIC 设置
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
    //检查数据完整性,执行数据转换
    uint8_t		i;
    uint8_t		buf[32];
    uint8_t		commacount	= 0, count = 0;
    uint8_t		*psrc		= pinfo + 7; //指向开始位置
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
        case 1: /*协议相关内容，4字节*/
            break;

        case 2: /*硬件相关版本等，3字节*/
            break;

        case 3: /*设备上线时长，千时，百时，十时，个时，十分，个分；6字节*/
            break;

        case 4: /*平滑处理后的液位值；5字节*/
            if(count < 5)
                return 0;
            oil_high_average = AssicBufToUL(buf, 5);
            rt_kprintf("\n 液位平均值=%d", oil_high_average);
            break;
        case 5: /*数据滤波等级；1字节*/
            break;
        case 6: /*车辆运行和停止状态；2字节*/
            break;
        case 7: /*当前实时液位正负1cm范围内的液位个数；2字节*/
            break;
        case 8: /*接收信号灵明度，共85级；2字节*/
            break;
        case 9: /*信号强度，最大99；2字节*/
            break;
        case 10: /*实时液位，精度0.1mm；5字节*/
            if(count < 5)
                return 0;
            oil_high = AssicBufToUL(buf, 5);
            rt_kprintf(" 实时值=%d", oil_high);
            break;
        case 11: /*满量程，默认为800；5字节*/
            if(count < 5)
                return 0;
            temp_u32data = AssicBufToUL(buf, 5);
            //rt_kprintf(" 满量程=%d",temp_u32data);
            break;
        case 12: /*从"*"开始前面52个字符的和；2字节*/
            Ascii_To_Hex(&crc2, buf, 1);
            //rt_kprintf("\n   CRC2    =0x%2X",i);
            if((crc2 == crc) && (jt808_param.id_0xF04B))
            {
                temp_u32data = oil_high_average;
                temp_u32data = temp_u32data * jt808_param.id_0xF04B / 10000;
                if(last_tick_oil == 0)
                {
                    debug_write("油耗模块通信正常");
                }
                if((oil_value != temp_u32data) || (last_tick_oil == 0))
                {
                    oil_value = temp_u32data;
                    sprintf(buf, "油hv=%d,h=%d,%d", oil_high_average / 10, oil_high / 10, oil_value);
                    debug_write(buf);
                }
                last_tick_oil = rt_tick_get();
                rt_kprintf(" 剩余量=%d.%d升", oil_value / 10, oil_value % 10);
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
  *函数名称:void rs232_oil_buf_init(void)
  *功能描述:初始化油耗数据
  *输	入	:	none
  *输	出	:	none
  *返 回 值	:	void
  *作	者:白养民
  *创建日期:2014-10-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void rs232_oil_buf_init(void)
{
    memset(oil_buf_rx, 0, sizeof(oil_buf_rx));
    oil_buf_rx_len = 0;
}

/*********************************************************************************
  *函数名称:void rs232_oil(void)
  *功能描述:油耗处理函数，这个函数需要定时调用
  *输	入	:	none
  *输	出	:	none
  *返 回 值:	void
  *作	者:白养民
  *创建日期:2014-10-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
            debug_write("油耗模块异常!");
        oil_value = 0;
    }
    else
    {
        ;//oil_pack_proc();
    }
}


/*********************************************************************************
  *函数名称:void rs232_oil_rx(uint8_t ch)
  *功能描述:处理RS232接口上油耗模块的接收操作，这个函数放在RS232收到每个字节的处理函数中
  *输	入	:	ch	:接收到的单个字节
  *输	出	:	none
  *返 回 值	:	0:表示没有正确处理，1，表示接收到了合法数据
  *作	者:白养民
  *创建日期:2014-10-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
                //去掉后面的不可见字符
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
  *函数名称:uint8_t iccard_rx_proc(void)
  *功能描述:处理IC卡接收到数据包后的函数
  *输	入	:	none
  *输	出	:	none
  *返 回 值	:	0:表示没有正确处理，1，表示接收到了合法数据
  *作	者:白养民
  *创建日期:2014-10-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void  iccard_send_data(u8 CMD_TYPE, u8 *Srcstr, u16 inlen )
{
    u16 		len = 0, wr = 0, i = 0;
    u8			Reg[100];
    char		buf_send[2] = {0x7E, 0x00};
    char		buf_send1[2] = {0x7D, 0x01};
    char		buf_send2[2] = {0x7D, 0x02};

    //  stuff orginal info
    Reg[0]	= 0;                        // 留作校验
    Reg[1]	= 0x00;                     // 版本编号
    Reg[2]	= 0x01;
    Reg[3]	= 0x01;                     // 厂商ID
    Reg[4]	= 0x00;
    Reg[5]	= 0x0B;                 	// 外设类型
    Reg[6]	= CMD_TYPE;                 // 命令类型
    memcpy( Reg + 7, Srcstr, inlen );   //  信息内容
    len = wr = 7 + inlen;

    // caculate  add fcs
    for( i = 3; i < wr; i++ )
    {
        Reg[0] += Reg[i];
    }

    rt_kprintf("\n%d>TX_CARD:7E", rt_tick_get());
    printf_hex_data( Reg, wr );
    rt_kprintf("7E");
    //发送数据到ICCARD
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
  *函数名称:uint8_t iccard_rx_proc(void)
  *功能描述:处理IC卡接收到数据包后的函数
  *输	入	:	none
  *输	出	:	none
  *返 回 值	:	0:表示没有正确处理，1，表示接收到了合法数据
  *作	者:白养民
  *创建日期:2014-10-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
            debug_write("IC 卡读卡成功");
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
                memcpy( tempbuf + 1, iccard_buf_rx + 8, 64 ); //获取64个字节的卡片信息
                ic_card_trans_step = 1;					   	///开始传输第一步
                jt808_tx_ack_ex(iccard_socket->linkno, 0x0900, tempbuf, 65);
                debug_write("IC上传0900数据");
                return 1;
            }
            /*
            else
            {													   //Off line
            	tempbuf[0] = 0x01;
            	iccard_send_data( 0x40, tempbuf, 1 );
            	debug_write("尚未连到IC卡中心");
            	return 1;
            }
            */
            break;
        }
        case 0x01:  /*等待20分钟，使用0x43命令主动触发读取*/
        {
            debug_write( "IC卡未插入" );
            ic_card_para.card_change_mytime = mytime_now;
            if(ic_card_para.IC_Card_Checked == 2)
            {
                ic_card_para.IC_Card_Checked = 0;
                ic_card_para.card_state = 3;
                ///发送0702数据到IC卡服务器
                IC_CARD_jt808_0x0702(mast_socket->linkno, 0);
            }
            //  send back
            tempbuf[0] = 0x03;
            iccard_send_data(0x40, tempbuf, 1);
            break;
        }
        case 0x02:
        {
            debug_write( "IC卡读取失败" );
            //  send back
            tempbuf[0] = 0x03;
            iccard_send_data(0x40, tempbuf, 1);
            tts_write("IC卡读取失败", 12);
            break;
        }
        case 0x03:
        {
            debug_write("非从业资格证卡");
            //  send back
            tempbuf[0] = 0x03;
            iccard_send_data(0x40, tempbuf, 1);
            tts_write("非从业资格证卡", 14);
            break;
        }
        case 0x04:
        {
            debug_write( "IC卡被锁定" );
            //  send back
            tempbuf[0] = 0x03;
            iccard_send_data(0x40, tempbuf, 1);
            ic_card_para.card_state = 2;
            IC_CARD_jt808_0x0702(mast_socket->linkno, 0);
            tts_write("IC卡被锁定", 10);
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
        if( iccard_buf_rx[7] == 0x00 )				   // 获取驾驶身份信息内容
        {
            rt_kprintf( "\n读取驾驶员身份信息成功" );
            ic_card_para.card_state	= 0;
            memset(jt808_param.id_0xF008, 0, sizeof(jt808_param.id_0xF008));
            memset(jt808_param.id_0xF009, 0, sizeof(jt808_param.id_0xF009));
            memset(jt808_param.id_0xF00B, 0, sizeof(jt808_param.id_0xF00B));
            memset(jt808_param.id_0xF00C, 0, sizeof(jt808_param.id_0xF00C));
            i = *(iccard_buf_rx + 8);  /*姓名长度*/
            strncpy(jt808_param.id_0xF008, iccard_buf_rx + 9, i);

            strncpy(jt808_param.id_0xF00B, iccard_buf_rx + 9 + i, 20 );
            j = *(iccard_buf_rx + 29 + i); /*发证机关长度*/
            strncpy(jt808_param.id_0xF00C, iccard_buf_rx + 30 + i, j);

            memcpy(ic_card_para.IC_Card_valitidy, iccard_buf_rx + 30 + i + j, 4);

            rt_kprintf("\n-姓名:%s", jt808_param.id_0xF008);
            rt_kprintf("\n-证件号:%s", jt808_param.id_0xF00B);
            rt_kprintf("\n-发证机关:%s", jt808_param.id_0xF00C);
            rt_kprintf("\n-有效期至:");
            printf_hex_data(ic_card_para.IC_Card_valitidy, 4);
            IC_CARD_jt808_0x0702(mast_socket->linkno, 0);
        }
        else
        {
            rt_kprintf( "\n IC卡驾驶员信息读取失败" );
            tts_write("IC卡驾驶员信息读取失败", 22);
        }
        //  send back
        iccard_send_data( 0x41, NULL, 0 );
        break;
    }
    case 0x42:
    {
        rt_kprintf( "\n IC卡拔出0x42" );
        ic_card_para.card_change_mytime = mytime_now;
        if(ic_card_para.IC_Card_Checked == 2)
        {
            ic_card_para.IC_Card_Checked = 0;
            ic_card_para.card_state = 0;
            ///发送0702数据到IC卡服务器
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
  *函数名称:void rs232_iccard_buf_init(void)
  *功能描述:初始化IC卡数据
  *输	入	:	none
  *输	出	:	none
  *返 回 值	:	void
  *作	者:白养民
  *创建日期:2014-10-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void rs232_iccard_buf_init(void)
{
    memset(iccard_buf_rx, 0, sizeof(iccard_buf_rx));
    iccard_buf_rx_len = 0;
}

/*********************************************************************************
  *函数名称:void rs232_iccard(void)
  *功能描述:IC卡处理函数，这个函数需要定时调用
  *输	入	:	none
  *输	出	:	none
  *返 回 值	:	void
  *作	者:白养民
  *创建日期:2014-10-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void rs232_iccard(void)
{
    return;
}



/*********************************************************************************
  *函数名称:void rs232_iccard_rx(uint8_t ch)
  *功能描述:处理RS232接口上IC卡读写器的接收操作，这个函数放在RS232收到每个字节的处理函数中
  *输	入	:	ch	:接收到的单个字节
  *输	出	:	none
  *返 回 值	:	0:表示没有正确处理，1，表示接收到了合法数据
  *作	者:白养民
  *创建日期:2014-10-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t rs232_iccard_rx(uint8_t ch)
{
    uint8_t 	ret = 0;
    uint8_t 	fcs = 0;
    uint16_t 	i;
    static uint8_t	fstuff = 0;		///7D 7E转移使用该值

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
        if(iccard_buf_rx_len >= 7)		///小于7个字节不满足最少数据包要求，包头部分为7字节
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
  *函数名称:void rs232_proc(void)
  *功能描述:处理RS232接口上的所有数据收发操作
  *输	入:	para	:拍照处理结构体
   pic_id	:图片ID
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2014-10-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
    /*串口是否收到数据*/
    while( RS232_read_char( &ch ) )
    {
        if(rs232_oil_rx(ch))		///如果油耗数据处理OK则将其他数据清空
        {
            rs232_iccard_buf_init();
        }
        //else
        if(rs232_iccard_rx(ch))	///如果IC卡数据处理OK则将其他数据清空
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
