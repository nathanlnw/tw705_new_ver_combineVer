/*
   vdr Vichle Driver Record 车辆行驶记录
 */

#include <rtthread.h>
#include <finsh.h>
#include "include.h"

#include <rtdevice.h>
#include <dfs_posix.h>

#include <time.h>

#include "sst25.h"
#include "hmi.h"
#include "jt808.h"
#include "jt808_SpeedFatigue.h"
#include "jt808_gps.h"
#include "jt808_param.h"
#include "jt808_util.h"
#include "jt808_config.h"

#define VDR_DEBUG

#define SPEED_LIMIT				10   /*速度门限 大于此值认为启动，小于此值认为停止*/
#define SPEED_LIMIT_DURATION	1  /*速度门限持续时间*/



/*
   每1000毫秒检测并更新状态,只能在1000ms定时器中调用
 */
void SpeedFatigue_1000ms( void )
{
    ///计算当天累计超时
    ///比较当前时间和最后一次的时间是否在同一天
    if(utc_now / 86400 != jt808_param_bk.utc_car_today / 86400)
    {
        jt808_param_bk.utc_car_today = utc_now;
        jt808_param_bk.car_run_time  = 0;
    }
    if(jt808_param_bk.car_stop_run == RUN)
    {
        jt808_param_bk.car_run_time++;
    }
    if(jt808_param_bk.car_run_time >= jt808_param.id_0x0058)
    {
        jt808_param_bk.car_alarm |= BIT_ALARM_TODAY_OVERTIME;
    }
    else
    {
        jt808_param_bk.car_alarm &= ~(BIT_ALARM_TODAY_OVERTIME);
    }
}





void over_time_init_run(void)
{
    if((jt808_param_bk.utc_car_run != 0)\
            && (jt808_param_bk.utc_car_run < jt808_param_bk.utc_over_time_end)\
            && (jt808_param_bk.utc_over_time_end < utc_now) \
            && (jt808_param_bk.utc_over_time_end - jt808_param_bk.utc_car_run >= jt808_param.id_0x0057))
    {
        ;//vdr_11_put( );
    }
    jt808_param_bk.utc_car_run 		= utc_now;
    jt808_param_bk.vdr11_lati_start	= gps_lati;
    jt808_param_bk.vdr11_longi_start	= gps_longi;
    jt808_param_bk.vdr11_alti_start	= gps_alti;
}



//复位和超时驾驶，休息超时
void over_time_reset(void)
{
    memset((void *)&jt808_param_bk.utc_car_stop, 0, 8);
    memset((void *)&jt808_param_bk.utc_over_time_start, 0, 28);
}

/*
   每秒判断车辆状态,gps中的RMC语句触发
   行驶里程
   默认都是使用GPS速度，如果gps未定位，也有秒语句输出
 */

/*gps有效情况下检查车辆行驶状态*/
void process_overtime( void )
{
    static uint32_t utc_run		= 0;
    static uint32_t utc_stop	= 0;

    /*判断车辆行驶状态*/
    if( jt808_param_bk.car_stop_run == STOP )     /*认为车辆停止,判断启动*/
    {
        if( jt808_param_bk.utc_car_stop == 0 )     /*停车时刻尚未初始化，默认停驶*/
        {
            jt808_param_bk.utc_car_stop = utc_now;
        }
        if( current_speed >= SPEED_LIMIT )                       /*速度大于门限值*/
        {
            if( utc_run == 0 )                               /*还没有记录行驶的时刻*/
            {
                utc_run = utc_now;                               /*记录开始时刻*/
            }
            if( ( utc_now - utc_run ) >= SPEED_LIMIT_DURATION )     /*超过了持续时间*/
            {
                utc_stop = utc_now;
                jt808_param_bk.car_stop_run = RUN;                    /*认为车辆行驶*/
                rt_kprintf( "\n%d>车辆行驶", rt_tick_get( ) );
                jt808_param_bk.car_alarm &= ~BIT_ALARM_STOP_OVERTIME;
                if( jt808_param_bk.utc_car_run == 0 )        /*停车时刻尚未初始化，默认停驶*/
                {
                    over_time_init_run();
                    jt808_param_bk.utc_car_run 		= utc_run;
                }
            }
        }
        else	///车辆停止，休息中。
        {
            jt808_param_bk.car_alarm &= ~(BIT_ALARM_PRE_OVERTIME);
            //jt808_param_bk.car_alarm &= ~(BIT_ALARM_OVERTIME);
            if( utc_now - jt808_param_bk.utc_car_stop > jt808_param.id_0x005A )   /*判断停车最长时间*/
            {
                over_time_init_run();
                if( ( jt808_param.id_0x0050 & BIT_ALARM_STOP_OVERTIME ) == 0 )
                    jt808_param_bk.car_alarm |= BIT_ALARM_STOP_OVERTIME;
                jt808_param_bk.car_alarm &= ~(BIT_ALARM_OVERTIME);
                //rt_kprintf( "\n%d>达到停车最长时间", rt_tick_get( ));
            }
            else if( utc_now - jt808_param_bk.utc_car_stop > jt808_param.id_0x0059)  /*判断停车最短时间，达到该时间表示休息了一次*/
            {
                over_time_init_run();
                jt808_param_bk.car_alarm &= ~(BIT_ALARM_OVERTIME);
                //rt_kprintf( "\n%d>达到停车最少时间", rt_tick_get( ));
            }

            utc_run = 0;
        }
    }
    else          /*车辆状态为已启动*/
    {
        if( jt808_param_bk.utc_car_run == 0 )        /*停车时刻尚未初始化，默认停驶*/
        {
            over_time_init_run();
        }
        if( current_speed <= SPEED_LIMIT )                /*速度小于门限值*/
        {
            if( utc_stop == 0 )
            {
                utc_stop = utc_now;
            }
            if( ( utc_now - utc_stop ) >= SPEED_LIMIT_DURATION ) /*超过了持续时间,等待判断停驶*/
            {
                utc_run = utc_now;
                //jt808_param_bk.utc_car_stop = utc_now;
                jt808_param_bk.car_stop_run = STOP;     /*认为车辆停驶*/
                rt_kprintf( "\n%d>车辆停驶", rt_tick_get( ) );
                //vdr_10_put( mytime_now );     /*生成VDR_10数据,事故疑点*/
                if( jt808_param_bk.utc_car_stop == 0 )     /*停车时刻尚未初始化，默认停驶*/
                {
                    jt808_param_bk.utc_car_stop = utc_stop;
                }
            }
        }
        else  ///车辆行驶中。
        {
            if( utc_now - jt808_param_bk.utc_car_run > jt808_param.id_0x0057 )  /*判断连续驾驶最长时间*/
            {
                //rt_kprintf( "\n%d>疲劳驾驶", rt_tick_get( ) );
                jt808_param_bk.car_alarm &= ~(BIT_ALARM_PRE_OVERTIME);
                if( ( jt808_param.id_0x0050 & BIT_ALARM_OVERTIME ) == 0 )
                    jt808_param_bk.car_alarm |= BIT_ALARM_OVERTIME;
                jt808_param_bk.utc_over_time_start	= jt808_param_bk.utc_car_run;
                jt808_param_bk.utc_over_time_end	= utc_now;
                jt808_param_bk.vdr11_lati_end		= gps_lati;
                jt808_param_bk.vdr11_longi_end		= gps_longi;
                jt808_param_bk.vdr11_alti_end		= gps_alti;
            }
            else if( utc_now - jt808_param_bk.utc_car_run > ( jt808_param.id_0x0057 - jt808_param.id_0x005C ) ) /*疲劳驾驶预警*/
            {
                //rt_kprintf( "\n%d>超时预警", rt_tick_get( ) );
                if( ( jt808_param.id_0x0050 & BIT_ALARM_PRE_OVERTIME ) == 0 )
                    jt808_param_bk.car_alarm |= BIT_ALARM_PRE_OVERTIME;
            }

            jt808_param_bk.car_alarm &= ~(BIT_ALARM_STOP_OVERTIME);
            jt808_param_bk.utc_car_stop = utc_now;
            utc_stop = 0;
        }
    }
}

/*超速、超速预警判断*/
void process_overspeed( void )
{
    static uint8_t	overspeed_flag = 0;
    static uint32_t utc_overspeed_start = 0;
    static MYTIME	mytime_overspeed_start;
    static uint32_t overspeed_sum	= 0;
    static uint32_t overspeed_count = 0;
    static uint16_t overspeed_min	= 0xFF;
    static uint16_t overspeed_max	= 0;

    uint16_t		gps_speed_10x	= current_speed * 10;
    uint16_t		limit_speed_10x = jt808_param.id_0x0055 * 10;

    if( current_speed >= jt808_param.id_0x0055 )                                /*超过最高速度*/
    {
        if( ( jt808_param.id_0x0050 & BIT_ALARM_OVERSPEED ) == 0 )  /*报警屏蔽字*/
            beep( 10, 1, 1 );
        if( utc_overspeed_start )                                           /*判断超速持续事件*/
        {
            if( utc_now - utc_overspeed_start >= jt808_param.id_0x0056 )    /*已经持续超速*/
            {
                if( ( jt808_param.id_0x0050 & BIT_ALARM_OVERSPEED ) == 0 )  /*报警屏蔽字*/
                {
                    jt808_param_bk.car_alarm |= BIT_ALARM_OVERSPEED;
                    //rt_kprintf( "\n%d>超速报警", rt_tick_get( ));
                }
                if(overspeed_count == 0)
                {
                    jt808_param_bk.utc_over_speed_start	= utc_overspeed_start;
                }
                jt808_param_bk.utc_over_speed_end 		= utc_now;
                overspeed_flag	= 2;                                        /*已经超速了*/
                overspeed_sum	+= current_speed;
                overspeed_count++;
                if( current_speed > overspeed_max )
                {
                    overspeed_max = current_speed;
                    jt808_param_bk.over_speed_max = current_speed;
                }
                if( current_speed < overspeed_min )
                {
                    overspeed_min = current_speed;
                }
            }
        }
        else                                                                /*没有超速或超速预警，记录开始超速，*/
        {
            save_0x0200_data = 1;
            utc_overspeed_start		= utc_now;                              /*记录超速开始的时刻*/
            mytime_overspeed_start	= mytime_now;
        }
    }
    else
    {
        if( gps_speed_10x >= ( limit_speed_10x - jt808_param.id_0x005B ) )  /*超速预警*/
        {
            if( ( jt808_param.id_0x0050 & BIT_ALARM_PRE_OVERSPEED ) == 0 )  /*报警屏蔽字*/
            {
                jt808_param_bk.car_alarm |= ( BIT_ALARM_PRE_OVERSPEED );
                jt808_param_bk.car_alarm &= ~( BIT_ALARM_OVERSPEED );
                //beep( 10, 1, 1 );
            }
            overspeed_flag = 1;
        }
        else                                                                /*没有超速，也没有预警,清除标志位*/
        {
            if( overspeed_flag > 1 )                                        /*已经超速了,现在速度正常，要记录当前的超速记录*/
            {
                ;//vdr_16_put( utc_to_mytime(jt808_param_bk.utc_over_speed_start), utc_to_mytime(jt808_param_bk.utc_over_speed_end), overspeed_min, overspeed_max, overspeed_sum / overspeed_count );
            }
            overspeed_flag	= 0;
            jt808_param_bk.car_alarm		&= ~( BIT_ALARM_OVERSPEED | BIT_ALARM_PRE_OVERSPEED );
        }
        utc_overspeed_start = 0;                                            /*准备记录超速的时刻*/
        overspeed_max		= 0;
        overspeed_min		= 0xFF;
        overspeed_count		= 0;
        overspeed_sum		= 0;
        if(overspeed_count)
        {
            jt808_param_bk.utc_over_speed_end = utc_now;
        }
    }
}

/*
 处理808协议中的超速和疲劳驾驶相关
 */
void SpeedingFatigueProc( void )
{
    process_overspeed( );
    process_overtime( );
}

/************************************** The End Of File **************************************/
