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

#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "include.h"
#include <board.h>
#include <finsh.h>

#include "jt808.h"
#include "jt808_param.h"
#include "jt808_sms.h"
#include "jt808_gps.h"
#include "camera.h"

#include "m66.h"
#include "hmi.h"

typedef rt_err_t ( *AT_RESP )( char *p, uint16_t len );

typedef struct
{
    char			*atcmd;
    enum RESP_TYPE	type;           /*判断是处理字符串比较,还是有响应函数*/
    AT_RESP			resp;
    char			*compare_str;   /*要比较的字符串*/
    uint16_t		timeout;
    uint16_t		retry;
} AT_CMD_RESP;

enum _sms_state
{
    SMS_IDLE = 0,
    SMS_WAIT_CMGR_DATA,
    SMS_WAIT_CMGR_OK,
    //SMS_WAIT_CMGD_OK,
    //SMS_WAIT_CMGS_GREATER,  /*等待发送的>*/
    //SMS_WAIT_CMGS_OK,       /*等待发送的>*/
};


enum _record_state
{
    RD_IDLE = 0,
    RD_RECORDING,
    RD_READING,
};

typedef enum
{
    MODEM_RES_NONE = 0,
    MODEM_RES_OK,
    MODEM_RES_ERROR,
} _MODEM_RES;


enum _sms_state sms_state = SMS_IDLE;
///定义GPRS串口相关宏 START
#define GSM_UART		 			UART4
#define GSM_UART_IRQ		 		UART4_IRQn
#define GSM_GPIO_AF_UART		 	GPIO_AF_UART4
#define GSM_RCC_APBxPeriph_UART		RCC_APB1Periph_UART4

#define GSM_RCC_APBPeriphGPIO		RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD

#define GSM_TX_GPIO					GPIOC
#define GSM_TX_PIN					GPIO_Pin_10
#define GSM_TX_PIN_SOURCE			GPIO_PinSource10

#define GSM_RX_GPIO					GPIOC
#define GSM_RX_PIN					GPIO_Pin_11
#define GSM_RX_PIN_SOURCE			GPIO_PinSource11

#define GSM_PWR_PORT				GPIOD
#define GSM_PWR_PIN					GPIO_Pin_13

#define GSM_TERMON_PORT 			GPIOD
#define GSM_TERMON_PIN				GPIO_Pin_12

#define GSM_RST_PORT				GPIOD
#define GSM_RST_PIN					GPIO_Pin_11

#define GSM_SOUND_PORT				GPIOD
#define GSM_SOUND_PIN				GPIO_Pin_9
///定义GPRS串口相关宏 END

#define GSM_RESET_TIME				360

/*声明一个gsm设备*/
static struct rt_device dev_gsm;

/*声明一个uart设备指针,同gsm模块连接的串口  指向一个已经打开的串口 */
static struct rt_mailbox	mb_gsmrx;
#define MB_GSMRX_POOL_SIZE 32
static uint8_t				mb_gsmrx_pool[MB_GSMRX_POOL_SIZE];


/*
   AT命令发送使用的mailbox
   供 VOICE TTS SMS TTS使用
 */
#define MB_TTS_POOL_SIZE 32
static struct rt_mailbox	mb_tts;
static uint8_t				mb_tts_pool[MB_TTS_POOL_SIZE];

#define MB_AT_TX_POOL_SIZE 32
static struct rt_mailbox	mb_at_tx;
static uint8_t				mb_at_tx_pool[MB_AT_TX_POOL_SIZE];

/* 消息邮箱控制块*/
static struct rt_mailbox mb_sms;
#define MB_SMS_RX_POOL_SIZE 32
/* 消息邮箱中用到的放置消息的内存池*/
static uint8_t mb_sms_pool[MB_SMS_RX_POOL_SIZE];

/*gsm 命令交互使用的信号量*/

static struct rt_semaphore sem_at;

#define GSM_RX_SIZE 2048
static uint8_t		gsm_rx[GSM_RX_SIZE];
static uint16_t		gsm_rx_wr = 0;

static T_GSM_STATE	gsm_state = GSM_IDLE;
static uint16_t 	gprs_rx_len = 0;


/*串口接收缓存区定义*/
#define UART_GSM_RX_SIZE 1024
static uint8_t	uart_gsm_rxbuf[UART_GSM_RX_SIZE];
static uint16_t uart_gsm_rxbuf_wr = 0, uart_gsm_rxbuf_rd = 0;

/*控制输出多少条信息*/
static uint32_t fgsm_rawdata_out = 0xfffffff;

/*最近一次收到串口数据的时刻,不使用sem作为超时判断*/
static uint32_t last_tick;

uint32_t modem_poweron_count = 0;			///模块没有正常登网连续重复开机次数
uint8_t	modem_creg_state = 0;				///模块注册状态，1和5表示注册成功，3表示注册被拒绝
uint8_t	modem_no_sim	= 0;				///该值为非0表示没有插入SIM卡
ENUM_MODEM_STYLE	modem_style = 0;			///模块类型，0表示未知，1表示为M66,2表示为M50。
static  uint8_t mobile_cops = 0;				///移动运行商，0表示中国移动，1表示中国联通



static uint32_t 	record_time = 0;			//单位为tick,该值为非0表示需要录音，并表示录音的时间长度
static char 		record_handle[32];		//M50模块打开的文件句柄
static uint8_t		record_save_ok = 0;		//为非0表示录音保存OK
static uint32_t       record_M66_bufwr = 0;   // M66 录音情况下 写数据地址寄存器
enum _record_state 	record_state = RD_IDLE;	//录音状态
uint8_t				ICCID[10];				//表示SIM卡的ICCID

uint8_t 			tel_state = 0;			//	0表示没有语言电话，1表示正常语音电话，2表示语音监听
uint8_t				tel_phonenum[16];		//	表示语音呼叫号码
extern MsgList	 *list_jt808_tx;


/*拨号登网的参数*/
struct _dial_param
{
    char	*apn;
    char	*user;
    char	*psw;
    uint8_t fconnect;
}				dial_param;

static uint8_t	sms_index;
STRUCT_GSM_PARAM gsm_param;


/***********************************************************
* Function:
* Description: uart4的中断服务函数
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void UART4_IRQHandler( void )
{
    rt_interrupt_enter( );
    if( USART_GetITStatus( GSM_UART, USART_IT_RXNE ) != RESET )
    {
        uart_gsm_rxbuf[uart_gsm_rxbuf_wr++]	= USART_ReceiveData( GSM_UART );
        uart_gsm_rxbuf_wr					%= UART_GSM_RX_SIZE;
        USART_ClearITPendingBit( GSM_UART, USART_IT_RXNE );
        last_tick = rt_tick_get( );
    }

    /*
       if (USART_GetITStatus(GSM_UART, USART_IT_TC) != RESET)
       {
       USART_ClearITPendingBit(GSM_UART, USART_IT_TC);
       }
     */
    rt_interrupt_leave( );
}

/***********************************************************
* Function:
* Description: 将小于0x20的字符忽略掉。并在结尾添加0，转为
   可见的字符串。
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/


/***********************************************************
* Function:
* Description: 将小于0x20的字符忽略掉。只保留数字部分。
               并在结尾添加0，转为可见的字符串。
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static uint16_t strip_numstring( char *str )
{
    char		*psrc, *pdst;
    uint16_t	len = 0;
    psrc	= str;
    pdst	= str;
    while( *psrc )
    {
        if( ( *psrc >= '0' ) && ( *psrc <= '9' ) )
        {
            *pdst++ = *psrc;
            len++;
        }
        else
        {
            if( len )
            {
                break;
            }
        }
        psrc++;
    }
    *pdst = 0;
    return len;
}
//uint32_t m66_ipr=115200;
uint32_t m66_ipr = 57600;

/***********************************************************
* Function:
* Description: 配置控电管脚，配置对应的串口设备uart4
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t m66_init( rt_device_t dev )
{
    GPIO_InitTypeDef	GPIO_InitStructure;
    USART_InitTypeDef	USART_InitStructure;
    NVIC_InitTypeDef	NVIC_InitStructure;

    /*开启相关外设时钟*/
    RCC_AHB1PeriphClockCmd( GSM_RCC_APBPeriphGPIO, ENABLE );
    RCC_APB1PeriphClockCmd( GSM_RCC_APBxPeriph_UART, ENABLE );

    /*相关IO口设置*/
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

    ///模块电源
    GPIO_InitStructure.GPIO_Pin = GSM_PWR_PIN;
    GPIO_Init( GSM_PWR_PORT, &GPIO_InitStructure );
    GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );

    ///开关模块控制
    GPIO_InitStructure.GPIO_Pin = GSM_TERMON_PIN;
    GPIO_Init( GSM_TERMON_PORT, &GPIO_InitStructure );
    GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );

    ///音频功放电源
#ifdef GSM_SOUND_PORT
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin		= GSM_SOUND_PIN;
    GPIO_Init( GSM_SOUND_PORT, &GPIO_InitStructure );
    GPIO_SetBits( GSM_SOUND_PORT, GSM_SOUND_PIN );
#endif

    ///模块reset管脚控制
    ///RESET在开机过程不需要做任何时序配合（和通常CPU 的 reset不同）。
    ///建议该管脚接OC输出的GPIO，开机时 OC 输出高阻。
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Pin = GSM_RST_PIN;
    GPIO_Init( GSM_RST_PORT, &GPIO_InitStructure );
    GPIO_SetBits( GSM_RST_PORT, GSM_RST_PIN );

    /*uartX 管脚设置*/
    /* Configure USART Tx as alternate function  */
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin		=  GSM_RX_PIN;
    GPIO_Init( GSM_RX_GPIO, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin		= GSM_TX_PIN;
    GPIO_Init( GSM_TX_GPIO, &GPIO_InitStructure );

    GPIO_PinAFConfig( GSM_TX_GPIO, GSM_TX_PIN_SOURCE, GSM_GPIO_AF_UART );
    GPIO_PinAFConfig( GSM_RX_GPIO, GSM_RX_PIN_SOURCE, GSM_GPIO_AF_UART );

    //NVIC 设置
    NVIC_InitStructure.NVIC_IRQChannel						= GSM_UART_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    USART_InitStructure.USART_BaudRate				= m66_ipr;
    USART_InitStructure.USART_WordLength			= USART_WordLength_8b;
    USART_InitStructure.USART_StopBits				= USART_StopBits_1;
    USART_InitStructure.USART_Parity				= USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode					= USART_Mode_Rx | USART_Mode_Tx;
    USART_Init( GSM_UART, &USART_InitStructure );
    /// Enable USART
    USART_Cmd( GSM_UART, ENABLE );
    /// ENABLE RX interrupts
    USART_ITConfig( GSM_UART, USART_IT_RXNE, ENABLE );

    return RT_EOK;
}

void set_ipr(u32 value)
{
    rt_device_t dev = RT_NULL;
    m66_ipr = value;
    m66_init(dev);
    gsmstate(GSM_POWEROFF);
}
FINSH_FUNCTION_EXPORT( set_ipr, control set_m66_ipr );

/***********************************************************
* Function:	提供给其他thread调用，打开设备，超时判断
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t m66_open( rt_device_t dev, rt_uint16_t oflag )
{
    if( gsm_state == GSM_IDLE )
    {
        gsm_state = GSM_POWERON; //置位上电过程中
    }
    return RT_EOK;
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
static rt_err_t m66_close( rt_device_t dev )
{
    gsm_state = GSM_POWEROFF; //置位断电过程中
    return RT_EOK;
}

/***********************************************************
* Function:m66_read
* Description:数据模式下读取数据
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_size_t m66_read( rt_device_t dev, rt_off_t pos, void *buff, rt_size_t count )
{
    return RT_EOK;
}

/* write one character to serial, must not trigger interrupt */
static void uart4_putc( const char c )
{
    USART_SendData( GSM_UART, c );
    while( !( GSM_UART->SR & USART_FLAG_TC ) )
    {
        ;
    }
    GSM_UART->DR = ( c & 0x1FF );
}

/***********************************************************
* Function:		m66_write
* Description:	数据模式下发送数据，要对数据进行封装
* Input:		const void* buff	要发送的原始数据
       rt_size_t count	要发送数据的长度
       rt_off_t pos		使用的socket编号
* Output:
* Return:
* Others:
***********************************************************/

static rt_size_t m66_write( rt_device_t dev, rt_off_t pos, const void *buff, rt_size_t count )
{
    rt_size_t	len = count;
    uint8_t		*p	= (uint8_t *)buff + pos;

    while( len )
    {
        USART_SendData( GSM_UART, *p++ );
        while( USART_GetFlagStatus( GSM_UART, USART_FLAG_TC ) == RESET )
        {
        }
        len--;
    }
    return RT_EOK;
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
static rt_err_t m66_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
    switch( cmd )
    {
    case CTL_STATUS:
        *( (int *)arg ) = gsm_state;
        break;
    case CTL_AT_CMD: //发送at命令,结果要返回
        break;
    case CTL_PPP:
        break;
    case CTL_SOCKET:
        break;
    }
    return RT_EOK;
}

#define RESP_PROCESS

/**/
_MODEM_RES resp_common_proc( char *p, uint16_t len )
{
    char *psrc = p;

    if( strncmp( psrc, "OK", 2 ) == 0 )
    {
        return MODEM_RES_OK;
    }

    if( strncmp( psrc, "ERROR", 5 ) == 0 )
    {
        return MODEM_RES_ERROR;
    }
    return MODEM_RES_NONE;
}



/**/
rt_err_t resp_strOK( char *p, uint16_t len )
{
    char *psrc = p;

    if( strstr( psrc, "OK" ) == RT_NULL )
    {
        return -RT_ERROR;
    }
    return RT_EOK;
}


/*
   相应ATI命令，根据该命令判断模块的类型M50或M66

 */
rt_err_t resp_ATI( char *p, uint16_t len )
{
    //rt_kprintf( "\ncimi len=%d  %02x %02x", len, *p, *( p + 1 ) );
    static char mobile[6];

    if( strstr( p, "Quectel_M50" ) != RT_NULL )
    {
        modem_style	= MODEM_M50;
        rt_kprintf("\n MODEM IS M50");
        return RT_EOK;
    }
    else if( strstr( p, "M66" ) != RT_NULL )
    {
        modem_style	= MODEM_M66;
        rt_kprintf("\n MODEM IS M66");
        return RT_EOK;
    }
    else if( strstr( p, "M69" ) != RT_NULL )
    {
        modem_style	= MODEM_M69;
        rt_kprintf("\n MODEM IS M69");
        return RT_EOK;
    }
    return RT_ERROR;
}



/*
   响应CREG或CGREG
   00017796 gsm_rx<+CREG: 0,3

   00018596 gsm_rx<+CREG: 0,1

   用
   i = sscanf( p, "%*[^:]:%d,%d", &n, &code );
   找有问题!(跟代码位置有关?)

 */
rt_err_t resp_CGREG( char *p, uint16_t len )
{
    char	 *pfind;
    char	 *psrc;
    char	c;
    psrc	= p;
    pfind	= strchr( psrc, ',' );
    if( pfind != RT_NULL )
    {
        c = *( pfind + 1 );
        if( ( c == '1' ) || ( c == '5' ) )
        {
            modem_creg_state = c - '0';
            return RT_EOK;
        }
        else if( c == '3' )
        {
            modem_creg_state = 3;
        }
    }
    return RT_ERROR;
}


/*
"%TSIM 0":无sim卡
"OK"
"%TSIM 1":有sim卡
"OK"
 */
rt_err_t resp_TSIM_M66( char *p, uint16_t len )
{
    static uint8_t no_sim_counter = 0;
    if( strncmp( p, "%TSIM 0", 7 ) == 0 )    /*没有SIM卡*/
    {
        no_sim_counter++;

        if(no_sim_counter >= 6)
        {
            modem_no_sim = 1;
            return RT_EOK;
        }
    }

    if( strncmp( p, "%TSIM 1", 7 ) == 0 )    /*有SIM卡*/
    {
        no_sim_counter	= 0;
        modem_no_sim 	= 0;
        return RT_EOK;
    }
    return RT_ERROR;
}

/*
   SIM卡的IMSI号码为4600 00783208249，
      460 00 18 23 20 86 42

   接口号数据字段的内容为IMSI号码的后12位
   其6个字节的内容为 0x00 0x07 0x83 0x20 0x82 0x49

 */
rt_err_t resp_CIMI( char *p, uint16_t len )
{
    //rt_kprintf( "\ncimi len=%d  %02x %02x", len, *p, *( p + 1 ) );
    static char mobile[6];
    if( len < 15 )
    {
        return RT_ERROR;
    }
    strip_numstring( p );
    mobile[0]	= ( ( *( p + 3 ) - '0' ) << 4 ) | ( *( p + 4 ) - '0' );
    mobile[1]	= ( ( *( p + 5 ) - '0' ) << 4 ) | ( *( p + 6 ) - '0' );
    mobile[2]	= ( ( *( p + 7 ) - '0' ) << 4 ) | ( *( p + 8 ) - '0' );
    mobile[3]	= ( ( *( p + 9 ) - '0' ) << 4 ) | ( *( p + 10 ) - '0' );
    mobile[4]	= ( ( *( p + 11 ) - '0' ) << 4 ) | ( *( p + 12 ) - '0' );
    mobile[5]	= ( ( *( p + 13 ) - '0' ) << 4 ) | ( *( p + 14 ) - '0' );
    strcpy( gsm_param.imsi, p );

    return RT_EOK;
}


/*
   SIM卡的IMSI号码为4600 00783208249，
      460 00 18 23 20 86 42

   接口号数据字段的内容为IMSI号码的后12位
   其6个字节的内容为 0x00 0x07 0x83 0x20 0x82 0x49

 */
rt_err_t resp_CIMI_M50( char *p, uint16_t len )
{
    //rt_kprintf( "\ncimi len=%d  %02x %02x", len, *p, *( p + 1 ) );
    static char mobile[6];
    static uint8_t no_sim_counter = 0;
    if(MODEM_RES_ERROR == resp_common_proc(p, len))
    {
        no_sim_counter++;
        if(no_sim_counter >= 6)
        {
            modem_no_sim = 1;
            return RT_EOK;
        }
    }
    if( len < 15 )
    {
        return RT_ERROR;
    }
    strip_numstring( p );
    mobile[0]	= ( ( *( p + 3 ) - '0' ) << 4 ) | ( *( p + 4 ) - '0' );
    mobile[1]	= ( ( *( p + 5 ) - '0' ) << 4 ) | ( *( p + 6 ) - '0' );
    mobile[2]	= ( ( *( p + 7 ) - '0' ) << 4 ) | ( *( p + 8 ) - '0' );
    mobile[3]	= ( ( *( p + 9 ) - '0' ) << 4 ) | ( *( p + 10 ) - '0' );
    mobile[4]	= ( ( *( p + 11 ) - '0' ) << 4 ) | ( *( p + 12 ) - '0' );
    mobile[5]	= ( ( *( p + 13 ) - '0' ) << 4 ) | ( *( p + 14 ) - '0' );
    strcpy( gsm_param.imsi, p );
    modem_no_sim = 0;
    no_sim_counter = 0;
    return RT_EOK;
}


/*
   SIM卡的ICCID号码为89860030020212620582
   接口号数据字段的内容为IMSI号码的后12位
   其6个字节的内容为 0x00 0x07 0x83 0x20 0x82 0x49

 */
rt_err_t resp_ICCID( char *p, uint16_t len )
{
    //rt_kprintf( "\ncimi len=%d  %02x %02x", len, *p, *( p + 1 ) );
    uint8_t	i;
    static uint8_t no_sim_counter = 0;
    if(MODEM_RES_ERROR == resp_common_proc(p, len))
    {
        no_sim_counter++;
        if(no_sim_counter >= 6)
        {
            modem_no_sim = 1;
            return RT_EOK;
        }
    }
    if( len < 20 )
    {
        return RT_ERROR;
    }

    for(i = 0; i < 20; i += 2)
    {
        ICCID[i / 2]	= ( ( p[i] - '0' ) << 4 ) | (  p[i + 1] - '0' );
    }
    modem_no_sim = 0;
    no_sim_counter = 0;
    return RT_EOK;
}


/*响应CGSN**/
rt_err_t resp_CGSN( char *p, uint16_t len )
{
    if( len < 15 )
    {
        return RT_ERROR;
    }
    strip_numstring( p );
    strcpy( gsm_param.imsi, p );
    return RT_EOK;
}

/* +CSQ: 31, 99 */
rt_err_t resp_CSQ( char *p, uint16_t len )
{
    uint32_t i, n, code;
    //i = sscanf( p, "+CSQ%*[^:]:%d,%d", &n, &code );
    i = sscanf( p, "+CSQ:%d,%d", &n, &code );
    gsm_param.csq = 0;

    if( i != 2 )
    {
        return RT_ERROR;
    }
    if((n < 99) && (n > 8))
    {
        gsm_param.csq = n;
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

/*
   %ETCPIP:1,"10.24.44.142","0.0.0.0","0.0.0.0"
   只查找第一对IP,  LocalIP

 */
rt_err_t resp_ETCPIP( char *p, uint16_t len )
{
    uint8_t stage	= 0;
    char	*psrc	= p;
    char	*pdst	= gsm_param.ip;

    memset( gsm_param.ip, 0, 15 );
    while( 1 )
    {
        if( stage == 0 )
        {
            if( *psrc == '"' )
            {
                stage = 1;
            }
            psrc++;
        }
        else
        {
            if( *psrc == '"' )
            {
                break;
            }
            *pdst = *psrc;
            pdst++;
            psrc++;
        }
    }
    rt_kprintf( "\nip=%s", gsm_param.ip );
    return RT_EOK;
}

/*
   AT%DNSR="www.google.com"
   %DNSR:74.125.153.147
   OK
 */

rt_err_t resp_DNSR_M50( char *p, uint16_t len )
{
    char *psrc = p;
    unsigned char ip[4];
    if(sscanf(psrc, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]) == 4)
    {
        memset( pcurr_socket->ip_addr, 0, 15 );
        memcpy( pcurr_socket->ip_addr, psrc, len );

        rt_kprintf( "\ndns ip=%s", pcurr_socket->ip_addr );
        return RT_EOK;
    }
    return RT_ERROR;
}

/*
   AT%DNSR="www.google.com"
   %DNSR:74.125.153.147
   OK
 */

rt_err_t resp_DNSR_M66( char *p, uint16_t len )
{
    char *psrc = p;

    if( strstr( psrc, "%DNSR:" ) == RT_NULL )
    {
        return RT_ERROR;
    }
    psrc = p;
    if( pcurr_socket != RT_NULL )
    {
        memset( pcurr_socket->ip_addr, 0, 15 );
        memcpy( pcurr_socket->ip_addr, psrc + 6, len - 6 );

        rt_kprintf( "\ndns ip=%s", pcurr_socket->ip_addr );
    }
    return RT_EOK;
}


rt_err_t resp_linking_M50( char *p, uint16_t len )
{
    char *psrc = p;
    char buf[64];
    sprintf( buf, "%d, CONNECT ", pcurr_socket->linkno);
    if( strncmp( p, buf, 11) == 0 )
    {
        if( strncmp( p + 11, "FAIL", 4) == 0 )
        {
            return RT_ERROR;
        }
        if( strncmp( p + 11, "OK", 2) == 0 )
        {
            return RT_EOK;
        }
    }
}


/**/
rt_err_t resp_IPOPENX( char *p, uint16_t len )
{
    char *psrc = p;
    if( strstr( psrc, "CONNECT" ) != RT_NULL )
    {
        return RT_EOK;
    }
    return RT_ERROR;
}

/**/
rt_err_t resp_DEBUG( char *p, uint16_t len )
{
    rt_kprintf( "\nresp_debug>%s", p );
    return RT_ERROR;
}



/*拨打电话回调函数**/
rt_err_t resp_TEL( char *p, uint16_t len )
{
    static uint8_t no_sim_counter = 0;
    if( strcmp( p, "NO DIALTONE" ) == 0 )    /*没有SIM卡*/
    {
        return RT_EOK;
    }
    else if( strcmp( p, "BUSY" ) == 0 )    /*没有SIM卡*/
    {
        return RT_EOK;
    }
    else if( strcmp( p, "NO CARRIER" ) == 0 )    /*没有SIM卡*/
    {
        return RT_EOK;
    }
    else if( strcmp( p, "OK" ) == 0 )    /*没有SIM卡*/
    {
        rt_kprintf("\n%d> tel ok!", rt_tick_get());
        return RT_EOK;
    }
    return RT_ERROR;
}





///////////////////////录音相关处理函数	START

rt_err_t resp_record_del_M50( char *p, uint16_t len )
{
    char *psrc = p;
    char buf[64];
    uint32_t temp_int;
    if(sscanf( p, "+CME ERROR:%d", &temp_int ) == 1)
    {
        if(temp_int == 4010)
            return RT_EOK;
    }
    if( strncmp( p, "OK", 2) == 0 )
    {
        rt_kprintf( "\n resp_record_del_OK!");
        return RT_EOK;
    }
    return RT_ERROR;
}



rt_err_t resp_recording_M50( char *p, uint16_t len )
{
    char *psrc = p;
    char buf[64];
    if( strncmp( p, "+QAUDRD: 0", 10) == 0 )
    {
        return RT_EOK;
    }
    if( strncmp( p, "+QAUDPIND:", 10) == 0 )
    {
        return RT_EOK;
    }
    return RT_ERROR;
}


rt_err_t resp_record_open_M50( char *p, uint16_t len )
{
    char *psrc = p;

    if( strncmp( p, "+QFOPEN:", 8) == 0 )
    {
        if(strlen(p + 8) < sizeof(record_handle))
        {
            memset(record_handle, 0, sizeof(record_handle));
            strcpy(record_handle, p + 8);
            strtrim(record_handle, ' ');
            return RT_EOK;
        }
    }
    return RT_ERROR;
}


rt_err_t resp_record_read_M50( char *p, uint16_t len )
{
    char *psrc = p;
    static uint32_t	infolen = 0;
    uint32_t		i, j;
    //static uint8_t	*buftemp[4096];
    uint8_t	*buftemp = photo_ram;
    static uint32_t	bufwr = 0;
    TypeDF_PackageHead	pack_head;

    if(p == RT_NULL)
    {
        infolen	= 0;					///
        bufwr	= 64;					///长度预留包头部分
        memset(buftemp, 0xFF, 4096);		///将包头部分清空
        record_save_ok = 0;
        return RT_ERROR;
    }

    rt_kprintf("\n resp_record_read_M50=%d", len);
    if((infolen) && (len == infolen))
    {
        rt_kprintf( "\n%d gsm<", rt_tick_get( ));
        printf_hex_data(psrc, infolen);
        for(i = 0; i < len; i++)
        {
            buftemp[(bufwr++) % 4096] = psrc[i];
            if(bufwr % 4096 == 0)
            {
                Cam_Flash_SaveMediaData(bufwr - 4096, buftemp, 4096);
                memset(buftemp, 0xFF, 4096);
            }
        }

        if(len < 1024)
        {
            if(bufwr % 4096)
                Cam_Flash_SaveMediaData(bufwr / 4096 * 4096, buftemp, bufwr % 4096);
            goto FUNC_RECORD_END;
        }
        return RT_EOK;
    }
    else
    {
        infolen	= 0;
    }

    if(sscanf( p, "CONNECT%d", &infolen ) == 1)
    {
        rt_kprintf("\n Read Media_2=%d; \n", infolen);
        if(infolen == 0)
        {
            goto FUNC_RECORD_END;
        }
        return RT_ERROR;
    }

    return RT_ERROR;

FUNC_RECORD_END:
    rt_kprintf("\n Save Mdeia!");
    pack_head.Channel_ID	= 1;
    pack_head.TiggerStyle	= 1;
    pack_head.Media_Format	= 5;
    pack_head.Media_Style	= 1;
    pack_head.Time			= mytime_now;
    pack_head.Head			= CAM_HEAD;
    pack_head.State			= 0xFF;
    pack_head.id			= DF_PicParam.Last.Data_ID + 1;
    pack_head.Len 			= bufwr;
    memcpy( (uint8_t *) & ( pack_head.position ), (uint8_t *)&gps_baseinfo, 28 );
    Cam_Flash_SaveMediaData(0, (uint8_t *)&pack_head, sizeof(TypeDF_PackageHead));
    record_save_ok = 1;

    rt_kprintf("\n Media:\n");
    for(i = 0, j = 0;; j++)
    {
        if(Cam_Flash_RdPic(buftemp, (uint16_t *)&i, pack_head.id, j) != RT_EOK)
        {
            break;
        }

        printf_hex_data(buftemp, i);
    }
    rt_kprintf("\nEND\n");
    return RT_EOK;
}



/*
   获取设备网络运营商信息

 */
rt_err_t resp_COPS( char *p, uint16_t len )
{
    //rt_kprintf( "\ncimi len=%d  %02x %02x", len, *p, *( p + 1 ) );
    static char mobile[6];

    if( strstr( p, "CHINA UNICOM" ) != RT_NULL )		///中国联通
    {
        mobile_cops = 1;
        return RT_EOK;
    }
    else if( strstr( p, "CHINA MOBILE" ) != RT_NULL )	///中国移动
    {
        mobile_cops = 0;
        return RT_EOK;
    }
    else if( strncmp( p, "+COPS:" , 6) == 0 )			///其它网络都认为是中国移动
    {
        mobile_cops = 0;
        return RT_EOK;
    }
    return RT_ERROR;
}


///////////////////////录音相关处理函数	END


/*
   发送AT命令，并等待响应函数处理
   参照 code.google.com/p/gsm-playground的实现
 */

//rt_err_t gsm_send( AT_CMD_RESP* pat_cmd_resp )
rt_err_t gsm_send( char *atcmd,
                   RESP_FUNC respfunc,
                   char *compare_str,
                   uint8_t type,
                   uint32_t timeout,
                   uint8_t retry )

{
    rt_err_t		err;
    uint8_t			i;
    char			*pmsg;
    uint32_t		tick_start, tick_end;
    uint32_t		tm;
    __IO uint8_t	flag_wait;

    char			 *pinfo;
    uint16_t		len;

    ///2013.12.25 新增加的部分，目的是防止前面有漏处理的数据，不要影响当前要发送的命令的处理
    if(strlen(atcmd))
    {
        while( RT_EOK == rt_mb_recv( &mb_gsmrx, (rt_uint32_t *)&pmsg, 1 ))
        {
            rt_free( pmsg );
        }
    }

    for( i = 0; i < retry; i++ )
    {
        tick_start	= rt_tick_get( );
        tick_end	= tick_start + timeout;
        tm			= timeout;
        flag_wait	= 1;
        if( strlen( atcmd ) )                                       /*要发送字符串*/
        {
            rt_kprintf( "\n%d gsm>%s", tick_start, atcmd );
            m66_write( &dev_gsm, 0, atcmd, strlen( atcmd ) );
        }
        if( type == RESP_TYPE_NONE )                                /*不等，只是光发送，严格上retry=1,即只发送一次*/
        {
            return RT_EOK;
        }
        while( flag_wait )
        {
            err = rt_mb_recv( &mb_gsmrx, (rt_uint32_t *)&pmsg, tm );
            if( err == RT_EOK )                                     /*没有超时,判断信息是否正确*/
            {
                len		= ( *pmsg << 8 ) | ( *( pmsg + 1 ) );
                pinfo	= pmsg + 2;

                if( type >= RESP_TYPE_STR )
                {
                    if( strstr( pinfo, compare_str ) != RT_NULL )   /*找到了 todo:如果不是期望的字符串 如ERROR如何处理*/
                    {
                        rt_free( pmsg );
                        if( type == RESP_TYPE_STR_WITHOK )
                        {
                            goto lbl_send_wait_ok;
                        }
                        return RT_EOK;
                    }
                    /*
                    else if ( strncmp( pinfo, "ERROR", 5 ) == 0 )
                    {
                    	flag_wait = 0;
                    }
                    */
                }
                else if( respfunc( pinfo, len ) == RT_EOK )     /*找到了, 函数输入参数在这里*/
                {
                    rt_free( pmsg );                            /*释放*/
                    if( type == RESP_TYPE_FUNC_WITHOK )
                    {
                        goto lbl_send_wait_ok;
                    }
                    return RT_EOK;
                }
                rt_free( pmsg );                                /*释放*/
                /*计算剩下的超时时间,由于其他任务执行的延时，会溢出,要判断*/
                if( rt_tick_get( ) < tick_end )                 /*还没有超时*/
                {
                    tm = tick_end - rt_tick_get( );
                }
                else
                {
                    flag_wait = 0;
                }
            }
            else  /*已经超时*/
            {
                flag_wait = 0;
            }
        }
    }

    return ( -RT_ETIMEOUT );

lbl_send_wait_ok:
    pmsg	= RT_NULL;  /*重新开始等待*/
    err		= rt_mb_recv( &mb_gsmrx, (rt_uint32_t *)&pmsg, tm );
    if( err == RT_EOK ) /*没有超时,判断信息是否正确*/
    {
        if( strstr( pmsg + 2, "OK" ) != RT_NULL )
        {
            rt_free( pmsg );
            return RT_EOK;
        }
    }
    rt_free( pmsg );
    return RT_ERROR;
}

/*
   判断一个字符串是不是表示ip的str
   如果由[0..9|.] 组成
   '.' 0x2e   '/' 0x2f   '0' 0x30  '9' 0x39   简化一下。不判断 '/'
   返回值
   0:表示域名的地址
   1:表示是IP地址

 */
static uint8_t is_ipaddr( char *str )
{
    char *p = str;
    while( *p != NULL )
    {
        if( ( *p > '9' ) || ( *p < '.' ) )
        {
            return 0;
        }
        p++;
    }
    return 1;
}

#define MODULE_PROCESS

extern uint32_t 	updata_tick;


/*********************************************************************************
  *函数名称:void modem_reset_proc(void)
  *功能描述:当模块基本AT命令通信异常时调用该函数
  			当模块需要重启的时候调用该函数，该函数会根据模块重启的次数判断模块重启后
  			等待的时间，防止模块重复快速重启，对模块造成的损坏.
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-09-10
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void modem_reset_proc1(void)
{
    gsmstate(GSM_POWEROFF);
    if(modem_poweron_count > 6 )
    {
        device_control.off_rf_counter = GSM_RESET_TIME;
        rt_kprintf("\n GSM模块AT错误6次,将关闭%d秒后重启", GSM_RESET_TIME);
        if(rt_tick_get() < 120000)
            beep(4, 4, 5);
    }
}


/*********************************************************************************
  *函数名称:void modem_reset_proc(void)
  *功能描述:当模块登网异常时调用该函数
  			当模块需要重启的时候调用该函数，该函数会根据模块重启的次数判断模块重启后
  			等待的时间，防止模块重复快速重启，对模块造成的损坏
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-09-10
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void modem_reset_proc2(void)
{
    if(modem_poweron_count < 6 )
    {
        gsmstate(GSM_POWEROFF);
    }
    else
    {
        control_device(DEVICE_OFF_LINK, GSM_RESET_TIME);
        if(rt_tick_get() < 120000)
            beep(4, 4, 6);
    }
}

/*gsm供电的处理纤程*/
static void rt_thread_gsm_power_on( void *parameter )
{
    int			i, at_num;

    const AT_CMD_RESP *p_at_init;

    const AT_CMD_RESP at_init[] =
    {
        { "AT\r\n",		 	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 5  },
        { "ATE0\r\n",		 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },
        { "ATI\r\n",		 RESP_TYPE_FUNC, 		resp_ATI,	RT_NULL,		RT_TICK_PER_SECOND * 3, 3  },
        { "ATV1\r\n",		 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 5, 3  },
        { "AT+CGMR\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///
    };
    /*
    const AT_CMD_RESP at_init_m66[] =
    {
    	{ "AT%TSIM\r\n",	 RESP_TYPE_FUNC_WITHOK,	resp_TSIM_M66,RT_NULL,		RT_TICK_PER_SECOND * 2, 8  },
    	{ "AT+CMGF=0\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///短信模式0:PDU,1:TEXT
    	{ "AT+CNMI=1,1\r\n", RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///短信是否直接发送给TE
    	{ "AT+CPIN?\r\n",	 RESP_TYPE_STR_WITHOK,	RT_NULL,	"+CPIN: READY", RT_TICK_PER_SECOND * 2, 30 },	///查询PIN码
    	{ "AT+CIMI\r\n",	 RESP_TYPE_FUNC_WITHOK, resp_CIMI,	RT_NULL,		RT_TICK_PER_SECOND * 2, 10 },	///查询CIMI
    	{ "AT+QCCID\r\n",	 RESP_TYPE_FUNC,		resp_ICCID,RT_NULL,			RT_TICK_PER_SECOND * 2, 3  },		///这个是M66的命令
    	{ "AT+CLIP=1\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 10 },
    	{ "AT+CSQ\r\n",	 	 RESP_TYPE_FUNC_WITHOK,	resp_CSQ,	RT_NULL,		RT_TICK_PER_SECOND * 2, 30 },
    	{ "AT+CREG?\r\n",	 RESP_TYPE_FUNC_WITHOK, resp_CGREG, RT_NULL,		RT_TICK_PER_SECOND * 2, 30 },
    	{ "AT+COPS?\r\n",	 RESP_TYPE_FUNC_WITHOK,	resp_COPS,	RT_NULL,		RT_TICK_PER_SECOND * 2, 3  }
    };
    */
    const AT_CMD_RESP at_init_m69[] =
    {
#if 0
        { "AT+CLVL=6\r\n",   RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置喇叭音量为6,范围0~6
        { "AT%SNFS=1\r\n",   RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置音频输出通道选择 第二路
        { "AT%NFI=1,12,0,0\r\n",   RESP_TYPE_STR,	RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置音频输入通道 选择第 二路
        { "AT%NFO=1,6,0\r\n",	 RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///设置输出音量
        { "AT%NFV=5\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///扬声器设置  --没有音频功放 没用
        { "AT%NFW=1\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///扬声器设置  --没有音频功放 没用
        { "AT%NFS=1\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///扬声器设置  --没有音频功放 没用
#endif
        { "AT+CLVL=6\r\n",   RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置喇叭音量为6,范围0~6
        { "AT%SNFS=1\r\n",   RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置音频输出通道选择 第二路
        { "AT%NFI=1,8,0,0\r\n",   RESP_TYPE_STR,	RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置音频输入通道 选择第 二路
        { "AT%NFO=1,6,0\r\n",	 RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///设置输出音量
        { "AT%NFV=5\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///扬声器设置  --没有音频功放 没用
        { "AT%NFW=2\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///扬声器设置  --没有音频功放 没用
        { "AT%NFS=2\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///扬声器设置  --没有音频功放 没用
        { "AT+SSAM=1\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///设置声音模式
        //{ "AT+SPEAKER=1,0\r\n",	 RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///设置MIC和SPEAKER的通道，0为主通道，1为辅助通道
        //{ "AT%VLB=1\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///开启回波消除
        { "AT+VGR=9\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 3  },	///通话最大音量
        { "AT%TSIM\r\n",	 RESP_TYPE_FUNC_WITHOK,	resp_TSIM_M66, RT_NULL,		RT_TICK_PER_SECOND * 2, 8  },
        { "AT+CMGF=0\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///短信模式0:PDU,1:TEXT
        { "AT+CNMI=1,1\r\n", RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///短信是否直接发送给TE
        { "AT+CPIN?\r\n",	 RESP_TYPE_STR_WITHOK,	RT_NULL,	"+CPIN: READY", RT_TICK_PER_SECOND * 2, 30 },	///查询PIN码
        { "AT+CIMI\r\n",	 RESP_TYPE_FUNC_WITHOK, resp_CIMI,	RT_NULL,		RT_TICK_PER_SECOND * 2, 10 },	///查询CIMI
        //{ "AT+QCCID\r\n",	 RESP_TYPE_FUNC,		resp_ICCID,RT_NULL,		RT_TICK_PER_SECOND * 2, 3 },		///这个是M66的命令
        { "AT+CLIP=1\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 10 },
        { "AT+CSQ\r\n",	 	 RESP_TYPE_FUNC_WITHOK,	resp_CSQ,	RT_NULL,		RT_TICK_PER_SECOND * 2, 30 },
        { "AT+CREG?\r\n",	 RESP_TYPE_FUNC_WITHOK, resp_CGREG, RT_NULL,		RT_TICK_PER_SECOND * 2, 30 },
        { "AT+COPS?\r\n",	 RESP_TYPE_FUNC_WITHOK,	resp_COPS,	RT_NULL,		RT_TICK_PER_SECOND * 2, 3  }
    };
    const AT_CMD_RESP at_init_m50[] =
    {
        { "AT+QIMUX=1\r\n", 	RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///允许多路连接
        { "AT+IPR=57600\r\n",	RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },
        { "AT+QAUDCH=2\r\n",	RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置音频通道,范围:0,1,2
        { "AT+CLVL=30\r\n", 	RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置喇叭音量为30,范围0~100
        { "AT+CRSL=30\r\n", 	RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置媒体播放音量为30,范围0~100
        { "AT+QMEDVL=30\r\n",	RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///设置铃音音量为30,范围0~100
        { "AT+CIMI\r\n",		RESP_TYPE_FUNC,		resp_CIMI_M50, RT_NULL,		RT_TICK_PER_SECOND * 2, 10 },
        { "AT+QCCID\r\n",		RESP_TYPE_FUNC,		resp_ICCID,	RT_NULL,		RT_TICK_PER_SECOND * 2, 3 },
        { "AT+CSQ\r\n", 		RESP_TYPE_FUNC_WITHOK, resp_CSQ,	RT_NULL,	RT_TICK_PER_SECOND * 2, 30 },
        //{ "AT+CSQ\r\n", 		RESP_TYPE_STR, 		RT_NULL,	"ERROR",		RT_TICK_PER_SECOND * 2, 5 },
        { "AT+CMGF=0\r\n",	 	RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///短信模式0:PDU,1:TEXT
        { "AT+CSCS=\"GSM\"\r\n", RESP_TYPE_STR,		RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },	///短信模式0:PDU,1:TEXT
        { "AT+CREG?\r\n",	 RESP_TYPE_FUNC_WITHOK, resp_CGREG, RT_NULL,		RT_TICK_PER_SECOND * 2, 30 },
        { "AT+COPS?\r\n",	 RESP_TYPE_FUNC_WITHOK,	resp_COPS,	RT_NULL,		RT_TICK_PER_SECOND * 2, 3  }
    };
    gsm_param.csq = 0;
    //if(jt808_param.id_0xF00F == 0)
    //	return;
    modem_poweron_count++;
    updata_tick = 0;
lbl_poweron_start:
    debug_write("通信模块上电!");
    modem_creg_state = 0;
    ///先关掉电源
    GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );
    GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
    ///等待一小会放电
    rt_thread_delay( RT_TICK_PER_SECOND * 3);
    ///开电源然后等待500ms
    GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );
    rt_kprintf( "\n%d gsm_power_on>start", rt_tick_get( ) );
    rt_thread_delay( RT_TICK_PER_SECOND / 2 );
    ///开PWRKEY管脚为低 然后等待2秒
    GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
    rt_thread_delay( RT_TICK_PER_SECOND * 3);
    GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );			//20141122
    modem_no_sim = 0;
    ///先发送通用命令，检测模块类型
    for( i = 0; i < sizeof( at_init ) / sizeof( AT_CMD_RESP ); i++ )
    {
        if( gsm_send( at_init[i].atcmd, \
                      at_init[i].resp, \
                      at_init[i].compare_str, \
                      at_init[i].type, \
                      at_init[i].timeout, \
                      at_init[i].retry ) != RT_EOK )
        {
            /*todo 错误计数，通知显示*/
            rt_kprintf( "\n%d 模块AT错误=%d", rt_tick_get( ), i );
            debug_write("模块AT错误");
            modem_reset_proc1( );
            return;
            //goto lbl_poweron_start;
        }
    }

    if(modem_style == MODEM_M66)
    {
        p_at_init 	= at_init_m69;
        at_num		= sizeof( at_init_m69) / sizeof( AT_CMD_RESP );
    }
    else if(modem_style == MODEM_M50)
    {
        p_at_init = at_init_m50;
        at_num		= sizeof( at_init_m50) / sizeof( AT_CMD_RESP );
    }
    else if(modem_style == MODEM_M69)
    {
        p_at_init = at_init_m69;
        at_num		= sizeof( at_init_m69) / sizeof( AT_CMD_RESP );
    }
    else
    {
        modem_reset_proc1();
        return;
        //goto lbl_poweron_start;
    }
    for( i = 0; i < at_num; i++ )
    {
        if( gsm_send( p_at_init[i].atcmd, \
                      p_at_init[i].resp, \
                      p_at_init[i].compare_str, \
                      p_at_init[i].type, \
                      p_at_init[i].timeout, \
                      p_at_init[i].retry ) != RT_EOK )
        {
            /*todo 错误计数，通知显示*/
            rt_kprintf( "\n%d gsm_power_on>error index=%d", rt_tick_get( ), i );
            if( 3 == modem_creg_state)
            {
                if(modem_poweron_count > 6 )
                {
                    device_control.off_link_counter = GSM_RESET_TIME;
                }
                else
                {
                    device_control.off_link_counter = 10;
                }
                debug_write("SIM卡注册被拒绝");
                rt_kprintf("\n SIM卡注册被拒绝，等待%d秒后重启模块", device_control.off_link_counter);
                if((rt_tick_get() < 18000) && (printf_on))
                    tts("数据卡欠费或网络异常");
                break;
            }

            modem_reset_proc1();
            return;
            //goto lbl_poweron_start;
        }
        if( modem_no_sim )
        {
            debug_write("无SIM卡");
            beep(2, 2, 3);
            break;
        }
        /*
        if( 3 == modem_creg_state)
        {
        	rt_kprintf("\n SIM卡注册被拒绝");
        	beep(2,2,1);
        	break;
        }
        */
    }
    debug_write("通信模块初始化OK!");
    gsm_state = GSM_AT; /*当前出于AT状态,可以拨号，连接*/
}



static void rt_thread_gsm_power_off( void )
{
    if(modem_style == MODEM_M50)
    {
        ///设置PWRKEY管脚为低 然后等待800ms
        GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
        rt_thread_delay( RT_TICK_PER_SECOND);
        GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
        rt_thread_delay( RT_TICK_PER_SECOND / 10 * 8 );
        ///设置PWRKEY管脚为高 然后等待13s
        GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
        rt_thread_delay( RT_TICK_PER_SECOND * 13 );
        ///关掉模块的电源
        GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );
    }
    else
    {
        if(modem_style == MODEM_M66)
        {
            gsm_send( "AT+CFUN=0", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
            gsm_send( "AT%MSO", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        }
        GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
        rt_thread_delay( RT_TICK_PER_SECOND / 10 * 7 );
        ///设置PWRKEY管脚为高 然后等待6s
        GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
        rt_thread_delay( RT_TICK_PER_SECOND * 6 );
        ///关掉模块的电源
        GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );
        //////////////end
    }
    gsm_state = GSM_IDLE;
    sd_write_console("GSM模块POWER_OFF!");
}


/*控制登网，还是断网*/
static void rt_thread_gsm_gprs_M50( void *parameter )
{
    char		buf[128];
    rt_err_t	err;

    /*判断要执行怎样的动作*/

    if( dial_param.fconnect == 0 ) /*断网*/
    {
        err = gsm_send( "AT+QICLOSE=1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        err = gsm_send( "AT+QICLOSE=2\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );

        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        err = gsm_send( "AT+QICLOSE=3\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );

        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        err = gsm_send( "AT+CGATT=0\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
        //err = gsm_send( "", RT_NULL, "%IPCLOSE:5", RESP_TYPE_STR, RT_TICK_PER_SECOND, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        gsm_state = GSM_AT;
        goto lbl_gsm_gprs_end;
    }

    if( dial_param.fconnect == 1 ) /*允许登网*/
    {
        err = gsm_send( "AT+CGATT=1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 12 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        sprintf( buf, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", dial_param.apn );

        err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        err = gsm_send( "AT+CGACT=1,1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 3 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        err = gsm_send( "AT+CGPADDR=1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        gsm_state = GSM_TCPIP;
        debug_write("通信模块登网OK!");
        goto lbl_gsm_gprs_end;
    }

lbl_gsm_gprs_end_err:
    debug_write("通信模块登网错误!");
    modem_reset_proc2();
    //gsm_state = GSM_POWEROFF;
lbl_gsm_gprs_end:
    rt_kprintf( "\n%08d gsm_gprs>gsm_state=%d", rt_tick_get( ), gsm_state );
}


/*控制登网，还是断网*/
static void rt_thread_gsm_gprs_M66_OLD( void *parameter )
{
    char		buf[128];
    rt_err_t	err;

    /*判断要执行怎样的动作*/

    if( dial_param.fconnect == 0 ) /*断网*/
    {
        err = gsm_send( "AT%IPCLOSE=1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        err = gsm_send( "AT%IPCLOSE=2\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );

        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        err = gsm_send( "AT%IPCLOSE=3\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );

        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        err = gsm_send( "AT%IPCLOSE=5\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
        err = gsm_send( "", RT_NULL, "%IPCLOSE:5", RESP_TYPE_STR, RT_TICK_PER_SECOND, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        gsm_state = GSM_AT;
        goto lbl_gsm_gprs_end;
    }

    if( dial_param.fconnect == 1 ) /*允许登网*/
    {
        err = gsm_send( "AT+CGATT?\r\n", RT_NULL, "+CGATT: 1", RESP_TYPE_STR_WITHOK, RT_TICK_PER_SECOND * 2, 20 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        sprintf( buf, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", dial_param.apn );

        err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 2 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        if( ( strlen( dial_param.user ) == 0 ) && ( strlen( dial_param.psw ) == 0 ) )
        {
            err = gsm_send( "AT%ETCPIP\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 151, 1 );
        }
        else
        {
            sprintf( buf, "AT%ETCPIP=\"%s\",\"%s\"\r\n", dial_param.user, dial_param.psw );
            err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 151, 1 );
        }
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        err = gsm_send( "AT%ETCPIP?\r\n", resp_ETCPIP, RT_NULL, RESP_TYPE_FUNC_WITHOK, RT_TICK_PER_SECOND * 10, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        err = gsm_send( "AT%IOMODE=1,2,1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        gsm_state = GSM_TCPIP;
        goto lbl_gsm_gprs_end;
    }
lbl_gsm_gprs_end_err:
    rt_kprintf( "\n登网错误" );
    modem_reset_proc2();
    //gsm_state = GSM_POWEROFF;
lbl_gsm_gprs_end:
    rt_kprintf( "\n%08d gsm_gprs>gsm_state=%d", rt_tick_get( ), gsm_state );
}



/*控制登网，还是断网*/
static void rt_thread_gsm_gprs_M69( void *parameter )
{
    char		buf[128];
    rt_err_t	err;

    /*判断要执行怎样的动作*/

    if( dial_param.fconnect == 0 ) /*断网*/
    {
        err = gsm_send( "AT%IPCLOSE=1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        err = gsm_send( "AT%IPCLOSE=2\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );

        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        err = gsm_send( "AT%IPCLOSE=3\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );

        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        err = gsm_send( "AT%IPCLOSE=5\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
        err = gsm_send( "", RT_NULL, "%IPCLOSE:5", RESP_TYPE_STR, RT_TICK_PER_SECOND, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }
        gsm_state = GSM_AT;
        goto lbl_gsm_gprs_end;
    }

    if( dial_param.fconnect == 1 ) /*允许登网*/
    {
        /*
        	err = gsm_send( "AT+CGATT=1\r\n", RT_NULL, "OK", RESP_TYPE_STR_WITHOK, RT_TICK_PER_SECOND * 2, 20 );
        	if( err != RT_EOK )
        	{
        		goto lbl_gsm_gprs_end_err;
        	}

        	err = gsm_send( "AT+CGATT?\r\n", RT_NULL, "+CGATT: 1", RESP_TYPE_STR_WITHOK, RT_TICK_PER_SECOND * 2, 20 );
        	if( err != RT_EOK )
        	{
        		goto lbl_gsm_gprs_end_err;
        	}
        	*/

        sprintf( buf, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", dial_param.apn );

        err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 2 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        err = gsm_send( "AT%IOMODE=1,2,1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        if( ( strlen( dial_param.user ) == 0 ) && ( strlen( dial_param.psw ) == 0 ) )
        {
            err = gsm_send( "AT%ETCPIP\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 60, 1 );
        }
        else
        {
            sprintf( buf, "AT%ETCPIP=\"%s\",\"%s\"\r\n", dial_param.user, dial_param.psw );
            err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 60, 1 );
        }
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        err = gsm_send( "AT%ETCPIP?\r\n", resp_ETCPIP, RT_NULL, RESP_TYPE_FUNC_WITHOK, RT_TICK_PER_SECOND * 10, 1 );
        if( err != RT_EOK )
        {
            goto lbl_gsm_gprs_end_err;
        }

        gsm_state = GSM_TCPIP;
        debug_write("通信模块登网OK!");
        goto lbl_gsm_gprs_end;
    }
lbl_gsm_gprs_end_err:
    debug_write("通信模块登网错误!");
    modem_reset_proc2();
    //gsm_state = GSM_POWEROFF;
lbl_gsm_gprs_end:
    rt_kprintf( "\n%08d gsm_gprs>gsm_state=%d", rt_tick_get( ), gsm_state );
}


/*控制登网，还是断网*/
static void rt_thread_gsm_gprs( void *parameter )
{
    //if((modem_no_sim)||(device_control.off_link_counter)||(jt808_param.id_0xF00F == 0))
    if((modem_no_sim) || (device_control.off_link_counter))
        return;
    if( 3 == modem_creg_state)
    {
        gsmstate( GSM_POWEROFF );
        return;
    }
    if(modem_style == MODEM_M50)
    {
        rt_thread_gsm_gprs_M50(parameter);
    }
    else
    {
        rt_thread_gsm_gprs_M69(parameter);
    }
}


/*关于链路维护,只维护一个，多链路由上层处理*/
static void rt_thread_gsm_socket_M50( void *parameter )
{
    char		buf[128];
    char		rx_buf[32];
    rt_err_t	err;
    AT_CMD_RESP at_cmd_resp;

    if( pcurr_socket == RT_NULL )
    {
        goto lbl_gsm_socket_end;
    }
    if(( pcurr_socket->proc == SOCK_CLOSE) || (pcurr_socket->proc == SOCK_RECONNECT)) /*挂断连接*/
    {
        sprintf( buf, "AT+QICLOSE=%d\r\n", pcurr_socket->linkno );
        sprintf( rx_buf, "%d, CLOSE OK", pcurr_socket->linkno );
        err = gsm_send( buf, RT_NULL, rx_buf, RESP_TYPE_STR, RT_TICK_PER_SECOND * 5, 3 );

        if(pcurr_socket->proc == SOCK_RECONNECT)
            pcurr_socket->proc = SOCK_CONNECT;
        if( err != RT_EOK )
        {
            pcurr_socket->err_no	= CONNECT_CLOSING;
            gsmstate(GSM_POWEROFF);
            return;
        }
        else
        {
            pcurr_socket->state = CONNECT_IDLE;
        }
    }

    if( pcurr_socket->proc >= SOCK_CONNECT )       /*建立连接*/
    {
        if( is_ipaddr( pcurr_socket->ipstr ) == 0 ) /*不是IP地址，执行DNS*/
        {
            sprintf( buf, "AT+QIDNSGIP=\"%s\"\r\n", pcurr_socket->ipstr );
            err = gsm_send( buf, resp_DNSR_M50, RT_NULL, RESP_TYPE_FUNC, RT_TICK_PER_SECOND * 30, 1 );
            if( err != RT_EOK )
            {
                pcurr_socket->state		= CONNECT_ERROR;
                pcurr_socket->err_no	= CONNECT_PEER;
                debug_write("域名解析错误!");
                goto lbl_gsm_socket_end;
            }
        }
        else
        {
            strcpy( pcurr_socket->ip_addr, pcurr_socket->ipstr );
            debug_write("域名解析OK!");
        }
        if( pcurr_socket->type == 'u' )
        {
            sprintf( buf, "AT+QIOPEN=%d,\"UDP\",\"%s\",\"%d\"\r\n", pcurr_socket->linkno, pcurr_socket->ip_addr, pcurr_socket->port );
        }
        else
        {
            sprintf( buf, "AT+QIOPEN=%d,\"TCP\",\"%s\",\"%d\"\r\n", pcurr_socket->linkno, pcurr_socket->ip_addr, pcurr_socket->port );
        }

        err = gsm_send( buf, resp_linking_M50, RT_NULL, RESP_TYPE_FUNC, RT_TICK_PER_SECOND * 3, 5 );
        if( err != RT_EOK )
        {
            pcurr_socket->state		= CONNECT_ERROR;
            pcurr_socket->err_no	= CONNECT_PEER;
            debug_write("连接服务器失败!");
        }
        else
        {
            debug_write("连接服务器成功!");
            pcurr_socket->state = CONNECTED;
        }
    }

lbl_gsm_socket_end:
    gsm_state = GSM_TCPIP; /*socket过程处理完成，结果在state中*/
    rt_kprintf( "\n%d socket_%d_state=%d", rt_tick_get( ), pcurr_socket->linkno, pcurr_socket->state );
}


/*关于链路维护,只维护一个，多链路由上层处理*/
static void rt_thread_gsm_socket_M69( void *parameter )
{
    char		buf[128];
    rt_err_t	err;
    AT_CMD_RESP at_cmd_resp;

    if( pcurr_socket == RT_NULL )
    {
        rt_kprintf( "\n pcurr_socket=0" );
        goto lbl_gsm_socket_end;
    }
    if(( pcurr_socket->proc == SOCK_CLOSE) || (pcurr_socket->proc == SOCK_RECONNECT)) /*挂断连接*/
    {
        sprintf( buf, "AT%%IPCLOSE=%d\r\n", pcurr_socket->linkno );
        err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 5, 3 );

        if(pcurr_socket->proc == SOCK_RECONNECT)
            pcurr_socket->proc = SOCK_CONNECT;
        if( err != RT_EOK )	///关闭socket失败
        {
            pcurr_socket->err_no	= CONNECT_CLOSING;
            gsmstate(GSM_POWEROFF);
            return;
        }
        else				///正常关断了socket链接
        {
            pcurr_socket->state = CONNECT_IDLE;
        }
        //goto lbl_gsm_socket_end;
    }


    if( pcurr_socket->proc >= SOCK_CONNECT )       /*建立连接*/
    {
        if( is_ipaddr( pcurr_socket->ipstr ) == 0 ) /*不是IP地址，执行DNS*/
        {
            sprintf( buf, "AT%%DNSR=\"%s\"\r\n", pcurr_socket->ipstr );
            err = gsm_send( buf, resp_DNSR_M66, RT_NULL, RESP_TYPE_FUNC_WITHOK, RT_TICK_PER_SECOND * 30, 1 );
            if( err != RT_EOK )
            {
                debug_write("域名解析错误!");
                pcurr_socket->state		= CONNECT_ERROR;
                pcurr_socket->err_no	= CONNECT_PEER;
                goto lbl_gsm_socket_end;
            }
            debug_write("域名解析OK!");
        }
        else
        {
            strcpy( pcurr_socket->ip_addr, pcurr_socket->ipstr );
        }
        if( pcurr_socket->type == 'u' )
        {
            sprintf( buf, "AT%%IPOPENX=%d,\"UDP\",\"%s\",%d\r\n", pcurr_socket->linkno, pcurr_socket->ip_addr, pcurr_socket->port );
        }
        else
        {
            sprintf( buf, "AT%%IPOPENX=%d,\"TCP\",\"%s\",%d\r\n", pcurr_socket->linkno, pcurr_socket->ip_addr, pcurr_socket->port );
        }
        err = gsm_send( buf, RT_NULL, "CONNECT", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
        if( err != RT_EOK )
        {
            debug_write("连接服务器失败!");
            pcurr_socket->state		= CONNECT_ERROR;
            pcurr_socket->err_no	= CONNECT_PEER;
        }
        else
        {
            debug_write("连接服务器成功!");
            pcurr_socket->state = CONNECTED;
        }
    }
    rt_kprintf( "\n%d socket_%d_state=%d", rt_tick_get( ), pcurr_socket->linkno, pcurr_socket->state );

lbl_gsm_socket_end:
    gsm_state = GSM_TCPIP; /*socket过程处理完成，结果在state中*/

}


/*控制登网，还是断网*/
static void rt_thread_gsm_socket( void *parameter )
{
    MsgListNode			 *pnode;
    JT808_TX_NODEDATA	 *pnodedata;
    //if((modem_no_sim)||(device_control.off_link_counter)||(jt808_param.id_0xF00F == 0))
    if((modem_no_sim) || (device_control.off_link_counter))
        return;
    if(modem_style == MODEM_M50)
    {
        rt_thread_gsm_socket_M50(parameter);
    }
    else
    {
        rt_thread_gsm_socket_M69(parameter);
    }
    if((pcurr_socket == mast_socket) && (device_unlock))
    {
        while(1)
        {
            pnode = list_jt808_tx->first;
            if( pnode != RT_NULL )
            {
                pnodedata = ( JT808_TX_NODEDATA * )( pnode->data );
                if(pnodedata->type <= SINGLE_FIRST)
                {
                    rt_free( pnodedata->user_para );
                    rt_free( pnodedata );               /*删除节点数据*/
                    list_jt808_tx->first = pnode->next;  /*指向下一个*/
                    memset(pnode, 0, sizeof(MsgListNode));
                    if(list_jt808_tx->first)
                        list_jt808_tx->first->prev = RT_NULL;
                    rt_free( pnode );
                    rt_kprintf("\n清空鉴权清空消息!");
                    continue;
                }
            }
            break;
        }
        /*初始化其他信息*/
        if( strlen( jt808_param.id_0xF003 ) )   /*是否已有鉴权码*/
        {
            jt808_state = JT808_AUTH;
        }
        else
        {
            jt808_state = JT808_REGISTER;
        }
    }
    else if(device_unlock == 0)
    {
        jt808_state = JT808_WAIT;
    }
}


/*调试信息控制输出*/
rt_err_t dbgmsg( uint32_t i )
{
    if( i == 0 )
    {
        rt_kprintf( "\ndebmsg=%d", fgsm_rawdata_out );
    }
    else
    {
        fgsm_rawdata_out = i;
    }
    return RT_EOK;
}

FINSH_FUNCTION_EXPORT( dbgmsg, dbgmsg count );

#define SOCKET_PROCESS

/***********************************************************
* Function: 直接发送信息，要做808转义和m66转义
* Description:
* Input:   原始信息
* Output:
* Return:
* Others:
***********************************************************/
rt_size_t socket_write_M50( uint8_t linkno, uint8_t *buff, rt_size_t count )
{
    rt_size_t	len = count;
    rt_size_t	i;
    uint8_t		*p	= (uint8_t *)buff;
    uint8_t		c;

    rt_err_t	ret;

    uint8_t		fcs = 0;

    char		buf_start[20];
    char		buf_end[4] = { '"', 0x0d, 0x0a, 0x0 };
    char		buf_send[2] = {0x7E, 0x00};
    char		buf_send1[2] = {0x7D, 0x01};
    char		buf_send2[2] = {0x7D, 0x02};

    for(i = 0; i < len; i++)
    {
        fcs ^= buff[i]; /*计算fcs*/
        if( buff[i] == 0x7E )
        {
            count++;
        }
        else if( buff[i] == 0x7D )
        {
            count++;
        }
    }
    if( fcs == 0x7E )
    {
        count++;
    }
    else if( fcs == 0x7D )
    {
        count++;
    }
    count += 3;		///包头包尾的7E和校验，共3字节

    sprintf( buf_start, "AT+QISEND=%d,%d\r\n", linkno, count);
    ret = gsm_send( buf_start, RT_NULL, ">", RESP_TYPE_STR, RT_TICK_PER_SECOND, 1 );
    //m66_write( &dev_gsm, 0, buf_start, strlen( buf_start ) );
    //rt_kprintf( "%s", buf_start );
    //rt_thread_delay( RT_TICK_PER_SECOND / 20 );
    m66_write( &dev_gsm, 0, buf_send, 1 );
    printf_hex_data(buf_send, 1);
    while( len )
    {
        c	= *p++;
        //fcs ^= c; /*计算fcs*/
        if( c == 0x7E )
        {
            m66_write( &dev_gsm, 0, buf_send2, 2 );
            printf_hex_data(buf_send2, 2);
        }
        else if( c == 0x7D )
        {
            m66_write( &dev_gsm, 0, buf_send1, 2 );
            printf_hex_data(buf_send1, 2);
        }
        else
        {
            m66_write( &dev_gsm, 0, &c, 1 );
            printf_hex_data(&c, 1);
        }
        len--;
    }
    /*再发送fcs*/
    if( fcs == 0x7E )
    {
        m66_write( &dev_gsm, 0, buf_send2, 2 );
        printf_hex_data(buf_send2, 2);
    }
    else if( fcs == 0x7D )
    {
        m66_write( &dev_gsm, 0, buf_send1, 2 );
        printf_hex_data(buf_send1, 2);
    }
    else
    {
        m66_write( &dev_gsm, 0, &fcs, 1 );
        printf_hex_data(&fcs, 1);
    }
    /*再发送7E尾*/
    m66_write( &dev_gsm, 0, buf_send, 1 );
    printf_hex_data(buf_send, 1);
    rt_kprintf( "\r\n");

    ret = gsm_send( "", RT_NULL, "SEND OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
    return ret;
}


/***********************************************************
* Function: 直接发送信息，要做808转义和m66转义
* Description:
* Input:   原始信息
* Output:
* Return:
* Others:
***********************************************************/
rt_size_t socket_write_M66( uint8_t linkno, uint8_t *buff, rt_size_t count )
{
    rt_size_t	len = count;
    uint8_t		*p	= (uint8_t *)buff;
    uint8_t		c;

    rt_err_t	ret;

    uint8_t		fcs = 0;

    char		buf_start[20];
    char		buf_end[4] = { '"', 0x0d, 0x0a, 0x0 };

    sprintf( buf_start, "AT%%IPSENDX=%d,\"", linkno );
    m66_write( &dev_gsm, 0, buf_start, strlen( buf_start ) );
    rt_kprintf( "%s", buf_start );
    m66_write( &dev_gsm, 0, "7E", 2 );
    rt_kprintf( "%s", "7E" );
    while( len )
    {
        c	= *p++;
        fcs ^= c; /*计算fcs*/
        if( c == 0x7E )
        {
            m66_write( &dev_gsm, 0, "7D02", 4 );
            rt_kprintf( "%s", "7D02" );
        }
        else if( c == 0x7D )
        {
            m66_write( &dev_gsm, 0, "7D01", 4 );
            rt_kprintf( "%s", "7D01" );
        }
        else
        {
            USART_SendData( GSM_UART, tbl_hex_to_assic[c >> 4] );
            while( USART_GetFlagStatus( GSM_UART, USART_FLAG_TC ) == RESET )
            {
            }
            rt_kprintf( "%c", tbl_hex_to_assic[c >> 4] );
            USART_SendData( GSM_UART, tbl_hex_to_assic[c & 0x0f] );
            while( USART_GetFlagStatus( GSM_UART, USART_FLAG_TC ) == RESET )
            {
            }
            rt_kprintf( "%c", tbl_hex_to_assic[c & 0x0f] );
        }
        len--;
    }
    /*再发送fcs*/
    if( fcs == 0x7E )
    {
        m66_write( &dev_gsm, 0, "7D02", 4 );
        rt_kprintf( "%s", "7D02" );
    }
    else if( fcs == 0x7D )
    {
        m66_write( &dev_gsm, 0, "7D01", 4 );
        rt_kprintf( "%s", "7D01" );
    }
    else
    {
        USART_SendData( GSM_UART, tbl_hex_to_assic[fcs >> 4] );
        while( USART_GetFlagStatus( GSM_UART, USART_FLAG_TC ) == RESET )
        {
        }
        rt_kprintf( "%c", tbl_hex_to_assic[fcs >> 4] );
        USART_SendData( GSM_UART, tbl_hex_to_assic[fcs & 0x0f] );
        while( USART_GetFlagStatus( GSM_UART, USART_FLAG_TC ) == RESET )
        {
        }
        rt_kprintf( "%c", tbl_hex_to_assic[fcs & 0x0f] );
    }
    /*再发送7E尾*/
    m66_write( &dev_gsm, 0, "7E", 2 );
    rt_kprintf( "%s", "7E" );

    m66_write( &dev_gsm, 0, buf_end, 3 );
    rt_kprintf( "%s", buf_end );

    ret = gsm_send( "", RT_NULL, "%IPSENDX:", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
    return ret;
}


/*控制登网，还是断网*/
rt_size_t socket_write( uint8_t linkno, uint8_t *buff, rt_size_t count )
{
    if(modem_style == MODEM_M50)
    {
        socket_write_M50(linkno, buff, count);
    }
    else
    {
        socket_write_M66(linkno, buff, count);
    }
}


/*控制gsm状态 0 查询*/
T_GSM_STATE gsmstate( T_GSM_STATE cmd )
{
    uint8_t i;
    if( cmd != GSM_STATE_GET )
    {
        gsm_state = cmd;
    }
    if(( cmd == GSM_POWEROFF  ) || (cmd == GSM_POWERON) || (cmd == GSM_IDLE))
    {
        socket_para_init();
        tick_receive_last_pack 	= rt_tick_get( );
        jt808_state = JT808_REGISTER;
    }
    return gsm_state;
}

FINSH_FUNCTION_EXPORT( gsmstate, control gsm state );

/*控制登录到gprs*/
void ctl_gprs( char *apn, char *user, char *psw, uint8_t fdial )
{
    if(mobile_cops == 0)
    {
        dial_param.apn		= "CMNET";
    }
    else
    {
        dial_param.apn		= "UNINET";
    }

    dial_param.user		= user;
    dial_param.psw		= psw;
    dial_param.fconnect = fdial;
    gsm_state			= GSM_GPRS;
}

#define TTS_PROCESS



/*
   tts语音播报的处理

   是通过
   %TTS: 0 判断tts状态(怀疑并不是每次都有输出)
   还是AT%TTS? 查询状态
 */
void tts_proc_M50( void )
{
    rt_size_t	len;
    uint8_t		*pinfo, *p;
    uint8_t		c;
    T_GSM_STATE oldstate;
    uint8_t i;

    char		buf[20];

    //for(i = 0; i < 15; i++)
    {
        /*是否有信息要播报*/
        if( rt_mb_recv( &mb_tts, (rt_uint32_t *)&pinfo, 0 ) != RT_EOK )
        {
            return;
        }

        oldstate	= gsm_state;
        gsm_state	= GSM_AT_SEND;
#ifdef GSM_SOUND_PORT
        GPIO_ResetBits( GSM_SOUND_PORT, GSM_SOUND_PIN ); /*开功放*/
#endif
        gsm_send( "AT+CLVL=30\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 2, 2 );
        gsm_send( "AT+QMEDVL=30\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 2, 2 );
        rt_kprintf( "\n%d>", rt_tick_get( ) );
        sprintf( buf, "AT+QTTS=2,\"" );
        rt_kprintf( "%s", buf );
        rt_device_write( &dev_gsm, 0, buf, strlen( buf ) );
        len = ( *pinfo << 8 ) | ( *( pinfo + 1 ) );
        p	= pinfo + 2;
        while( len-- )
        {
            c		= *p++;
            rt_device_write( &dev_gsm, 0, &c, 1 );
            rt_kprintf( "%c", c );
        }
        sprintf( buf, "\",4\r\n" );
        rt_device_write( &dev_gsm, 0, buf, strlen(buf) );
        rt_kprintf( "%s", buf );
        /*不判断，在gsmrx_cb中处理 %TTS: 0*/
        rt_free( pinfo );
        if( gsm_send( "", RT_NULL, "+QTTS:0", RESP_TYPE_STR, RT_TICK_PER_SECOND * 50, 1 ) == RT_EOK )
        {
            rt_kprintf( "\n播报结束" );
        }
#ifdef GSM_SOUND_PORT
        GPIO_SetBits( GSM_SOUND_PORT, GSM_SOUND_PIN ); /*关功放*/
#endif
        gsm_state = oldstate;
    }
}


/*
   tts语音播报的处理

   是通过
   %TTS: 0 判断tts状态(怀疑并不是每次都有输出)
   还是AT%TTS? 查询状态
 */
void tts_proc_M66( void )
{
    rt_size_t	len;
    uint8_t		*pinfo, *p;
    uint8_t		c;
    T_GSM_STATE oldstate;
    uint8_t i;

    char		buf[20];


    //for(i = 0; i < 15; i++)
    {
        /*是否有信息要播报*/
        if( rt_mb_recv( &mb_tts, (rt_uint32_t *)&pinfo, 0 ) != RT_EOK )
        {
            return;
        }

        oldstate	= gsm_state;
        gsm_state	= GSM_AT_SEND;
#ifdef GSM_SOUND_PORT
        GPIO_ResetBits( GSM_SOUND_PORT, GSM_SOUND_PIN ); /*开功放*/
        rt_thread_delay(20);
#endif
        gsm_send("AT+SSAM=1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        rt_kprintf( "\n%d>", rt_tick_get( ) );
        sprintf( buf, "AT%%TTS=2,3,6,\"" );
        rt_device_write( &dev_gsm, 0, buf, strlen( buf ) );
        rt_kprintf( "%s", buf );
        len = ( *pinfo << 8 ) | ( *( pinfo + 1 ) );
        p	= pinfo + 2;
        while( len-- )
        {
            c		= *p++;
            buf[0]	= tbl_hex_to_assic[c >> 4];
            buf[1]	= tbl_hex_to_assic[c & 0x0f];
            rt_device_write( &dev_gsm, 0, buf, 2 );
            rt_kprintf( "%c%c", buf[0], buf[1] );
        }
        buf[0]	= '"';
        buf[1]	= 0x0d;
        buf[2]	= 0x0a;
        buf[3]	= 0;
        rt_device_write( &dev_gsm, 0, buf, 3 );
        rt_kprintf( "%s", buf );
        /*不判断，在gsmrx_cb中处理 %TTS: 0*/
        rt_free( pinfo );
        if( gsm_send( "", RT_NULL, "%TTS: 0", RESP_TYPE_STR, RT_TICK_PER_SECOND * 50, 1 ) == RT_EOK )
        {
            rt_kprintf( "\n播报结束" );
        }
#ifdef GSM_SOUND_PORT
        GPIO_SetBits( GSM_SOUND_PORT, GSM_SOUND_PIN ); /*关功放*/
#endif
        gsm_state = oldstate;
    }
}


/*
   tts语音播报的处理

   是通过
   %TTS: 0 判断tts状态(怀疑并不是每次都有输出)
   还是AT%TTS? 查询状态
 */
void tts_proc( void )
{
    ///如果模块没有开机或者已经关机则直接退出
    if(( gsm_state < GSM_AT) || (gsm_state == GSM_POWEROFF))
    {
        return;
    }
    if(modem_style == MODEM_M50)
    {
        tts_proc_M50();
    }
    else
    {
        tts_proc_M66();
    }
}

/*
   收到tts信息并发送
   返回0:OK
    1:分配RAM错误
 */
rt_size_t tts_write( char *info, uint16_t len )
{
    uint8_t *pmsg;
    /*直接发送到Mailbox中,内部处理*/
    pmsg = rt_malloc( len + 2 );
    if( pmsg != RT_NULL )
    {
        *pmsg			= len >> 8;
        *( pmsg + 1 )	= len & 0xff;
        memcpy( pmsg + 2, info, len );
        rt_mb_send( &mb_tts, (rt_uint32_t)pmsg );
        return 0;
    }
    return 1;
}

/**/
rt_err_t tts( char *s )
{
    tts_write( s, strlen( s ) );
    return RT_EOK;
}

FINSH_FUNCTION_EXPORT( tts, tts send );



uint8_t record_rx_M50( char *pinfo, uint16_t size )
{
    int			st, index, count;
    char		buf[40];                                        /*160个OCT需要320byte*/
    uint32_t	tick = rt_tick_get( );
    uint32_t	infolen;

    if(record_state != RD_RECORDING)
    {
        return 0;
    }
    if(sscanf( pinfo, "CONNECT%d", &infolen ) == 1)
    {
        gprs_rx_len = infolen;
        rt_kprintf("\n Read Media=%d; \n", infolen);
    }
    return 0;
}


void record_proc_M50(void)
{
    rt_err_t		err;
    char			*pmsg;
    uint32_t   		i;
    char			buf[64];
    static uint32_t	sktart_tick = 0;

    if(record_time == 0)
    {
        return;
    }
    ///如果模块没有开机或者已经关机则直接退出
    if(( gsm_state < GSM_AT) || (gsm_state == GSM_POWEROFF))
    {
        return;
    }

    gsm_send( "", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND / 5, 1 );
    sprintf(buf, "AT+QFDEL=\"aaa.amr\"\r\n");
    err = gsm_send( buf, resp_record_del_M50, RT_NULL, RESP_TYPE_FUNC, RT_TICK_PER_SECOND * 3, 3 );
    if( err != RT_EOK )
    {
        record_time = 0;
        return;
    }

    ///开始录音
    err = gsm_send( "AT+QAUDRD=1,\"aaa.amr\",3\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
    if( err != RT_EOK )
    {
        record_time = 0;
        return;
    }
    sktart_tick = rt_tick_get();
    ///等待录音结束
    while(rt_tick_get() - sktart_tick < record_time * RT_TICK_PER_SECOND)
    {
        err = gsm_send( "AT+QAUDRD?\r\n", resp_recording_M50, RT_NULL, RESP_TYPE_FUNC, RT_TICK_PER_SECOND * 3, 1 );
        if( err == RT_EOK )
        {
            break;
        }
    }
    record_time = 0;
    ///录音结束命令
    err = gsm_send( "AT+QAUDRD=0,\"aaa.amr\",3\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 6 );
    if( err != RT_EOK )
    {
        return;
    }
    ///打开文件
    err = gsm_send( "AT+QFOPEN=\"aaa.amr\"\r\n", resp_record_open_M50, RT_NULL, RESP_TYPE_FUNC, RT_TICK_PER_SECOND * 3, 3 );
    if( err != RT_EOK )
    {
        return;
    }
    resp_record_read_M50(RT_NULL, 0);
    sprintf(buf, "AT+QFREAD=%s,1024\r\n", record_handle);

    ///30秒等待拍照结束，防止存储混乱
    for(i = 0; i < 1500; i++)
    {
        if(media_state == MEDIA_CAMERA)
            rt_thread_delay( RT_TICK_PER_SECOND / 50 );
        else
        {
            break;
        }
    }
    media_state  = MEDIA_RECORD;
    record_state = RD_RECORDING;
    for(i = 0; i < 32; i++)
    {
        err = gsm_send( buf, resp_record_read_M50, RT_NULL, RESP_TYPE_FUNC, RT_TICK_PER_SECOND * 3, 2 );
        if( err != RT_EOK )
        {
            break;
        }
        if(record_save_ok)
        {
            break;
        }
    }
    record_state = RD_IDLE;
    media_state  = MEDIA_NONE;
    sprintf(buf, "AT+QFCLOSE=%s\r\n", record_handle);
    err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
    if( err != RT_EOK )
    {
        return;
    }
}


/*
        M66  录音
*/
rt_err_t record_saving_M66(char *p, uint16_t len)
{
    // 录音存储数据
    char *psrc = p;
    static uint32_t infolen = 0;
    uint32_t		  i, j;
    //static uint8_t	  *buftemp[4096];
    uint8_t *buftemp = photo_ram;
    static uint32_t bufwr = 0;
    TypeDF_PackageHead  pack_head;

    if(p == RT_NULL)
    {
        infolen = 0;					  ///
        bufwr   = 64; 				  ///长度预留包头部分
        memset(buftemp, 0xFF, 4096);	 ///将包头部分清空

        ///add   AMR  head
        ///AMR 文件头     23 21 41 4D 52 0A
        buftemp[bufwr++]	= 0x23;
        buftemp[bufwr++]	= 0x21;
        buftemp[bufwr++]	= 0x41;
        buftemp[bufwr++]	= 0x4D;
        buftemp[bufwr++]	= 0x52;
        buftemp[bufwr++]	= 0x0A;
        ///
        record_save_ok = 0;
        return RT_ERROR;
    }
    rt_kprintf("\n Recod_len=%d", len);
    // if((infolen)&&(len == infolen))
    if(len)
    {
        //rt_kprintf( "\n%d 接收的数据信息   ", rt_tick_get( ));
        //printf_hex_data(psrc, len);
        infolen += len;
        rt_kprintf("  Total=%d ", infolen);
        for(i = 0; i < len; i++)
        {
            buftemp[(bufwr++) % 4096] = psrc[i];
            if(bufwr % 4096 == 0)
            {
                Cam_Flash_SaveMediaData(bufwr - 4096, buftemp, 4096);
                memset(buftemp, 0xFF, 4096);
            }
        }
        record_M66_bufwr = bufwr; // update value

        if(len < 280)
        {
            goto FUNC_RECORD_END;
        }
        return RT_EOK;
    }
    else
    {
        infolen = 0;
    }

    return RT_ERROR;

FUNC_RECORD_END:

    rt_kprintf("\n RecrodFile_size=%d  ", infolen);
    return RT_EOK;
}

rt_err_t record_Receiving_M66( char *p, uint16_t len )
{
    //  录音过程中接数据
    uint16_t  commer = 0; //,j=0;
    uint16_t  namelen = 0, len_local = 0;
    char   rec_hex[560];
    uint16_t  rec_hex_len = 0;
    uint8_t *buftemp = photo_ram;
    uint32_t  rx_filesize = 0;
    uint32_t  i = 0, j = 0;

    TypeDF_PackageHead	pack_head;

    if( strncmp( p, "%RECDATA:", 9) == 0 )
    {
        for(i = 0; i < 20; i++) //  从前20个字节中找第一个"
        {
            if(p[i] == ':')
                commer = i;
            if(p[i] == '"')
                break;
        }
        //  1. 获取长度
        sscanf(p + commer + 1, "%d", (u32 *)&len_local);
        rt_kprintf("\n Rxlen=%d	RxinfoLen=%d ", len_local, len_local * 28); //
        len_local = len_local * 28;
        //for(j=0;j<len;j++)
        //	rt_kprintf("%c",instr[i+1+j]);

        // 2.  转换
        rec_hex_len = Ascii_To_Hex(rec_hex, p + i + 1, len_local);
        if(rec_hex_len != (len_local >> 1))
        {
            rt_kprintf("\nHEX_len=%d  len=%d  ASCII to HEX error!", rec_hex_len, (len_local >> 1));
            return RT_ERROR;
        }
        // 3.	write and  check
        record_saving_M66(rec_hex, rec_hex_len);
    }

    if( strncmp( p, "%RECSTOP:", 9) == 0 )
    {
        for(i = 0; i < 20; i++) //  从前20个字节中找第一个,
        {
            if(p[i] == ',')
                break;
        }
        if(record_M66_bufwr % 4096)
        {
            Cam_Flash_SaveMediaData(record_M66_bufwr / 4096 * 4096, buftemp, record_M66_bufwr % 4096);
        }

        sscanf((const char *)p + i + 1, "%u", (u32 *)&rx_filesize);

        // 1. 对比
        rt_kprintf("\nRxTotal Size: %d", rx_filesize);

        rt_kprintf("\n Save Mdeia!");
        pack_head.Channel_ID    = 1;
        pack_head.TiggerStyle   = 1;
        pack_head.Media_Format  = 5;
        pack_head.Media_Style   = 1;
        pack_head.Time		   = mytime_now;
        pack_head.Head		   = CAM_HEAD;
        pack_head.State		   = 0xFF;
        pack_head.id 		   = DF_PicParam.Last.Data_ID + 1;
        pack_head.Len		   = record_M66_bufwr;
        memcpy( (uint8_t *) & ( pack_head.position ), (uint8_t *)&gps_baseinfo, 28 );
        Cam_Flash_SaveMediaData(0, (uint8_t *)&pack_head, sizeof(TypeDF_PackageHead));
        record_save_ok = 1;

        // mask   debug  Out
#if   0
        rt_kprintf("\n Media:\n");
        for(i = 0, j = 0;; j++)
        {
            if(Cam_Flash_RdPic(buftemp, (uint16_t *)&i, pack_head.id, j) != RT_EOK)
            {
                break;
            }

            printf_hex_data(buftemp, i);
        }
        rt_kprintf("\nEND\n");
#endif
        return RT_EOK;
    }
    return RT_ERROR;
}
void record_proc_M66(void)
{
    rt_err_t		err;
    char			*pmsg;
    uint32_t   		i;
    char			buf[64];
    static uint32_t	sktart_tick = 0;

    if(record_time == 0)
    {
        return;
    }
    ///如果模块没有开机或者已经关机则直接退出
    if(( gsm_state < GSM_AT) || (gsm_state == GSM_POWEROFF))
    {
        return;
    }


    memset(buf, 0, sizeof(buf));
    sprintf(buf, "AT%%RECSTART=\"aaa.amr\",%d\r\n", record_time);
    sktart_tick = rt_tick_get();

    //开始录音并等待录音结束
    ///30秒等待拍照结束，防止存储混乱
    for(i = 0; i < 1500; i++)
    {
        if(media_state == MEDIA_CAMERA)
            rt_thread_delay( RT_TICK_PER_SECOND / 50 );
        else
        {
            break;
        }
    }
    media_state  = MEDIA_RECORD;
    record_state = RD_RECORDING;
    record_saving_M66(RT_NULL, 0);  // 初始化
    err = gsm_send(buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 5, 2 );
    err = gsm_send("", record_Receiving_M66, RT_NULL, RESP_TYPE_FUNC, RT_TICK_PER_SECOND * ( 10 + record_time ), 1 );
    record_state = RD_IDLE;
    media_state  = MEDIA_NONE;
    record_time = 0;
}


void record(uint32_t time)
{
    record_time = time;
    rt_kprintf("\n record=%ds", time);
}
FINSH_FUNCTION_EXPORT( record, record send );

void record_Process(void)
{
    //   录音处理函数
    switch(modem_style)
    {
    case MODEM_M50 :
        record_proc_M50();
        break;
    case MODEM_M66 :
    case MODEM_M69 :
        record_proc_M66();
        break;
    default:
        break;

    }


}

#define AT_CMD_PROCESS


/*
   通常的AT命令，发出去交由系统处理，例如语音播报
   对于短信操作需要确认，尤其是多个短息连续到来时
   at命令处理，收到OK或超时退出
 */
void at_cmd_proc( void )
{
    rt_err_t	ret;
    uint8_t		*pinfo, *p;
    T_GSM_STATE oldstate;
    static uint32_t tick = 0;

    /*是否有信息要发送*/
    if( rt_mb_recv( &mb_at_tx, (rt_uint32_t *)&pinfo, 0 ) != RT_EOK )
    {
        return;
    }
    oldstate	= gsm_state;
    gsm_state	= GSM_AT_SEND;
    p			= pinfo + 2;
    ret			= gsm_send( (char *)p, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 1 );
    //ret = gsm_send( (char*)p, RT_NULL, "OK", RESP_TYPE_NONE, RT_TICK_PER_SECOND , 1 );
    rt_kprintf( "\nat_tx=%d", ret );
    rt_free( pinfo );
    if(gsm_state == GSM_AT_SEND)
        gsm_state = oldstate;
}

/*
   发送AT命令
   如何保证不干扰,其他的执行，传入等待的时间

 */
rt_size_t at( char *sinfo )
{
    char		*pmsg;
    uint16_t	count;

    for(count = 0; count < strlen(sinfo); count++)
    {
        if(sinfo[count] == '\'')
        {
            sinfo[count] = '\"';
        }
    }

    count = strlen( sinfo );
    /*直接发送到Mailbox中,内部处理*/
    pmsg = rt_malloc( count + 3 );
    if( pmsg != RT_NULL )
    {
        count					= sprintf( pmsg + 2, "%s\r\n", sinfo );
        pmsg[0]					= count >> 8;
        pmsg[1]					= count & 0xff;
        *( pmsg + count + 2 )	= 0;
        rt_mb_send( &mb_at_tx, (rt_uint32_t)pmsg );
        return 0;
    }
    return 1;
}

FINSH_FUNCTION_EXPORT( at, write gsm );

#define SMS_PROCESS


/* +CSQ: 31, 99 */
rt_err_t resp_SMS_RX( char *p, uint16_t len )
{
    uint32_t i;
    for(i = 0; i < (len & 0xFFFE); i++)		///len&0xFFFE  是因为M69在短信后面多发送了一个'a';
    {
        if(((p[i] >= '0') && (p[i] <= '9')) || ((p[i] >= 'A') && (p[i] <= 'F')))
        {
            continue;
        }
        else
        {
            rt_kprintf("\n resp_SMS_RX ERROR=%d", i);
            return RT_ERROR;
        }
    }
    jt808_sms_rx( p, len );
    return RT_EOK;
}


/*
   0:没有短信处理内容 非零值 有后续的处理

   4381 gsm<+CMTI: "SM",4
   4381 gsm<+CMTI: "SM",5
   4381 gsm>AT+CMGR=5


   4386 gsm<+CMTI: "SM",6
   4386 gsm>AT+CMGR=6
 */
uint8_t sms_rx( char *pinfo, uint16_t size )
{
    int			st, index, count;
    char		buf[40];                                        /*160个OCT需要320byte*/
    uint32_t	tick = rt_tick_get( );

    /*主动接收中心发来的信息*/
    if( strncmp( pinfo, "+CMTI: \"SM\",", 12 ) == 0 )           /*SM中满的时候，会存到ME中，如何读*/
    {
        if( sscanf( pinfo + 12, "%d", &index ) )
        {
            rt_mb_send( &mb_sms, index );                       /*通知要读短信*/
            return 1;
        }
    }

#if 0
    else if( strncmp( pinfo, "+CMGR: ", 7 ) == 0 )             /*+CMGR: 0,156,有可能是串口读的短信*/
    {
        if( sscanf( pinfo + 7, "%d,%d", &st, &count ) == 2 )    /*得到信息的长度*/
        {
            sms_state = SMS_WAIT_CMGR_DATA;
            return 1;                                           /*直接返回，等待下一包*/
        }
    }
    else if( strncmp( pinfo, "+CMT:", 5 ) == 0 )                /*没有主动弹出的短信*/
    {
    }

    switch( sms_state )
    {
    case SMS_WAIT_CMGR_DATA:                                /*收到短信数据*/
        jt808_sms_rx( pinfo, size );                        /*解析数据*/
        sms_state = SMS_WAIT_CMGR_OK;
        break;
    case SMS_WAIT_CMGR_OK:
        if( strncmp( pinfo, "OK", 2 ) == 0 )                /*读完了信息,删除*/
        {
            sprintf( buf, "AT+CMGD=%d\r\n", sms_index );
            gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND, 1 );
            sms_state = SMS_IDLE;
        }
        break;
    }
#endif


    /*
       if( tick - sms_tick > RT_TICK_PER_SECOND * 10 )
       {
       rt_kprintf( "\n靠,超时了" );
       sms_state = SMS_IDLE;
       }
     */
    return sms_state;
}

/*发送短信息,包含接收方,SMSC,7bit编码后的信息*/
void sms_tx( char *info, uint8_t tp_len )
{
    uint8_t len = strlen( info );
    char	*p;
    p = rt_malloc( len + 2 );
    if( p != RT_NULL )
    {
        p[0] = tp_len; /*tp_len AT+CMGS时需要*/
        strcpy( p + 1, info );
        rt_mb_send( &mb_sms, (uint32_t)p );
    }
}

/*
   增加这个函数主要是中心有多个消息过来时候，如果处理不及时，会乱
   使用mailbox进行排队处理读取或删除短信
 */
void sms_proc( void )
{
    uint32_t	value;
    uint32_t	j;
    char		*sms_send_str;
    char		buf[40];
    rt_err_t	ret;
    uint16_t	len;
    static uint32_t	sms_delay_tick = 0;

    T_GSM_STATE oldstate;

    if(rt_tick_get() < sms_delay_tick)
        return;

    sms_delay_tick	= 0;

    if( sms_state != SMS_IDLE )
    {
        return;
    }
    /*检查是否出于AT命令就绪的状态,可以进行短信操作*/
    ///如果模块没有开机或者已经关机则直接退出
    if(( gsm_state < GSM_AT) || (gsm_state == GSM_POWEROFF))
    {
        return;
    }

    if( rt_mb_recv( &mb_sms, &value, 0 ) != RT_EOK ) /*收到短信操作*/
    {
        return;
    }
    oldstate	= gsm_state;
    gsm_state	= GSM_AT_SEND;

    if( value < 0xFF )
    {
        sprintf( buf, "AT+CMGR=%d\r\n", value );
        sms_index	= value;
        ret	= gsm_send( buf, RT_NULL, "+CMGR:", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        if( ret == RT_EOK )
        {
            ret	= gsm_send( "", resp_SMS_RX, RT_NULL, RESP_TYPE_FUNC_WITHOK, RT_TICK_PER_SECOND * 6, 1 );
        }

        sprintf( buf, "AT+CMGD=%d\r\n", sms_index );
        gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        sms_state = SMS_IDLE;
        if(modem_style == MODEM_M50)		///M50的模块删除后不能立即发送数据，否则会容易发送不成功，等待一小会就OK
            sms_delay_tick = rt_tick_get() + RT_TICK_PER_SECOND;
    }
    else                                    /*发送信息*/
    {
        sms_send_str = (char *)value;
        len			= sms_send_str[0];          /*tp_len*/
        sprintf( buf, "AT+CMGS=%d\r", len );
        ret = gsm_send( buf, RT_NULL, ">", RESP_TYPE_STR, RT_TICK_PER_SECOND * 5, 1 );
        if( ret == RT_EOK )
        {
            len = strlen( sms_send_str + 1 );   /*真实消息长度,包括CTRL_Z*/
            rt_kprintf("\nSMS>SEND(len=%d):%s", len, sms_send_str + 1);
            m66_write( &dev_gsm, 1, sms_send_str, len );
        }
        buf[0]	= 0x1A;
        buf[1]	= 0x0;
        ret		= gsm_send( buf, RT_NULL, "+CMGS:", RESP_TYPE_STR_WITHOK, RT_TICK_PER_SECOND * 30, 1 );
        rt_free( (void *)value );
    }
    if(gsm_state == GSM_AT_SEND)
        gsm_state = oldstate;
}



/*
 拨打电话处理函数
 */
void tel_proc( void )
{
    uint32_t	value;
    uint32_t	j;
    char		*sms_send_str;
    char		buf[40];
    rt_err_t	ret;
    uint16_t	len;
    static uint32_t	sms_delay_tick = 0;

    T_GSM_STATE oldstate;

    if(rt_tick_get() < sms_delay_tick)
        return;

    sms_delay_tick	= 0;

    /*检查是否出于AT命令就绪的状态,可以进行短信操作*/
    ///如果模块没有开机或者已经关机则直接退出
    if(( gsm_state < GSM_AT) || (gsm_state == GSM_POWEROFF))
    {
        return;
    }

    if( tel_state == 0) /*收到短信操作*/
    {
        return;
    }
    gsm_send( "ATH\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
    if(modem_style == MODEM_M50)
    {
        if(tel_state == 1)
        {
#ifdef GSM_SOUND_PORT
            GPIO_ResetBits( GSM_SOUND_PORT, GSM_SOUND_PIN );                /*ON功放*/
#endif
            gsm_send( "AT+CLVL=30\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
            gsm_send( "AT+QMEDVL=30\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        }
        else if(tel_state == 2)
        {
#ifdef GSM_SOUND_PORT
            GPIO_SetBits( GSM_SOUND_PORT, GSM_SOUND_PIN );                /*OFF功放*/
#endif
            gsm_send( "AT+CLVL=0\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
            gsm_send( "AT+QMEDVL=0\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        }
    }
    else
    {
        if(tel_state == 1)
        {
#ifdef GSM_SOUND_PORT
            GPIO_ResetBits( GSM_SOUND_PORT, GSM_SOUND_PIN );                /*ON功放*/
#endif
            gsm_send( "AT+SSAM=1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        }
        else if(tel_state == 2)
        {
#ifdef GSM_SOUND_PORT
            GPIO_SetBits( GSM_SOUND_PORT, GSM_SOUND_PIN );                /*OFF功放*/
#endif
            gsm_send( "AT+SSAM=0\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 3, 3 );
        }
    }
    sprintf( buf, "ATD%s;\r\n", tel_phonenum);
    ret	= gsm_send( buf, resp_TEL, RT_NULL, RESP_TYPE_FUNC, RT_TICK_PER_SECOND * 6, 1 );
    if( ret == RT_EOK )
    {
        ret	= gsm_send( "", resp_TEL, RT_NULL, RESP_TYPE_FUNC_WITHOK, RT_TICK_PER_SECOND * 3, 3 );
    }
TEL_END:

    tel_state = 0;
}


void tel(char *phonenum, uint8_t state)
{
    memset(tel_phonenum, 0, sizeof(tel_phonenum));
    strcpy(tel_phonenum, phonenum);
    tel_state = state;
    rt_kprintf("\n准备拨号,phone=%s,state=%d", tel_phonenum, tel_state);
}
FINSH_FUNCTION_EXPORT( tel, tel_phone );
/***********************************************************
* Function:		gsmrx_cb
* Description:	gsm收到信息的处理
* Input:			char *s     信息
    uint16_t len 长度
* Output:
* Return:
* Others:
***********************************************************/
static void gsmrx_cb_M50( char *pInfo, uint16_t size )
{
    int		i, len = size;
    char	c, *pmsg;
    char	*psrc = RT_NULL, *pdst = RT_NULL;
    static int32_t infolen = 0, linkno = 0;
    char 	temp_buf[16];

    psrc	= pInfo;
    pdst	= pInfo;
    /*网络侧的信息，直接通知上层软件*/
    if( fgsm_rawdata_out )
    {
        if((infolen) && (len == infolen))
        {
            rt_kprintf( "\n%d gsm1<", rt_tick_get( ));
            printf_hex_data(psrc, infolen);
            gprs_rx( linkno, psrc, infolen );
            return;
        }
        else
        {
            linkno	= 0;
            infolen	= 0;
            rt_kprintf( "\n%d gsm<%s", rt_tick_get( ), pInfo );
        }
        fgsm_rawdata_out--;
    }

    /*判读并处理*/
    //rt_kprintf("\ngsm_rx>%s",psrc);
    if( strncmp( psrc, "+RECEIVE:", 9 ) == 0 )
    {
        /*解析出净信息,编译器会优化掉pdst*/
        i = sscanf( psrc, "+RECEIVE: %d, %d", &linkno, &infolen );
        if( i != 2 )
        {
            linkno	= 0;
            infolen	= 0;
        }
        gprs_rx_len = infolen;
        return;
    }


    if( sms_rx( pInfo, size ) )                 /*一个完整的处理过程?*/
    {
        return;
    }


    if( record_rx_M50( pInfo, size ) )               /*一个完整的处理过程?*/
    {
        return;
    }

    if( strncmp( psrc, "+PDP DEACT", 10 ) == 0 )    ///模块出现异常，需要重新复位
    {
        gsmstate( GSM_POWEROFF );
        return;
    }
    /*EG:"1, CLOSED"*/
    for(i = 1; i < 6; i++)								///socket 连接异常断开
    {
        sprintf(temp_buf, "%d, CLOSED", i);
        if( strncmp( psrc, temp_buf, 9 ) == 0 )
        {
            cb_socket_close( i );
            return;
        }
        sprintf(temp_buf, "%d, CLOSE OK", i);
        if( strncmp( psrc, temp_buf, 11 ) == 0 )
        {
            cb_socket_close( i );
            return;
        }
    }

    /*直接发送到Mailbox中,内部处理*/
    pmsg = rt_malloc( len + 3 );		///多分配一个字节的目的是为了保证最后一个字节是0x00
    if( pmsg != RT_NULL )
    {
        *pmsg			= len >> 8;
        *( pmsg + 1 )	= len;
        memcpy( pmsg + 2, pInfo, len );
        pmsg[len + 2]		= 0;
        rt_mb_send( &mb_gsmrx, (rt_uint32_t)pmsg );
    }
    return;
}

/***********************************************************
* Function:		gsmrx_cb
* Description:	gsm收到信息的处理
* Input:			char *s     信息
    uint16_t len 长度
* Output:
* Return:
* Others:
***********************************************************/
static void gsmrx_cb_M66( char *pInfo, uint16_t size )
{
    int		i, len = size;
    char	c, *pmsg;
    char	*psrc = RT_NULL, *pdst = RT_NULL;
    int32_t infolen, linkno;

    /*网络侧的信息，直接通知上层软件*/
    if( fgsm_rawdata_out )
    {
        rt_kprintf( "\n%d gsm<%s", rt_tick_get( ), pInfo );
        fgsm_rawdata_out--;
    }

    /*判读并处理*/
    psrc	= pInfo;
    pdst	= pInfo;
    if( strncmp( psrc, "%IPDATA:", 7 ) == 0 )
    {
        /*解析出净信息,编译器会优化掉pdst*/
        i = sscanf( psrc, "%%IPDATA:%d,%d,%s", &linkno, &infolen, pdst );
        if( i != 3 )
        {
            return;
        }
        if( ( infolen + 1 ) * 2 != strlen( pdst ) ) /*有时会长度不足*/
        {
            rt_kprintf( "\n长度不足" );
            return;
        }
        if( *pdst != '"' )
        {
            return;
        }
        psrc	= pdst;     /*指向""内容*/
        pmsg	= pdst + 1; /*指向下一个位置*/

        for( i = 0; i < infolen; i++ )
        {
            c		= tbl_assic_to_hex[*pmsg++ - '0'] << 4;
            c		|= tbl_assic_to_hex[*pmsg++ - '0'];
            *pdst++ = c;
        }
        gprs_rx( linkno, psrc, infolen );
        return;
    }
    /*	00002381 gsm<%IPCLOSE:1*/

    if( strncmp( psrc, "%IPCLOSE:", 9 ) == 0 )
    {
        c = *( psrc + 9 ) - 0x30;
        cb_socket_close( c );
        return;
    }

    if(( strncmp( psrc, "NO CARRIER", 10 ) == 0 ) || ( strncmp( psrc, "^CEND: 1", 8 ) == 0 ))	//主动挂断or对方挂断电话连接
    {
        //at("AT+SSAM=1");
        GPIO_SetBits( GSM_SOUND_PORT, GSM_SOUND_PIN );
    }

    if( sms_rx( pInfo, size ) )                 /*一个完整的处理过程?*/
    {
        return;
    }

    //record_Receiving_M66( pInfo, size );   //  录音接收处理

    /*直接发送到Mailbox中,内部处理*/
    pmsg = rt_malloc( len + 3 );		///多分配一个字节的目的是为了保证最后一个字节是0x00
    if( pmsg != RT_NULL )
    {
        *pmsg			= len >> 8;
        *( pmsg + 1 )	= len;
        memcpy( pmsg + 2, pInfo, len );
        pmsg[len + 2]		= 0;
        rt_mb_send( &mb_gsmrx, (rt_uint32_t)pmsg );
    }
    return;
}

/***********************************************************
* Function:		gsmrx_cb
* Description:	gsm收到信息的处理
* Input:			char *s     信息
    uint16_t len 长度
* Output:
* Return:
* Others:
***********************************************************/
static void gsmrx_cb( char *pInfo, uint16_t size )
{
    if(modem_style == MODEM_M50)
    {
        gsmrx_cb_M50(pInfo, size);
    }
    else
    {
        gsmrx_cb_M66(pInfo, size);
    }
}

#define RT_THREAD_ENTRY_GSM

ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_stack[2048];
//static char thread_gsm_stack[2048]  __attribute__((section("CCM_RT_STACK")));
struct rt_thread thread_gsm;


/*
   状态转换，同时处理开机、登网、短信、TTS、录音等过程
 */
static void rt_thread_entry_gsm( void *parameter )
{
    uint16_t tick_checksq = 0;

    while( 1 )
    {
        wdg_thread_counter[4] = 1;

        switch( gsm_state )
        {
        case GSM_POWERON:
            rt_thread_gsm_power_on( RT_NULL );
            break;
        case GSM_POWEROFF:
            rt_thread_gsm_power_off();
            break;
        case  GSM_GPRS:
            rt_thread_gsm_gprs( RT_NULL );
            break;
        case GSM_SOCKET_PROC:
            rt_thread_gsm_socket( RT_NULL );
            break;
        case GSM_IDLE:
            break;
        case GSM_TCPIP: /*在TCPIP状态下检测sq信号*/
            jt808_tx_data_proc();
            tick_checksq++;
            if(tick_checksq > 600)
            {
                if(RT_EOK == gsm_send( "AT+CSQ\r\n", resp_CSQ, RT_NULL, RESP_TYPE_FUNC_WITHOK, RT_TICK_PER_SECOND * 3, 1 ))
                {
                    tick_checksq = 0;
                }
            }
            break;
        case GSM_AT:
            break;
        case GSM_AT_SEND:
            break;
        }
        tel_proc( );
        sms_proc( );
        tts_proc( );    /*tts处理*/
        at_cmd_proc( ); /*at命令处理*/
        record_Process( );  /* 录音处理*/
        rt_thread_delay( RT_TICK_PER_SECOND / 20 );
    }
}

#define RT_THREAD_ENTRY_GSM_RX

ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_rx_stack[1024];
struct rt_thread thread_gsm_rx;


/*
   状态转换，同时处理短信、TTS、录音等过程
 */
static void rt_thread_entry_gsm_rx( void *parameter )
{
    unsigned char ch;
    int rx_command;
    while( 1 )
    {
        rx_command = 1;
        while( uart_gsm_rxbuf_rd != uart_gsm_rxbuf_wr )   /*有数据时，保存数据*/
        {
            ch				= uart_gsm_rxbuf[uart_gsm_rxbuf_rd++];
            uart_gsm_rxbuf_rd	%= UART_GSM_RX_SIZE;

            if(gprs_rx_len)
            {
                if((0 == gsm_rx_wr) && (0x0A == ch) && rx_command)
                {
                    rx_command = 0;
                    continue;
                }
                --gprs_rx_len;
                gsm_rx[gsm_rx_wr++] = ch;
                gsm_rx_wr			%= GSM_RX_SIZE;
                gsm_rx[gsm_rx_wr]	= 0;
                if(0 == gprs_rx_len)
                {
                    gsmrx_cb( (char *)gsm_rx, gsm_rx_wr );          /*接收信息的处理函数*/
                    gsm_rx_wr = 0;
                }
                continue;
            }
            else if( ch > 0x1F )                         /*可见字符才保存*/
            {
                gsm_rx[gsm_rx_wr++] = ch;
                gsm_rx_wr			%= GSM_RX_SIZE;
                gsm_rx[gsm_rx_wr]	= 0;
            }
            if( ch == 0x0d )
            {
                if( gsm_rx_wr )
                {
                    gsmrx_cb( (char *)gsm_rx, gsm_rx_wr );          /*接收信息的处理函数*/
                }
                gsm_rx_wr = 0;
            }
            rx_command = 1;
        }
        if( rt_tick_get( ) - last_tick > RT_TICK_PER_SECOND / 5 )   //等待200ms,实际上就是变长的延时,最迟100ms处理完一个数据包
        {
            if( gsm_rx_wr )
            {
                gsmrx_cb( (char *)gsm_rx, gsm_rx_wr );              /*接收信息的处理函数*/
                gsm_rx_wr = 0;
            }
        }
        rt_thread_delay( RT_TICK_PER_SECOND / 20 );
    }
}

/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void gsm_init( void )
{
    rt_sem_init( &sem_at, "sem_at", 1, RT_IPC_FLAG_FIFO );
    rt_mb_init( &mb_gsmrx, "mb_gsmrx", &mb_gsmrx_pool, MB_GSMRX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
    rt_mb_init( &mb_tts, "mb_tts", &mb_tts_pool, MB_TTS_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
    rt_mb_init( &mb_at_tx, "mb_at_tx", &mb_at_tx_pool, MB_AT_TX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
    rt_mb_init( &mb_sms, "mb_sms_rx", &mb_sms_pool, MB_SMS_RX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

    rt_thread_init( &thread_gsm,
                    "gsm",
                    rt_thread_entry_gsm,
                    RT_NULL,
                    &thread_gsm_stack[0],
                    sizeof( thread_gsm_stack ), 9, 5 );
    rt_thread_startup( &thread_gsm );

    rt_thread_init( &thread_gsm_rx,
                    "gsm_rx",
                    rt_thread_entry_gsm_rx,
                    RT_NULL,
                    &thread_gsm_rx_stack[0],
                    sizeof( thread_gsm_rx_stack ), 6, 5 );
    rt_thread_startup( &thread_gsm_rx );

    dev_gsm.type	= RT_Device_Class_Char;
    dev_gsm.init	= m66_init;
    dev_gsm.open	= m66_open;
    dev_gsm.close	= m66_close;
    dev_gsm.read	= m66_read;
    dev_gsm.write	= m66_write;
    dev_gsm.control = m66_control;

    rt_device_register( &dev_gsm, "gsm", RT_DEVICE_FLAG_RDWR );
    rt_device_init( &dev_gsm );
}

/************************************** The End Of File **************************************/
