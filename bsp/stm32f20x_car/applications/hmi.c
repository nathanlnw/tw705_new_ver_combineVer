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

#include <stdio.h>
#include <string.h>
#include "include.h"
#include <board.h>
#include <rtthread.h>
#include "jt808.h"
#include "menu_include.h"
#include "LCD_Driver.h"

#include "sle4442.h"

#define KEY_NONE 0

typedef struct _KEY
{
    GPIO_TypeDef	*port;
    uint32_t		pin;
    uint32_t		tick;
    uint32_t		status;             /*��¼ÿ��������״̬*/
} KEY;

static KEY		keys[] =
{
    { GPIOD, GPIO_Pin_3, 0, KEY_NONE }, /*menu*/
    { GPIOC, GPIO_Pin_8, 0, KEY_NONE }, /*ok*/
    { GPIOC, GPIO_Pin_9, 0, KEY_NONE }, /*up*/
    { GPIOA, GPIO_Pin_8, 0, KEY_NONE }, /*down*/
};

static uint8_t	beep_high_ticks = 0;
static uint8_t	beep_low_ticks	= 0;
static uint8_t	beep_state		= 0;
static uint32_t beep_ticks;             /*����ʱ�����*/
static uint16_t beep_count = 0;
void buzzer_on(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    /*�����ýṹ���еĲ��ֳ�Ա����������£��û�Ӧ�����ȵ��ú���PPP_SturcInit(..)
    ����ʼ������PPP_InitStructure��Ȼ�����޸�������Ҫ�޸ĵĳ�Ա���������Ա�֤����
    ��Ա��ֵ����Ϊȱʡֵ������ȷ���롣
     */
    GPIO_StructInit(&GPIO_InitStructure);

    /*����GPIOA_Pin_5����ΪTIM2_Channel1 PWM���*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; 			//ָ����������
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;		//ģʽ����Ϊ���ã�
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//Ƶ��Ϊ����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;		//��������PWM������Ӱ��
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_TIM2); //����GPIOA_Pin1ΪTIM2_Ch2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_TIM2); //����GPIOA_Pin5ΪTIM2_Ch1,
}

void buzzer_off(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_StructInit(&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; 			//ָ����������
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;		//ģʽΪ����
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//Ƶ��Ϊ����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;		//�����Ա��ʡ����
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*
   ����������
   �� high tick
   �� low
 */
void beep( uint8_t high_50ms_count, uint8_t low_50ms_count, uint16_t count )
{
    if(device_control.off_counter)
    {
        return;
    }
    beep_high_ticks = high_50ms_count;
    beep_low_ticks	= low_50ms_count;
    beep_state		= 1;                    /*����*/
    beep_ticks		= beep_high_ticks;
    beep_count		= count;
    buzzer_on();
}

///�ú�����Ҫ��ʱ���ã�����Ƶ��Ϊ50ms
void buzzer_pro(void)
{
    if( beep_count )                                        /*������ʾ*/
    {
        beep_ticks--;
        if( beep_ticks == 0 )
        {
            if( beep_state == 1 )                           /*���״̬*/
            {
                buzzer_off();
                beep_ticks	= beep_low_ticks;
                beep_state	= 0;
            }
            else
            {
                beep_count--;
                if( beep_count )                            /*û�칻*/
                {
                    buzzer_on();
                    beep_ticks	= beep_high_ticks;
                    beep_state	= 1;
                }
            }
        }
    }
}
/*
   50ms���һ�ΰ���,ֻ����λ��Ӧ�ļ����������ж���ϼ�����
 */

static uint32_t  keycheck( void )
{
    int			i;
    uint32_t	tmp_key = 0;
    for( i = 0; i < 4; i++ )
    {
        if( GPIO_ReadInputDataBit( keys[i].port, keys[i].pin ) )    /*��̧��*/
        {
            if( ( keys[i].tick > 50 ) && ( keys[i].tick < 500 ) )   /*�̰�,ȥ��*/
            {
                keys[i].status = ( 1 << i );
            }
            else
            {
                keys[i].status = 0;                                 /*��ն�Ӧ�ı�־λ*/
            }
            keys[i].tick = 0;
        }
        else  /*������*/
        {
            keys[i].tick += 50;                                     /*ÿ������50ms*/
            if(i < 2)
            {
                if( ( keys[i].tick % 1000 ) == 0 )
                {
                    keys[i].status = ( 1 << i ) << 4;
                    //keys[i].tick	= 0;
                }
            }
            else
            {
                if( ( keys[i].tick % 200  == 0 ) && (keys[i].tick >= 800))
                {
                    keys[i].status = ( 1 << i );
                    //keys[i].tick	= 0;
                }
            }
        }
    }
    tmp_key = keys[0].status | keys[1].status | keys[2].status | keys[3].status;
    for( i = 0; i < 4; i++ )
    {
        keys[i].status = 0;
    }
#if 0
    if( tmp_key )
    {
        rt_kprintf( "%04x\r\n", tmp_key );
    }
#endif
    return ( tmp_key );
}

/*************************************************
Function:    void GPIO_Config(void)
Description: GPIO���ú���
Input: ��
Output:��
Return:��
*************************************************/
void GPIO_Config_PWM(void)
{
    /*������һ��GPIO_InitStructure�Ľṹ�壬����һ��ʹ�� */
    GPIO_InitTypeDef GPIO_InitStructure;
    /* ʹ��GPIOGʱ�ӣ�ʱ�ӽṹ�μ���stm32ͼ��.pdf����*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
    /*�����ýṹ���еĲ��ֳ�Ա����������£��û�Ӧ�����ȵ��ú���PPP_SturcInit(..)
    ����ʼ������PPP_InitStructure��Ȼ�����޸�������Ҫ�޸ĵĳ�Ա���������Ա�֤����
    ��Ա��ֵ����Ϊȱʡֵ������ȷ���롣
     */
    GPIO_StructInit(&GPIO_InitStructure);

    /*����GPIOA_Pin_8����ΪTIM1_Channel2 PWM���*/
    //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_5; //ָ����������
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //ָ����������
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;    //ģʽ����Ϊ���ã�
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   //Ƶ��Ϊ����
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;          //��������PWM������Ӱ��
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_TIM2); //����GPIOA_Pin1ΪTIM2_Ch2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_TIM2); //����GPIOA_Pin5ΪTIM2_Ch1,
}


/*************************************************
Function:    void TIM_Config(void)
Description: ��ʱ�����ú���
Input:       ��
Output:      ��
*************************************************/
void TIM_Config_PWM(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_DeInit(TIM2);//��ʼ��TIM2�Ĵ���
    /*��Ƶ�����ڼ��㹫ʽ��
      Prescaler = (TIMxCLK / TIMx counter clock) - 1;
      Period = (TIMx counter clock / TIM3 output clock) - 1
      TIMx counter clockΪ������Ҫ��TXM�Ķ�ʱ��ʱ��
      */
    TIM_TimeBaseStructure.TIM_Period = 10 - 1; //�������ֲ��֪��TIM2��TIM5Ϊ32λ�Զ�װ�أ���������
    /*��system_stm32f4xx.c�����õ�APB1 Prescaler = 4 ,��֪
      APB1ʱ��Ϊ168M/4*2,��Ϊ���APB1��Ƶ��Ϊ1����ʱʱ��*2
     */
#ifdef STM32F4XX
    TIM_TimeBaseStructure.TIM_Prescaler = 2100 - 1;			///168M  4000HZ
#else
    TIM_TimeBaseStructure.TIM_Prescaler = 1500 - 1;			///120M  4000HZ
#endif
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//���ϼ���
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /*��������Ƚϣ�����ռ�ձ�Ϊ20%��PWM����*/
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//PWM1Ϊ����ռ�ձ�ģʽ��PWM2Ϊ������ģʽ
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    //TIM_OCInitStructure.TIM_Pulse = 2;//����CCR��ռ�ձ���ֵ��
    TIM_OCInitStructure.TIM_Pulse = 5;//����CCR��ռ�ձ���ֵ��
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;//HighΪռ�ձȸ߼��ԣ���ʱռ�ձ�Ϊ20%��Low��Ϊ�����ԣ�ռ�ձ�Ϊ80%

    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);//CCR�Զ�װ��Ĭ��Ҳ�Ǵ򿪵�

    TIM_ARRPreloadConfig(TIM2, ENABLE);  //ARR�Զ�װ��Ĭ���Ǵ򿪵ģ����Բ�����

    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE); //ʹ��TIM2��ʱ��
    /* ʹ��GPIOGʱ�ӣ�ʱ�ӽṹ�μ���stm32ͼ��.pdf����*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
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
void key_lcd_port_init( void )
{
    GPIO_InitTypeDef	GPIO_InitStructure;
    int					i;

    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );

    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;

    for( i = 0; i < 4; i++ )
    {
        GPIO_InitStructure.GPIO_Pin = keys[i].pin;
        GPIO_Init( keys[i].port, &GPIO_InitStructure );
    }

    //OUT	(/MR  SHCP	 DS   STCP	 STCP)
    GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_Init( GPIOE, &GPIO_InitStructure );

    //BUZZER
    //GPIO_Config_PWM();
    TIM_Config_PWM();
    buzzer_off();
    //rt_kprintf("\r\n PWM_INIT OK!");

    ControlBitShift( 0 );
    DataBitShift( 0 );
    LCD_BL(1);
    lcd_init( );
    lcd_fill(0);
#ifdef TJ_MIXERS_V01
    lcd_text12(0, 0, "���賵ר��:", 13, LCD_MODE_SET);
#else
    lcd_text12(0, 0, "��׼�豸:", 9, LCD_MODE_SET);
#endif
    lcd_text12(26, 14, "�Լ��С�����", 12, LCD_MODE_SET);
    lcd_update_all();

    memset(debug_msg, 0x20, sizeof(debug_msg));
}

void self_test_pro(void)
{
    uint32_t	last_tick = 0;
    ///�ȴ���һ��ʱ�����Լ�
    while(1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND / 10);
        if(rt_tick_get() > 400)
        {
            break;
        }
    }
    lcd_fill(0);
    if( jt808_param_bk.car_alarm & (BIT_ALARM_LOST_PWR | BIT_ALARM_GPS_SHORT | BIT_ALARM_GPS_OPEN ))
    {
        if( jt808_param_bk.car_alarm & BIT_ALARM_GPS_SHORT)
        {
            lcd_text12(36, 3, "���߶�·", 8, LCD_MODE_SET);
        }
        else if( jt808_param_bk.car_alarm & BIT_ALARM_GPS_OPEN )
        {
            lcd_text12(36, 3, "���߿�·", 8, LCD_MODE_SET);
        }
        if( jt808_param_bk.car_alarm & BIT_ALARM_LOST_PWR)
        {
            lcd_text12(24, 18, "�ڲ���ع���", 12, LCD_MODE_SET);
        }
        lcd_update_all();
        last_tick = rt_tick_get();
        while(1)
        {
            rt_thread_delay(RT_TICK_PER_SECOND / 20);
            if(keycheck( ) == KEY_OK)
            {
                break;
            }
        }
    }
    else
    {
        lcd_text12(36, 10, "�Լ�����", 8, LCD_MODE_SET);
        lcd_update_all();
        last_tick = rt_tick_get();
        while(1)
        {
            rt_thread_delay(RT_TICK_PER_SECOND / 20);
            if((rt_tick_get() - last_tick > RT_TICK_PER_SECOND) || (keycheck( )))
            {
                break;
            }
        }
    }
}

ALIGN( RT_ALIGN_SIZE )
static char thread_hmi_stack[2048];
//static char thread_hmi_stack[2048]  __attribute__((section("CCM_RT_STACK")));
struct rt_thread thread_hmi;
/*hmi�߳�*/
static void rt_thread_entry_hmi( void *parameter )
{
    uint32_t key;
    static uint32_t bl_counter = 0;
    uint8_t	last_unlock_val = 0xAA;

    //key_lcd_port_init( );
    //rt_thread_delay( RT_TICK_PER_SECOND / 20 );
    //LCD_BL(1);
    //lcd_init( );
    last_unlock_val = device_unlock ^ 0xFF;
    if(device_flash_err)
    {
        pMenuItem = &Menu_flash_err;
        pMenuItem->show( );
    }
    else
    {
        self_test_pro();
    }
    beep(5, 5, 1);
    while( 1 )
    {
        if((last_unlock_val != device_unlock) && (device_flash_err == 0))
        {
            if(device_unlock)
            {
                pMenuItem = &Menu_1_Idle;
                sock_proc(mast_socket, SOCK_CONNECT);
                rt_kprintf("\r\n������Ϣ  �������");
            }
            else
            {
                pMenuItem = &Menu_0_0_password;//Menu_0_loggingin;//
                sock_proc(mast_socket, SOCK_CLOSE);
                rt_kprintf("\r\n������Ϣ  ��δ����");
            }
            pMenuItem->tick = rt_tick_get( );
            pMenuItem->show( );
            last_unlock_val = device_unlock;
        }
        bl_counter++;
        wdg_thread_counter[2] = 1;
        rt_thread_delay( RT_TICK_PER_SECOND / 20 ); /*50ms����һ��*///ÿ���Ӳ˵��� ��ʾ�ĸ��� ����  ʱ��Դ�� ����ִ������



        if( Control_74HC_1 != Control_74HC_1_Used)
        {
            ControlBitShift(Control_74HC_1);
        }
        if(device_control.off_counter)
        {
            Control_74HC_1	= 0;
            continue;
        }
        LCD_BL_2(1);					///��Һ������
        CheckICCard( );
        key = keycheck( );
        if( key )                                               /*�м����£��򿪱���*/
        {
            bl_counter = 0;
            LCD_BL(1);
            pMenuItem->keypress( key );                         //ÿ���Ӳ˵��� �������  ʱ��Դ50ms timer
            pMenuItem->tick = rt_tick_get( );
        }
        pMenuItem->timetick( rt_tick_get( ) );
        /*
        if(param_factset_tick)
        {
        	if((rt_tick_get() - pMenuItem->tick > 30 * RT_TICK_PER_SECOND)&&(rt_tick_get()-param_factset_tick > 60 * RT_TICK_PER_SECOND))
        	{
        		if(jt808_param.id_0xF00F==0)
        		{
        			pMenuItem = &Menu_0_0_password;//Menu_0_loggingin;//
        			pMenuItem->show( );
        		}
        		gsmstate(GSM_POWEROFF);
        		param_factset_tick = 0;
        	}
        }
        */
    }
}

/*
   ���õ���������ʾ��Ϣ��������һ��ʱ��
 */
void pop_msg( char *msg, uint32_t interval )
{
    Menu_Popup.parent	= pMenuItem;
    pMenuItem			= &Menu_Popup;
    pMenuItem->tick		= interval; /*��ʱ��tick����һ�²���*/
    pMenuItem->show( );
    pMenuItem->msg( (void *)msg );
}

/**/
void hmi_init( void )
{
    rt_thread_init( &thread_hmi,
                    "hmi",
                    rt_thread_entry_hmi,
                    RT_NULL,
                    &thread_hmi_stack[0],
                    sizeof( thread_hmi_stack ), 8, 5 );
    rt_thread_startup( &thread_hmi );
}

/************************************** The End Of File **************************************/
