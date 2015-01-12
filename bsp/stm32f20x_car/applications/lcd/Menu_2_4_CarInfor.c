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
#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

static unsigned char page;

//static car_calor

/*����ʹ����·AD��� PA1 ���� PC3 ��ת*/
const char *car_color[4] = { "��ɫ", "��ɫ", "��ɫ", "��ɫ"};


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
        sprintf( buf, "�� �� ��:%s", jt808_param.id_0x0083 );
        lcd_text12( 0, 4, (char*)buf, strlen( buf ), LCD_MODE_SET );
        sprintf( buf, "��������:%s", jt808_param.id_0xF00A );
        lcd_text12( 0, 18, (char*)buf, strlen( buf ), LCD_MODE_SET );
        */


        if(jt808_param.id_0x0084 == 0)
        {
            sprintf( buf, "���ƺ�:δ �� ��", jt808_param.id_0x0083 );
        }
        else if(jt808_param.id_0x0084 >= 5)
        {
            sprintf( buf, "���ƺ�:%s", jt808_param.id_0x0083 );
        }
        else
        {
            sprintf( buf, "���ƺ�:%s,%s", jt808_param.id_0x0083, car_color[jt808_param.id_0x0084 - 1]);
        }
        lcd_text12( 0, 4, (char *)buf, strlen( buf ), LCD_MODE_SET );
        sprintf( buf, "��������:%s", jt808_param.id_0xF00A );
        lcd_text12( 0, 18, (char *)buf, strlen( buf ), LCD_MODE_SET );
        break;
    case 1:
        sprintf( buf, "����ID:%s", jt808_param.id_0xF006);
        lcd_text12( 0, 4, (char *)buf, strlen( buf ), LCD_MODE_SET );
        sprintf( buf, "[%s]", jt808_param.id_0xF005 );
        lcd_text12( 0, 18, (char *)buf, strlen( buf ), LCD_MODE_SET );
        break;
    case 2:
        if(jt808_param.id_0xF00D == 1)
            sprintf( buf, "����ģʽ:����һΣ" );
        else
            sprintf( buf, "����ģʽ:����ģʽ" );
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
    else if( ( tick - last_tick ) >= RT_TICK_PER_SECOND / 2)
    {
        last_tick = tick;
        display();
    }
}

MENUITEM Menu_2_4_CarInfor =
{
    "������Ϣ�鿴",
    12,				  0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/
