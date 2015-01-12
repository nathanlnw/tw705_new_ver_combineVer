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

static uint8_t fsend; /*�Ƿ��ѷ���*/


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
    lcd_text12( 0, 10, "��[ȷ��]���͵����˵�", 20, LCD_MODE_SET );
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
            sprintf(buf + 4, "�����˵���ʱ��=%02d-%02d-%02d %02d:%02d:%02d",
                    YEAR( mytime_now ),
                    MONTH( mytime_now ),
                    DAY( mytime_now ),
                    HOUR( mytime_now ),
                    MINUTE( mytime_now ),
                    SEC( mytime_now ) );

            buf[3] = strlen(buf + 4);
            //memcpy(buf+4,"�����˵�",8);
            //buf[4]='1';
            jt808_tx(1, 0x0701, buf, buf[3] );
            lcd_fill( 0 );
            lcd_text12( 10, 10, "�����˵��ϱ����", 16, LCD_MODE_SET );
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
    "�����˵�����",
    12,				  0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/