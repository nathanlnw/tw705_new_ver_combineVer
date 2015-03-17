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
        lcd_text12( 0, 4, "BD/GPSͳ�����", 14, LCD_MODE_SET );
        sprintf( buf, "���:%06d.%02d����", jt808_param_bk.car_mileage / 36000 % 10000000, jt808_param_bk.car_mileage / 36 % 1000 / 10 );
        lcd_text12( 0, 16, (char *)buf, strlen( buf ), LCD_MODE_SET );
        break;
    case 1:
        lcd_text12( 0, 4, "������ͳ�����", 14, LCD_MODE_SET );
        sprintf( buf, "���:%06d.%02d����", jt808_param_bk.mileage_pulse / 1000 % 10000000, jt808_param_bk.mileage_pulse % 1000 / 10 );
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

/*����Ƿ�ص�������*/
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
    "�����Ϣ�鿴",
    12,				  0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};
/************************************** The End Of File **************************************/
