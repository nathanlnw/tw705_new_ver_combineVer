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
#include <dfs_posix.h>

#include "include.h"
#include "rs485.h"
#include "jt808.h"
#include "camera.h"
#include "large_lcd.h"

#include <finsh.h>

#define LENGTH_BUF_SIZE				256
typedef __packed struct
{
    uint16_t	wr;
    uint8_t		body[LENGTH_BUF_SIZE];
} LENGTH_BUF;

#define UART_RS485_RXBUF_SIZE 		1024

/*串口接收缓存区定义*/
static uint8_t	uart_rs485_rxbuf[UART_RS485_RXBUF_SIZE];
static uint16_t	uart_rs485_rxbuf_wr	= 0, uart_rs485_rxbuf_rd = 0;
static uint32_t	last_rx_tick	= 0;


/*声明一个RS485设备*/
static struct rt_device dev_RS485;

/*RS485原始信息数据区定义*/
#define RS485_RAWINFO_SIZE 2048
static uint8_t					RS485_rawinfo[RS485_RAWINFO_SIZE];
static struct rt_messagequeue	mq_RS485;


/*
   RS485接收中断处理
 */
void USART2_IRQHandler( void )
{
    rt_interrupt_enter( );
    if( USART_GetITStatus( RS485_UART, USART_IT_RXNE ) != RESET )
    {
        uart_rs485_rxbuf[uart_rs485_rxbuf_wr++]	= USART_ReceiveData( RS485_UART );
        uart_rs485_rxbuf_wr					%= UART_RS485_RXBUF_SIZE;
        USART_ClearITPendingBit( RS485_UART, USART_IT_RXNE );
        last_rx_tick = rt_tick_get( );
    }
    rt_interrupt_leave( );
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
static void RS485_baud( int baud )
{
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate				= baud;
    USART_InitStructure.USART_WordLength			= USART_WordLength_8b;
    USART_InitStructure.USART_StopBits				= USART_StopBits_1;
    USART_InitStructure.USART_Parity				= USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode					= USART_Mode_Rx | USART_Mode_Tx;
    USART_Init( RS485_UART, &USART_InitStructure );
}

FINSH_FUNCTION_EXPORT( RS485_baud, config gsp_baud );

/*初始化*/
static rt_err_t dev_RS485_init( rt_device_t dev )
{
    GPIO_InitTypeDef	GPIO_InitStructure;
    NVIC_InitTypeDef	NVIC_InitStructure;

    /*开启相关外设时钟*/
    RCC_AHB1PeriphClockCmd( RS485_RCC_APBPeriphGPIO, ENABLE );
    RCC_APB1PeriphClockCmd( RS485_RCC_APBxPeriph_UART, ENABLE );

    /*相关IO口设置*/
    /*485收发转换口，5V2电源口初始化*/
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

    ///485收发转换口，默认为485接收
    GPIO_InitStructure.GPIO_Pin = RS485_RXTX_PIN;
    GPIO_Init( RS485_RXTX_PORT, &GPIO_InitStructure );
    GPIO_ResetBits( RS485_RXTX_PORT, RS485_RXTX_PIN );

    ///5V2电源口初始化
    GPIO_InitStructure.GPIO_Pin = RS485_PWR_PIN;
    GPIO_Init( RS485_PWR_PORT, &GPIO_InitStructure );
    GPIO_ResetBits( RS485_PWR_PORT, RS485_PWR_PIN );

    Power_485CH1_ON;  // 初始化完就
    /*uartX 管脚设置*/
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin		= RS485_TX_PIN;
    GPIO_Init( RS485_TX_GPIO, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin		= RS485_RX_PIN;
    GPIO_Init( RS485_RX_GPIO, &GPIO_InitStructure );

    GPIO_PinAFConfig( RS485_TX_GPIO, RS485_TX_PIN_SOURCE, RS485_GPIO_AF_UART );
    GPIO_PinAFConfig( RS485_RX_GPIO, RS485_RX_PIN_SOURCE, RS485_GPIO_AF_UART );

    //NVIC 设置
    NVIC_InitStructure.NVIC_IRQChannel						= RS485_UART_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    RS485_baud( 57600 );
    /// Enable USART
    USART_Cmd( RS485_UART, ENABLE );
    /// ENABLE RX interrupts
    USART_ITConfig( RS485_UART, USART_IT_RXNE, ENABLE );
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
static rt_err_t dev_RS485_open( rt_device_t dev, rt_uint16_t oflag )
{
    RX_485const;
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
static rt_err_t dev_RS485_close( rt_device_t dev )
{
    return RT_EOK;
}

/***********************************************************
* Function:RS485_read
* Description:数据模式下读取数据
* Input:void* buff,要读取的数据结构体，改结构体为*LENGTH_BUF,
* Input:rt_size_t count,接收端可以接收的最大长度
* Output:buff
* Return:rt_size_t
* Others:
***********************************************************/
static rt_size_t dev_RS485_read( rt_device_t dev, rt_off_t pos, void *buff, rt_size_t count )
{
    LENGTH_BUF *readBuf;
    if(count < 3)
        return RT_ERROR;
    readBuf		= (LENGTH_BUF *)buff;
    readBuf->wr = 0;
    for( readBuf->wr = 0; readBuf->wr < count; readBuf->wr++ )
    {
        if( uart_rs485_rxbuf_wr != uart_rs485_rxbuf_rd )
        {
            readBuf->body[readBuf->wr] = uart_rs485_rxbuf[uart_rs485_rxbuf_rd++];
            if( uart_rs485_rxbuf_rd >= sizeof( uart_rs485_rxbuf ) )
            {
                uart_rs485_rxbuf_rd = 0;
            }
        }
        else
        {
            break;
        }
    }
    return RT_EOK;
}

/***********************************************************
* Function:		RS485_write
* Description:	数据模式下发送数据，要对数据进行封装
* Input:		const void* buff	要发送的原始数据
       rt_size_t count	要发送数据的长度
* Output:
* Return:
* Others:
***********************************************************/
static rt_size_t dev_RS485_write( rt_device_t dev, rt_off_t pos, const void *buff, rt_size_t count )
{
    rt_size_t	len = count;
    uint8_t		*p	= (uint8_t *)buff;
    //USART_ITConfig( RS485_UART, USART_IT_RXNE, DISABLE );
    TX_485const;
    while( len )
    {
        USART_SendData( RS485_UART, *p++ );
        while( USART_GetFlagStatus( RS485_UART, USART_FLAG_TC ) == RESET )
        {
        }
        len--;
    }
    RX_485const;
    //USART_ITConfig( RS485_UART, USART_IT_RXNE, ENABLE );
    return RT_EOK;
}

/***********************************************************
* Function:		RS485_control
* Description:	控制模块
* Input:		rt_uint8_t cmd	命令类型
    void *arg       参数,依据cmd的不同，传递的数据格式不同
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t dev_RS485_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
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
rt_size_t RS485_write( uint8_t *p, uint8_t len )
{
    rt_kprintf("\n TX_485:");
    printf_hex_data(p, len);
    return dev_RS485_write( &dev_RS485, 0, p, len );
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
uint8_t RS485_read_char( u8 *c )
{
    if( uart_rs485_rxbuf_wr != uart_rs485_rxbuf_rd )
    {
        *c = uart_rs485_rxbuf[uart_rs485_rxbuf_rd++];
        if( uart_rs485_rxbuf_rd >= sizeof( uart_rs485_rxbuf ) )
        {
            uart_rs485_rxbuf_rd = 0;
        }
        printf_hex_data(c, 1);
        return 1;
    }
    return 0;
}

FINSH_FUNCTION_EXPORT( RS485_write, write to RS485 );
#if 0
void  DwinLCD_Data_Process(void)
{
    u16   ADD_ID = 0, i = 0;
    u16   ContentLen = 0; // 实体信息长度
    u8    dwin_reg[50], endtailFlag = 0;

    //AA 55 08 83 30 00 02 1E FF FF 00
    //      08 是长度       83  : 指令  33 00 : 地址   02 1E FF FF 00: 内容

    if(!DwinLCD.Process_Enable)
        return   ;
    DwinLCD.Process_Enable = 0; // clear
    //   Debug
    rt_kprintf("\r\n DwinLCD Rx:");
    for(ADD_ID = 0; ADD_ID < (DwinLCD.RxInfolen + 3); ADD_ID++)
        rt_kprintf("%2X ", DwinLCD.RxInfo[ADD_ID]);
    rt_kprintf("\r\n ");

    if(DwinLCD.RxInfolen >= 3)
    {
        ContentLen = DwinLCD.RxInfolen - 3;
        ADD_ID = ((u16)DwinLCD.RxInfo[4] << 8) + (u16)DwinLCD.RxInfo[5];

        //   Get useful  infomation
        memset(dwin_reg, 0, sizeof(dwin_reg));
        for(i = 0; i < ContentLen; i++)
        {
            if(DwinLCD.RxInfo[6 + i] == 0xFF)
            {
                switch(endtailFlag)
                {
                case 0:
                    endtailFlag = 1; // 有一个FF
                    break;
                case  1:
                    endtailFlag = 2; // 有2个 FF 结束了
                    break;
                default:
                    return;   // 异常情况返回不处理
                }
            }
            else
                dwin_reg[i] = DwinLCD.RxInfo[6 + i];
            //----------------------------------
            if(endtailFlag == 2)
                break;
        }

        switch(ADD_ID)
        {
        case 0x3000:// 车牌号输入   AA 55 0E 83 30 00 05 BD F2 41 37 37 38 38 FF FF 00
            /*
                  输入津A67823输出如下：
                               AA 55 0E 83 30 00 05 BD F2 41 36 37 38 32 33 FF FF
            */

            /*
                  //------------------------------------------------------------
            memset((u8*)&JT808Conf_struct.Vechicle_Info.Vech_Num,0,sizeof(JT808Conf_struct.Vechicle_Info.Vech_Num));

            //clear
            			  // memcpy(JT808Conf_struct.Vechicle_Info.Vech_Num,dwin_reg,strlen(dwin_reg));
            			   memcpy(JT808Conf_struct.Vechicle_Info.Vech_Num,dwin_reg,7); // 国标规定车牌长度7个字节
            			   Api_Config_Recwrite_Large(jt808,0,(u8*)&JT808Conf_struct,sizeof(JT808Conf_struct));
            			    //------------------------------------------------------------
                                           rt_kprintf("\r\n Dwin设置车牌号:%12s",JT808Conf_struct.Vechicle_Info.Vech_Num);
            			  */

            break;
        case 0x3200: // APN        AA 55 14 83 32 00 08 77 77 77 2E 62 61 69 64 75 2E 63 6F 6D FF FF 00
            /*    memset(APN_String,0,sizeof(APN_String));
            memcpy(APN_String,(char*)dwin_reg,strlen(dwin_reg));
            			  memset((u8*)SysConf_struct.APN_str,0,sizeof(APN_String));
            			  memcpy((u8*)SysConf_struct.APN_str,(char*)dwin_reg,strlen(dwin_reg));
            			 Api_Config_write(config,ID_CONF_SYS,(u8*)&SysConf_struct,sizeof(SysConf_struct));


                                          DataLink_APN_Set(APN_String,1);
                                          */
            break;
        case  0x3300:// IP	        AA 55 12 83 33 00 07 31 39 32 2E 31 36 38 2E 31 2E 31 32 FF FF
            //-------------------------------------------------------------------------
            /*  i = str2ipport((char*)dwin_reg, RemoteIP_main, &RemotePort_main);
            if (i <= 4) break;;

            memset(dwin_reg,0,sizeof(dwin_reg));
            IP_Str((char*)dwin_reg, *( u32 * ) RemoteIP_main);
            strcat((char*)dwin_reg, " :");
            sprintf((char*)dwin_reg+strlen((const char*)dwin_reg), "%u\r\n", RemotePort_main);
               memcpy((char*)SysConf_struct.IP_Main,RemoteIP_main,4);
            SysConf_struct.Port_main=RemotePort_main;
            Api_Config_write(config,ID_CONF_SYS,(u8*)&SysConf_struct,sizeof(SysConf_struct));

                  DataLink_MainSocket_set(RemoteIP_main,RemotePort_main,1);
            	 DataLink_EndFlag=1; //AT_End();
            */
            //--------------------------------------------------------------------------

            break;
        case  0x0999:// 拍照  AA 55 06 83 09 99 01 00 03
            //   DEV_regist.Enable_sd=1; // set 发送注册标志位
            // rt_kprintf("\r\n 5 寸屏手动发送注册\r\n");
            break;
        case  0x0004://车辆信息 AA 55 06 83 00 04 01 00 02
            //                       AA 55 06 83 00 04 01 00 02
            DwinLCD.Type = LCD_VechInfo;
            break;
        case   0x3600:  //一键拨号
            Speak_ON;
            rt_kprintf("\r\n   5 寸屏 一键拨号");

            memset(JT808Conf_struct.LISTEN_Num, 0, sizeof(JT808Conf_struct.LISTEN_Num));
            memcpy(JT808Conf_struct.LISTEN_Num, "13051953513", 11);


            Api_Config_Recwrite_Large(jt808, 0, (u8 *)&JT808Conf_struct, sizeof(JT808Conf_struct));
            CallState = CallState_rdytoDialLis; // 准备开始拨打监听号码


            break;
            //---------------    Add  later --------------------------------------------
        case   0x4100:  //   入网ID
            //AA 55 12 83 41 00 07 30 31 33 36 30 32 30 35 39 31 39 31 FF FF

            break;
        case    0x4114:  //  车辆类型  ---货运
            //   AA 55 06 83 41 14 01 00 06
            // 车辆类型
            rt_kprintf("\r\n 5 寸屏手动  车辆类型:货运\r\n");

            break;
        case    0x4118:  //  车辆类型  ---危险
            // AA 55 06 83 41 18 01 00 07
            rt_kprintf("\r\n 5 寸屏手动  车辆类型:危险\r\n");
            break;
        case    0x4120:  //  车辆类型  ---客运
            // AA 55 06 83 41 20 01 00 08
            rt_kprintf("\r\n 5 寸屏手动  车辆类型:客运\r\n");
            break;
        case    0x4122:  //  车辆类型  ---出租
            //  AA 55 06 83 41 22 01 00 09
            rt_kprintf("\r\n 5 寸屏手动  车辆类型:出租\r\n");
            break;
            //-------------------------------------------
        case    0x4110:  //  速度类型  ---GPS
            //  AA 55 06 83 41 10 01 00 04
            JT808Conf_struct.Speed_GetType = 1;
            JT808Conf_struct.DF_K_adjustState = 1;
            ModuleStatus |= Status_Pcheck;
            //  存储
            Api_Config_Recwrite_Large(jt808, 0, (u8 *)&JT808Conf_struct, sizeof(JT808Conf_struct));
            rt_kprintf("\r\n 5 寸屏手动速度:GPS\r\n");
            break;
        case    0x4112:  //  速度类型  ---传感器
            //AA 55 06 83 41 12 01 00 05
            JT808Conf_struct.Speed_GetType = 0;
            JT808Conf_struct.DF_K_adjustState = 0;
            ModuleStatus &= ~Status_Pcheck;
            //  存储
            Api_Config_Recwrite_Large(jt808, 0, (u8 *)&JT808Conf_struct, sizeof(JT808Conf_struct));
            rt_kprintf("\r\n 5 寸屏手动速度:传感器\r\n");
            break;
            //-------------------------------------------
        case    0x4105:  //  颜色  ---黑
            //  AA 55 06 83 41 05 01 00 01
            Menu_color_num = 3;
            JT808Conf_struct.Vechicle_Info.Dev_Color = Menu_color_num;
            Api_Config_Recwrite_Large(jt808, 0, (u8 *)&JT808Conf_struct, sizeof(JT808Conf_struct));
            rt_kprintf("\r\n 5 寸屏手动颜色: 黑\r\n");
            break;
        case    0x4107:  //  颜色---黄
            //AA 55 06 83 41 07 01 00 02
            Menu_color_num = 2;
            JT808Conf_struct.Vechicle_Info.Dev_Color = Menu_color_num;
            Api_Config_Recwrite_Large(jt808, 0, (u8 *)&JT808Conf_struct, sizeof(JT808Conf_struct));
            rt_kprintf("\r\n 5 寸屏手动颜色: 黄\r\n");

            break;
        case    0x4108:  //  颜色 ---蓝
            // AA 55 06 83 41 08 01 00 03
            Menu_color_num = 1;
            JT808Conf_struct.Vechicle_Info.Dev_Color = Menu_color_num;
            Api_Config_Recwrite_Large(jt808, 0, (u8 *)&JT808Conf_struct, sizeof(JT808Conf_struct));
            rt_kprintf("\r\n 5 寸屏手动颜色: 蓝 \r\n");

            break;
            //----------------------------------------
        case   0x4128://   鉴权
            // AA 55 06 83 41 28 01 00 12
            DEV_Login.Operate_enable = 1;
            DEV_Login.Enable_sd = 1;
            rt_kprintf("\r\n 5 寸屏手动发送鉴权\r\n");
            break;
        case   0x4124://   注册
            //  AA 55 06 83 41 24 01 00 00
            DEV_regist.Enable_sd = 1; // set 发送注册标志位
            rt_kprintf("\r\n 5 寸屏手动发送注册\r\n");
            //  要发送短信
            SMS_send_enable();
            break;
        case   0x4126: // 清空鉴权
            // AA 55 06 83 41 26 01 00 11

            break;
        default:
            break;

        }


    }
    else
        return;
}
#endif


/* write one character to serial, must not trigger interrupt */
static void usart2_putc( const char c )
{
    USART_SendData( RS485_UART, c );
    while( !( RS485_UART->SR & USART_FLAG_TXE ) )
    {
        ;
    }
    RS485_UART->DR = ( c & 0x1FF );
}

ALIGN( RT_ALIGN_SIZE )
static char thread_RS485_stack[1024];
struct rt_thread thread_RS485;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_RS485( void *parameter )
{
    rt_err_t res;
    while( 1 )
    {
        wdg_thread_counter[7] = 1;
        if( Camera_Process( ) == 0 )
        {
#ifdef LCD_5inch
            large_lcd_process();
#endif
        }
        rs232_proc();
        rt_thread_delay( RT_TICK_PER_SECOND / 20 );
    }
}

/*RS485设备初始化*/
void RS485_init( void )
{
    //rt_sem_init( &sem_RS485, "sem_RS485", 0, 0 );
    rt_mq_init( &mq_RS485, "mq_RS485", &RS485_rawinfo[0], 128 - sizeof( void * ), RS485_RAWINFO_SIZE, RT_IPC_FLAG_FIFO );

    dev_RS485.type		= RT_Device_Class_Char;
    dev_RS485.init		= dev_RS485_init;
    dev_RS485.open		= dev_RS485_open;
    dev_RS485.close		= dev_RS485_close;
    dev_RS485.read		= dev_RS485_read;
    dev_RS485.write		= dev_RS485_write;
    dev_RS485.control	= dev_RS485_control;

    rt_device_register( &dev_RS485, "RS485", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE );
    rt_device_init( &dev_RS485 );
    Cam_Device_init( );

    rt_thread_init( &thread_RS485,
                    "RS485",
                    rt_thread_entry_RS485,
                    RT_NULL,
                    &thread_RS485_stack[0],
                    sizeof( thread_RS485_stack ), 7, 5 );
    rt_thread_startup( &thread_RS485 );
}

/*RS485开关*/
rt_err_t RS485_onoff( uint8_t openflag )
{
    return 0;
}

FINSH_FUNCTION_EXPORT( RS485_onoff, RS485_onoff([1 | 0] ) );
/************************************** The End Of File **************************************/
