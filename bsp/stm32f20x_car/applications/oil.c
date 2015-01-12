/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// 文件名
 * Author:			// 作者
 * Date:			// 日期
 * Description:		// 模块描述
 * Version:			// 版本信息
 * Function List:	// 主要函数及其功能:功能为将油耗数据保存在flash中，并且如果有U盘插入的话通过提示按键将油耗数据导入到U
 										盘中，需要注意的是，导出数据必须在车台加电30秒后操作，另外只有参数JT808_param.id_0xF04B有效时才能导出数据
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
#include "ff.h"       /* FATFS */
#include "include.h"
#include "usbh_usr.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "jt808_param.h"
#include "math.h"
#include "jt808_config.h"
#include "rs232.h"
#include "Menu_Include.h"
#include "oil.h"

#define OIL_DATA_SAVE_SPACE			300				///油量数据存储间隔，单位为秒，默认为5分钟，及300秒

#define OIL_PACK_SECT_SIZE			16
#define PACK_HEAD_OIL				0xABCDAAA2
#define OIL_DATA_LEN				8


#ifndef PACK_NONET_NOREPORT
#define PACK_NONET_NOREPORT 		0xFF    /*未登网 未上报 */
#define PACK_REPORTED_OK			0x7F    /*已上报 OK		*/
#endif


//#define ADDR_DF_OIL_PACK_END		0x37E000				///gps原始压缩数据存储区域结束位置
//#define ADDR_DF_OIL_PACK_SECT		10						///扇区总数量

typedef __packed struct
{
    uint32_t	mn;
    uint32_t	id;
    //uint32_t	msg_id;		/*上报的消息头0x0200*/
} OIL_PACK_HEAD;

typedef struct
{
    uint32_t	addr;
    uint32_t	id;
    uint8_t		attr;
    uint16_t	len;
    uint16_t	pack_count;		//还没有上报的记录数
    uint16_t	get_count;    	//获取的未上报记录数
    uint32_t 	get_addr;		//获取的未上报记录最小id的存储地址
    uint32_t	pack_addr[4];	//上报多个记录的地址
} OIL_PACK_INFO;


typedef struct
{
    uint32_t 	time;			//实时时间，格式为mytime
    uint16_t	value1;			//计算出来的液面高度值
    uint16_t	value2;			//实时值液面高度值
} OIL_PACK;
extern uint8_t				debug_stop;			///该参数在Menu_5_9_debug中，表示是否刷新屏幕
OIL_PACK_INFO		oil_pack_curr = { 0, 0, 0, 0, 0, 0 };

/*********************************************************************************
  *函数名称:void gps_pack_init( void )
  *功能描述:查找最后一个写入的位置,  查找未上报记录最小id的位置,
  			初始化数据存储相关参数
  			注意:该函数只能用来初始化存储区域，不可增加只能初始化一次的代码，不如不能初始化信号量
  			不能初始化线程等
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-03-5
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void oil_pack_init( void )
{
    uint32_t		addr;
    OIL_PACK_HEAD 	head;
    uint32_t		read_id = 0xFFFFFFFF;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    oil_pack_curr.pack_count		= 0; /*重新查找*/
    oil_pack_curr.addr		= 0;
    oil_pack_curr.id		= 0;
    oil_pack_curr.len		= 0;

    memset( oil_pack_curr.pack_addr, 0, sizeof( oil_pack_curr.pack_addr ) );

    for( addr = ADDR_DF_OIL_PACK_START; addr < ADDR_DF_OIL_PACK_END; addr += OIL_PACK_SECT_SIZE )
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( OIL_PACK_HEAD ) );
        if( head.mn == PACK_HEAD_OIL )         /*有效记录*/
        {
            if( head.id >= oil_pack_curr.id ) /*查找最近记录,当前写入的位置*/
            {
                oil_pack_curr.id		= head.id;
                oil_pack_curr.addr		= addr; /*当前记录的地址*/
                oil_pack_curr.len		= OIL_DATA_LEN;
            }
            if( read_id > head.id )     /*要找到最小id的地址*/
            {
                oil_pack_curr.get_addr = addr;
                read_id			= head.id;
            }
            oil_pack_curr.pack_count++;             /*统计记录数*/
        }
    }
    if(oil_pack_curr.pack_count == 0)
    {
        oil_pack_curr.get_addr =  oil_pack_curr.addr = ADDR_DF_OIL_PACK_START;
    }
    rt_sem_release( &sem_dataflash );
    rt_kprintf( "\n%d>(%d)OIL_PACK写入位置:%x id=%d 读出位置:%x id=%d",
                rt_tick_get( ),
                oil_pack_curr.pack_count,
                oil_pack_curr.addr,
                oil_pack_curr.id,
                oil_pack_curr.get_addr,
                read_id );
}

/*保存油耗数据数据*/
void oil_pack_put( uint8_t *pinfo, uint16_t len )
{
    OIL_PACK_HEAD head;
    uint32_t  last_addr;

    if( oil_pack_curr.addr )                  /*当先的上报地址*/
    {
        last_addr			= oil_pack_curr.addr;
        oil_pack_curr.addr	+= ( sizeof( OIL_PACK_HEAD ) + OIL_DATA_LEN + OIL_PACK_SECT_SIZE - 1 );	//加63是为了取一个比较大的值
        //gps_pack_curr.addr	&= 0xFFFFFFC0;  /*指向下一个地址*/
        oil_pack_curr.addr	= oil_pack_curr.addr / OIL_PACK_SECT_SIZE * OIL_PACK_SECT_SIZE;  /*指向下一个地址*/
        //目的是当写入的位置超过了要读取的位置是将读取的位置后移一个数据位
        if(( oil_pack_curr.get_addr > last_addr) && (oil_pack_curr.get_addr <= oil_pack_curr.addr) && ( oil_pack_curr.pack_count ))
        {
            oil_pack_curr.get_addr	= oil_pack_curr.addr + OIL_PACK_SECT_SIZE;
            oil_pack_curr.pack_count--;
            if( oil_pack_curr.get_addr >= ADDR_DF_OIL_PACK_END )
            {
                oil_pack_curr.get_addr = oil_pack_curr.get_addr - ADDR_DF_OIL_PACK_END + ADDR_DF_OIL_PACK_START;
            }
        }
    }
    else                                    /*还没有记录*/
    {
        oil_pack_curr.addr = ADDR_DF_OIL_PACK_START;
    }

    if( oil_pack_curr.addr >= ADDR_DF_OIL_PACK_END )
    {
        oil_pack_curr.addr = oil_pack_curr.addr - ADDR_DF_OIL_PACK_END + ADDR_DF_OIL_PACK_START;
        //目的是当写入的位置超过了要读取的位置是将读取的位置后移一个数据位
        if(( oil_pack_curr.get_addr <= oil_pack_curr.addr) && (oil_pack_curr.pack_count))
        {
            oil_pack_curr.get_addr	= oil_pack_curr.addr + OIL_PACK_SECT_SIZE;
            oil_pack_curr.pack_count--;
        }
    }
    /*
    if( ( gps_pack_curr.addr & 0xFFF ) == 0 )
    {
    	sst25_erase_4k( gps_pack_curr.addr );
    }
    */

    oil_pack_curr.id++;
    oil_pack_curr.len = len;

    head.mn		= PACK_HEAD_OIL;
    head.id		= oil_pack_curr.id;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    //sst25_write_through( gps_pack_curr.addr, (uint8_t*)&head, sizeof( GPS_PACK_HEAD ) );
    //sst25_write_through( gps_pack_curr.addr + sizeof( GPS_PACK_HEAD ), pinfo, len );
    sst25_write_auto( oil_pack_curr.addr, (uint8_t *)&head, sizeof( OIL_PACK_HEAD ) , ADDR_DF_OIL_PACK_START, ADDR_DF_OIL_PACK_END);
    sst25_write_auto( oil_pack_curr.addr + sizeof( OIL_PACK_HEAD ), pinfo, len , ADDR_DF_OIL_PACK_START, ADDR_DF_OIL_PACK_END);
    rt_sem_release( &sem_dataflash );

    rt_kprintf("\n oil_save_addr=0x%08X,id=%d", oil_pack_curr.addr, oil_pack_curr.id);
    oil_pack_curr.pack_count++;                     /*增加未上报记录数*/
    if( oil_pack_curr.pack_count == 1 )
    {
        oil_pack_curr.get_addr = oil_pack_curr.addr;
    }
    //rt_kprintf("\n PUT_gps_pack:get_addr=0x%08X,put_addr=0x%08X,id=%05d,count=%05d",gps_pack_curr.get_addr,gps_pack_curr.addr,head.id,gps_pack_curr.pack_count);

}

void redraw_debug_lcd(void)
{
    pMenuItem = &Menu_5_9_debug;
    pMenuItem->tick = rt_tick_get( );
    pMenuItem->show( );
}
/*保存原始定位数据*/
void oil_pack_proc( void )
{
    uint8_t 		i;
    static rt_tick_t last_put_tick = 0;
    static uint16_t last_oil_high = 0;
    uint16_t		temp_u16data = 0;
    OIL_PACK 		pack;
    static uint8_t	int_ok = 0;
    static uint8_t 	usb_insert_ok = 0;

    ///初始化该区域相关参数
    if(int_ok == 0)
    {
        int_ok = 1;
        oil_pack_init();
    }

    ///将油耗数据写入flash中
    temp_u16data = abs(last_oil_high - oil_high_average);
    if((temp_u16data > 50) || (rt_tick_get() - last_put_tick > RT_TICK_PER_SECOND * OIL_DATA_SAVE_SPACE))
    {
        pack.time = mytime_now;
        pack.value1 = oil_high_average;
        pack.value2 = oil_high;
        oil_pack_put((uint8_t *)&pack, OIL_DATA_LEN);
        if(rt_tick_get() - last_put_tick > RT_TICK_PER_SECOND * (OIL_DATA_SAVE_SPACE + 30) )
            last_put_tick = rt_tick_get();
        else
            last_put_tick += RT_TICK_PER_SECOND * OIL_DATA_SAVE_SPACE;
        last_oil_high = oil_high_average;
    }
    ///检查是否有U盘插入，然后根据用户操作决定是否写入油耗数据
    if((rt_tick_get() > 30 * RT_TICK_PER_SECOND) && (diskinited) && (usb_insert_ok != diskinited))
    {
        debug_write("检测到U盘插入");
        debug_write("按'确定'键导出油耗");
        redraw_debug_lcd();
        for(i = 0; i < 50; i++)
        {
            if(debug_stop)
                break;
            rt_thread_delay( RT_TICK_PER_SECOND / 10 );
        }
        if(debug_stop)
        {
            debug_stop = 0;
            debug_write("油耗数据导出中...");
            redraw_debug_lcd();
            debug_stop = 0;
            if(oil_packt_get())
            {
                debug_write("油耗数据导出失败");
            }
            //redraw_debug_lcd();
        }
        else
        {
            debug_write("导出时间超时，油耗数据没有导出!");
            pMenuItem = &Menu_1_Idle;
            pMenuItem->tick = rt_tick_get( );
            pMenuItem->show( );
        }
    }
    usb_insert_ok = diskinited;
}

/*
   获取原始定位数据

   使用0900上报
 */
uint8_t oil_packt_get( void )
{
    OIL_PACK_HEAD 	head;
    uint8_t			databuf[256];              /*单包上报的字节不大于256*/
    uint8_t			writebuf[64];              /*单包上报的字节不大于256*/
    uint32_t		i, j;
    uint32_t		addr;
    uint16_t		pos;
    uint16_t		read_count;             /*要读取的记录数*/
    uint16_t		out_pack_num = 0;
    OIL_PACK		*pack;
    FRESULT 		res;
    uint32_t  		len;
    uint32_t		usb_error = 0;
    uint8_t 		usb_insert_ok = diskinited;

    oil_pack_init();
    if( oil_pack_curr.pack_count == 0 )                 /*没有有未上报记录*/
    {
        return 0;
    }
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    ///OPEN文件
    if(usb_insert_ok)
    {
        rt_enter_critical( );
        res = f_open(&file_USB, "1:油量.txt", FA_READ | FA_WRITE | FA_OPEN_ALWAYS );
        if(res == 0)
        {
            f_lseek( &file_USB, file_USB.fsize);
        }
        else
        {
            usb_error += res;
            rt_kprintf("\r\n f_open = %d", res);
        }
    }
    else
    {
        usb_error++;
        rt_kprintf("\n U盘没有插入!");
    }

    /*批量查找记录*/
    oil_pack_curr.get_count	= 0;
    addr				= oil_pack_curr.get_addr;
    pos					= 0;
    read_count			= 0;                 /*还没有报的记录总数*/
    rt_kprintf("\n");
    sprintf(writebuf, "序号:    数据记录时间   ,高 度,实时高度\r\n");
    if(usb_insert_ok == 0)
        rt_kprintf(writebuf);
    if(usb_error == 0)
    {
        res = f_write(&file_USB, writebuf, strlen(writebuf), &len);
        usb_error += res;
        if(res)
        {
            usb_error += res;
            rt_kprintf("\r\n f_write = %d", res);
        }

    }
    for( i = 0; i < ADDR_DF_OIL_PACK_SECT * (4096 / OIL_PACK_SECT_SIZE); i++ )   /*每记录20字节*/
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( OIL_PACK_HEAD ) );
        if( head.mn == PACK_HEAD_OIL )                     /*有效记录*/
        {
            sst25_read_auto( addr + sizeof(OIL_PACK_HEAD), databuf + pos, OIL_DATA_LEN , ADDR_DF_OIL_PACK_START, ADDR_DF_OIL_PACK_END);
            pos								+=  OIL_DATA_LEN;
            addr	+= ( sizeof(OIL_PACK_HEAD) + OIL_DATA_LEN + OIL_PACK_SECT_SIZE - 1 );	//加63是为了取一个比较大的值
            addr	= addr / OIL_PACK_SECT_SIZE * OIL_PACK_SECT_SIZE;			//指向下一个地址
            read_count++;
            oil_pack_curr.get_count++;
            //rt_kprintf("\n GET_gps_pack:addr=0x%08X,id=%d",addr,head.id);
        }
        else
        {
            addr += OIL_PACK_SECT_SIZE;
        }
        if( addr >= ADDR_DF_OIL_PACK_END )
        {
            addr = ADDR_DF_OIL_PACK_START;
        }
        //rt_sem_release( &sem_dataflash );
        if((pos >= sizeof(databuf) - OIL_DATA_LEN) || ( oil_pack_curr.get_count >= oil_pack_curr.pack_count ))
        {
            ///输出信息到U盘
            for(j = 0; j < read_count; j++)
            {
                out_pack_num++;
                pack = (void *)&databuf[j * OIL_DATA_LEN];
                sprintf(writebuf, "%04d:20%02d-%02d-%02d %02d:%02d:%02d,%05d,%05d\r\n",
                        out_pack_num,
                        YEAR( pack->time ),
                        MONTH( pack->time ),
                        DAY( pack->time ),
                        HOUR( pack->time ),
                        MINUTE( pack->time ),
                        SEC( pack->time ),
                        pack->value1,
                        pack->value2
                       );
                if(usb_insert_ok == 0)
                    rt_kprintf(writebuf);
                ///WRITE文件
                if(usb_error == 0)
                {
                    res = f_write(&file_USB, writebuf, strlen(writebuf), &len);
                    usb_error += res;
                    if(res)
                    {
                        usb_error += res;
                        rt_kprintf("\r\n f_write = %d", res);
                    }
                }
            }
            //最后清空相关数据
            pos = 0;
            read_count = 0;
        }
        if( oil_pack_curr.get_count >= oil_pack_curr.pack_count ) /*收到指定条数数据*/
        {
            break;
        }
    }
    ///CLOSE文件
    if(usb_insert_ok)
    {
        res = f_close(&file_USB);
        if(res)
        {
            rt_kprintf("\n f_close = %d", res);
            usb_error += res;
        }
        if(0 == usb_error)
        {
            debug_write("油耗数据导出完成");
            redraw_debug_lcd();
        }
        ///开启线程调用
        rt_exit_critical( );
    }

    rt_kprintf("\n USB_WRITE_ERROR = %d", usb_error);
    return usb_error;
    //rt_kprintf( "\n%d>(%d)读出位置:%x,记录数:%d,长度:%d", rt_tick_get( ), gps_pack_curr.pack_count, gps_pack_curr.get_addr, gps_pack_curr.get_count, pos );
}

//FINSH_FUNCTION_EXPORT( oil_packt_get, oil_packt_get );
/************************************** The End Of File **************************************/
