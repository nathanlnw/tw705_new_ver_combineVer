#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "include.h"
#include <rtthread.h>

#include "jt808.h"
#include "jt808_param.h"
#include "jt808_gps.h"

#define MIXERS_STOP_NUM		150				///单位为秒，表示多久没有触发传感器就认为搅拌车停止搅拌
#define MIXERS_CHECK_NUM	1				///单位为秒，表示当两个触点触发到信号多断直接判断出正传或反转


uint32_t mixers_sensor_tick_A 	= 0;		///搅拌车的传感器A接收到霍尔信号的时刻
uint32_t mixers_sensor_tick_B 	= 0;		///搅拌车的传感器B接收到霍尔信号的时刻
uint32_t mixers_sensor_tick_AB 	= 0;		///搅拌车的传感器A到传感器B的时间间隔
uint32_t mixers_sensor_tick_BA 	= 0;		///搅拌车的传感器B到传感器A的时间间隔
uint32_t mixers_sensor_tick 	= 0;		///搅拌车的最后一个传感器接收到霍尔信号的时刻
uint32_t mixers_state 			= 0;		///搅拌车的状态，0表示停转，1表示正传，2表示反转




/*********************************************************************************
  *函数名称:void mixers_set( uint8_t state )
  *功能描述:搅拌状态设置
  *输	入:state	:0表示停转，1表示正传，2表示反转
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-06-09
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void mixers_set( uint8_t state )
{
    if(state == mixers_state)
    {
        return;
    }
    mixers_state = state;
    jt808_status &= ~(BIT(24));
    jt808_status &= ~(BIT(25));

    jt808_status |= (mixers_state << 24);
}

#if 0
/*********************************************************************************
  *函数名称:void EXTI0_IRQHandler(void)
  *功能描述:外部中断0处理函数,传感器A触发
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-06-09
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void EXTI0_IRQHandler( void )
{
    if( EXTI_GetITStatus( EXTI_Line0 ) != RESET )
    {
        EXTI_ClearITPendingBit( EXTI_Line0 );
        rt_kprintf("\n%d>传感器A触发", rt_tick_get());
        if(rt_tick_get() <= mixers_sensor_tick)
        {
            mixers_sensor_tick++;
        }
        else
        {
            mixers_sensor_tick = rt_tick_get();
        }
        mixers_sensor_tick_A = mixers_sensor_tick;
        if(mixers_sensor_tick_B)
        {
            ///如果从B到A的时间小于1秒则立即可以判断出来是正转
            if(mixers_sensor_tick_A - mixers_sensor_tick_B < MIXERS_CHECK_NUM * RT_TICK_PER_SECOND )
            {
                mixers_set(1);
            }
            mixers_sensor_tick_BA = mixers_sensor_tick_A - mixers_sensor_tick_B;
        }
    }
}
#endif


/*********************************************************************************
  *函数名称:void EXTI6_IRQ_proc(void)
  *功能描述:外部中断6处理函数,传感器B触发
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-06-09
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void EXTI6_IRQ_proc( void )
{
    if( EXTI_GetITStatus( EXTI_Line6 ) != RESET )
    {
        EXTI_ClearITPendingBit( EXTI_Line6 );
        rt_kprintf("\n%d>传感器B触发", rt_tick_get());
        if(rt_tick_get() <= mixers_sensor_tick)
        {
            mixers_sensor_tick++;
        }
        else
        {
            mixers_sensor_tick = rt_tick_get();
        }
        mixers_sensor_tick_B = mixers_sensor_tick;
        if(mixers_sensor_tick_A)
        {
            ///如果从A到B的时间小于1秒则立即可以判断出来是反传
            if(mixers_sensor_tick_B - mixers_sensor_tick_A < MIXERS_CHECK_NUM * RT_TICK_PER_SECOND )
            {
                mixers_set(2);
            }
            mixers_sensor_tick_AB = mixers_sensor_tick_B - mixers_sensor_tick_A;
        }
    }
}


/*********************************************************************************
  *函数名称:void EXTI0_IRQHandler(void)
  *功能描述:外部中断0处理函数,传感器A触发
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-06-09
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void EXTI7_IRQ_proc( void )
{
    if( EXTI_GetITStatus( EXTI_Line7 ) != RESET )
    {
        EXTI_ClearITPendingBit( EXTI_Line7 );
        rt_kprintf("\n%d>传感器A触发", rt_tick_get());
        if(rt_tick_get() <= mixers_sensor_tick)
        {
            mixers_sensor_tick++;
        }
        else
        {
            mixers_sensor_tick = rt_tick_get();
        }
        mixers_sensor_tick_A = mixers_sensor_tick;
        if(mixers_sensor_tick_B)
        {
            ///如果从B到A的时间小于1秒则立即可以判断出来是正转
            if(mixers_sensor_tick_A - mixers_sensor_tick_B < MIXERS_CHECK_NUM * RT_TICK_PER_SECOND )
            {
                mixers_set(1);
            }
            mixers_sensor_tick_BA = mixers_sensor_tick_A - mixers_sensor_tick_B;
        }
    }
}


/*********************************************************************************
  *函数名称:void mixers_proc(void)
  *功能描述:搅拌出处理函数，该函数需要放在定时器中定时处理，定时间隔为1秒
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-06-09
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void mixers_proc( void )
{
    static uint8_t last_mixers_state = 0;
    if(last_mixers_state != mixers_state)
    {

        rt_kprintf("\n%d>搅拌车状态=%d", rt_tick_get(), mixers_state);
        if(mixers_state == 1)
        {
            tts("搅拌桶正转");
            beep(6, 1, 1);
        }
        else if(mixers_state == 2)
        {
            tts("搅拌桶反转");
            beep(3, 1, 2);
        }
        else
        {
            tts("搅拌桶停转");
            beep(3, 1, 3);
        }
        last_mixers_state = mixers_state;
    }
    if(rt_tick_get() <= mixers_sensor_tick)
        return;
    ///60秒没有触发任何一个传感器表示停转
    if(rt_tick_get() - mixers_sensor_tick > MIXERS_STOP_NUM * RT_TICK_PER_SECOND )
    {
        ////停转
        mixers_set(0);
        mixers_sensor_tick_BA = 0;
        mixers_sensor_tick_AB = 0;
        return;
    }
    if((mixers_sensor_tick_BA == 0) || (mixers_sensor_tick_AB == 0))
        return;
    if(mixers_sensor_tick_BA > mixers_sensor_tick_AB * 8)		///B到A的时间比A到B的时间多8倍则表示反传
    {
        mixers_set(2);
    }
    else if(mixers_sensor_tick_AB > mixers_sensor_tick_BA * 8)	///A到B的时间比B到A的时间多8倍则表示正传
    {
        mixers_set(1);
    }
}


/*********************************************************************************
  *函数名称:void tj_mixers_init(void)
  *功能描述:初始化搅拌车霍尔传感器相关中断口
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
void tj_mixers_init(void)
{
    GPIO_InitTypeDef	GPIO_InitStructure;
    NVIC_InitTypeDef	NVIC_InitStructure;
    EXTI_InitTypeDef	EXTI_InitStructure;

    mixers_sensor_tick	= 0;
    mixers_sensor_tick_A = 0;
    mixers_sensor_tick_B = 0;
    /* Enable SYSCFG clock */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_SYSCFG, ENABLE );

    ///EXIT PA6 INT
    /* Enable GPIOA clock */
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );

    /* Configure PA6 pin as input floating */
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_7;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    /* Connect EXTI Line6 to PA6 pin */
    SYSCFG_EXTILineConfig( EXTI_PortSourceGPIOA, EXTI_PinSource6 );

    /* Connect EXTI Line6 to PA7 pin */
    SYSCFG_EXTILineConfig( EXTI_PortSourceGPIOA, EXTI_PinSource7 );


    /* Configure EXTI Line6 */
    EXTI_InitStructure.EXTI_Line	= EXTI_Line6;
    EXTI_InitStructure.EXTI_Mode	= EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init( &EXTI_InitStructure );

    /* Configure EXTI Line7 */
    EXTI_InitStructure.EXTI_Line	= EXTI_Line7;
    EXTI_Init( &EXTI_InitStructure );

    /* Enable and set EXTI Line6 Line7 Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel						= EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 2;		// old is 1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
    NVIC_Init( &NVIC_InitStructure );
}


/*********************************************************************************
  *函数名称:void tj_mixers_init(void)
  *功能描述:初始化搅拌车霍尔传感器相关中断口
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
void tj_mixers_init_old(void)
{
    GPIO_InitTypeDef	GPIO_InitStructure;
    NVIC_InitTypeDef	NVIC_InitStructure;
    EXTI_InitTypeDef	EXTI_InitStructure;

    mixers_sensor_tick	= 0;
    mixers_sensor_tick_A = 0;
    mixers_sensor_tick_B = 0;
    /* Enable SYSCFG clock */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_SYSCFG, ENABLE );

    ///EXIT PA6 INT
    /* Enable GPIOA clock */
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );

    /* Configure PA6 pin as input floating */
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    /* Connect EXTI Line6 to PA6 pin */
    SYSCFG_EXTILineConfig( EXTI_PortSourceGPIOA, EXTI_PinSource6 );

    /* Configure EXTI Line6 */
    EXTI_InitStructure.EXTI_Line	= EXTI_Line6;
    EXTI_InitStructure.EXTI_Mode	= EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init( &EXTI_InitStructure );

    /* Enable and set EXTI Line0 Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel						= EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 2;		// old is 1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    ///EXIT PC0 INT
    /* Enable GPIOC clock */
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );

    /* Configure PC0 pin as input floating */
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_Init( GPIOC, &GPIO_InitStructure );

    /* Connect EXTI Line0 to PC0 pin */
    SYSCFG_EXTILineConfig( EXTI_PortSourceGPIOC, EXTI_PinSource0 );

    /* Configure EXTI Line0 */
    EXTI_InitStructure.EXTI_Line	= EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode	= EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init( &EXTI_InitStructure );

    /* Enable and set EXTI Line0 Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel						= EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 2;		// old is 1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
    NVIC_Init( &NVIC_InitStructure );
}



