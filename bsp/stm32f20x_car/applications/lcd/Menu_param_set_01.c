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

static const uint8_t	menu_num = 3;

static int16_t 	pos;
static uint8_t	file_menu1 = 1; 		/*��ǰ��ʾ�Ľ���״̬*/
static uint8_t	file_menu2 = 0; 		/*��ǰ��ʾ�Ľ���״̬*/

typedef struct
{
    uint8_t 		left_val;
    uint8_t 		top_val;
    char			*caption;	/*�˵����������Ϣ*/
} TYPE_MENU;

TYPE_MENU	lcd_menu[menu_num] =
{
    {0,		4,	"��ղ���"},
    {0,		18,	"�������"},
    {60,	4,	"��̨��λ"},
    //{60,	18,	"�������"},
};

/**/
static void display( void )
{
    uint8_t lcd_mode, i;

    lcd_fill( 0 );
    if(file_menu2 == 0)
    {
        for(i = 0; i < menu_num; i++)
        {
            if(file_menu1 - 1 == i)
            {
                lcd_mode = 3;
            }
            else
            {
                lcd_mode = 1;
            }
            lcd_text12( lcd_menu[i].left_val,  lcd_menu[i].top_val, lcd_menu[i].caption, strlen(lcd_menu[i].caption), lcd_mode);
        }
        //lcd_text12( 5,  4, "1.��ղ���", 10, 3 - (file_menu1-1) * 2 );
        //lcd_text12( 5, 18, "2.�������", 10, (file_menu1-1) * 2 + 1 );
    }
    else if(file_menu1 == 1)
    {
        if(file_menu2 == 1)
            lcd_text12( 10, 10, "��ȷ�ϼ���ղ���", 16, LCD_MODE_SET );
        else
            lcd_text12( 20, 10, "�����Ѿ����", 12, LCD_MODE_SET );
    }
    else if(file_menu1 == 2)
    {
        if(file_menu2 == 1)
            lcd_text12( 10, 10, "��ȷ�ϼ��������", 16, LCD_MODE_SET );
        else
        {
            lcd_text12( 20, 10, "�����Ѿ����", 12, LCD_MODE_SET );
            lcd_update_all( );
            reset(5);
        }
    }
    else if(file_menu1 == 3)
    {
        if(file_menu2 == 1)
            lcd_text12( 10, 10, "��ȷ�ϼ���λ��̨", 16, LCD_MODE_SET );
        else
        {
            lcd_text12( 18, 10, "��̨��λ��....", 14, LCD_MODE_SET );
            lcd_update_all( );
            reset(6);
        }
    }
    else
    {
        file_menu1 = 1;
        file_menu2 = 0;
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
    pos				= 0;
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
        if(file_menu2)
        {
            show();
        }
        else if(device_unlock)
        {
            pMenuItem = &Menu_1_menu;
            pMenuItem->show( );
        }
        else
        {
            CarSet_0_counter = 0;
            pMenuItem = &Menu_0_0_password;//Menu_0_loggingin;//
            pMenuItem->show( );
        }
        break;
    case KEY_OK:
        if(file_menu2 == 0)
        {
            file_menu2 = 1;
        }
        else if(file_menu1 == 1)
        {
            if(file_menu2 == 1)
            {
                factory_ex(1, 0);
                file_menu2 = 2;
            }
        }
        else if(file_menu1 == 2)
        {
            if(file_menu2 == 1)
            {
                lcd_fill( 0 );
                lcd_text12( 20, 10, "���������...", 13, LCD_MODE_SET );
                lcd_update_all( );
                factory_ex(2, 0);
                pMenuItem->tick = rt_tick_get();
                file_menu2 = 2;
            }
        }
        else if(file_menu1 == 3)
        {
            if(file_menu2 == 1)
            {
                file_menu2 = 2;
            }
        }
        else if(file_menu1 == 4)
        {
            jt808_tx_lock(mast_socket->linkno);
        }
        else
        {
            file_menu1 = 1;
            file_menu2 = 0;
        }
        break;
    case KEY_UP:
        if(file_menu2 == 0)
        {
            file_menu1++;
            if(file_menu1 > menu_num)
                file_menu1 = 1;
        }
        else if(file_menu1 == 1)
        {
            ;
        }
        else if(file_menu1 == 2)
        {
            ;
        }
        else
        {
            file_menu1 = 1;
            file_menu2 = 0;
        }
        break;
    case KEY_DOWN:
        if(file_menu2 == 0)
        {
            file_menu1--;
            if((file_menu1 > menu_num) || (file_menu1 == 0))
                file_menu1 = menu_num;
        }
        else if(file_menu1 == 1)
        {
            ;
        }
        else if(file_menu1 == 2)
        {
            ;
        }
        else
        {
            file_menu1 = 1;
            file_menu2 = 0;
        }
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
    if(device_unlock)
    {
        timetick_default(tick);
    }
}


MENUITEM Menu_param_set_01 =
{
    "��ղ���������",
    14,				  0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/

