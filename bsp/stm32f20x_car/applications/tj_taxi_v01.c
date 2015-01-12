
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "include.h"
#include <rtthread.h>

#include "jt808.h"
#include "jt808_param.h"
#include "jt808_gps.h"



/*********************************************************************************
  *函数名称:void taxi_measurement_start(void)
  *功能描述:出租车开始计费
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-05-29
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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

        rt_kprintf( "\n%d>计程开始", rt_tick_get( ));
        tts("计程开始");
    }
}


/*********************************************************************************
  *函数名称:void taxi_measurement_end(void)
  *功能描述:出租车开始计费
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-05-29
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void taxi_measurement_end(void)
{
    char	buf[100];
    if((jt808_param_bk.mileage_pulse_utc_end == 0) && (jt808_param_bk.mileage_pulse_utc_start))
    {
        beep(5, 5, 1);
        jt808_param_bk.mileage_pulse_utc_end	= utc_now;
        jt808_param_bk.mileage_pulse_end		= jt808_param_bk.mileage_pulse;
        rt_kprintf( "\n%d>计价结束", rt_tick_get( ));

        sprintf( buf, "请您带好随身物品准备下车!本次乘车行驶里程:%d.%02d公里,等待时长:%d分钟",
                 (jt808_param_bk.mileage_pulse_end - jt808_param_bk.mileage_pulse_start) / 1000,
                 (jt808_param_bk.mileage_pulse_end - jt808_param_bk.mileage_pulse_start) % 1000 / 10,
                 jt808_param_bk.low_speed_wait_time / 60
               );
        tts( buf );
    }
}


