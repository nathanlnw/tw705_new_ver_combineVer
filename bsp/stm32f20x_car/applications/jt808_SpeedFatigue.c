/*
   vdr Vichle Driver Record ������ʻ��¼
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

#define SPEED_LIMIT				10   /*�ٶ����� ���ڴ�ֵ��Ϊ������С�ڴ�ֵ��Ϊֹͣ*/
#define SPEED_LIMIT_DURATION	1  /*�ٶ����޳���ʱ��*/



/*
   ÿ1000�����Ⲣ����״̬,ֻ����1000ms��ʱ���е���
 */
void SpeedFatigue_1000ms( void )
{
    ///���㵱���ۼƳ�ʱ
    ///�Ƚϵ�ǰʱ������һ�ε�ʱ���Ƿ���ͬһ��
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



//��λ�ͳ�ʱ��ʻ����Ϣ��ʱ
void over_time_reset(void)
{
    memset((void *)&jt808_param_bk.utc_car_stop, 0, 8);
    memset((void *)&jt808_param_bk.utc_over_time_start, 0, 28);
}

/*
   ÿ���жϳ���״̬,gps�е�RMC��䴥��
   ��ʻ���
   Ĭ�϶���ʹ��GPS�ٶȣ����gpsδ��λ��Ҳ����������
 */

/*gps��Ч����¼�鳵����ʻ״̬*/
void process_overtime( void )
{
    static uint32_t utc_run		= 0;
    static uint32_t utc_stop	= 0;

    /*�жϳ�����ʻ״̬*/
    if( jt808_param_bk.car_stop_run == STOP )     /*��Ϊ����ֹͣ,�ж�����*/
    {
        if( jt808_param_bk.utc_car_stop == 0 )     /*ͣ��ʱ����δ��ʼ����Ĭ��ͣʻ*/
        {
            jt808_param_bk.utc_car_stop = utc_now;
        }
        if( current_speed >= SPEED_LIMIT )                       /*�ٶȴ�������ֵ*/
        {
            if( utc_run == 0 )                               /*��û�м�¼��ʻ��ʱ��*/
            {
                utc_run = utc_now;                               /*��¼��ʼʱ��*/
            }
            if( ( utc_now - utc_run ) >= SPEED_LIMIT_DURATION )     /*�����˳���ʱ��*/
            {
                utc_stop = utc_now;
                jt808_param_bk.car_stop_run = RUN;                    /*��Ϊ������ʻ*/
                rt_kprintf( "\n%d>������ʻ", rt_tick_get( ) );
                jt808_param_bk.car_alarm &= ~BIT_ALARM_STOP_OVERTIME;
                if( jt808_param_bk.utc_car_run == 0 )        /*ͣ��ʱ����δ��ʼ����Ĭ��ͣʻ*/
                {
                    over_time_init_run();
                    jt808_param_bk.utc_car_run 		= utc_run;
                }
            }
        }
        else	///����ֹͣ����Ϣ�С�
        {
            jt808_param_bk.car_alarm &= ~(BIT_ALARM_PRE_OVERTIME);
            //jt808_param_bk.car_alarm &= ~(BIT_ALARM_OVERTIME);
            if( utc_now - jt808_param_bk.utc_car_stop > jt808_param.id_0x005A )   /*�ж�ͣ���ʱ��*/
            {
                over_time_init_run();
                if( ( jt808_param.id_0x0050 & BIT_ALARM_STOP_OVERTIME ) == 0 )
                    jt808_param_bk.car_alarm |= BIT_ALARM_STOP_OVERTIME;
                jt808_param_bk.car_alarm &= ~(BIT_ALARM_OVERTIME);
                //rt_kprintf( "\n%d>�ﵽͣ���ʱ��", rt_tick_get( ));
            }
            else if( utc_now - jt808_param_bk.utc_car_stop > jt808_param.id_0x0059)  /*�ж�ͣ�����ʱ�䣬�ﵽ��ʱ���ʾ��Ϣ��һ��*/
            {
                over_time_init_run();
                jt808_param_bk.car_alarm &= ~(BIT_ALARM_OVERTIME);
                //rt_kprintf( "\n%d>�ﵽͣ������ʱ��", rt_tick_get( ));
            }

            utc_run = 0;
        }
    }
    else          /*����״̬Ϊ������*/
    {
        if( jt808_param_bk.utc_car_run == 0 )        /*ͣ��ʱ����δ��ʼ����Ĭ��ͣʻ*/
        {
            over_time_init_run();
        }
        if( current_speed <= SPEED_LIMIT )                /*�ٶ�С������ֵ*/
        {
            if( utc_stop == 0 )
            {
                utc_stop = utc_now;
            }
            if( ( utc_now - utc_stop ) >= SPEED_LIMIT_DURATION ) /*�����˳���ʱ��,�ȴ��ж�ͣʻ*/
            {
                utc_run = utc_now;
                //jt808_param_bk.utc_car_stop = utc_now;
                jt808_param_bk.car_stop_run = STOP;     /*��Ϊ����ͣʻ*/
                rt_kprintf( "\n%d>����ͣʻ", rt_tick_get( ) );
                //vdr_10_put( mytime_now );     /*����VDR_10����,�¹��ɵ�*/
                if( jt808_param_bk.utc_car_stop == 0 )     /*ͣ��ʱ����δ��ʼ����Ĭ��ͣʻ*/
                {
                    jt808_param_bk.utc_car_stop = utc_stop;
                }
            }
        }
        else  ///������ʻ�С�
        {
            if( utc_now - jt808_param_bk.utc_car_run > jt808_param.id_0x0057 )  /*�ж�������ʻ�ʱ��*/
            {
                //rt_kprintf( "\n%d>ƣ�ͼ�ʻ", rt_tick_get( ) );
                jt808_param_bk.car_alarm &= ~(BIT_ALARM_PRE_OVERTIME);
                if( ( jt808_param.id_0x0050 & BIT_ALARM_OVERTIME ) == 0 )
                    jt808_param_bk.car_alarm |= BIT_ALARM_OVERTIME;
                jt808_param_bk.utc_over_time_start	= jt808_param_bk.utc_car_run;
                jt808_param_bk.utc_over_time_end	= utc_now;
                jt808_param_bk.vdr11_lati_end		= gps_lati;
                jt808_param_bk.vdr11_longi_end		= gps_longi;
                jt808_param_bk.vdr11_alti_end		= gps_alti;
            }
            else if( utc_now - jt808_param_bk.utc_car_run > ( jt808_param.id_0x0057 - jt808_param.id_0x005C ) ) /*ƣ�ͼ�ʻԤ��*/
            {
                //rt_kprintf( "\n%d>��ʱԤ��", rt_tick_get( ) );
                if( ( jt808_param.id_0x0050 & BIT_ALARM_PRE_OVERTIME ) == 0 )
                    jt808_param_bk.car_alarm |= BIT_ALARM_PRE_OVERTIME;
            }

            jt808_param_bk.car_alarm &= ~(BIT_ALARM_STOP_OVERTIME);
            jt808_param_bk.utc_car_stop = utc_now;
            utc_stop = 0;
        }
    }
}

/*���١�����Ԥ���ж�*/
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

    if( current_speed >= jt808_param.id_0x0055 )                                /*��������ٶ�*/
    {
        if( ( jt808_param.id_0x0050 & BIT_ALARM_OVERSPEED ) == 0 )  /*����������*/
            beep( 10, 1, 1 );
        if( utc_overspeed_start )                                           /*�жϳ��ٳ����¼�*/
        {
            if( utc_now - utc_overspeed_start >= jt808_param.id_0x0056 )    /*�Ѿ���������*/
            {
                if( ( jt808_param.id_0x0050 & BIT_ALARM_OVERSPEED ) == 0 )  /*����������*/
                {
                    jt808_param_bk.car_alarm |= BIT_ALARM_OVERSPEED;
                    //rt_kprintf( "\n%d>���ٱ���", rt_tick_get( ));
                }
                if(overspeed_count == 0)
                {
                    jt808_param_bk.utc_over_speed_start	= utc_overspeed_start;
                }
                jt808_param_bk.utc_over_speed_end 		= utc_now;
                overspeed_flag	= 2;                                        /*�Ѿ�������*/
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
        else                                                                /*û�г��ٻ���Ԥ������¼��ʼ���٣�*/
        {
            save_0x0200_data = 1;
            utc_overspeed_start		= utc_now;                              /*��¼���ٿ�ʼ��ʱ��*/
            mytime_overspeed_start	= mytime_now;
        }
    }
    else
    {
        if( gps_speed_10x >= ( limit_speed_10x - jt808_param.id_0x005B ) )  /*����Ԥ��*/
        {
            if( ( jt808_param.id_0x0050 & BIT_ALARM_PRE_OVERSPEED ) == 0 )  /*����������*/
            {
                jt808_param_bk.car_alarm |= ( BIT_ALARM_PRE_OVERSPEED );
                jt808_param_bk.car_alarm &= ~( BIT_ALARM_OVERSPEED );
                //beep( 10, 1, 1 );
            }
            overspeed_flag = 1;
        }
        else                                                                /*û�г��٣�Ҳû��Ԥ��,�����־λ*/
        {
            if( overspeed_flag > 1 )                                        /*�Ѿ�������,�����ٶ�������Ҫ��¼��ǰ�ĳ��ټ�¼*/
            {
                ;//vdr_16_put( utc_to_mytime(jt808_param_bk.utc_over_speed_start), utc_to_mytime(jt808_param_bk.utc_over_speed_end), overspeed_min, overspeed_max, overspeed_sum / overspeed_count );
            }
            overspeed_flag	= 0;
            jt808_param_bk.car_alarm		&= ~( BIT_ALARM_OVERSPEED | BIT_ALARM_PRE_OVERSPEED );
        }
        utc_overspeed_start = 0;                                            /*׼����¼���ٵ�ʱ��*/
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
 ����808Э���еĳ��ٺ�ƣ�ͼ�ʻ���
 */
void SpeedingFatigueProc( void )
{
    process_overspeed( );
    process_overtime( );
}

/************************************** The End Of File **************************************/
