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
#include "sed1520.h"

///����Ϊ��ղ���������

static uint8_t	file_menu1 = 1; 		/*��ǰ��ʾ�Ľ���״̬*/
static uint8_t	file_menu2 = 0; 		/*��ǰ��ʾ�Ľ���״̬*/


/**/
static void display( void )
{
    lcd_fill( 0 );
    if(file_menu2 == 1)
    {
        lcd_text12( 12,  11, "��ȷ����������̨", 16, 1 );
        lcd_text12( 24,  11, "ȷ��", 4, 3 );
    }
    else if(file_menu2 == 2)
    {
        lcd_text12( 21, 11, "��̨������...", 13, LCD_MODE_SET );
        lcd_update_all( );
        reset(7);
        return;
    }
    else if(file_menu2 == 3)
    {
        lcd_text12( 0,  11, "��ȷ�����ָ���������", 20, 1 );
        lcd_text12( 12,  11, "ȷ��", 4, 3 );
    }
    else if(file_menu2 == 4)
    {
        lcd_text12( 9, 11, "�ָ�����������...", 17, LCD_MODE_SET );
        lcd_update_all( );
        factory(3);
        return;
    }
    else
    {
        file_menu2 = 0;
        lcd_text12( 18,  4, "��̨�洢������", 14, 1);
        lcd_text12( 9, 18, "��ϵ��������̨!", 17, 1 );
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

/*�����������ջ��ϴ�����������ж�?*/
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


