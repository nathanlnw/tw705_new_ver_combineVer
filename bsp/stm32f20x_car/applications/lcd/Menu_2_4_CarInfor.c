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
#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

static unsigned char page;

//static car_calor

/*北斗使用两路AD检测 PA1 喇叭 PC3 左转*/
const char *car_color[4] = { "蓝色", "黄色", "黑色", "白色"};


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
    switch( page )
    {
    case 0:
        /*
        sprintf( buf, "车 牌 号:%s", jt808_param.id_0x0083 );
        lcd_text12( 0, 4, (char*)buf, strlen( buf ), LCD_MODE_SET );
        sprintf( buf, "车辆类型:%s", jt808_param.id_0xF00A );
        lcd_text12( 0, 18, (char*)buf, strlen( buf ), LCD_MODE_SET );
        */


        if(jt808_param.id_0x0084 == 0)
        {
            sprintf( buf, "车牌号:未 上 牌", jt808_param.id_0x0083 );
        }
        else if(jt808_param.id_0x0084 >= 5)
        {
            sprintf( buf, "车牌号:%s", jt808_param.id_0x0083 );
        }
        else
        {
            sprintf( buf, "车牌号:%s,%s", jt808_param.id_0x0083, car_color[jt808_param.id_0x0084 - 1]);
        }
        lcd_text12( 0, 4, (char *)buf, strlen( buf ), LCD_MODE_SET );
        sprintf( buf, "车辆类型:%s", jt808_param.id_0xF00A );
        lcd_text12( 0, 18, (char *)buf, strlen( buf ), LCD_MODE_SET );
        break;
    case 1:
        sprintf( buf, "入网ID:%s", jt808_param.id_0xF006);
        lcd_text12( 0, 4, (char *)buf, strlen( buf ), LCD_MODE_SET );
        sprintf( buf, "[%s]", jt808_param.id_0xF005 );
        lcd_text12( 0, 18, (char *)buf, strlen( buf ), LCD_MODE_SET );
        break;
    case 2:
        if(jt808_param.id_0xF00D == 1)
            sprintf( buf, "连接模式:两客一危" );
        else
            sprintf( buf, "连接模式:货运模式" );
        lcd_text12( 0, 4, (char *)buf, strlen( buf ), LCD_MODE_SET );

        break;
    }
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
    page			= 0;
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
static void keypress( unsigned int key )
{
    switch( key )
    {
    case KEY_MENU:
        pMenuItem = &Menu_4_recorder;
        pMenuItem->show( );
    case KEY_OK:
        break;
    case KEY_UP:
        if( page )
            page--;
        else
            page = 2;
        display( );
        break;
    case KEY_DOWN:
        page++;
        if(page > 2)
            page = 0;
        display( );
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
    else if( ( tick - last_tick ) >= RT_TICK_PER_SECOND / 2)
    {
        last_tick = tick;
        display();
    }
}

MENUITEM Menu_2_4_CarInfor =
{
    "车辆信息查看",
    12,				  0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/
