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
#include "Menu_Include.h"
#include <string.h>
#include "LCD_Driver.h"

#include "jt808_vehicle.h"

/*北斗使用两路AD检测 PA1 喇叭 PC3 左转*/
char			 *caption[8] = { "紧急", "启动", "远光", "车门", "左转", "右转", "刹车", "雨刷" };
static uint8_t	page;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void draw( void )
{
    uint8_t i;
    char buf[24];
    lcd_fill( 0 );
    if( page == 0 )
    {
        for( i = 0; i < 4; i++ )
        {
            lcd_text12( i * 32, 4, caption[i], 4, PIN_IN[i].value * 2 + 1 );            // SET=1 INVERT=3
            lcd_text12( i * 32, 20, caption[i + 4], 4, PIN_IN[i + 4].value * 2 + 1 );   // SET=1 INVERT=3
        }
    }
    else if( page == 1 )
    {
        sprintf(buf, "电压 %d [%d-%d]", AD_Volte, AD_Volte_Min, AD_Volte_Max);
        lcd_text12( 0, 4, buf, strlen(buf), LCD_MODE_SET );
    }
    else if( page == 2 )
    {
        sprintf(buf, "脉速 %d ", Frequency);
        lcd_text12( 0, 4, buf, strlen(buf), LCD_MODE_SET );
    }
    lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
    pMenuItem->tick = rt_tick_get( );
    page			= 0;
    draw( );
}

/**/
static void keypress( unsigned int key )
{
    switch( key )
    {
    case KEY_MENU:
        pMenuItem = &Menu_2_InforCheck;
        pMenuItem->show( );
        break;
    case KEY_UP:
        if( page == 0 )
        {
            page = 3;
        }
        page--;
        draw( );
        break;
    case KEY_DOWN:
        page++;
        page %= 3;
        draw( );
        break;
    }
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
static void timetick( unsigned int tick )
{
    static uint8_t counter = 0;
    static uint32_t last_status = 0;
    uint32_t current_status = 0;
    uint8_t i;
    for(i = 0; i < 8; i++)
    {
        if(PIN_IN[i].value)
        {
            current_status |= BIT(i);
        }
    }
    if((current_status != last_status) || (++counter >= 10))
    {
        counter	= 0;
        last_status	= current_status;
        draw( );
    }
    timetick_default( tick );
}

MENUITEM Menu_2_1_Status8 =
{
    "信号线状态",
    10,			 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/
