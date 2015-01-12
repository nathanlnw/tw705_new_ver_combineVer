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

extern uint8_t tel_state;
extern uint8_t tel_phonenum[16];


u8 tel_screen = 0;


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
    pMenuItem->tick = rt_tick_get();

    lcd_fill( 0 );
    lcd_text12( 20, 0, "һ���ز�����", 12, LCD_MODE_SET );
    lcd_text12( 20, 16, jt808_param.id_0x0040, strlen(jt808_param.id_0x0040), LCD_MODE_SET );
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
    switch( key )
    {
    case KEY_MENU:
        pMenuItem = &Menu_5_other;
        pMenuItem->show( );
        break;
    case KEY_OK:
        if( tel_state == 0 )
        {
            //OneKeyCallFlag = 1;

            lcd_fill( 0 );
            lcd_text12( 42, 10, "�ز���", 6, LCD_MODE_SET );
            lcd_update_all( );
            //---------  һ������------

            tel_state = 1;
            memcpy(tel_phonenum, jt808_param.id_0x0040, 16);
            /*OneKeyCallFlag=1;
               One_largeCounter=0;
               One_smallCounter=0;*/
#if NEED_TODO
            if( DataLink_Status( ) && ( CallState == CallState_Idle ) ) //�绰���������������
            {
                Speak_ON;                                               //��������
                rt_kprintf( "\n  һ���ز�(��������)-->��ͨͨ��\n" );
            }
            CallState = CallState_rdytoDialLis;                         // ׼����ʼ�����������
#endif
        }

        break;
    case KEY_UP:
        break;
    case KEY_DOWN:
        break;
    }
}


MENUITEM Menu_5_2_TelAtd =
{
    "һ���ز�",
    8, 0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/