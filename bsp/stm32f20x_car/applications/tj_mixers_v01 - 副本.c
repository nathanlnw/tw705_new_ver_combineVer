#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "include.h"
#include <rtthread.h>

#include "jt808.h"
#include "jt808_param.h"
#include "jt808_gps.h"

#define MIXERS_STOP_NUM		150				///��λΪ�룬��ʾ���û�д�������������Ϊ���賵ֹͣ����
#define MIXERS_CHECK_NUM	1				///��λΪ�룬��ʾ���������㴥�����źŶ��ֱ���жϳ�������ת


uint32_t mixers_sensor_tick_A 	= 0;		///���賵�Ĵ�����A���յ������źŵ�ʱ��
uint32_t mixers_sensor_tick_B 	= 0;		///���賵�Ĵ�����B���յ������źŵ�ʱ��
uint32_t mixers_sensor_tick_AB 	= 0;		///���賵�Ĵ�����A��������B��ʱ����
uint32_t mixers_sensor_tick_BA 	= 0;		///���賵�Ĵ�����B��������A��ʱ����
uint32_t mixers_sensor_tick 	= 0;		///���賵�����һ�����������յ������źŵ�ʱ��
uint32_t mixers_state 			= 0;		///���賵��״̬��0��ʾͣת��1��ʾ������2��ʾ��ת




/*********************************************************************************
  *��������:void mixers_set( uint8_t state )
  *��������:����״̬����
  *��	��:state	:0��ʾͣת��1��ʾ������2��ʾ��ת
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-06-09
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
  *��������:void EXTI0_IRQHandler(void)
  *��������:�ⲿ�ж�0������,������A����
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-06-09
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void EXTI0_IRQHandler( void )
{
    if( EXTI_GetITStatus( EXTI_Line0 ) != RESET )
    {
        EXTI_ClearITPendingBit( EXTI_Line0 );
        rt_kprintf("\n%d>������A����", rt_tick_get());
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
            ///�����B��A��ʱ��С��1�������������жϳ�������ת
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
  *��������:void EXTI6_IRQ_proc(void)
  *��������:�ⲿ�ж�6������,������B����
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-06-09
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void EXTI6_IRQ_proc( void )
{
    if( EXTI_GetITStatus( EXTI_Line6 ) != RESET )
    {
        EXTI_ClearITPendingBit( EXTI_Line6 );
        rt_kprintf("\n%d>������B����", rt_tick_get());
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
            ///�����A��B��ʱ��С��1�������������жϳ����Ƿ���
            if(mixers_sensor_tick_B - mixers_sensor_tick_A < MIXERS_CHECK_NUM * RT_TICK_PER_SECOND )
            {
                mixers_set(2);
            }
            mixers_sensor_tick_AB = mixers_sensor_tick_B - mixers_sensor_tick_A;
        }
    }
}


/*********************************************************************************
  *��������:void EXTI0_IRQHandler(void)
  *��������:�ⲿ�ж�0������,������A����
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-06-09
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void EXTI7_IRQ_proc( void )
{
    if( EXTI_GetITStatus( EXTI_Line7 ) != RESET )
    {
        EXTI_ClearITPendingBit( EXTI_Line7 );
        rt_kprintf("\n%d>������A����", rt_tick_get());
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
            ///�����B��A��ʱ��С��1�������������жϳ�������ת
            if(mixers_sensor_tick_A - mixers_sensor_tick_B < MIXERS_CHECK_NUM * RT_TICK_PER_SECOND )
            {
                mixers_set(1);
            }
            mixers_sensor_tick_BA = mixers_sensor_tick_A - mixers_sensor_tick_B;
        }
    }
}


/*********************************************************************************
  *��������:void mixers_proc(void)
  *��������:��������������ú�����Ҫ���ڶ�ʱ���ж�ʱ������ʱ���Ϊ1��
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-06-09
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void mixers_proc( void )
{
    static uint8_t last_mixers_state = 0;
    if(last_mixers_state != mixers_state)
    {

        rt_kprintf("\n%d>���賵״̬=%d", rt_tick_get(), mixers_state);
        if(mixers_state == 1)
        {
            tts("����Ͱ��ת");
            beep(6, 1, 1);
        }
        else if(mixers_state == 2)
        {
            tts("����Ͱ��ת");
            beep(3, 1, 2);
        }
        else
        {
            tts("����Ͱͣת");
            beep(3, 1, 3);
        }
        last_mixers_state = mixers_state;
    }
    if(rt_tick_get() <= mixers_sensor_tick)
        return;
    ///60��û�д����κ�һ����������ʾͣת
    if(rt_tick_get() - mixers_sensor_tick > MIXERS_STOP_NUM * RT_TICK_PER_SECOND )
    {
        ////ͣת
        mixers_set(0);
        mixers_sensor_tick_BA = 0;
        mixers_sensor_tick_AB = 0;
        return;
    }
    if((mixers_sensor_tick_BA == 0) || (mixers_sensor_tick_AB == 0))
        return;
    if(mixers_sensor_tick_BA > mixers_sensor_tick_AB * 8)		///B��A��ʱ���A��B��ʱ���8�����ʾ����
    {
        mixers_set(2);
    }
    else if(mixers_sensor_tick_AB > mixers_sensor_tick_BA * 8)	///A��B��ʱ���B��A��ʱ���8�����ʾ����
    {
        mixers_set(1);
    }
}


/*********************************************************************************
  *��������:void tj_mixers_init(void)
  *��������:��ʼ�����賵��������������жϿ�
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
  *��������:void tj_mixers_init(void)
  *��������:��ʼ�����賵��������������жϿ�
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



