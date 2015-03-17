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
    uint32_t		status;             /*记录每个按键的状态*/
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
static uint32_t beep_ticks;             /*持续时间计数*/
static uint16_t beep_count = 0;
void buzzer_on(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    /*仅设置结构体中的部分成员：这种情况下，用户应当首先调用函数PPP_SturcInit(..)
    来初始化变量PPP_InitStructure，然后再修改其中需要修改的成员。这样可以保证其他
    成员的值（多为缺省值）被正确填入。
     */
    GPIO_StructInit(&GPIO_InitStructure);

    /*配置GPIOA_Pin_5，作为TIM2_Channel1 PWM输出*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; 			//指定复用引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;		//模式必须为复用！
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//频率为快速
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;		//上拉与否对PWM产生无影响
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_TIM2); //复用GPIOA_Pin1为TIM2_Ch2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_TIM2); //复用GPIOA_Pin5为TIM2_Ch1,
}

void buzzer_off(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_StructInit(&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; 			//指定复用引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;		//模式为输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//频率为快速
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;		//下拉以便节省电能
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*
   蜂鸣器发声
   响 high tick
   灭 low
 */
void beep( uint8_t high_50ms_count, uint8_t low_50ms_count, uint16_t count )
{
    if(device_control.off_counter)
    {
        return;
    }
    beep_high_ticks = high_50ms_count;
    beep_low_ticks	= low_50ms_count;
    beep_state		= 1;                    /*发声*/
    beep_ticks		= beep_high_ticks;
    beep_count		= count;
    buzzer_on();
}

///该函数需要定时调用，调用频率为50ms
void buzzer_pro(void)
{
    if( beep_count )                                        /*声音提示*/
    {
        beep_ticks--;
        if( beep_ticks == 0 )
        {
            if( beep_state == 1 )                           /*响的状态*/
            {
                buzzer_off();
                beep_ticks	= beep_low_ticks;
                beep_state	= 0;
            }
            else
            {
                beep_count--;
                if( beep_count )                            /*没响够*/
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
   50ms检查一次按键,只是置位对应的键，程序中判断组合键按下
 */

static uint32_t  keycheck( void )
{
    int			i;
    uint32_t	tmp_key = 0;
    for( i = 0; i < 4; i++ )
    {
        if( GPIO_ReadInputDataBit( keys[i].port, keys[i].pin ) )    /*键抬起*/
        {
            if( ( keys[i].tick > 50 ) && ( keys[i].tick < 500 ) )   /*短按,去抖*/
            {
                keys[i].status = ( 1 << i );
            }
            else
            {
                keys[i].status = 0;                                 /*清空对应的标志位*/
            }
            keys[i].tick = 0;
        }
        else  /*键按下*/
        {
            keys[i].tick += 50;                                     /*每次增加50ms*/
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
Description: GPIO配置函数
Input: 无
Output:无
Return:无
*************************************************/
void GPIO_Config_PWM(void)
{
    /*定义了一个GPIO_InitStructure的结构体，方便一下使用 */
    GPIO_InitTypeDef GPIO_InitStructure;
    /* 使能GPIOG时钟（时钟结构参见“stm32图解.pdf”）*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
    /*仅设置结构体中的部分成员：这种情况下，用户应当首先调用函数PPP_SturcInit(..)
    来初始化变量PPP_InitStructure，然后再修改其中需要修改的成员。这样可以保证其他
    成员的值（多为缺省值）被正确填入。
     */
    GPIO_StructInit(&GPIO_InitStructure);

    /*配置GPIOA_Pin_8，作为TIM1_Channel2 PWM输出*/
    //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_5; //指定复用引脚
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //指定复用引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;    //模式必须为复用！
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   //频率为快速
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;          //上拉与否对PWM产生无影响
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_TIM2); //复用GPIOA_Pin1为TIM2_Ch2
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_TIM2); //复用GPIOA_Pin5为TIM2_Ch1,
}


/*************************************************
Function:    void TIM_Config(void)
Description: 定时器配置函数
Input:       无
Output:      无
*************************************************/
void TIM_Config_PWM(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_DeInit(TIM2);//初始化TIM2寄存器
    /*分频和周期计算公式：
      Prescaler = (TIMxCLK / TIMx counter clock) - 1;
      Period = (TIMx counter clock / TIM3 output clock) - 1
      TIMx counter clock为你所需要的TXM的定时器时钟
      */
    TIM_TimeBaseStructure.TIM_Period = 10 - 1; //查数据手册可知，TIM2与TIM5为32位自动装载，计数周期
    /*在system_stm32f4xx.c中设置的APB1 Prescaler = 4 ,可知
      APB1时钟为168M/4*2,因为如果APB1分频不为1，则定时时钟*2
     */
#ifdef STM32F4XX
    TIM_TimeBaseStructure.TIM_Prescaler = 2100 - 1;			///168M  4000HZ
#else
    TIM_TimeBaseStructure.TIM_Prescaler = 1500 - 1;			///120M  4000HZ
#endif
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//向上计数
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /*配置输出比较，产生占空比为20%的PWM方波*/
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//PWM1为正常占空比模式，PWM2为反极性模式
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    //TIM_OCInitStructure.TIM_Pulse = 2;//输入CCR（占空比数值）
    TIM_OCInitStructure.TIM_Pulse = 5;//输入CCR（占空比数值）
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;//High为占空比高极性，此时占空比为20%；Low则为反极性，占空比为80%

    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);//CCR自动装载默认也是打开的

    TIM_ARRPreloadConfig(TIM2, ENABLE);  //ARR自动装载默认是打开的，可以不设置

    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE); //使能TIM2定时器
    /* 使能GPIOG时钟（时钟结构参见“stm32图解.pdf”）*/
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
    lcd_text12(0, 0, "搅拌车专用:", 13, LCD_MODE_SET);
#else
    lcd_text12(0, 0, "标准设备:", 9, LCD_MODE_SET);
#endif
    lcd_text12(26, 14, "自检中。。。", 12, LCD_MODE_SET);
    lcd_update_all();

    memset(debug_msg, 0x20, sizeof(debug_msg));
}

void self_test_pro(void)
{
    uint32_t	last_tick = 0;
    ///等待着一段时间在自检
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
            lcd_text12(36, 3, "天线短路", 8, LCD_MODE_SET);
        }
        else if( jt808_param_bk.car_alarm & BIT_ALARM_GPS_OPEN )
        {
            lcd_text12(36, 3, "天线开路", 8, LCD_MODE_SET);
        }
        if( jt808_param_bk.car_alarm & BIT_ALARM_LOST_PWR)
        {
            lcd_text12(24, 18, "内部电池供电", 12, LCD_MODE_SET);
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
        lcd_text12(36, 10, "自检正常", 8, LCD_MODE_SET);
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
/*hmi线程*/
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
                rt_kprintf("\r\n车辆信息  设置完成");
            }
            else
            {
                pMenuItem = &Menu_0_0_password;//Menu_0_loggingin;//
                sock_proc(mast_socket, SOCK_CLOSE);
                rt_kprintf("\r\n车辆信息  尚未设置");
            }
            pMenuItem->tick = rt_tick_get( );
            pMenuItem->show( );
            last_unlock_val = device_unlock;
        }
        bl_counter++;
        wdg_thread_counter[2] = 1;
        rt_thread_delay( RT_TICK_PER_SECOND / 20 ); /*50ms调用一次*///每个子菜单下 显示的更新 操作  时钟源是 任务执行周期



        if( Control_74HC_1 != Control_74HC_1_Used)
        {
            ControlBitShift(Control_74HC_1);
        }
        if(device_control.off_counter)
        {
            Control_74HC_1	= 0;
            continue;
        }
        LCD_BL_2(1);					///开液晶背光
        CheckICCard( );
        key = keycheck( );
        if( key )                                               /*有键按下，打开背光*/
        {
            bl_counter = 0;
            LCD_BL(1);
            pMenuItem->keypress( key );                         //每个子菜单的 按键检测  时钟源50ms timer
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
   调用弹出窗体显示信息，并持续一定时间
 */
void pop_msg( char *msg, uint32_t interval )
{
    Menu_Popup.parent	= pMenuItem;
    pMenuItem			= &Menu_Popup;
    pMenuItem->tick		= interval; /*临时用tick传递一下参数*/
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
