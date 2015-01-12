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
#include "jt808_gps.h"
#include "jt808_GB19056.h"



#define 	GPS_GGHYPT
#define 	GPS_DATA_SECT_SIZE		16
#define 	GPS_DATA_HEAD			0xABCDAAA4


typedef struct _GPSPoint
{
    int sign;
    int deg;
    int min;
    int sec;
} GPSPoint;

uint32_t gps_sec_count = 0;                         /*gps秒脉冲输出*/

/*要用union类型保存，位域，访问吗?*/
uint32_t		jt808_alarm			= 0x0;
uint32_t		jt808_alarm_last	= 0x0;          /*上一次的上报状态*/

uint32_t		jt808_status		= 0x0;
uint32_t		jt808_status_last	= 0x0;          /*上一次的状态信息*/

static uint32_t jt808_report_interval	= 60;       /*GPS上报时间间隔，为0:停止上报*/
static uint32_t jt808_report_distance	= 1000;     /*GPS上报距离间隔,为0 停止上报*/

static uint32_t total_distance	= 0;                /*总的累计里程*/

uint16_t		jt808_8202_track_interval	= 0;    /*jt808_8202 临时位置跟踪控制*/
uint32_t		jt808_8202_track_duration	= 0;
uint16_t		jt808_8202_track_counter;

uint32_t		jt808_8203_manual_ack_seq	= 0;    /*人工确认报警的标识位 0,3,20,21,22,27,28*/
uint16_t		jt808_8203_manual_ack_value = 0;

#if 0


/*
   区域的定义,使用list关联起来，如果node过多的话，
   RAM是否够用
   使用dataflash存储，以4k作为cache,缓存访问
   每秒的位置信息都要判断
 */
struct
{
    uint32_t	id;                 /*区域ID*/
    uint16_t	attr;               /*属性*/
    uint32_t	latitude;           /*中心纬度*/
    uint32_t	logitude;           /*中心经度*/
    uint32_t	radius;             /*半径*/
    uint8_t		datetime_start[6];  /*开始时刻，使用utc是不是更好?*/
    uint8_t		datetime_end[6];
    uint16_t	speed;
    uint8_t		duration;           /*持续时间*/
} circle;

struct
{
    uint32_t	id;                 /*区域ID*/
    uint16_t	attr;               /*属性*/
    uint32_t	latitude;           /*中心纬度*/
    uint32_t	logitude;           /*中心经度*/
    uint32_t	radius;             /*半径*/
    uint8_t		datetime_start[6];  /*开始时刻，使用utc是不是更好?*/
    uint8_t		datetime_end[6];
    uint16_t	speed;
    uint8_t		duration;           /*持续时间*/
} rectangle;

#endif

uint32_t	gps_lati;
uint32_t	gps_longi;
uint16_t	gps_speed;		//单位为0.1km/h

uint16_t	gps_cog; /*course over ground*/
uint16_t	gps_alti;
uint8_t		gps_datetime[6];

/*记录上一次的位置，用来计算距离用*/
uint32_t gps_lati_last	= 0;
uint32_t gps_longi_last	= 0;

/*保存gps基本位置信息*/
GPS_BASEINFO	gps_baseinfo;
/*gps的状态*/
GPS_STATUS		gps_status = { 0x3020, MODE_BDGPS, 0, 0x0, 0 };
extern uint16_t		ADC_ConValue[3];        //   3  个通道ID    0 : 电池 1: 灰线   2:  绿线

/*
   Epoch指的是一个特定的时间：1970-01-01 00:00:00 UTC
   UNIX时间戳：Unix时间戳（英文为Unix time, POSIX time 或 Unix timestamp）
   是从Epoch（1970年1月1日00:00:00 UTC）开始所经过的秒数，不考虑闰秒。

 */

uint32_t	utc_now		= 0;
MYTIME		mytime_now	= 0;
uint8_t		save_0x0200_data = 0;		//为0不保存0200数据，为1需要保存0200数据

uint8_t		ACC_status;     /*0:ACC关   1:ACC开  */
uint32_t	ACC_ticks;      /*ACC状态发生变化时的tick值，此时GPS可能未定位*/


uint32_t	gps_notfixed_count = 0;

struct
{
    uint8_t		mode;       /*上报模式 0:定时 1:定距 2:定时定距*/
    uint8_t		userlogin;  /*是否使用登录*/
    uint32_t	time_unlog;
    uint32_t	time_sleep;
    uint32_t	time_emg;
    uint32_t	time_default;
    uint32_t	distance_unlog;
    uint32_t	distance_sleep;
    uint32_t	distance_emg;
    uint32_t	distance_default;

    uint32_t	last_tick;      /*上一次上报的时刻*/
    uint32_t	last_distance;  /*上一次上报时的里程*/
} jt808_report;
extern uint16_t	gps_reset_time;			///gps模块多长时间不定位需要复位，单位为秒;

#define DEBUG_GPS

#ifdef DEBUG_GPS
uint8_t		speed_add	= 0;
uint32_t	speed_count = 0;
#endif

/*hmi最近15分钟速度*/
static void process_hmi_15min_speed( void )
{
    static uint8_t	hmi_15min_speed_count	= 0;                                    /*·??ó?úμ?????êy*/
    static uint32_t hmi_15min_speed_sum		= 0;                                    /*?ù?èà??óoí*/
    MYTIME		temp_mytime_now	= mytime_now;

    if( ( temp_mytime_now & 0xFFFFFFC0 ) > jt808_param_bk.speed_15min[jt808_param_bk.speed_curr_index].time ) /*D?ê±?ì,??è·μ?·??ó*/
    {
        //hmi_15min_speed[hmi_15min_speed_curr].speed=hmi_15min_speed_sum/hmi_15min_speed_count;
        jt808_param_bk.speed_curr_index = (jt808_param_bk.speed_curr_index + 1) % 15;
        jt808_param_bk.speed_15min[jt808_param_bk.speed_curr_index].time	= temp_mytime_now & 0xFFFFFFC0;
        //rt_kprintf("\n%d>speed_curr_index=%d",rt_tick_get(),jt808_param_bk.speed_curr_index);
        hmi_15min_speed_sum		= 0;
        hmi_15min_speed_count	= 0;
    }
    else if( ( temp_mytime_now & 0xFFFFFFC0 ) + 10 < jt808_param_bk.speed_15min[jt808_param_bk.speed_curr_index].time )
    {
        jt808_param_bk.speed_curr_index = 0;
        memset(jt808_param_bk.speed_15min, 0, sizeof(jt808_param_bk.speed_15min));
    }
    jt808_param_bk.speed_curr_index	%= 15;
    //jt808_param_bk.speed_15min[jt808_param_bk.speed_curr_index].time	= temp_mytime_now & 0xFFFFFFC0;
    hmi_15min_speed_sum	+= current_speed;
    hmi_15min_speed_count++;
    jt808_param_bk.speed_15min[jt808_param_bk.speed_curr_index].speed = hmi_15min_speed_sum / hmi_15min_speed_count; /*??ê±?üD?*/
}

/*
   Linux源码中的mktime算法解析
 */
static __inline unsigned long linux_mktime( unsigned int year, unsigned int mon,
        unsigned int day, unsigned int hour,
        unsigned int min, unsigned int sec )
{
    if( 0 >= (int)( mon -= 2 ) )    /**//* 1..12 -> 11,12,1..10 */
    {
        mon		+= 12;              /**//* Puts Feb last since it has leap day */
        year	-= 1;
    }

    return ( ( ( (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day ) +
                 year * 365 - 719499
               ) * 24 + hour      /**//* now have hours */
             ) * 60 + min         /**//* now have minutes */
           ) * 60 + sec;          /**//* finally seconds */
}

/*计算距离*/
uint32_t calc_distance( void )
{
    static uint32_t utc_distance_last = 0;
    static uint16_t	last_speed_10x = 0;

    ///根据速度类型选择速度
    if(jt808_param.id_0xF043 == 1)
    {
        current_speed_10x	= gps_speed;
    }
    else
    {
        current_speed_10x	= car_data.get_speed;
    }
#ifndef JT808_TEST_JTB
    ///这个if else 的目的是将货运车辆和客运车辆超过速度太大的值去掉
    if(jt808_param.id_0xF00D == 1)
    {
        if(current_speed_10x > 1200)
            current_speed_10x = 1200;
    }
    else
    {
        if(current_speed_10x > 1500)
            current_speed_10x = 1500;
    }
#endif
    current_speed = current_speed_10x / 10;
    ///速度积分
    if( ( ( utc_now - utc_distance_last ) >= 1 ) || ( utc_distance_last == 0 ) )
    {
        //对累计路程进行积分
        jt808_param_bk.car_mileage	+= current_speed_10x;
        total_distance				+= current_speed / 3.6;
        //jt808_data.id_0xFA01		= total_distance;   /*总里程m*/
        jt808_data.id_0xFA01 		= jt808_param_bk.car_mileage / 36;		//单位为M.
        utc_distance_last			= utc_now;
    }
    last_speed_10x = current_speed_10x;
}

#if 0
/**/
static double gpsToRad( GPSPoint point )
{
    return point.sign * ( point.deg + ( point.min + point.sec / 60.0 ) / 60.0 ) * 3.141592654 / 180.0;
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
static double getDistance( GPSPoint latFrom, GPSPoint lngFrom, GPSPoint latTo, GPSPoint lngTo )
{
    double	latFromRad	= gpsToRad( latFrom );
    double	lngFromRad	= gpsToRad( lngFrom );
    double	latToRad	= gpsToRad( latTo );
    double	lngToRad	= gpsToRad( lngTo );
    double	lngDiff		= lngToRad - lngFromRad;
    double	part1		= pow( cos( latToRad ) * sin( lngDiff ), 2 );
    //double part2 = pow( cos(latFromRad)*sin(latToRad)*cos(lngDiff) , 2);
    double	part2 = pow( cos( latFromRad ) * sin( latToRad ) - sin( latFromRad ) * cos( latToRad ) * cos( lngDiff ), 2 );

    double	part3 = sin( latFromRad ) * sin( latToRad ) + cos( latFromRad ) * cos( latToRad ) * cos( lngDiff );
    //double centralAngle = atan2( sqrt(part1 + part2) / part3 );
    double	centralAngle = atan( sqrt( part1 + part2 ) / part3 );
    return 6371.01 * 1000.0 * centralAngle; //Return Distance in meter
}

#endif


/*只能做1->0的写入*/
#define ATTR_NONET_NOREPORT 0xFF    /*未登网 未上报 */
#define ATTR_ONNET_NOREPORT 0xFE    /*已登网 上报失败 */
#define ATTR_REPORTED		0xFC    /*已上报 11111100*/

typedef __packed struct
{
    uint32_t	mn;
    uint32_t	id;
    uint8_t		attr;
    uint8_t		unused;
    uint16_t	len;
    //uint32_t	msg_id;		/*上报的消息头0x0200*/
} GPS_REPORT_HEAD;

typedef struct
{
    uint32_t	addr;
    uint32_t	id;
    uint8_t		attr;
    uint16_t	len;
} REPORT_INFO;

REPORT_INFO		report_curr = { 0, 0, 0, 0 };

static uint32_t report_addr[20];            /*上报记录的地址*/
static uint8_t	report_get_count	= 0;    /*获取的未上报记录数*/
static uint32_t report_get_addr		= 0;    /*获取的未上报记录最小id的地址*/

static uint16_t report_count = 0;           /*还没有上报的记录数*/
uint8_t	report_trans_normal = 1;	//0表示当前发送的是补报数据(发送命令0x0704)，1表示正常数据(命令0x0200)；
static uint32_t utc_report_last = 0;

extern STRUCT_GSM_PARAM gsm_param;


extern void sd_write_console(char *str);

/*********************************************************************************
  *函数名称:void gps_fact_set( void )
  *功能描述:格式化GPS定位数据存储区域
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-12-01
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void gps_fact_set(void)
{
    u32					TempAddress;
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    for( TempAddress = ADDR_DF_POSTREPORT_START; TempAddress < ADDR_DF_POSTREPORT_END; )
    {
        sst25_erase_4k( TempAddress );
        TempAddress += 4096;
    }
    rt_sem_release( &sem_dataflash );
}

/*
   上报信息收到响应,通用应答 0x8001
 */
static JT808_MSG_STATE jt808_report_response( JT808_TX_NODEDATA *nodedata, uint8_t *pmsg )
{
    uint8_t			 *msg = pmsg + 12;
    uint16_t		ack_id;
    uint16_t		ack_seq;
    uint8_t			ack_res;
    uint32_t		addr;

    GPS_REPORT_HEAD head;
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
        if( ack_id == 0x0200 )              /*在线上报*/
        {
            //rt_kprintf("_3");
            debug_write("定位数据上报OK!");
            addr = *( (uint32_t *)( nodedata->user_para ) );
            sst25_read( addr, (uint8_t *)&head, sizeof( GPS_REPORT_HEAD ) );
            //rt_kprintf("   write addr=%X",addr);
            //rt_kprintf("   head=");
            //printf_hex_data((const u8 *)&head,sizeof(head));
            if(( head.mn == GPS_DATA_HEAD ) && (head.attr >= ATTR_ONNET_NOREPORT))
            {
                head.attr &= 0x7F;          /*最高位置0*/
                sst25_write_through( addr, (uint8_t *)&head, sizeof( GPS_REPORT_HEAD ) );
                report_get_addr = addr;
                if( report_count )
                {
                    report_count--;
                }
            }
            //nodedata->user_para = RT_NULL;  /*传递进来只是指明flash地址，并不是要释放*/
        }
        else if( ack_id == 0x0704 )              /*批量上报*/
        {
            for( i = 0; i < report_get_count; i++ )
            {
                sst25_read( report_addr[i], (uint8_t *)&head, sizeof( GPS_REPORT_HEAD ) );
                if(( head.mn == GPS_DATA_HEAD ) && (head.attr >= ATTR_ONNET_NOREPORT))
                {
                    head.attr &= 0x7F;      /*最高位置0*/
                    sst25_write_through( report_addr[i], (uint8_t *)&head, sizeof( GPS_REPORT_HEAD ) );
                    report_get_addr = report_addr[i];
                    if( report_count )
                    {
                        report_count--;
                    }
                }
            }
        }
        rt_sem_release( &sem_dataflash );
    }

    return ACK_OK;
}

/*
   消息发送超时,保存数据为补报
 */
static JT808_MSG_STATE jt808_report_timeout( JT808_TX_NODEDATA *nodedata )
{
    //nodedata->user_para = RT_NULL; /*传递进来只是指明flash地址，并不是要释放*/
    return ACK_TIMEOUT;
}


/*********************************************************************************
  *函数名称:void jt808_report_init( void )
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
void jt808_report_init( void )
{
    uint32_t		addr;
    GPS_REPORT_HEAD head;
    uint32_t		read_id = 0xFFFFFFFF;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    report_count		= 0; /*重新查找*/
    report_curr.addr	= 0;
    report_curr.id		= 0;
    report_curr.len		= 0;

    memset( report_addr, 0, sizeof( report_addr ) );

    for( addr = ADDR_DF_POSTREPORT_START; addr < ADDR_DF_POSTREPORT_END;  )
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( GPS_REPORT_HEAD ) );
        if( head.mn == GPS_DATA_HEAD )         /*有效记录*/
        {
            if( head.id >= report_curr.id ) /*查找最近记录,当前写入的位置*/
            {
                report_curr.id		= head.id;
                report_curr.addr	= addr; /*当前记录的地址*/
                report_curr.len		= head.len;
            }
            if( head.attr > 0x7F )          /*bit7=1 还没有发送*/
            {
                report_count++;             /*统计未上报记录数*/
                if( read_id > head.id )     /*要找到最小id的地址*/
                {
                    report_get_addr = addr;
                    read_id			= head.id;
                }
            }
            addr	+= ( sizeof(GPS_REPORT_HEAD) + head.len + GPS_DATA_SECT_SIZE - 1 );	//加GPS_DATA_SECT_SIZE - 1 是为了取一个比较大的值
            addr	= addr / GPS_DATA_SECT_SIZE * GPS_DATA_SECT_SIZE;
        }
        else
        {
            addr += GPS_DATA_SECT_SIZE;
        }
    }
    if(report_count == 0)
    {
        report_get_addr =  report_curr.addr;
    }
    memset(&gps_baseinfo, 0, sizeof(gps_baseinfo));
    if( report_curr.addr )
    {
        sst25_read( report_curr.addr, (uint8_t *)&head, sizeof( GPS_REPORT_HEAD ) );
        if( head.mn == GPS_DATA_HEAD )                     /*有效记录*/
        {
            sst25_read( report_curr.addr + sizeof( GPS_REPORT_HEAD ), (uint8_t *)&gps_baseinfo, sizeof(gps_baseinfo) );
            gps_baseinfo.status &= ~(BYTESWAP4( BIT_STATUS_FIXED ));			///当前定位信息设置为未定位；

            if( jt808_param_bk.car_mileage == 0 )
            {
                jt808_param_bk.car_mileage	= BYTESWAP4( gps_baseinfo.mileage );
                jt808_param_bk.car_mileage	*= 3600LL;
            }
        }
    }
    gps_baseinfo.mileage_id		= 0x01;
    gps_baseinfo.mileage_len	= 0x04;
    gps_baseinfo.csq_id			= 0x30;
    gps_baseinfo.csq_len		= 0x01;
    gps_baseinfo.NoSV_id		= 0x31;
    gps_baseinfo.NoSV_len		= 0x01;

    jt808_param_bk.car_alarm	&= (BIT(0) | BIT(3) | BIT(20) | BIT(21) | BIT(22) | BIT(27) | BIT(28));
    jt808_alarm_last			= jt808_param_bk.car_alarm;          /*上一次的上报状态*/
    rt_sem_release( &sem_dataflash );
    rt_kprintf( "\n%d>(%d)GPS_DATA写入位置:%x id=%d 读出位置:%x id=%d",
                rt_tick_get( ),
                report_count,
                report_curr.addr,
                report_curr.id,
                report_get_addr,
                read_id );
}

/*补报数据保存*/
void jt808_report_put( uint8_t *pinfo, uint16_t len )
{
    GPS_REPORT_HEAD head;
    uint32_t *p_addr;
    uint32_t  last_addr;
    char tempbuf[64];
    /*
    if((report_curr.id == 0)&&((jt808_status & BIT_STATUS_FIXED) == 0))
    	{
    	gps_reset_time = 600;
    	return;
    	}
    	*/

    //rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    if( report_curr.addr )                  /*当先的上报地址*/
    {
        last_addr			= report_curr.addr;
        report_curr.addr	+= ( sizeof( GPS_REPORT_HEAD ) + report_curr.len + GPS_DATA_SECT_SIZE - 1 );	//加63是为了取一个比较大的值
        report_curr.addr	= report_curr.addr / GPS_DATA_SECT_SIZE * GPS_DATA_SECT_SIZE;  /*指向下一个地址*/
        //目的是当写入的位置超过了要读取的位置是将读取的位置后移一个数据位
        if(( report_get_addr > last_addr) && (report_get_addr <= report_curr.addr) && (report_count))
        {
            report_get_addr	= report_curr.addr + GPS_DATA_SECT_SIZE;
            report_count--;
            if( report_get_addr >= ADDR_DF_POSTREPORT_END )
            {
                report_get_addr = report_get_addr - ADDR_DF_POSTREPORT_END + ADDR_DF_POSTREPORT_START;
            }
        }

    }
    else                                    /*还没有记录*/
    {
        report_curr.addr = ADDR_DF_POSTREPORT_START;
    }

    if( report_curr.addr >= ADDR_DF_POSTREPORT_END )
    {
        report_curr.addr = report_curr.addr - ADDR_DF_POSTREPORT_END + ADDR_DF_POSTREPORT_START;
        //目的是当写入的位置超过了要读取的位置是将读取的位置后移一个数据位
        if(( report_get_addr <= report_curr.addr) && (report_count))
        {
            report_get_addr	= report_curr.addr + GPS_DATA_SECT_SIZE;
            report_count--;
        }
    }
    /*
    if( ( report_curr.addr & 0xFFF ) == 0 )
    {
    	sst25_erase_4k( report_curr.addr );
    }
    */

    report_curr.id++;
    report_curr.len = len;

    head.mn		= GPS_DATA_HEAD;
    head.id		= report_curr.id;
    head.len	= len;

    if(( mast_socket->state == CONNECTED ) && (jt808_state == JT808_REPORT)) /*当前在线，保存并直接上报*/
    {
        head.attr = ATTR_ONNET_NOREPORT;
        //jt808_add_tx( 1, SINGLE_CMD, 0x0200, -1, jt808_report_timeout, jt808_report_response, len, pinfo, (void*)report_curr.addr );
    }
    else  /*是需要补报的信息*/
    {
        //report_trans_normal = 1;
        head.attr = ATTR_NONET_NOREPORT;
    }

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    //sst25_write_through( report_curr.addr, (uint8_t*)&head, sizeof( GPS_REPORT_HEAD ) );
    //sst25_write_through( report_curr.addr + 12, pinfo, len );
    sst25_write_auto( report_curr.addr, (uint8_t *)&head, sizeof( GPS_REPORT_HEAD ) , ADDR_DF_POSTREPORT_START, ADDR_DF_POSTREPORT_END);
    sst25_write_auto( report_curr.addr + sizeof( GPS_REPORT_HEAD ), pinfo, len , ADDR_DF_POSTREPORT_START, ADDR_DF_POSTREPORT_END);
    rt_sem_release( &sem_dataflash );

    //sprintf(tempbuf,"写入GPS数据,ID=%05d,状态=%08X,报警=%08X,速度=%05d,addr=0x%08X",head.id,jt808_status,jt808_param_bk.car_alarm,BYTESWAP2(gps_baseinfo.speed_10x),report_curr.addr);
    //sd_write_console(tempbuf);
    rt_kprintf("\n20%02d-%02d-%02d-%02d:%02d:%02d->",
               YEAR( mytime_now ),
               MONTH( mytime_now ),
               DAY( mytime_now ),
               HOUR( mytime_now ),
               MINUTE( mytime_now ),
               SEC( mytime_now )
              );
    rt_kprintf("写入GPS数据,ID=%05d,状态=%08X,报警=%08X,速度=%05d,addr=0x%08X,time=%d  ", head.id, jt808_status, jt808_param_bk.car_alarm, BYTESWAP2(gps_baseinfo.speed_10x), report_curr.addr, utc_report_last);
    //rt_kprintf("\nUTC=%d,utc_report_last=%d,jt808_report_interval=%d",utc_now,utc_report_last,jt808_report_interval);
    //printf_hex_data(gps_baseinfo.datetime,6);
    report_count++;                     /*增加未上报记录数*/
    if( report_count == 1 )
    {
        report_get_addr = report_curr.addr;
        if(( mast_socket->state == CONNECTED ) && (jt808_state == JT808_REPORT))
        {
            report_trans_normal = 1;		///表示补包数据已经发送完成，后面的数据以正常格式发送
        }
    }
    //如果当前存入的数据包是最后一包或者当前数据是上位机请求的临时位置跟踪信息，则需要上报
    if(( report_count == 1 ) || ( jt808_8202_track_duration ))           /*要找到最小的*/
    {
        ///如果注册成功，则发送数据
        if(( mast_socket->state == CONNECTED ) && (jt808_state == JT808_REPORT))
        {
            p_addr = rt_malloc(sizeof(uint32_t));
            if(p_addr != RT_NULL)
            {
                *p_addr = report_curr.addr;
                jt808_add_tx( mast_socket->linkno, SINGLE_CMD, 0x0200, -1, jt808_report_timeout, jt808_report_response, len, pinfo, (void *)p_addr );
            }
        }
        //rt_kprintf( "\n%d>(%d)写入位置:%x id=%d", rt_tick_get( ), report_count, report_curr.addr, report_curr.id );
    }
    ///设备休眠后有了新数据，恢复正常
    if(( jt808_status & BIT_STATUS_ACC ) == 0 )         /*当前状态为ACC关*/
    {
        if(jt808_param.id_0x0027 > 120 )
        {
            //control_device(DEVICE_NORMAL,0);
            if(mast_socket->state != CONNECTED)
                sock_proc(mast_socket, SOCK_CONNECT);
        }
    }
}


void set_0704(uint8_t set)
{
    report_trans_normal = set;
    rt_kprintf("\n report_trans_normal = %d", report_trans_normal);
}
FINSH_FUNCTION_EXPORT(set_0704, set_0704);

/*
   补报数据更新原先的并读取新纪录
   每次收到中心应答后调用
   每次重新拨号成功后都要从新确认一下
   区分首次读还是上报中的读记录

   使用0704上报，要拼包
 */
uint8_t jt808_report_get( void )
{
    GPS_REPORT_HEAD head;
    uint8_t			buf[1024];              /*单包上报的字节不大于256*/
    uint32_t		i;
    uint32_t		addr;
    uint16_t		get_size;               /*获取记录的大小*/
    uint16_t		pos;
    uint8_t			read_count;             /*要读取的记录数*/
    uint32_t 		*p_addr;

    if( report_count == 0 )                 /*没有有未上报记录*/
    {
        return 0;
    }

    //if(( mast_socket->state != CONNECTED )||(jt808_state != JT808_REPORT))  /*不在线*/
    //{
    //	return 0;
    //}

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    /*批量查找记录*/
    addr				= report_get_addr;
    report_get_count	= 0;
    get_size			= 3;
    pos					= 3;                            /*0x0704数据包，开始的偏移*/
    read_count			= report_count;                 /*还没有报的记录总数*/
    if( report_count > sizeof(report_addr) / sizeof(uint32_t) )                           /*最多1次上报10条记录 303字节*/
    {
        read_count = sizeof(report_addr) / sizeof(uint32_t);
    }
    for( i = 0; i < ADDR_DF_POSTREPORT_SECT * 64; i++ )   /*每记录最小64字节,每sector(4096Byte)有64记录*/
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( GPS_REPORT_HEAD ) );
        if( head.mn == GPS_DATA_HEAD )                     /*有效记录*/
        {
            if( head.attr >= ATTR_ONNET_NOREPORT )      /*还没有发送*/
            {
                //rt_kprintf("\r\njt808_report_get addr=%X",addr);
                //rt_kprintf("   head=");
                //printf_hex_data((const u8 *)&head,sizeof(head));
                ///盲区补报数据用命令0x0704，但是只有公共货运平台支持该命令
                //判断需要发送的总长度是否超过单包发送的最大长度
                if( ( pos + head.len + 2 ) >= JT808_PACKAGE_MAX )
                {
                    break;
                }
                buf[pos++]	= head.len >> 8;
                buf[pos++]	= head.len;
                //sst25_read( addr + 12, buf + pos, head.len );
                sst25_read_auto( addr + sizeof(GPS_REPORT_HEAD), buf + pos, head.len , ADDR_DF_POSTREPORT_START, ADDR_DF_POSTREPORT_END);
                pos								+= ( head.len );
                report_addr[report_get_count]	= addr;
                report_get_count++;
                if( report_get_count == read_count ) /*收到指定条数数据*/
                {
                    break;
                }
#ifndef GPS_GGHYPT
                break;
#endif
                if( report_trans_normal )		///正常发送数据用命令0x0200
                {
                    break;
                }
            }
            addr	+= ( sizeof(GPS_REPORT_HEAD) + head.len + GPS_DATA_SECT_SIZE - 1 );	//加63是为了取一个比较大的值
            addr	= addr / GPS_DATA_SECT_SIZE * GPS_DATA_SECT_SIZE;;  			//指向下一个地址
        }
        else
        {
            addr += GPS_DATA_SECT_SIZE;
        }
        if( addr >= ADDR_DF_POSTREPORT_END )
        {
            addr = ADDR_DF_POSTREPORT_START;
        }
    }
    rt_sem_release( &sem_dataflash );

    if( report_get_count ) /*得到的记录数*/
    {
        if( report_trans_normal )		///正常发送数据用命令0x0200
        {
            p_addr = rt_malloc(sizeof(uint32_t));
            if(p_addr != RT_NULL)
            {
                *p_addr = addr;
                jt808_add_tx( mast_socket->linkno, SINGLE_CMD, 0x0200, -1, jt808_report_timeout, jt808_report_response, pos - 5, buf + 5, (void *)p_addr );
            }
            return;
        }
#ifdef GPS_GGHYPT
        buf[0]	= report_get_count >> 8;
        buf[1]	= report_get_count & 0xff;
        buf[2]	= 1;
        jt808_add_tx( mast_socket->linkno, SINGLE_CMD, 0x0704, -1, RT_NULL, jt808_report_response, pos, buf, RT_NULL );
#else
        p_addr = rt_malloc(sizeof(uint32_t));
        if(p_addr != RT_NULL)
        {
            *p_addr = addr;
            jt808_add_tx( mast_socket->linkno, SINGLE_CMD, 0x0200, -1, jt808_report_timeout, jt808_report_response, pos - 5, buf + 5, (void *)p_addr );
        }
#endif
        return 1;
    }
    else
    {
        report_count = 0; /*没有得到要上报的数据*/
    }
    return 0;
    //rt_kprintf( "\n%d>(%d)读出位置:%x,记录数:%d,长度:%d", rt_tick_get( ), report_count, report_get_addr, report_get_count, pos );
}

/*
   处理gps信息,有多种条件组合，上报
   此时已收到争取的
 */
//#define GPS_TEST

#define FLAG_SEND_AREA			0x80
#define FLAG_SEND_STATUS		0x01
#define FLAG_SEND_ALARM			0x02
#define FLAG_SEND_FIX_TIME		0x04
#define FLAG_SEND_FIX_DISTANCE	0x08
#define FLAG_SEND_CENTER		0x80

/*处理上报*/
uint8_t process_gps_report( void )
{
    uint32_t		tmp_s, tmp_a;
    uint8_t			flag_send	= 0; /*默认不上报*/
    uint8_t			*palarmdata = RT_NULL;
    uint16_t		alarm_length;
    uint32_t		alarm_bits;
    uint8_t 		ret = 0;
    uint16_t		len = sizeof(gps_baseinfo);
    uint8_t			buf[300];
    uint32_t		mytime_last_gps;
    static uint32_t	report_interval = 30;		///上次的定时间隔
    static uint32_t distance_report_last = 0;	///最后一次等间距上报的距离
    char			tempbuf[20];

    if(device_unlock == 0)
        return 0;

    /*区域路线处理*/
    jt808_param_bk.car_alarm &= ~(BIT(20) + BIT(21) + BIT(22));
    alarm_bits = area_get_alarm( (uint32_t *)&palarmdata, &alarm_length );
    if( alarm_bits ) /*有告警*/
    {
        rt_kprintf( "\n区域有告警" );
        memcpy( buf + len, palarmdata, alarm_length );
        len += alarm_length;
        flag_send = FLAG_SEND_AREA;
    }
    jt808_param_bk.car_alarm			|= alarm_bits;
    /////////增加其他附加消息start
    buf[len++] = 0x03;							///附加消息，行驶记录功能获取到的速度4字节
    buf[len++] = 0x02;
    if(jt808_param.id_0xF032 & BIT(31))			///对于没有校准脉冲系数的车辆，将该值填充为GPS速度
    {
        len += data_to_buf(buf + len, gps_speed, 2);
    }
    else
    {
        len += data_to_buf(buf + len, car_data.get_speed, 2);
    }
    buf[len++] = 0x25;							///附加消息，扩展车辆信息
    buf[len++] = 0x04;
    len += data_to_buf(buf + len, car_state_ex, 4);
    buf[len++] = 0x2B;							///附加消息，扩展车辆信息模拟量1和2
    buf[len++] = 0x04;
    len += data_to_buf(buf + len, ADC_ConValue[2], 2);
    len += data_to_buf(buf + len, ADC_ConValue[1], 2);
    /////////增加其他附加消息end
    gps_baseinfo.alarm	= BYTESWAP4( jt808_param_bk.car_alarm );
    gps_baseinfo.status = BYTESWAP4( jt808_status );
    memcpy( buf, (uint8_t *)&gps_baseinfo, sizeof(gps_baseinfo) );

    tmp_s = jt808_status ^ jt808_status_last;				/*状态位发生变化*/
    //tmp_a = ( jt808_param_bk.car_alarm ^ jt808_alarm_last );				/*告警位变化*/
    tmp_a = jt808_param_bk.car_alarm & (~jt808_alarm_last );				//有了新的告警位才发送数据

    ////忽略需要不需要立即上报的状态位和报警位
    tmp_s &= ~(BIT_STATUS_FIXED);			///忽略定位信息，否则会有间隔不准的情况
    //tmp_a &=~(BIT_ALARM_DEVIATE);			///忽略偏离报警信息，否则会有间隔不准的情况

    jt808_status_last	= jt808_status;
    jt808_alarm_last	= jt808_param_bk.car_alarm;
    //jt808_alarm_last	&= ~(BIT(20)+BIT(21)+BIT(22));

    /*中心追踪,直接上报，并返回*/
    if( jt808_8202_track_duration ) //要追踪
    {
        jt808_8202_track_counter++;
        if( jt808_8202_track_counter >= jt808_8202_track_interval )
        {
            //rt_kprintf("中心追踪数据");
            jt808_8202_track_counter = 0;
            ///////jt808_tx( 0x0200, buf, 28 + alarm_length );
            ///////rt_kprintf( "\n%d>上报gps(%02x)", rt_tick_get( ), flag_send );
            //jt808_report_put( buf, len );
            //ret = 1;
            flag_send				|= FLAG_SEND_CENTER;
            if( jt808_8202_track_duration > jt808_8202_track_interval )
            {
                jt808_8202_track_duration -= jt808_8202_track_interval;
            }
            else
            {
                jt808_8202_track_duration = 0;
                jt808_8202_track_interval = 0;
            }
        }
        //return ret;
    }

    /*数据上报方式,如何组合出各种情况 */
    if( tmp_s )                                       /*状态发生变化，要上报,*/
    {
        flag_send				|= FLAG_SEND_STATUS;
        //jt808_report_distance	= 0;
        //jt808_report_interval	= 0;
        //utc_report_last			= utc_now;          /*重新计时*/
        //distance				= 0;                /*重新计算距离*/
    }
    /*判断上报方式，中心下发修改参数时，要及时改变*/
    if( ( jt808_param.id_0x0020 & 0x01 ) == 0x00 )   /*有定时上报*/
    {
        if( jt808_status & BIT_STATUS_ACC )         /*当前状态为ACC开*/
        {
            jt808_report_interval = jt808_param.id_0x0029;
        }
        else
        {
            jt808_report_interval = jt808_param.id_0x0027;
        }
    }
    if( jt808_param.id_0x0020 )             /*有定距上报*/
    {
        if( jt808_status & BIT_STATUS_ACC ) /*当前状态为ACC开*/
        {
            jt808_report_distance = jt808_param.id_0x002C;
        }
        else
        {
            jt808_report_distance = jt808_param.id_0x002E;
        }
    }

    if( tmp_a )                                           /*告警发生变化，立即上报,*/
    {
        flag_send		|= FLAG_SEND_ALARM;
        sprintf(tempbuf, "报警%08X", jt808_param_bk.car_alarm);
        debug_write(tempbuf);
        //utc_report_last = utc_now;
        //distance		= 0;
    }

    if( jt808_param_bk.car_alarm & BIT_ALARM_EMG )                   /*紧急告警*/
    {
        if( ( jt808_param.id_0x0020 & 0x01 ) == 0x0 )   /*有定时上报*/
        {
            jt808_report_interval = jt808_param.id_0x0028;
        }
        if( jt808_param.id_0x0020 )                     /*有定距上报*/
        {
            jt808_report_distance = jt808_param.id_0x002F;
        }
    }

    /*计算定时上报*/
    if( ( jt808_param.id_0x0020 & 0x01 ) == 0x0 )       /*有定时上报*/
    {
#ifdef JT808_TEST_JTB
        if(jt808_report_interval == 1)
        {
            if( utc_now == utc_report_last )
            {
                ++utc_report_last;
                rt_kprintf("&RMC时间重复&");
            }
            else if( utc_now - utc_report_last >  2 )
            {
                utc_report_last = utc_now;
                rt_kprintf("&RMC时间跳秒&");
            }
            else
            {
                ++utc_report_last;
            }
            mytime_last_gps = utc_to_mytime(utc_report_last);
            flag_send		|= FLAG_SEND_FIX_TIME;
            gps_baseinfo.datetime[0]	= HEX2BCD( YEAR(mytime_last_gps) );
            gps_baseinfo.datetime[1]	= HEX2BCD( MONTH(mytime_last_gps) );
            gps_baseinfo.datetime[2]	= HEX2BCD( DAY(mytime_last_gps) );
            gps_baseinfo.datetime[3]	= HEX2BCD( HOUR(mytime_last_gps) );
            gps_baseinfo.datetime[4]	= HEX2BCD( MINUTE(mytime_last_gps) );
            gps_baseinfo.datetime[5]	= HEX2BCD( SEC(mytime_last_gps) );
        }
        else
#endif
        {
            if( utc_now - utc_report_last >= jt808_report_interval )
            {
                flag_send		|= FLAG_SEND_FIX_TIME;
                utc_report_last = utc_now;
            }
        }
        if(report_interval != jt808_report_interval)
        {
            utc_report_last = utc_now;
            report_interval	= jt808_report_interval;
        }
    }
    /*计算定距上报*/
    if( jt808_param.id_0x0020 ) /*有定距上报*/
    {
        if(jt808_data.id_0xFA01 - distance_report_last >= jt808_report_distance)
        {
            if(distance_report_last == 0 )
            {
                distance_report_last = jt808_data.id_0xFA01;
            }
            else
            {
                distance_report_last += jt808_report_distance;
            }
            flag_send	|= FLAG_SEND_FIX_DISTANCE;
        }
    }
    else
    {
        distance_report_last = jt808_data.id_0xFA01;
    }

    if(flag_send & FLAG_SEND_AREA)
    {
        //rt_kprintf("\n SAVE_GPS_DATA=");
        //printf_hex_data(buf, sizeof(gps_baseinfo) + alarm_length);
        ///目的是在进行交通部测试是有了区域报警后5秒后开始上报下一包正常数据
        if((jt808_report_interval >= 5) && (utc_now > jt808_report_interval))
            utc_report_last = utc_now + 5 - jt808_report_interval;
    }
#ifdef GPS_TEST
    if(gps_speed)
    {
        flag_send = 1;
    }
#endif
    if(((param_set_speed_state == 2) && (current_speed > 10)) || (save_0x0200_data))
    {
        flag_send = 1;
        utc_report_last = utc_now;
        param_set_speed_state = 0;
    }
    /*生成要上报的数据,在线时直接上报，还是都暂存再上报，后者。*/
    if( flag_send )
    {
        save_0x0200_data = 0;
        //rt_kprintf( "\n%d>上报gps(%02x)", rt_tick_get( ), flag_send );
        ret = 1;
        jt808_report_put( buf, len );
    }
    jt808_param_bk.car_alarm &= ~(BIT_ALARM_COLLIDE);		///碰撞报一次后就清空
    return ret;
}

/*
   $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNTXT,01,01,01,ANTENNA OK*2B7,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNGGA,074002.00,3905.291085,N,11733.138264,E,1,11,0.9,8.2,M,-1.6,M,,,1.4*68E
   $GNGLL,3905.291085,N,11733.138264,E,074002.00,A,0*02.9,8.2,M,-1.6,M,,,1.4*68E
   $GPGSA,A,3,18,05,08,02,26,29,15,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*3F8E
   $BDGSA,A,3,04,03,01,07,,,,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*220.9*3F8E
   $GPGSV,2,1,7,18,10,278,29,05,51,063,08,21,052,24,02,24,140,45*4C220.9*3F8E
   $GPGSV,2,2,7,26,72,055,24,29,35,244,37,15,66,224,37*76,24,140,45*4C220.9*3F8E
   $BDGSV,1,1,4,04,27,124,38,03,42,190,34,01,38,146,37,07,34,173,35*55220.9*3F8E

   返回处理的字段数，如果正确的话
 */

static uint8_t process_rmc( uint8_t *pinfo )
{
    //检查数据完整性,执行数据转换
    uint8_t		year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0, fDateModify = 0;
    uint32_t	degrees, minutes;
    uint8_t		commacount = 0, count = 0;

    uint32_t	lati		= 0, longi = 0;
    uint16_t	speed_10x	= 0;
    uint16_t	cog			= 0;    /*course over ground*/

    uint8_t		i;
    uint8_t		buf[22];
    uint8_t		*psrc = pinfo + 6;  /*指向开始位置 $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E*/

    /*因为自增了一次，所以从pinfo+6开始*/
    while( *psrc++ )
    {
        if( *psrc != ',' )
        {
            buf[count++]	= *psrc;
            buf[count]		= '0';
            buf[count + 1]	= 0;
            continue;
        }

        commacount++;
        switch( commacount )
        {
        case 1: /*时间*/
            if( count < 6 )
            {
                return 1;
            }

            i = ( buf[0] - 0x30 ) * 10 + ( buf[1] - 0x30 ) + 8;
            if( i > 23 )
            {
                fDateModify = 1;
                i			-= 24;
            }
            /*转成HEX格式*/
            hour	= i;
            min		= ( buf[2] - 0x30 ) * 10 + ( buf[3] - 0x30 );
            sec		= ( buf[4] - 0x30 ) * 10 + ( buf[5] - 0x30 );
            break;
        case 2:                         /*A_V*/
            if( buf[0] != 'A' )         /*未定位*/
            {
                jt808_status	&= ~BIT_STATUS_FIXED;
                gps_speed		= 0;
                return 2;
            }

#if 0
            if( buf[0] == 'A' )
            {
                jt808_status |= BIT_STATUS_GPS;
            }
            else if( buf[0] == 'V' )
            {
                jt808_status &= ~BIT_STATUS_GPS;
            }
            else
            {
                return 2;
            }
#endif
            break;
        case 3: /*纬度处理ddmm.mmmmmm*/
            if( count < 9 )
            {
                return 3;
            }

            degrees = ( ( buf [0] - 0x30 ) * 10 + ( buf [1] - 0x30 ) ) * 1000000;
            minutes = ( buf [2] - 0x30 ) * 1000000 +
                      ( buf [3] - 0x30 ) * 100000 +
                      ( buf [5] - 0x30 ) * 10000 +
                      ( buf [6] - 0x30 ) * 1000 +
                      ( buf [7] - 0x30 ) * 100 +
                      ( buf [8] - 0x30 ) * 10 +
                      ( buf [9] - 0x30 );   /*多加了一个位，想要保证精度*/
            lati = degrees + minutes / 6;
            break;
        case 4:                             /*N_S处理*/
            if( buf[0] == 'N' )
            {
                jt808_status &= ~BIT_STATUS_NS;
            }
            else if( buf[0] == 'S' )
            {
                jt808_status |= BIT_STATUS_NS;
            }
            else
            {
                return 4;
            }
            break;
        case 5: /*经度处理*/
            if( count < 10 )
            {
                return 5;
            }
            degrees = ( ( buf [0] - 0x30 ) * 100 + ( buf [1] - 0x30 ) * 10 + ( buf [2] - 0x30 ) ) * 1000000;
            minutes = ( buf [3] - 0x30 ) * 1000000 +
                      ( buf [4] - 0x30 ) * 100000 +
                      ( buf [6] - 0x30 ) * 10000 +
                      ( buf [7] - 0x30 ) * 1000 +
                      ( buf [8] - 0x30 ) * 100 +
                      ( buf [9] - 0x30 ) * 10 +
                      ( buf [10] - 0x30 );
            longi = degrees + minutes / 6;
            break;
        case 6: /*E_W处理*/
            if( buf[0] == 'E' )
            {
                jt808_status &= ~BIT_STATUS_EW;
            }
            else if( buf[0] == 'W' )
            {
                jt808_status |= BIT_STATUS_EW;
            }
            else
            {
                return 6;
            }
            break;
        case 7: /*速度处理 */
            speed_10x = 0;
            for( i = 0; i < count; i++ )
            {
                if( buf[i] == '.' )
                {
                    if(i + 1 < count)
                        speed_10x += ( buf[i + 1] - 0x30 );
                    break;
                }
                else
                {
                    speed_10x	+= ( buf[i] - 0x30 );
                    speed_10x	= speed_10x * 10;
                }
            }
            /*当前是0.1knot => 0.1Kmh  1海里=1.852Km  1852=1024+512+256+32+16+8+4*/
#ifdef DEBUG_GPS
            if( speed_count )
            {
                speed_10x += ( speed_add * 10 );
                speed_count--;
            }

#endif
            speed_10x *= 1.856;
            //i=speed_10x;
            //speed_10x=(i<<10)|(i<<9)|(i<<8)|(i<<5)|(i<<4)|(i<<3)|(i<<2);
            //speed_10x/=1000;
            break;

        case 8: /*方向处理*/
            cog = 0;
            for( i = 0; i < count; i++ )
            {
                if( buf[i] == '.' )
                {
                    break;
                }
                else
                {
                    cog = cog * 10;
                    cog += ( buf[i] - 0x30 );
                }
            }
            break;

        case 9: /*日期处理*/
            if( count < 6 )
            {
                return 9;
            }

            day		= ( ( buf [0] - 0x30 ) * 10 ) + ( buf [1] - 0x30 );
            mon		= ( ( buf [2] - 0x30 ) * 10 ) + ( buf [3] - 0x30 );
            year	= ( ( buf [4] - 0x30 ) * 10 ) + ( buf [5] - 0x30 );

            if( fDateModify )
            {
                day++;
                if( mon == 2 )
                {
                    if( ( year % 4 ) == 0 ) /*没有考虑整百时，要被400整除，NM都2100年*/
                    {
                        if( day == 30 )
                        {
                            day = 1;
                            mon++;
                        }
                    }
                    else if( day == 29 )
                    {
                        day = 1;
                        mon++;
                    }
                }
                else if( ( mon == 4 ) || ( mon == 6 ) || ( mon == 9 ) || ( mon == 11 ) )
                {
                    if( day == 31 )
                    {
                        mon++;
                        day = 1;
                    }
                }
                else
                {
                    if( day == 32 )
                    {
                        mon++;
                        day = 1;
                    }
                    if( mon == 13 )
                    {
                        mon = 1;
                        year++;
                    }
                }
            }

            /*都处理完了更新 gps_baseinfo,没有高程信息*/
            gps_lati_last	= gps_lati;
            gps_longi_last	= gps_longi;
            gps_lati		= lati;
            gps_longi		= longi;

            if(!((jt808_param.id_0xF04D >= 600) && (speed_10x > jt808_param.id_0xF04D)))
            {
                if(jia_speed)
                    gps_speed	= jia_speed;  // debug 80KM
                else
                    gps_speed	= speed_10x;
            }
            gps_cog			= cog;
            gps_datetime[0] = year;
            gps_datetime[1] = mon;
            gps_datetime[2] = day;
            gps_datetime[3] = hour;
            gps_datetime[4] = min;
            gps_datetime[5] = sec;

            gps_baseinfo.alarm		= BYTESWAP4( jt808_param_bk.car_alarm );
            gps_baseinfo.latitude	= BYTESWAP4( lati );
            gps_baseinfo.longitude	= BYTESWAP4( longi );
            gps_baseinfo.speed_10x	= BYTESWAP2( speed_10x );
            gps_baseinfo.cog		= BYTESWAP2( cog );

            gps_baseinfo.status		= BYTESWAP4( jt808_status );
            gps_baseinfo.datetime[0]	= HEX2BCD( year );
            gps_baseinfo.datetime[1]	= HEX2BCD( mon );
            gps_baseinfo.datetime[2]	= HEX2BCD( day );
            gps_baseinfo.datetime[3]	= HEX2BCD( hour );
            gps_baseinfo.datetime[4]	= HEX2BCD( min );
            gps_baseinfo.datetime[5]	= HEX2BCD( sec );

            //utc_now = linux_mktime( 2000 + year, mon, day, hour, min, sec );

            //对累计路程进行积分
            //jt808_param_bk.car_mileage	+= speed_10x;

            //附加消息体部分
            gps_baseinfo.mileage_id		= 0x01;
            gps_baseinfo.mileage_len	= 0x04;
            gps_baseinfo.csq_id			= 0x30;
            gps_baseinfo.csq_len		= 0x01;
            gps_baseinfo.NoSV_id		= 0x31;
            gps_baseinfo.NoSV_len		= 0x01;
            gps_baseinfo.mileage		= BYTESWAP4(jt808_param_bk.car_mileage / 3600);
            gps_baseinfo.csq			= gsm_param.csq;
            gps_baseinfo.NoSV			= gps_status.NoSV;
            if( ( jt808_status & BIT_STATUS_FIXED ) == 0 )
            {
                debug_write("定位成功!");
            }

            /*首次定位,校时*/
            //if( ( jt808_status & BIT_STATUS_FIXED ) == 0 )
            {
                mytime_now	= MYDATETIME( year, mon, day, hour, min, sec );
                utc_now 	= mytime_to_utc(mytime_now);
                jt808_param_bk.utctime = utc_now;
                param_save_bksram();
                date_set( year, mon, day );
                time_set( hour, min, sec );
                //if( ( jt808_status & BIT_STATUS_FIXED ) == 0 )
                //	rt_kprintf( "\n%d>rtc sync %02d-%02d-%02d %02d:%02d:%02d", rt_tick_get( ), year, mon, day, hour, min, sec );
                //sd_write_console(pinfo);
            }
            /*
            if( gps_datetime[5] == 0 )      //整分钟
            {
            	mytime_now								= MYDATETIME( year, mon, day, hour, min, sec );
            	//*(__IO uint32_t*)( BKPSRAM_BASE + 4 )	= utc_now;
            	jt808_param_bk.utctime = mytime_to_utc(mytime_now);
            	param_save_bksram();
            	if( gps_datetime[4] == 0 )  //整小时校准
            	{
            		date_set( year, mon, day );
            		time_set( hour, min, sec );
            	}
            }
            */
            /*
            if( ( jt808_status & BIT_STATUS_FIXED ) == 0 )
            {
            	if(jt808_report_interval == 1)
            		utc_report_last = utc_now - 1;		///-1可以保证1秒的定位也能保存
            }
            */

            //  GB  DOUBT3   generate  here
            if((gps_lati_last != 0) && (gps_longi_last != 0))
            {
                GB_doubt_Data3_Trigger(gps_lati_last, gps_longi_last, gps_lati, gps_longi);
            }
            // ---    end

            jt808_status |= BIT_STATUS_FIXED;
            return 0;
        }
        count	= 0;
        memset(buf, 0, sizeof(buf));
    }
    return 10;
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
uint8_t process_gga( uint8_t *pinfo )
{
    //检查数据完整性,执行数据转换
    uint8_t		NoSV = 0;
    uint8_t		i;
    uint8_t		buf[20];
    uint8_t		commacount	= 0, count = 0;
    uint8_t		*psrc		= pinfo + 7; //指向开始位置
    uint16_t	altitute;

    while( *psrc++ )
    {
        if( *psrc != ',' )
        {
            buf[count++]	= *psrc;
            buf[count]		= 0;
            continue;
        }
        commacount++;
        switch( commacount )
        {
        case 1: /*时间处理 */
            if( count < 6 )
            {
                gps_status.NoSV = NoSV;
                return 1;
            }
            break;

        case 2: /*纬度处理ddmm.mmmmmm*/
            break;

        case 3: /*N_S处理*/
            break;

        case 4: /*经度处理*/

            break;
        case 5: /*E_W处理*/
            break;
        case 6: /*定位类型*/
            break;
        case 7: /*NoSV,卫星数*/
            /*
            if( count < 1 )
            {
            	break;
            }
            */
            NoSV = 0;
            for( i = 0; i < count; i++ )
            {
                NoSV	= NoSV * 10;
                NoSV	+= ( buf[i] - 0x30 );
            }
            gps_status.NoSV = NoSV;
            break;
        case 8: /*HDOP*/
            break;

        case 9: /*MSL Altitute*/
            if( count < 1 )
            {
                break;
            }
            /*
            $GNRMC,102641.000,A,3955.2964,N,11600.0003,E,29.0,180.179,050313,,E,A*0A
            $GNGGA,100058.000,4000.0001,N,11600.0001,E,1,09,0.898,54.3,M,0.0,M,,0000,1.613*49
            */
            altitute = 0;
            for( i = 0; i < count; i++ )
            {
                if( buf[i] == '.' )
                {
                    break;
                }
                altitute	= altitute * 10;
                altitute	+= ( buf[i] - '0' );
            }
            gps_baseinfo.altitude	= BYTESWAP2( altitute );
            gps_alti				= altitute;

            return 0;
        }
        count	= 0;
        buf[0]	= 0;
    }
    //gps_status.NoSV = NoSV;
    return 9;
}


void gps_set_mode_old(void)
{
    /*
    BD2(冷启动)			$CCSIR,1,1*48<CR><LF>
    GPS(冷启动)			$CCSIR,2,1*4B<CR><LF>
    BD2/GPS双模(冷启动)	$CCSIR,3,1*4A<CR><LF>
    BD2(不重启)			$CCSIR,1,0*49<CR><LF>
    GPS(不重启)			$CCSIR,2,0*4A<CR><LF>
    BD2/GPS双模(不重启)	$CCSIR,3,0*4B<CR><LF>

    */
    const char *gps_string[] =
    {
        "$CCSIR,1,0*49\n\r",		///BD2
        "$CCSIR,2,0*4A\n\r",		///GPS
        "$CCSIR,3,0*4B\n\r",		///BDGPS
    };
    enum BDGPS_MODE mode;
    uint8_t set_mode = jt808_param.id_0x0090 & 0x0000000F;
    if(set_mode == 0x03)
    {
        mode = MODE_BDGPS;
    }
    else if(set_mode == 0x01)
    {
        mode = MODE_GPS;
    }
    else if(set_mode == 0x02)
    {
        mode = MODE_BD;
    }
    else
    {
        mode = MODE_BDGPS;
    }
    if(mode != gps_status.mode)
    {
        gps_write(gps_string[mode - 1]);
        rt_kprintf("\n 重新设置GPS模式=%d.%s", mode, gps_string[mode - 1]);
    }
}


void gps_set_mode(void)
{
    /*
    BD2(冷启动)			$CCSIR,1,1*48<CR><LF>
    GPS(冷启动)			$CCSIR,2,1*4B<CR><LF>
    BD2/GPS双模(冷启动)	$CCSIR,3,1*4A<CR><LF>
    BD2(不重启)			$CCSIR,1,0*49<CR><LF>
    GPS(不重启)			$CCSIR,2,0*4A<CR><LF>
    BD2/GPS双模(不重启)	$CCSIR,3,0*4B<CR><LF>

    */
    /*
     ///热启动模块
     const char* gps_string[]={
     	"$CCSIR,1,0*49\r\n",		///BD2
     	"$CCSIR,2,0*4A\r\n",		///GPS
     	"$CCSIR,3,0*4B\r\n",		///BDGPS
     	};
     	*/
    ///冷启动模块
    const char *gps_string[] =
    {
        "$CCSIR,1,1*48\r\n",		///BD2
        "$CCSIR,2,1*4B\r\n",		///GPS
        "$CCSIR,3,1*4A\r\n",		///BDGPS
    };
    enum BDGPS_MODE mode;
    uint8_t set_mode = jt808_param.id_0x0090 & 0x0000000F;
    if(set_mode == 0x03)
    {
        mode = MODE_BDGPS;
    }
    else if(set_mode == 0x01)
    {
        mode = MODE_GPS;
    }
    else if(set_mode == 0x02)
    {
        mode = MODE_BD;
    }
    else
    {
        mode = MODE_BDGPS;
    }
    if(mode != gps_status.mode)
    {
        gps_write(gps_string[mode - 1]);
        rt_kprintf("\n 重新设置GPS模式=%d.%s", mode, gps_string[mode - 1]);
    }
}

/***********************************************************
* Function:
* Description:gps收到信息后的处理，头两个字节为长度
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void gps_rx( uint8_t *pinfo, uint16_t length )
{
    uint8_t ret;
    uint16_t i;
    char	 *psrc;
    psrc = (char *)pinfo;

    *( psrc + length ) = 0;
    /*是否输出原始信息*/
    if( gps_status.Raw_Output )
    {
        rt_kprintf( "%s", psrc );
    }
    if(length < 6)
        return;
    /*保存RAW数据*/
    //jt808_gps_pack( (char*)pinfo, length );

    if( strncmp( psrc + 3, "GGA,", 4 ) == 0 )
    {
        if( strncmp( psrc + 1, "GN", 2 ) == 0 )
        {
            gps_status.mode = MODE_BDGPS;
        }
        else if( strncmp( psrc + 1, "GP", 2 ) == 0 )
        {
            gps_status.mode = MODE_GPS;
        }
        else if( strncmp( psrc + 1, "BD", 2 ) == 0 )
        {
            gps_status.mode = MODE_BD;
        }
        jt808_status &= ~(BIT(18) + BIT(19) + BIT(20) + BIT(21));
        jt808_status |= (jt808_param.id_0x0090 & 0x0000000F) << 18;
        gps_set_mode();
        process_gga( (uint8_t *)psrc );
    }

    if( strncmp( psrc + 3, "RMC,", 4 ) == 0 )
    {
        gps_sec_count++;

        //去掉后面的不可见字符
        for(i = length - 1; i > 0; i--)
        {
            if(pinfo[i] > 0x1F)
                break;
            else
            {
                pinfo[i] = 0;
            }
        }
        ret = process_rmc( (uint8_t *)psrc );

        if( ret == 0 )                   /*已定位*/
        {
            if(gps_baseinfo.NoSV == 0)
            {
                if(gps_status.NoSV == 0)
                    gps_baseinfo.NoSV = 4;
                else
                    gps_baseinfo.NoSV = gps_status.NoSV;
            }
            gps_notfixed_count = 0;
            if(device_unlock)
            {
                process_hmi_15min_speed( ); /*最近15分钟速度*/
                //vdr_rx_gps( );              /*行车记录仪数据处理*/
                area_process( );            /*区域线路告警*/
                //calc_distance( );
            }
        }
        else
        {
#ifdef JT808_TEST_JTB
            synctime();
            gps_baseinfo.latitude		= 0;
            gps_baseinfo.longitude		= 0;
            gps_baseinfo.altitude		= 0;
            gps_baseinfo.cog			= 0;
            gps_baseinfo.NoSV			= 0;
            jt808_status 			   &= ~(BIT(18) + BIT(19) + BIT(20) + BIT(21));
#endif
            gps_baseinfo.speed_10x		= 0;
            gps_baseinfo.datetime[0]	= HEX2BCD( YEAR(mytime_now) );
            gps_baseinfo.datetime[1]	= HEX2BCD( MONTH(mytime_now) );
            gps_baseinfo.datetime[2]	= HEX2BCD( DAY(mytime_now) );
            gps_baseinfo.datetime[3]	= HEX2BCD( HOUR(mytime_now) );
            gps_baseinfo.datetime[4]	= HEX2BCD( MINUTE(mytime_now) );
            gps_baseinfo.datetime[5]	= HEX2BCD( SEC(mytime_now) );
            //if(gps_notfixed_count%10 == 0)
            //sd_write_console(psrc);
            //adjust_mytime_now( );       /*调整mytime_now*/
            gps_notfixed_count++;
        }
        calc_distance( );
        SpeedingFatigueProc( );              /*行车记录仪数据处理*/
        process_gps_report( );      	/*处理GPS上报信息*/
    }
    /*
    else
    {
    	if( utc_now - utc_report_last >= jt808_report_interval )
    		process_gps_report( );      	///处理GPS上报信息
    }
    */
    if( strncmp( psrc + 3, "VTG,", 4 ) == 0 )		///只有华讯模块支持该命令
    {
        gps_write("$PHXM103,5,0,0,1*3B\r\n");
        rt_kprintf("\nOFF_VTG_CMD");
    }

    //$PHXM103,5,0,0,1*3B\r\n

    /*天线开短路检测 gps<
       $GNTXT,01,01,01,ANTENNA OK*2B
       $GNTXT,01,01,01,ANTENNA OPEN*3B
     */
    /*
    if( strncmp( psrc + 3, "TXT", 3 ) == 0 )
    {
    	if( jt808_param.id_0xF013 != 0x3017 ) ///型号不对,保存新的，并重启
    	{
    		rt_kprintf( "\njt808_param.id_0xF013=%04x", jt808_param.id_0xF013 );
    		jt808_param.id_0xF013 = 0x3017;
    		param_save( 1 );
    		reset( 3 );
    	}
    	if( strncmp( psrc + 24, "OK", 2 ) == 0 )
    	{
    		if((jt808_param_bk.car_alarm & ( BIT_ALARM_GPS_OPEN | BIT_ALARM_GPS_SHORT )))
    			sd_write_console("GPS天线OK");
    		gps_status.Antenna_Flag = 0;
    		jt808_param_bk.car_alarm				&= ~( BIT_ALARM_GPS_OPEN | BIT_ALARM_GPS_SHORT );
    	}else if( strncmp( psrc + 24, "OPEN", 4 ) == 0 )
    	{
    		if((jt808_param_bk.car_alarm & BIT_ALARM_GPS_OPEN) == 0)
    			{
    			sd_write_console("定位天线开路");
    			tts("定位天线开路");
    			}
    		gps_status.Antenna_Flag = 1;
    		jt808_param_bk.car_alarm				|= BIT_ALARM_GPS_OPEN; 	///bit5 天线开路
    		jt808_param_bk.car_alarm				&= ~( BIT_ALARM_GPS_SHORT );
    	}else if( strncmp( psrc + 24, "SHORT", 4 ) == 0 )
    	{
    		if((jt808_param_bk.car_alarm & BIT_ALARM_GPS_SHORT) == 0)
    			{
    			sd_write_console("定位天线短路");
    			tts("定位天线短路");
    			}
    		gps_status.Antenna_Flag = 1;
    		jt808_param_bk.car_alarm				|= BIT_ALARM_GPS_SHORT;
    		jt808_param_bk.car_alarm				&= ~( BIT_ALARM_GPS_OPEN );
    	}
    }
    */
}

/*初始化jt808 gps相关数据的处理*/
void jt808_gps_data_init( void )
{
    GPS_REPORT_HEAD head;
    data_load();		/*加载需要定期保存的设备数据*/
    jt808_report_init( );   /*上报区域初始化*/
    can_pack_init( );
    gps_pack_init( );		///只有需要压缩原始GPS数据才需要该函数
    area_data_init( );
    Cam_Flash_InitPara( 0 );
    jt808_misc_init( );
    ///将保存的最后一包定位位置设置为当前位置
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
void gps_dump( uint8_t mode )
{
    gps_status.Raw_Output = ~gps_status.Raw_Output;
}
FINSH_FUNCTION_EXPORT( gps_dump, dump gps raw info );




/************************************** The End Of File **************************************/

#ifdef DEBUG_GPS
/**模拟调试gps速度*/
void gps_speed_add( uint32_t sp, uint32_t count )
{
    speed_add	= sp * 1000 / 1856;
    speed_count = count;
    rt_kprintf("\n GPG speed add=%d", sp);
}

FINSH_FUNCTION_EXPORT_ALIAS( gps_speed_add, gps_speed, debug gps speed );
#endif

/************************************** The End Of File **************************************/


