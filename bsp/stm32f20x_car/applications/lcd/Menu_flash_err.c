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
#include "sed1520.h"

///功能为清空参数和数据

static uint8_t	file_menu1 = 1; 		/*当前显示的界面状态*/
static uint8_t	file_menu2 = 0; 		/*当前显示的界面状态*/


/**/
static void display( void )
{
    lcd_fill( 0 );
    if(file_menu2 == 1)
    {
        lcd_text12( 12,  11, "按确定键重启车台", 16, 1 );
        lcd_text12( 24,  11, "确定", 4, 3 );
    }
    else if(file_menu2 == 2)
    {
        lcd_text12( 21, 11, "车台重启中...", 13, LCD_MODE_SET );
        lcd_update_all( );
        reset(7);
        return;
    }
    else if(file_menu2 == 3)
    {
        lcd_text12( 0,  11, "按确定键恢复出厂设置", 20, 1 );
        lcd_text12( 12,  11, "确定", 4, 3 );
    }
    else if(file_menu2 == 4)
    {
        lcd_text12( 9, 11, "恢复出厂设置中...", 17, LCD_MODE_SET );
        lcd_update_all( );
        factory(3);
        return;
    }
    else
    {
        file_menu2 = 0;
        lcd_text12( 18,  4, "车台存储器错误", 14, 1);
        lcd_text12( 9, 18, "请断电后重启车台!", 17, 1 );
    }

    lcd_update_all( );
}

static void msg( void *p )
{
    pMenuItem->tick = rt_tick_get( );
}

/**/
static void show( void )
{
    pMenuItem->tick = rt_tick_get( );
    file_menu1		= 1;
    file_menu2		= 0;
    display( );
}

/*按键处理，拍照或上传过程中如何判断?*/
static void keypress( unsigned int key )
{
    switch( key )
    {
    case KEY_MENU:
        if(file_menu2 == 0 )
        {
            file_menu2 = 3;
        }
        else
        {
            file_menu2 = 0;
        }
        break;
    case KEY_OK:
        if(file_menu2 == 0)
        {
            file_menu2 = 1;
        }
        else
        {
            if(file_menu2 == 1)
            {
                file_menu2 = 2;
            }
            else if(file_menu2 == 3)
            {
                file_menu2 = 4;
            }
            else
            {
                file_menu2 = 0;
            }
        }
        break;
    case KEY_UP:
    case KEY_DOWN:
        file_menu2 = 0;
        break;
    }
    display( );
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
    static uint8_t power_off = 0;
    if( ( tick - pMenuItem->tick ) >= RT_TICK_PER_SECOND * 300 )
    {
        reset(7);
    }
    else if( ( tick - pMenuItem->tick ) >= RT_TICK_PER_SECOND * 290 )
    {
        if(power_off == 0)
        {
            power_off = 1;
            gsmstate( GSM_POWEROFF );
            device_control.off_rf_counter = 10000;
        }
    }
}


MENUITEM Menu_flash_err =
{
    "",
    0,				  0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/


