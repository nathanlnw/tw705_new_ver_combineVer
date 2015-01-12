/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/

#include "jt808_gps.h"
#include "include.h"
#include <rtthread.h>
#include <finsh.h>
#include "jt808.h"
#include "jt808_vehicle.h"
#include "jt808_param.h"
#include "jt808_camera.h"
#include "camera.h"
#include "mma8451.h"
#include "tj_taxi_v01.h"
#include "jt808_GB19056.h"
#include "jt808_SpeedFatigue.h"


//uint8_t car_stop_run = 0;  /*��������ֹͣ״̬*/

/*����һ����ʱ����������ʱ���AUX*/
struct rt_timer tmr_50ms;
static uint32_t test_io = 0;
uint32_t car_state_ex = 0;			//������չ�ź�




extern void WatchDog_Feed(void);

void set_status(uint32_t state)
{
    jt808_status = state;
}
FINSH_FUNCTION_EXPORT( set_status, set_status );


void set_alarm(uint32_t state)
{
    jt808_param_bk.car_alarm = state;
}
FINSH_FUNCTION_EXPORT( set_alarm, set_alarm );



/*��������Ĵ���*/
void onemg( uint8_t index, uint8_t value )
{
    if( value )
    {
        test_io	&= ~(BIT(index));
        jt808_param_bk.car_alarm  &= ~BIT_ALARM_EMG;
        debug_write("����������!");
    }
    else
    {
        jt808_param_bk.car_alarm |= BIT_ALARM_EMG;
        test_io	|= (BIT(index));
        debug_write("����������!");
        //Cam_takepic(0xFF,0,1,Cam_TRIGGER_ROBBERY);
    }
}

/*��������Ĵ���*/
void onemg_tw703( uint8_t index, uint8_t value )
{
    if( value )
    {
        jt808_param_bk.car_alarm |= BIT_ALARM_EMG;
        test_io	|= (BIT(index));
        rt_kprintf("\n �������� ON!");
        //Cam_takepic(0xFF,0,1,Cam_TRIGGER_ROBBERY);
    }
    else
    {
        test_io	&= ~(BIT(index));
        jt808_param_bk.car_alarm  &= ~BIT_ALARM_EMG;
        rt_kprintf("\n �������� OFF !");
    }
}

/*ACC״̬�仯*/
void onacc( uint8_t index, uint8_t value )
{
    if( value )
    {
        test_io	&= ~(BIT(index));
        jt808_status &= ~BIT_STATUS_ACC;
        debug_write("ACC�ر�!");
        data_save( );
    }
    else
    {
        test_io	|= (BIT(index));
        jt808_status |= BIT_STATUS_ACC;
        debug_write("ACC����!");
        //control_device(DEVICE_NORMAL,0);
        if(device_unlock)
            sock_proc(mast_socket, SOCK_CONNECT);
    }
}


/*���Ŵ�������*/
void ondoor( uint8_t index, uint8_t value )
{
    uint8_t					*pdestbuf;
    uint16_t				datalen;
    Style_Cam_Requset_Para	cam_para;
    CAM_TRIGGER 			Tigger;
    static uint32_t counter = 0;

    if( value )
    {
        debug_write( "���Ź�" );
#ifdef JT808_TEST_JTB
        car_state_ex &= ~ (BIT(8));
#endif
        Tigger = Cam_TRIGGER_CLOSEDOR;
        jt808_status &= ~BIT_STATUS_DOOR1;
    }
    else
    {
        debug_write( "���ſ�" );
#ifdef JT808_TEST_JTB
        car_state_ex |= (BIT(8));
#endif
        Tigger = Cam_TRIGGER_OPENDOR;
        jt808_status |= BIT_STATUS_DOOR1;
    }
    //if(( counter )&&( rt_tick_get() > RT_TICK_PER_SECOND * 2 ))
    //	Cam_takepic(0xFF,0,1,Tigger);
    counter++;
#if 0
    memset( &cam_para, 0, sizeof( cam_para ) );
    cam_para.Channel_ID = 0xFF;             /*��������ͨ��*/
    cam_para.PhotoTotal = 1;
    cam_para.PhotoSpace = 0;
    cam_para.SavePhoto	= 0;
    cam_para.SendPhoto	= 1;
    cam_para.user_para				= RT_NULL;
    cam_para.cb_response_cam_ok		= cam_ok;       ///һ����Ƭ���ճɹ��ص�����
    cam_para.cb_response_cam_end	= RT_NULL;      /*����Ҫ�������յ�Ӧ��*/
    rt_kprintf("\r\n ���շ�������ʼ!");
    take_pic_request( &cam_para );                  ///������������
    Cam_takepic(0xFF, 0, 1, Cam_TRIGGER_OPENDOR);
    rt_kprintf("\r\n ���շ����������!");
#endif
}

/*light left*/
void on_light_left( uint8_t index, uint8_t value )
{
    if( value )
    {
        car_state_ex &= ~ (BIT(3));
        test_io	&= ~(BIT(index));
        debug_write("��ת��ƹر�!");
    }
    else
    {
        car_state_ex |= (BIT(3));
        test_io	|= (BIT(index));
        debug_write("��ת��ƿ���!");
    }
}

/*light right*/
void on_light_right( uint8_t index, uint8_t value )
{
    if( value )
    {
        car_state_ex &= ~ (BIT(2));
        test_io	&= ~(BIT(index));
        debug_write("��ת��ƹر�!");
    }
    else
    {
        car_state_ex |= (BIT(2));
        test_io	|= (BIT(index));
        debug_write("��ת��ƿ���!");
    }
}

/* ɲ��*/
void on_sha_che( uint8_t index, uint8_t value )
{
    if( value )
    {
        car_state_ex &= ~ (BIT(4));
        test_io	&= ~(BIT(index));
        debug_write("ɲ���ر�!");
    }
    else
    {
        car_state_ex |= (BIT(4));
        test_io	|= (BIT(index));
        debug_write("ɲ������!");
    }
}

/* ��ˢ*/
void on_yu_shua( uint8_t index, uint8_t value )
{
    if( value )
    {
#ifdef JT808_TEST_JTB
        car_state_ex &= ~ (BIT(6));
#endif
#ifndef TJ_MIXERS_V01
        test_io	&= ~(BIT(index));
        debug_write("��ˢ�ر�!");
#endif
    }
    else
    {
#ifdef JT808_TEST_JTB
        car_state_ex |= (BIT(6));
#endif
#ifndef TJ_MIXERS_V01
        test_io	|= (BIT(index));
        debug_write("��ˢ����!");
#endif
    }
}

/* Ԥ��1,��Ϊ����*/
void on_yu_liu( uint8_t index, uint8_t value )
{
    if( value )
    {
#ifdef TJ_TAXI_V01
        taxi_measurement_start();
#endif
#ifndef TJ_MIXERS_V01
        car_state_ex &= ~ (BIT(0));
        test_io	&= ~(BIT(index));
        debug_write("Ԥ���ر�!");
#endif
    }
    else
    {
#ifdef TJ_TAXI_V01
        taxi_measurement_end();
#endif
#ifndef TJ_MIXERS_V01
        car_state_ex |= (BIT(0));
        test_io	|= (BIT(index));
        debug_write("Ԥ������!");
#endif
    }
}

/* Զ���*/
void on_light( uint8_t index, uint8_t value )
{
    if( value )
    {
        car_state_ex &= ~ (BIT(1));
        test_io	&= ~(BIT(index));
        debug_write("Զ��ƹر�!");
    }
    else
    {
        car_state_ex |= (BIT(1));
        test_io	|= (BIT(index));
        debug_write("Զ��ƿ���!");
    }
}

/* ����*/
void on_mei_su( uint8_t index, uint8_t value )
{
    if( value )
    {
        test_io	&= ~(BIT(index));
        debug_write("���ٹر�!");
    }
    else
    {
        test_io	|= (BIT(index));
        debug_write("���ٿ���!");
    }
}

/**/
void ondefault( uint8_t index, uint8_t value )
{

}

AUX_IN	PIN_IN[] =
{
    { GPIOE, GPIO_Pin_8,  1, 20,  0, 0, onemg	 		}, /*0.������ť*/
    { GPIOE, GPIO_Pin_9,  1, 60,  0, 0, onacc	 		}, /*1.ACC*/
    { GPIOC, GPIO_Pin_0,  1, 2,	 0, 0, on_light 		}, /*2.Զ��    4��*/
    { GPIOA, GPIO_Pin_1,  1, 2,  0, 0, ondoor	 		}, /*3.����    6��*/
    { GPIOE, GPIO_Pin_10, 1, 2,	 0, 0, on_light_left 	}, /*4.��ת    10��*/
    { GPIOC, GPIO_Pin_2,  1, 2,	 0, 0, on_light_right 	}, /*5.��ת    8��*/
    { GPIOE, GPIO_Pin_11, 1, 2,	 0, 0, on_sha_che 		}, /*6.ɲ��    9��*/
    { GPIOA, GPIO_Pin_7,  1, 2,	 0, 0, on_yu_shua 		}, /*7.��ˢ    7��*/
    { GPIOA, GPIO_Pin_6,  1, 2,	 0, 0, on_yu_liu		}, /*8.Ԥ��1   5��*/
};

AUX_OUT PIN_OUT[] =
{
    { GPIOB, GPIO_Pin_1, 0, 0 },                    /*�̵���*/
    //	{ GPIOB, GPIO_Pin_6, 0, 0 },                    /*������*/
};

/*��ӳ����ź�*/
__IO uint16_t	IC2Value	= 0;
__IO uint16_t	DutyCycle	= 0;
uint32_t		Frequency	= 0;

#define ADC1_DR_Address			( (uint32_t)0X4001204C )
#define    BD_IO_Pin6_7_A1C3            //  ����Ӧ���� PA1    6   ���� PC3   7  ����

uint16_t		ADC_ConValue[3];        //   3  ��ͨ��ID    0 : ��� 1: ����   2:  ����

static uint32_t ADC_ConvertedValue = 0; //��ص�ѹAD��ֵ

uint32_t		AD_Volte		= 0;
uint32_t		AD_Volte_Min	= 0xFFFFFFF, AD_Volte_Max = 0;

static uint32_t AD_2through[2];         //  ����2 ·AD ����ֵ

static uint8_t	power_lost_counter	= 0;
static uint8_t	power_low_counter	= 0;


extern void jt808_vehicle_process(void);

/*
   ��ȡ�����״̬
   ����ȥ��

   Ҫ��Ҫ���ݽ���tickֵ?

 */
static void cb_tmr_50ms( void *parameter )
{
    uint32_t data_u32;
    uint32_t data_u32_1;
    static uint32_t counter = 0;
    static uint32_t low_speed_counter = 0;

    counter++;
    buzzer_pro();
    if(car_data.get_speed < 120)
    {
        low_speed_counter++;
    }
    else
    {
        low_speed_counter = 0;
    }
    if(counter % 2 == 0) //  100ms  ��ʱ��
    {
        gb_100ms_timer();
    }

    if(counter % 20 == 0)
    {
        WatchDog_Feed();
        if((low_speed_counter > 20) && (jt808_param_bk.mileage_pulse_utc_end == 0) && (utc_now > jt808_param_bk.mileage_pulse_utc_start))
        {
            jt808_param_bk.low_speed_wait_time++;
        }
        SpeedFatigue_1000ms();
    }
    if(counter % 10 == 0)
    {
        data_u32 = rt_tick_get() - car_data.get_pulse_tick;
        ///�������500ms����û�м�⵽���壬����Ϊ�ٶ�Ϊ0
        if( data_u32 >= (RT_TICK_PER_SECOND / 2 - 5) )
        {
            data_u32--;		///����һ������ֹ��ʱ��׼
            if(Frequency > RT_TICK_PER_SECOND / data_u32)
                Frequency = RT_TICK_PER_SECOND / data_u32;
            if((jt808_param.id_0xF032 & 0xFFFF) > 600)
                data_u32 = 36000 * Frequency / (jt808_param.id_0xF032 & 0xFFFF);
            else
                data_u32 = 0;
            if(car_data.get_speed >= data_u32)
            {
                DutyCycle	= 0;
                car_data.get_speed = data_u32;
                car_data.get_speed_tick		= rt_tick_get();
                car_data.get_speed_cap_sum	= 0;
                car_data.get_speed_cap_num	= 0;
            }
            car_data.pulse_cap_num		= 0;
            car_data.pulse_mileage		= jt808_param_bk.car_mileage;
        }
    }
    //jt808_vehicle_process();
}


void get_state(void)
{
    u8 i;
    rt_kprintf("\n speed = %d", car_data.get_speed / 10);
    rt_kprintf("\n frequency = %d", Frequency);
    rt_kprintf("\n state =");
    for(i = 0; i < 9; i++)
    {
        if(test_io & (BIT(i)))
        {
            rt_kprintf("1");
        }
        else
        {
            rt_kprintf("0");
        }
    }
    rt_kprintf("\n ��Դ=%d", AD_Volte);
    rt_kprintf("\n ģ��1=%d", AD_2through[0]);
    rt_kprintf("\n ģ��2=%d", AD_2through[1]);
    for(i = 0; i < 3; i++)
    {
        rt_kprintf("\n ADC_%d=%d", i + 1, ADC_ConValue[i]);
    }

}
FINSH_FUNCTION_EXPORT( get_state, get_state );


extern __IO uint32_t	uwPeriodValue;
extern __IO uint32_t	uwCaptureNumber;
uint16_t				tmpCC4[2] = { 0, 0 };

/*TIM5_CH1,�����ж��ٶ�*/
void TIM5_IRQHandler( void )
{
    uint32_t  tempu32_data;
    uint32_t  tempu32_data2;
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq( &RCC_Clocks );
    //rt_kprintf("P");

    if( TIM_GetITStatus( TIM5, TIM_IT_CC2 ) != RESET )
    {
        TIM_ClearITPendingBit( TIM5, TIM_IT_CC2 );

        /* Get the Input Capture value */
        IC2Value = TIM_GetCapture2( TIM5 );

        if( IC2Value != 0 )
        {
            DutyCycle	= ( IC2Value * 100 ) / TIM_GetCapture1( TIM5 );
#ifdef STM32F4XX
            Frequency	= ( RCC_Clocks.HCLK_Frequency ) / 168 / TIM_GetCapture1( TIM5 );
#else
            Frequency	= ( RCC_Clocks.HCLK_Frequency ) / 120 / TIM_GetCapture1( TIM5 );
#endif
            car_data.get_speed_cap_sum += TIM_GetCapture1( TIM5 );
            car_data.get_speed_cap_num++;
            car_data.pulse_cap_num++;
            car_data.get_pulse_tick		= rt_tick_get();
            //����ϵ���Ѿ��궨�����Լ������
            if((jt808_param.id_0xF032 > 600) && (jt808_param.id_0xF032 <= 0xFFFF))
            {
                //��Ҫ��ÿ����������ݽ��м��㣬ֻ�������ۼƴﵽ10��ʱ�����ۼ�
                tempu32_data2	= jt808_param_bk.mileage_pulse % 100;
                tempu32_data 	= jt808_param.id_0xF032 * (tempu32_data2 + 1) / 100 - (jt808_param.id_0xF032 * tempu32_data2 / 100);

                if(car_data.pulse_cap_num >= tempu32_data)
                {
                    car_data.pulse_cap_num -= tempu32_data;
                    jt808_param_bk.mileage_pulse += 10;
                }
            }
            if((rt_tick_get() - car_data.get_speed_tick > RT_TICK_PER_SECOND / 10) || (car_data.get_speed_cap_num > 20))
            {
#ifdef STM32F4XX
                tempu32_data =  ( RCC_Clocks.HCLK_Frequency ) / 168 * car_data.get_speed_cap_num / car_data.get_speed_cap_sum;
#else
                tempu32_data =  ( RCC_Clocks.HCLK_Frequency ) / 120 * car_data.get_speed_cap_num / car_data.get_speed_cap_sum;
#endif
                if((jt808_param.id_0xF032 & 0xFFFF) > 600)
                    car_data.get_speed	= tempu32_data * 36000 / (jt808_param.id_0xF032 & 0xFFFF);
                else
                    car_data.get_speed	= 0;
                if((jt808_param.id_0xF04D >= 600) && (jt808_param.id_0xF032 < 0xFFFF))
                {
                    if(car_data.get_speed >= jt808_param.id_0xF04D)
                        car_data.get_speed = jt808_param.id_0xF04D;
                }
                car_data.get_speed_cap_num	= 0;
                car_data.get_speed_cap_sum	= 0;
                car_data.get_speed_tick	= rt_tick_get();
            }
        }
        else
        {
            DutyCycle	= 0;
            Frequency	= 0;
        }
        //rt_kprintf("\n cap1=%d",TIM_GetCapture1( TIM5 ));
        //rt_kprintf("\n cap1=%d,cap2=%d",TIM_GetCapture1( TIM5 ),TIM_GetCapture2( TIM5 ));
    }
#if USE_IWDG
    if( TIM_GetITStatus( TIM5, TIM_IT_CC4 ) != RESET )
    {
        /* Get the Input Capture value */
        tmpCC4[uwCaptureNumber++] = TIM_GetCapture4( TIM5 );

        /* Clear CC4 Interrupt pending bit */
        TIM_ClearITPendingBit( TIM5, TIM_IT_CC4 );

        if( uwCaptureNumber >= 2 )
        {
            /* Compute the period length */
            uwPeriodValue = (uint16_t)( 0xFFFF - tmpCC4[0] + tmpCC4[1] + 1 );
        }
    }
#endif
}


/*����PA.0 ��Ϊ�ⲿ�������*/
void pulse_init( void )
{
    GPIO_InitTypeDef	GPIO_InitStructure;
    NVIC_InitTypeDef	NVIC_InitStructure;
    TIM_ICInitTypeDef	TIM_ICInitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    /* TIM5 clock enable */
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM5, ENABLE );

    /* GPIOA clock enable */
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );

    /* TIM5 chennel1 configuration : PA.0 */
    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    /* Connect TIM pin to AF0 */
    GPIO_PinAFConfig( GPIOA, GPIO_PinSource0, GPIO_AF_TIM5 );

    /* Time base configuration */
#ifdef STM32F4XX
    TIM5->PSC	= 168 / 2 - 1;
#else
    TIM5->PSC	= 120 / 2 - 1;
#endif

    /* Enable the TIM5 global Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel						= TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 2;		//old is 0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 0;		//old is 1
    NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    TIM_ICInitStructure.TIM_Channel		= TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICPolarity	= TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter	= 0x0;

    TIM_PWMIConfig( TIM5, &TIM_ICInitStructure );

    /* Select the TIM5 Input Trigger: TI1FP1 */
    TIM_SelectInputTrigger( TIM5, TIM_TS_TI1FP1 );

    /* Select the slave Mode: Reset Mode */
    TIM_SelectSlaveMode( TIM5, TIM_SlaveMode_Reset );
    TIM_SelectMasterSlaveMode( TIM5, TIM_MasterSlaveMode_Enable );

    /* TIM enable counter */
    TIM_Cmd( TIM5, ENABLE );

    /* Enable the CC2 Interrupt Request */
    TIM_ITConfig( TIM5, TIM_IT_CC2, ENABLE );
}

/**/
void ad_init( void )
{
    uint32_t tempu32 = 0;
    ADC_InitTypeDef			ADC_InitStructure;
    GPIO_InitTypeDef		gpio_init;
    ADC_CommonInitTypeDef	ADC_CommonInitStructure;
    DMA_InitTypeDef			DMA_InitStructure;

    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC, ENABLE );
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_ADC1, ENABLE );

    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init.GPIO_Pin	= GPIO_Pin_5;		///ch
    GPIO_Init( GPIOC, &gpio_init );

    //gpio_init.GPIO_Pin = GPIO_Pin_1;
    //GPIO_Init( GPIOA, &gpio_init );

    gpio_init.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init( GPIOC, &gpio_init );

    gpio_init.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init( GPIOC, &gpio_init );

    //  3. ADC Common Init
    /* ADC Common configuration *************************************************/
    ADC_CommonInitStructure.ADC_Mode				= ADC_Mode_Independent;             /*�ڶ���ģʽ�� ÿ��ADC�ӿڶ�������*/
    ADC_CommonInitStructure.ADC_Prescaler			= ADC_Prescaler_Div8;               //ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode		= ADC_DMAAccessMode_1;              // ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay	= ADC_TwoSamplingDelay_20Cycles;    //ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit( &ADC_CommonInitStructure );

    ADC_InitStructure.ADC_Resolution			= ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode			= ENABLE;                               // if used  multi channels set enable
    ADC_InitStructure.ADC_ContinuousConvMode	= ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge	= ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv		= ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign				= ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion		= 3;                                    // number of   channel
    ADC_Init( ADC1, &ADC_InitStructure );

    //  4. DMA  Config
    /* DMA2 Stream0 channel0 configuration */
    DMA_InitStructure.DMA_Channel				= DMA_Channel_0;
    DMA_InitStructure.DMA_PeripheralBaseAddr	= (uint32_t)ADC1_DR_Address;
    DMA_InitStructure.DMA_Memory0BaseAddr		= (uint32_t)ADC_ConValue;
    DMA_InitStructure.DMA_DIR					= DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize			= 3;
    DMA_InitStructure.DMA_PeripheralInc			= DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc				= DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize	= DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize		= DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode					= DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority				= DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode				= DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold			= DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst			= DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst		= DMA_PeripheralBurst_Single;
    DMA_Init( DMA2_Stream0, &DMA_InitStructure );

    /* DMA2_Stream0 enable */
    DMA_Cmd( DMA2_Stream0, ENABLE );

    ADC_RegularChannelConfig( ADC1, ADC_Channel_15, 1, ADC_SampleTime_56Cycles );   // ͨ��1  ��ص���
    //ADC_RegularChannelConfig( ADC1, ADC_Channel_1, 2, ADC_SampleTime_56Cycles );    //  ͨ��2   ����
    ADC_RegularChannelConfig( ADC1, ADC_Channel_13, 2, ADC_SampleTime_56Cycles );   // ͨ��3   ����
    ADC_RegularChannelConfig( ADC1, ADC_Channel_11, 3, ADC_SampleTime_56Cycles );   // ͨ��3   ����

    /* Enable DMA request after last transfer (Single-ADC mode) */
    ADC_DMARequestAfterLastTransferCmd( ADC1, ENABLE );

    /* Enable ADC1 DMA */
    ADC_DMACmd( ADC1, ENABLE );

    /* Enable ADC3 */
    ADC_Cmd( ADC1, ENABLE );

    ADC_SoftwareStartConv( ADC1 );
}



void jt808_vehicle_process(void)
{
    uint8_t i;
    uint8_t st;
    for( i = 0; i < sizeof( PIN_IN ) / sizeof( AUX_IN ); i++ )
    {
        st = GPIO_ReadInputDataBit( PIN_IN[i].port, PIN_IN[i].pin );
        if( st ^ PIN_IN[i].value )                      /*ֵ��ͬ,�б仯*/
        {
            //rt_kprintf( "\r\nPin %d = %d ", i , st);
            if( PIN_IN[i].dithering_threshold == 0 )    /*��������*/
            {
                PIN_IN[i].value = st;
                PIN_IN[i].onchange( i, st );            /*���ô�����*/
            }
            else
            {
                PIN_IN[i].dithering_count++;
                if( PIN_IN[i].dithering_count >= PIN_IN[i].dithering_threshold )
                {
                    beep(4, 4, 1);
                    //rt_kprintf( "\nPin %d �ı�", i );
                    PIN_IN[i].value = st;
                    PIN_IN[i].onchange( i, st ); /*���ô�����*/
                    PIN_IN[i].dithering_count = 0;
                }
            }
        }
        else
        {
            PIN_IN[i].duration++;
            PIN_IN[i].dithering_count = 0;
        }
    }

    ADC_ConvertedValue = ADC_ConValue[0]; //ADC_GetConversionValue(ADC1);

    //rt_kprintf("\n=%d",ADC_ConvertedValue);

    AD_Volte = ( ( ADC_ConvertedValue * 543 ) >> 12 );
    //rt_kprintf ("\r\n  ��ȡ���ĵ��AD��ֵΪ:	%d	 AD��ѹΪ: %d V  ��Դ��ѹ: %d V\r\n",ADC_ConvertedValue,a,a+11);
    //  ---��ԴǷѹ����----
    AD_Volte = AD_Volte + 11 + 10;
    if( AD_Volte < AD_Volte_Min )
    {
        AD_Volte_Min = AD_Volte;
    }
    if( AD_Volte > AD_Volte_Max )
    {
        AD_Volte_Max = AD_Volte;
    }

    //  -----	����2 ·  AD �Ĳɼ���ѹֵת��
    // 1 .through	1  Voltage Value
    /*
    AD_2through[0]	= ( ( ( ADC_ConValue[1] - 70 ) * 543 ) >> 12 );
    AD_2through[0]	= AD_2through[0] + 11 + 10;
    AD_2through[0]	= AD_2through[0] * 100; // mV
    // 2 .through	2  Voltage Value
    AD_2through[1]	= ( ( ( ADC_ConValue[2] - 70 ) * 543 ) >> 12 );
    AD_2through[1]	= AD_2through[1] + 11 + 10;
    AD_2through[1]	= AD_2through[1] * 100;
    */

    AD_2through[0] = ADC_ConValue[2]; //ADC_GetConversionValue(ADC1);
    AD_2through[0] = ( ( AD_2through[0] * 11 * 33 ) >> 12 );
    //rt_kprintf("\n ģ��1=%d",AD_Volte);

    AD_2through[1] = ADC_ConValue[1]; //ADC_GetConversionValue(ADC1);
    AD_2through[1] = ( ( AD_2through[1] * 11 * 33 ) >> 12 );
    //rt_kprintf("\n ģ��2=%d",AD_Volte);


    if( AD_Volte < 50 )          //  С��500 ��Ϊ���ⲿ�ϵ�
    {
        power_lost_counter++;
        if( power_lost_counter == 40 )      /*20�� ����2s*/
        {
            if( ( jt808_param_bk.car_alarm & BIT_ALARM_LOST_PWR ) == 0 )
            {
                debug_write( "����Դ����!" );
                jt808_param_bk.car_alarm |= BIT_ALARM_LOST_PWR;
                jt808_param_bk.car_alarm &= ~BIT_ALARM_LOW_PWR;

                //  GB19056  ���� �¹��ɵ�2
                GB_DOUBT2_Generate();
            }
        }
    }
    else  /*��Դ���������*/
    {
        power_lost_counter = 0;
        if(( jt808_param_bk.car_alarm & BIT_ALARM_LOST_PWR ) && (AD_Volte > 80)) /*�Ѿ����磬�ָֻ���*/
        {
            debug_write( "����Դ����!" );
            jt808_param_bk.car_alarm &= ~BIT_ALARM_LOST_PWR;

            //  GB19056  �ⲿ�����¼
            //  ��Դ�ָ�������¼
            VDR_product_13H(0x01);
        }
    }
    //------------�ж�Ƿѹ������-----
    if( AD_Volte < 150 ) // 16V		<->  160
    {
        power_low_counter++;
        if( power_low_counter == 40 )		//��������
        {
            if( ( jt808_param_bk.car_alarm & BIT_ALARM_LOW_PWR ) == 0 )
            {
                if( ( jt808_param_bk.car_alarm & BIT_ALARM_LOST_PWR ) == 0 )
                {
                    jt808_param_bk.car_alarm |= BIT_ALARM_LOW_PWR;
                    debug_write( "Ƿѹ����!" );
                }
            }
        }
    }
    else
    {
        power_low_counter = 0;
        if(( jt808_param_bk.car_alarm & BIT_ALARM_LOW_PWR ) && ( AD_Volte > 110))
        {
            jt808_param_bk.car_alarm &= ~BIT_ALARM_LOW_PWR;
            debug_write( "Ƿѹ��ԭ!" );
        }
    }
}


/*
   �����ⲿ�����������
   todo:�Ƿ��뵽�洢�У������������

 */
void jt808_vehicle_init( void )
{
    GPIO_InitTypeDef	GPIO_InitStructure;
    int					i;
    GPIO_StructInit(&GPIO_InitStructure);
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );

    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_25MHz;

    for( i = 0; i < sizeof( PIN_IN ) / sizeof( AUX_IN ); i++ )
    {
        GPIO_InitStructure.GPIO_Pin = PIN_IN[i].pin;
        GPIO_Init( PIN_IN[i].port, &GPIO_InitStructure );
        //PIN_IN[i].onchange(i,PIN_IN[i].value);
    }


    mma8451_driver_init( );

    rt_timer_init( &tmr_50ms, "tmr_50ms",       /* ��ʱ�������� tmr_gps */
                   cb_tmr_50ms,                 /* ��ʱʱ�ص��Ĵ����� */
                   RT_NULL,                     /* ��ʱ��������ڲ��� */
                   RT_TICK_PER_SECOND / 20,     /* ��ʱ���ȣ���OS TickΪ��λ */
                   RT_TIMER_FLAG_PERIODIC );    /* �����Զ�ʱ�� */

    rt_timer_start( &tmr_50ms );
    pulse_init( );                              /*���������*/
    ad_init( );
    Init_4442( );
}

/************************************** The End Of File **************************************/
