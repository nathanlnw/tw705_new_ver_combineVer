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
#include <string.h>

#include <board.h>
#include <rtthread.h>
#include <finsh.h>

#include "include.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "jt808_param.h"
#include "rtc.h"
#include "jt808_util.h"
#include "jt808_vehicle.h"
#include "jt808_area.h"
#include "menu_include.h"
#include "jt808_gps_pack.h"
#include "math.h"
#include "jt808_config.h"
#include "can.h"
#include "jt808_can.h"



#define CAN_SECT_SIZE				256
#define CAN_HEAD					0xABCDAAA1

#define CAN_BUF_SIZE				490

#ifndef PACK_NONET_NOREPORT
#define PACK_NONET_NOREPORT 		0xFF    /*未登网 未上报 */
#define PACK_REPORTED_OK			0x7F    /*已上报 OK		*/
#endif

typedef __packed struct
{
    uint32_t	mn;
    uint32_t	id;
    uint8_t		attr;
    uint8_t		unused;
    uint16_t	len;
    //uint32_t	msg_id;		/*上报的消息头0x0200*/
} CAN_PACK_HEAD;

typedef struct
{
    uint32_t	addr;
    uint32_t	id;
    uint8_t		attr;
    uint16_t	len;
    uint16_t	pack_count;		//还没有上报的记录数
    uint8_t		get_count;    	//获取的未上报记录数
    uint32_t 	get_addr;		//获取的未上报记录最小id的存储地址
    uint32_t	pack_addr[4];	//上报多个记录的地址
} CAN_PACK_INFO;

u8			save_can_pack = 1;		///该值为非0表示需要CAN数据
CAN_PACK_INFO		can_pack_curr = { 0, 0, 0, 0, 0, 0 };

extern uint8_t	*jt808_param_get(uint16_t id );


/*
   上报信息收到响应,通用应答 0x8001
 */
static JT808_MSG_STATE can_pack_response( JT808_TX_NODEDATA *nodedata, uint8_t *pmsg )
{
    uint8_t			 *msg = pmsg + 12;
    uint16_t		ack_id;
    uint16_t		ack_seq;
    uint8_t			ack_res;
    uint32_t		addr;
    CAN_PACK_HEAD 	head;
    uint32_t		i;

    ack_seq = ( msg[0] << 8 ) | msg[1];
    ack_id	= ( msg[2] << 8 ) | msg[3];
    ack_res = *( msg + 4 );
    //rt_kprintf("\r\n JT808 TX 0x0200 RESPONSE_1");
    //if( ack_res == 0 )
    if((ack_res != 1) && (ack_res != 2))
    {
        //rt_kprintf("_2");
        rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
        if( ack_id == 0x0705 )              /*批量上报*/
        {
            for( i = 0; i < can_pack_curr.get_count; i++ )
            {
                sst25_read( can_pack_curr.pack_addr[i], (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) );
                if(( head.mn == CAN_HEAD) && (head.attr == PACK_NONET_NOREPORT))
                {

                    head.attr = PACK_REPORTED_OK;      /*最高位置0*/
                    rt_kprintf("\n DEL_can_pack:addr=0x%08X,id=%d", can_pack_curr.pack_addr[i], head.id);
                    sst25_write_through( can_pack_curr.pack_addr[i], (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) );
                    can_pack_curr.get_addr = can_pack_curr.pack_addr[i];
                    if( can_pack_curr.pack_count )
                    {
                        can_pack_curr.pack_count--;
                    }
                }
            }
        }
        rt_sem_release( &sem_dataflash );
    }

    return ACK_OK;
}

/*********************************************************************************
  *函数名称:void can_pack_init( void )
  *功能描述:查找最后一个写入的位置,  查找未上报记录最小id的位置,
  			初始化数据存储相关参数，初始化CAN压缩数据数据存储相关参数(can_pack_init)
  			注意:该函数只能用来初始化存储区域，不可增加只能初始化一次的代码，不如不能初始化信号量
  			不能初始化线程等
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-04-15
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void can_pack_init( void )
{
    uint32_t		addr;
    CAN_PACK_HEAD head;
    uint32_t		read_id = 0xFFFFFFFF;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    can_pack_curr.pack_count		= 0; /*重新查找*/
    can_pack_curr.addr	= 0;
    can_pack_curr.id		= 0;
    can_pack_curr.len		= 0;

    memset( can_pack_curr.pack_addr, 0, sizeof( can_pack_curr.pack_addr ) );

    for( addr = ADDR_DF_CAN_PACK_START; addr < ADDR_DF_CAN_PACK_END; addr += CAN_SECT_SIZE )
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) );
        if( head.mn == CAN_HEAD )         /*有效记录*/
        {
            if( head.id >= can_pack_curr.id ) /*查找最近记录,当前写入的位置*/
            {
                can_pack_curr.id		= head.id;
                can_pack_curr.addr		= addr; /*当前记录的地址*/
                can_pack_curr.len		= head.len;
            }
            if( head.attr == PACK_NONET_NOREPORT )          /*bit7=1 还没有发送*/
            {
                can_pack_curr.pack_count++;             /*统计未上报记录数*/
                if( read_id > head.id )     /*要找到最小id的地址*/
                {
                    can_pack_curr.get_addr = addr;
                    read_id			= head.id;
                }
            }
        }
    }
    if(can_pack_curr.pack_count == 0)
    {
        can_pack_curr.get_addr =  can_pack_curr.addr;
    }
    rt_sem_release( &sem_dataflash );
    rt_kprintf( "\n%d>(%d)CAN_PACK写入位置:%x id=%d 读出位置:%x id=%d",
                rt_tick_get( ),
                can_pack_curr.pack_count,
                can_pack_curr.addr,
                can_pack_curr.id,
                can_pack_curr.get_addr,
                read_id );
}



/*********************************************************************************
  *函数名称:uint16_t can_pack_put( uint8_t* pinfo, uint16_t len )
  *功能描述:将整包CAN原始数据存入flash
  *输 入:	pinfo	:pinfo 整包CAN数据的指针
  			len		:can数据长度
  *输 出: none
  *返 回 值:uint16_t
  *作 者:白养民
  *创建日期:2014-04-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void can_pack_put( uint8_t *pinfo, uint16_t len )
{
    CAN_PACK_HEAD head;
    uint32_t *p_addr;
    uint32_t  last_addr;

    if( can_pack_curr.addr )                  /*当先的上报地址*/
    {
        last_addr			= can_pack_curr.addr;
        can_pack_curr.addr	+= ( sizeof( CAN_PACK_HEAD ) + can_pack_curr.len + CAN_SECT_SIZE - 1 );	//加63是为了取一个比较大的值
        //can_pack_curr.addr	&= 0xFFFFFFC0;  /*指向下一个地址*/
        can_pack_curr.addr	= can_pack_curr.addr / CAN_SECT_SIZE * CAN_SECT_SIZE;  /*指向下一个地址*/
        //目的是当写入的位置超过了要读取的位置是将读取的位置后移一个数据位
        if(( can_pack_curr.get_addr > last_addr) && (can_pack_curr.get_addr <= can_pack_curr.addr) && (can_pack_curr.pack_count))
        {
            can_pack_curr.get_addr	= can_pack_curr.addr + CAN_SECT_SIZE;
            can_pack_curr.pack_count--;
            if( can_pack_curr.get_addr >= ADDR_DF_CAN_PACK_END )
            {
                can_pack_curr.get_addr = can_pack_curr.get_addr - ADDR_DF_CAN_PACK_END + ADDR_DF_CAN_PACK_START;
            }
        }
    }
    else                                    /*还没有记录*/
    {
        can_pack_curr.addr = ADDR_DF_CAN_PACK_START;
    }

    if( can_pack_curr.addr >= ADDR_DF_CAN_PACK_END )
    {
        can_pack_curr.addr = can_pack_curr.addr - ADDR_DF_CAN_PACK_END + ADDR_DF_CAN_PACK_START;
        //目的是当写入的位置超过了要读取的位置是将读取的位置后移一个数据位
        if(( can_pack_curr.get_addr <= can_pack_curr.addr) && (can_pack_curr.pack_count))
        {
            can_pack_curr.get_addr	= can_pack_curr.addr + CAN_SECT_SIZE;
            can_pack_curr.pack_count--;
        }
    }
    /*
    if( ( can_pack_curr.addr & 0xFFF ) == 0 )
    {
    	sst25_erase_4k( can_pack_curr.addr );
    }
    */

    can_pack_curr.id++;
    can_pack_curr.len = len;

    head.mn		= CAN_HEAD;
    head.id		= can_pack_curr.id;
    head.len	= len;
    head.attr 	= 0xFF;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    //sst25_write_through( can_pack_curr.addr, (uint8_t*)&head, sizeof( GPS_PACK_HEAD ) );
    //sst25_write_through( can_pack_curr.addr + sizeof( GPS_PACK_HEAD ), pinfo, len );
    sst25_write_auto( can_pack_curr.addr, (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) , ADDR_DF_CAN_PACK_START, ADDR_DF_CAN_PACK_END);
    sst25_write_auto( can_pack_curr.addr + sizeof( CAN_PACK_HEAD ), pinfo, len , ADDR_DF_CAN_PACK_START, ADDR_DF_CAN_PACK_END);
    rt_sem_release( &sem_dataflash );

    rt_kprintf("\n PUT_can_pack:addr=0x%08X,id=%d", can_pack_curr.addr, head.id);
    can_pack_curr.pack_count++;                     /*增加未上报记录数*/
    if( can_pack_curr.pack_count == 1 )
    {
        can_pack_curr.get_addr = can_pack_curr.addr;
    }
}


/*********************************************************************************
  *函数名称:uint16_t can_pack_save( uint8_t* pinfo, uint16_t len )
  *功能描述:将接受到的一条CAN数据存入buf中，如果buf满了，及调用函数can_pack_put将整包CAN原始数据存入flash
  *输 入:	pinfo	:pinfo 一条CAN数据的指针
  			len		:can数据长度
  *输 出: none
  *返 回 值:uint16_t
  *作 者:白养民
  *创建日期:2014-04-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint16_t can_pack_save( uint8_t *pinfo, uint16_t len )
{
    static uint8_t data_buf[CAN_BUF_SIZE];
    static uint16_t data_len = 0;
    uint16_t time_ms;
    static uint16_t can_save_num = 0;

    if((!save_can_pack) || ( len > 16 ))		///减去16没有太大含义，但是至少要减去sizeof(CAN_PACK_HEAD)+7
    {
        data_len = 0;
        return 0;
    }
    if( len > 4 )
    {
        //rt_kprintf("\n CAN_SAVE: ID=0x%08X",buf_to_data(pinfo,4));
        can_save_num++;
        rt_kprintf("\n %d>CAN_SAVE=%04d: ID=0x%08X,DATA=", rt_tick_get(), can_save_num, buf_to_data(pinfo, 4));
        printf_hex_data(pinfo + 4, len - 4);
    }
    ///len=0,表示超过一段时间没有can数据了，需要将之前的数据存储
    if(0 == len)
    {
        if(data_len)
        {
            can_pack_put(data_buf, data_len);
            data_len = 0;
        }
        rt_kprintf("\n CAN_SAVE_END");
        return 0;
    }
    if(data_len + len > sizeof(data_buf))
    {
        can_pack_put(data_buf, data_len);
        data_len = 0;
    }
    if(0 == data_len)
    {
        data_len = 7;
        time_ms = time_1ms_counter % 1000;
        memset(data_buf, 0, sizeof(data_buf));
        data_buf[2] = HOUR( mytime_now );
        data_buf[3] = MINUTE( mytime_now );
        data_buf[4] = SEC( mytime_now );
        data_buf[5] = HEX2BCD(time_ms / 100);
        data_buf[6] = SEC( time_ms % 100 );
    }
    if(data_buf[1] >= 255)
    {
        data_buf[0]++;
    }
    data_buf[1]++;

    memcpy(data_buf + data_len, pinfo, len);
    data_len += len;
    return data_len;
}

/*********************************************************************************
  *函数名称uint8_t jt808_can_get( void )
  *功能描述:获取CAN数据，并发送
  *输 入: none
  *输 出: none
  *返 回 值:uint8_t
  *作 者:白养民
  *创建日期:2014-04-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t jt808_can_get( void )
{
    CAN_PACK_HEAD 	head;
    uint8_t			buf[1024];              /*单包上报的字节不大于256*/
    uint32_t		i;
    uint32_t		addr;
    uint16_t		get_size;               /*获取记录的大小*/
    uint16_t		pos;
    uint8_t			read_count;             /*要读取的记录数*/
    uint32_t 		*p_addr;

    if( can_pack_curr.pack_count == 0 )                 /*没有有未上报记录*/
    {
        return 0;
    }

    //if(( mast_socket->state != CONNECTED )||(jt808_state != JT808_REPORT))  /*不在线*/
    //{
    //	return 0;
    //}


    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    /*批量查找记录*/
    can_pack_curr.get_count	= 0;
    addr				= can_pack_curr.get_addr;
    get_size			= 3;
    pos					= 0;                            /*0x0705数据包，开始的偏移*/
    read_count			= can_pack_curr.pack_count;                 /*还没有报的记录总数*/
    if( can_pack_curr.pack_count > sizeof(can_pack_curr.pack_addr) / sizeof(uint32_t) )                           /*最多1次上报10条记录 303字节*/
    {
        read_count = sizeof(can_pack_curr.pack_addr) / sizeof(uint32_t);
    }
    read_count = 1;		///获取一包数据，否则屏蔽该行
    for( i = 0; i < ADDR_DF_CAN_PACK_SECT * ( 4096 / CAN_SECT_SIZE ); i++ )   /*每记录64字节,每sector(4096Byte)有64记录*/
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) );
        if( head.mn == CAN_HEAD )                     /*有效记录*/
        {
            if( head.attr == PACK_NONET_NOREPORT)      /*还没有发送*/
            {
                //rt_kprintf("\r\njt808_report_get addr=%X",addr);
                //rt_kprintf("   head=");
                //printf_hex_data((const u8 *)&head,sizeof(head));
                //判断需要发送的总长度是否超过单包发送的最大长度
                if( ( pos + head.len  ) >= JT808_PACKAGE_MAX )
                {
                    break;
                }
                //sst25_read( addr + sizeof(GPS_PACK_HEAD), buf + pos, head.len );
                sst25_read_auto( addr + sizeof(CAN_PACK_HEAD), buf + pos, head.len , ADDR_DF_CAN_PACK_START, ADDR_DF_CAN_PACK_END);
                pos								+=  head.len;
                can_pack_curr.pack_addr[can_pack_curr.get_count++]	= addr;

                rt_kprintf("\n GET_can_pack:addr=0x%08X,id=%d", addr, head.id);
                if( can_pack_curr.get_count >= read_count ) /*收到指定条数数据*/
                {
                    break;
                }
            }
            addr	+= ( sizeof(CAN_PACK_HEAD) + head.len + CAN_SECT_SIZE - 1 );	//加63是为了取一个比较大的值
            //addr	&= 0xFFFFFFC0;  			//指向下一个地址
            addr	= addr / CAN_SECT_SIZE * CAN_SECT_SIZE;			//指向下一个地址
        }
        else
        {
            addr += CAN_SECT_SIZE;
        }
        if( addr >= ADDR_DF_CAN_PACK_END )
        {
            addr = ADDR_DF_CAN_PACK_START;
        }
    }
    rt_sem_release( &sem_dataflash );

    if( can_pack_curr.get_count ) /*得到的记录数*/
    {
        buf[0] = 0x00;
        jt808_add_tx( mast_socket->linkno, SINGLE_CMD, 0x0705, -1, RT_NULL, can_pack_response, pos, buf, RT_NULL);
        return 1;

    }
    else
    {
        can_pack_curr.pack_count = 0; /*没有得到要上报的数据*/
    }
    return 0;
    //rt_kprintf( "\n%d>(%d)读出位置:%x,记录数:%d,长度:%d", rt_tick_get( ), can_pack_curr.pack_count, can_pack_curr.get_addr, can_pack_curr.get_count, pos );
}


/*********************************************************************************
  *函数名称void can_proc(void)
  *功能描述:CAN数据保存
  *输 入: none
  *输 出: none
  *返 回 值:none
  *作 者:白养民
  *创建日期:2014-04-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void can_proc(void)
{
    uint32_t i;
    uint8_t *param;
    uint32_t can_id;
    uint8_t  buf[16];
    static uint32_t	last_len = 0;
    static uint32_t	last_tick = 0;
    uint32_t			temp_u32data = 0;
    uint8_t			save_data = 1;
    static uint8_t		can_rx_data = 0;
    static uint16_t	no_save_num = 0;

    while(can_message_rxbuf_rd != can_message_rxbuf_wr )
    {
        save_data = 1;
        if(can_rx_data == 0)
        {
            can_rx_data = 1;
            debug_write("收到CAN数据");
        }
        for(i = 0x0110; i < 0x0119; i++)
        {
            param = jt808_param_get(i);
            if(param == 0)
                continue;
            temp_u32data = buf_to_data(param, 4);
            can_id = buf_to_data(param + 4, 4);
            if(((can_id & 0x1FFFFFFF) == (can_message_rxbuf[can_message_rxbuf_rd].ExtId & 0x1FFFFFFF)) && (temp_u32data == 0))
            {
                no_save_num++;
                save_data = 0;
                rt_kprintf("\n %d>CAN_NNNN=%04d: ID=0x%08X,DATA=", rt_tick_get(), no_save_num, can_message_rxbuf[can_message_rxbuf_rd].ExtId & 0x1FFFFFFF);
                printf_hex_data(can_message_rxbuf[can_message_rxbuf_rd].Data, 8);
                break;
            }
        }
        /////
        if(save_data)
        {
            can_id = can_message_rxbuf[can_message_rxbuf_rd].ExtId & 0x1FFFFFFF;
            can_id &= 0x1FFFFFFF;
            can_id |= BIT(30);
            data_to_buf(buf, can_id, 4);
            memcpy(buf + 4, can_message_rxbuf[can_message_rxbuf_rd].Data, 8);
            last_len = can_pack_save(buf, 12);
            last_tick = rt_tick_get();
        }
        /////
        can_message_rxbuf_rd++;
        if(can_message_rxbuf_rd >= sizeof(can_message_rxbuf) / sizeof(CanRxMsg))
            can_message_rxbuf_rd = 0;
    }
    if((last_len) && (rt_tick_get() - last_tick > RT_TICK_PER_SECOND * 5))
    {
        memset(buf, 0, sizeof(buf));
        last_len = can_pack_save(buf, 0);
    }
}

void can_save(uint8_t i)
{
    save_can_pack = i;
    rt_kprintf("\n save_can_pack=%d", save_can_pack);
}
FINSH_FUNCTION_EXPORT( can_save, can_save_pack([1 | 0] ) );

/************************************** The End Of File **************************************/

