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
#include "LCD_Driver.h"

static uint8_t fsend; /*是否已发送*/


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void msg( void *p )
{
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
static void show( void )
{
    pMenuItem->tick = rt_tick_get( );
    fsend			= 0;
    lcd_fill( 0 );
    lcd_text12( 0, 10, "按[确认]发送电子运单", 20, LCD_MODE_SET );
    lcd_update_all( );
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
static void keypress( unsigned int key )
{
    char buf[64];
    switch( key )
    {
    case KEY_MENU:
        pMenuItem = &Menu_3_InforInteract;
        pMenuItem->show( );
        break;
    case KEY_OK:
        fsend++;
        if( fsend == 1 )
        {
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = 0;
            buf[3] = 8;
            sprintf(buf + 4, "电子运单，时间=%02d-%02d-%02d %02d:%02d:%02d",
                    YEAR( mytime_now ),
                    MONTH( mytime_now ),
                    DAY( mytime_now ),
                    HOUR( mytime_now ),
                    MINUTE( mytime_now ),
                    SEC( mytime_now ) );

            buf[3] = strlen(buf + 4);
            //memcpy(buf+4,"电子运单",8);
            //buf[4]='1';
            jt808_tx(1, 0x0701, buf, buf[3] );
            lcd_fill( 0 );
            lcd_text12( 10, 10, "电子运单上报完成", 16, LCD_MODE_SET );
            lcd_update_all( );
        }
        else
        {
            pMenuItem = &Menu_3_InforInteract;
            pMenuItem->show( );
        }
        break;
    case KEY_UP:
        break;
    case KEY_DOWN:
        break;
    }
}

MENUITEM Menu_3_3_ElectronicInfor =
{
    "电子运单发送",
    12,				  0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/
