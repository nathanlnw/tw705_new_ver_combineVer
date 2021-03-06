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
#include <stdio.h>
#include <string.h>
#include "LCD_Driver.h"


unsigned char	Menu_Logout			= 0;    //  1:准备注册再按确认键=2   =2:发送注册
unsigned char	LogInorOut_screen	= 0;    //选择鉴权/注销,选择好以后按确认键清0
unsigned char	LogInorOut			= 0;    //  1:选择鉴权	2:选择注册

extern uint8_t jt808_tx_register();
extern uint8_t jt808_tx_auth();

/**/
void confirm_login( unsigned char par )
{
    lcd_fill( 0 );
    if( par == 1 )
    {
        lcd_text12( 0, 10, "清空鉴权码", 10, LCD_MODE_INVERT );
        lcd_text12( 60, 10, " 鉴权 注册", 10, LCD_MODE_SET );
    }
    else if( par == 2 )
    {
        lcd_text12( 0, 10, "清空鉴权码", 10, LCD_MODE_SET );
        lcd_text12( 66, 10, "鉴权", 4, LCD_MODE_INVERT );
        lcd_text12( 96, 10, "注册", 4, LCD_MODE_SET );
    }
    else if( par == 3 )
    {
        lcd_text12( 0, 10, "清空鉴权码 鉴权 ", 16, LCD_MODE_SET );
        lcd_text12( 96, 10, "注册", 4, LCD_MODE_INVERT );
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
    confirm_login( 1 );
    LogInorOut_screen	= 1;
    LogInorOut			= 1;
}

/**/
static void keypress( unsigned int key )
{
    switch( key )
    {
    case KEY_MENU:
        if(Menu_Logout)
        {
            show();
        }
        else
        {
            pMenuItem = &Menu_3_InforInteract;
            pMenuItem->show( );

            LogInorOut_screen	= 0;
            LogInorOut			= 0;    //	1:鉴权	 2:注销
        }
        Menu_Logout = 0;
        break;
    case KEY_OK:
        if( LogInorOut == 1 )       //清空鉴权码
        {
            LogInorOut_screen	= 0;
            LogInorOut			= 0;
            lcd_fill( 0 );
            lcd_text12( 6, 10, "按确认键清空鉴权码", 18, LCD_MODE_SET );
            lcd_update_all( );
            Menu_Logout = 1;
        }
        else if( Menu_Logout == 1 )
        {
            Menu_Logout = 0;
            jt808_param_bk.updata_utctime = 0;
            memset( jt808_param.id_0xF003, 0, 32 );
            param_save(1);
            //gsmstate( GSM_POWEROFF );
            jt808_add_tx( mast_socket->linkno, SINGLE_FIRST, 0x0003, -1, NULL, NULL, NULL, NULL, NULL );
            lcd_fill( 0 );
            lcd_text12( 24, 10, "鉴权码已清空", 12, LCD_MODE_SET );
            lcd_update_all( );
        }
        else if( LogInorOut == 2 )  //鉴权
        {
            LogInorOut_screen	= 0;
            LogInorOut			= 0;
            lcd_fill( 0 );
            lcd_text12( 0, 10, "按确认键发送车台鉴权", 20, LCD_MODE_SET );
            lcd_update_all( );
            Menu_Logout = 2;
        }
        else if( Menu_Logout == 2 )  //鉴权已发送
        {
            Menu_Logout = 0;
            jt808_tx_auth();
            lcd_fill( 0 );
            lcd_text12( 30, 10, "鉴权已发送", 10, LCD_MODE_SET );
            lcd_update_all( );

        }
        else if( LogInorOut == 3 )  //注册
        {
            LogInorOut_screen	= 0;
            LogInorOut			= 0;
            lcd_fill( 0 );
            lcd_text12( 0, 10, "按确认键发送车台注册", 20, LCD_MODE_SET );
            lcd_update_all( );
            Menu_Logout = 3;
        }
        else if( Menu_Logout == 3 )
        {
            Menu_Logout = 0;
            jt808_tx_register();
            lcd_fill( 0 );
            lcd_text12( 30, 10, "注册已发送", 10, LCD_MODE_SET );
            lcd_update_all( );
        }
        break;
    case KEY_UP:
        if( LogInorOut_screen == 1 )
        {
            LogInorOut--;
            if( LogInorOut <= 1 )
            {
                LogInorOut = 1;
            }
            confirm_login( LogInorOut );
        }
        break;

    case KEY_DOWN:
        if( LogInorOut_screen == 1 )
        {
            LogInorOut++;
            if( LogInorOut >= 3 )
            {
                LogInorOut = 3;
            }
            confirm_login( LogInorOut );
        }
        break;
    }
}

MENUITEM Menu_3_8_LogOut =
{
    "鉴权注册",
    8,				  0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/
