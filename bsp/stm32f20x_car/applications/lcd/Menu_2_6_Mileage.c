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
#include <stdlib.h>
#include "LCD_Driver.h"
#include "jt808.h"
#include "jt808_gps.h"

static uint8_t  page = 0;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void display( void )
{
    char buf[32];
    lcd_fill( 0 );
    /*
    sprintf( buf, "20%02d/%02d/%02d %02d:%02d",
             YEAR( mytime_now ),
             MONTH( mytime_now ),
             DAY( mytime_now ),
             HOUR( mytime_now ),
             MINUTE( mytime_now ));
    lcd_text12( 0, 4, (char*)buf, strlen( buf ), LCD_MODE_SET );
    */
    switch( page )
    {
    case 0:
        lcd_text12( 0, 4, "BD/GPS统计里程", 14, LCD_MODE_SET );
        sprintf( buf, "里程:%06d.%02d公里", jt808_param_bk.car_mileage / 36000 % 10000000, jt808_param_bk.car_mileage / 36 % 1000 / 10 );
        lcd_text12( 0, 16, (char *)buf, strlen( buf ), LCD_MODE_SET );
        break;
    case 1:
        lcd_text12( 0, 4, "脉冲线统计里程", 14, LCD_MODE_SET );
        sprintf( buf, "里程:%06d.%02d公里", jt808_param_bk.mileage_pulse / 1000 % 10000000, jt808_param_bk.mileage_pulse % 1000 / 10 );
        lcd_text12( 0, 16, (char *)buf, strlen( buf ), LCD_MODE_SET );
        break;
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
    char buf[32];
    pMenuItem->tick = rt_tick_get();
    page = 0;
    display();
}

/**/
static void keypress( unsigned int key )
{
    switch( key )
    {
    case KEY_MENU:
        pMenuItem = &Menu_4_recorder;
        pMenuItem->show( );
        break;
    case KEY_OK:
        break;
    case KEY_UP:
    case KEY_DOWN:
        page ^= BIT(0);
        display();
        break;
    }

}

/*检查是否回到主界面*/
static void timetick( unsigned int tick )
{
    static uint32_t last_tick = 0;
    MENUITEM *tmp;
    if( ( tick - pMenuItem->tick ) >= RT_TICK_PER_SECOND * 60 )
    {
        if( pMenuItem->parent != (void *)0 )
        {
            tmp = pMenuItem->parent;
            pMenuItem->parent = (void *)0;
            pMenuItem = tmp;
        }
        else
        {
            pMenuItem = &Menu_1_Idle;
        }
        pMenuItem->show( );
    }
    else if( ( tick - last_tick ) >= RT_TICK_PER_SECOND)
    {
        last_tick = tick;
        display();
    }
}

MENUITEM Menu_2_6_Mileage =
{
    "里程信息查看",
    12,				  0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};
/************************************** The End Of File **************************************/
