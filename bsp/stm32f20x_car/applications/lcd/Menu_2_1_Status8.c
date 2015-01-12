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
#include "sed1520.h"

#include "jt808_vehicle.h"

/*����ʹ����·AD��� PA1 ���� PC3 ��ת*/
char			 *caption[8] = { "����", "����", "Զ��", "����", "��ת", "��ת", "ɲ��", "��ˢ" };
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
        sprintf(buf, "��ѹ %d [%d-%d]", AD_Volte, AD_Volte_Min, AD_Volte_Max);
        lcd_text12( 0, 4, buf, strlen(buf), LCD_MODE_SET );
    }
    else if( page == 2 )
    {
        sprintf(buf, "���� %d ", Frequency);
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
    "�ź���״̬",
    10,			 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/