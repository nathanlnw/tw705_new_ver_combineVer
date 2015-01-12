
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "include.h"
#include <rtthread.h>

#include "jt808.h"
#include "jt808_param.h"
#include "jt808_gps.h"



/*********************************************************************************
  *��������:void taxi_measurement_start(void)
  *��������:���⳵��ʼ�Ʒ�
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-05-29
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void taxi_measurement_start(void)
{
    if((jt808_param_bk.mileage_pulse_utc_end) || (jt808_param_bk.mileage_pulse_utc_start == 0))
    {
        beep(5, 5, 1);
        jt808_param_bk.mileage_pulse_utc_start 	= utc_now;
        jt808_param_bk.mileage_pulse_start 		= jt808_param_bk.mileage_pulse;
        jt808_param_bk.mileage_pulse_end		= 0;
        jt808_param_bk.mileage_pulse_utc_end	= 0;
        jt808_param_bk.low_speed_wait_time		= 0;

        rt_kprintf( "\n%d>�Ƴ̿�ʼ", rt_tick_get( ));
        tts("�Ƴ̿�ʼ");
    }
}


/*********************************************************************************
  *��������:void taxi_measurement_end(void)
  *��������:���⳵��ʼ�Ʒ�
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-05-29
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void taxi_measurement_end(void)
{
    char	buf[100];
    if((jt808_param_bk.mileage_pulse_utc_end == 0) && (jt808_param_bk.mileage_pulse_utc_start))
    {
        beep(5, 5, 1);
        jt808_param_bk.mileage_pulse_utc_end	= utc_now;
        jt808_param_bk.mileage_pulse_end		= jt808_param_bk.mileage_pulse;
        rt_kprintf( "\n%d>�Ƽ۽���", rt_tick_get( ));

        sprintf( buf, "��������������Ʒ׼���³�!���γ˳���ʻ���:%d.%02d����,�ȴ�ʱ��:%d����",
                 (jt808_param_bk.mileage_pulse_end - jt808_param_bk.mileage_pulse_start) / 1000,
                 (jt808_param_bk.mileage_pulse_end - jt808_param_bk.mileage_pulse_start) % 1000 / 10,
                 jt808_param_bk.low_speed_wait_time / 60
               );
        tts( buf );
    }
}


