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
#include "LCD_Driver.h"

#include <string.h>

#if 1

static uint8_t			count, pos;

static uint8_t			 *ptr_info_ondemand = RT_NULL;

static unsigned char	check[] =
{
    0xff,                                                                       /*[********]*/
    0xc3,                                                                       /*[**    **]*/
    0xa5,                                                                       /*[* *  * *]*/
    0x99,                                                                       /*[*  **  *]*/
    0x99,                                                                       /*[*  **  *]*/
    0xa5,                                                                       /*[* *  * *]*/
    0xc3,                                                                       /*[**    **]*/
    0xff,                                                                       /*[********]*/
};

static unsigned char	uncheck[] =
{
    0xff,                                                                       /*[********]*/
    0x81,                                                                       /*[*      *]*/
    0x81,                                                                       /*[*      *]*/
    0x81,                                                                       /*[*      *]*/
    0x81,                                                                       /*[*      *]*/
    0x81,                                                                       /*[*      *]*/
    0x81,                                                                       /*[*      *]*/
    0xff,                                                                       /*[********]*/
};

static uint8_t seting_info = 0;
static uint8_t info_is_set = 0;


DECL_BMP( 8, 8, check );
DECL_BMP( 8, 8, uncheck );

/*��ʾ*/
static void display( void )
{
    char			buf[32];
    INFO_ONDEMAND	 *info;
    uint8_t			index = pos & 0xFE; /*���뵽ż��ҳ*/
    lcd_fill( 0 );
    if( count == 0 )
    {
        lcd_text12( ( 122 - 8 * 12 ) / 2, 18, "[����Ϣ�㲥����]", 16, LCD_MODE_SET );
    }
    else
    {
        if(seting_info == 0)
        {
            info = (INFO_ONDEMAND *)( ptr_info_ondemand + 64 * index );
            sprintf( buf, "%02d.%s", index + 1, info->body );
            lcd_text12( 0, 4, buf, strlen( buf ), 3 - ( pos & 0x01 ) * 2 );         /*SET=1 INVERT=3*/
            if(info->st)
            {
                lcd_text12( 96, 4, "�㲥", 4, 3 - ( pos & 0x01 ) * 2 );         /*SET=1 INVERT=3*/
            }
            else
            {
                lcd_text12( 96, 4, "ȡ��", 4, 3 - ( pos & 0x01 ) * 2 );         /*SET=1 INVERT=3*/
            }
            if( ( index + 1 ) < count )
            {
                info = (INFO_ONDEMAND *)( ptr_info_ondemand + 64 * index + 64 );
                sprintf( buf, "%02d.%s", index + 2, info->body );
                lcd_text12( 0, 18, buf, strlen( buf ), ( pos & 0x01 ) * 2 + 1 );    /*SET=1 INVERT=3*/
                if(info->st)
                {
                    lcd_text12( 96, 18, "�㲥", 4, ( pos & 0x01 ) * 2 + 1 );         /*SET=1 INVERT=3*/
                }
                else
                {
                    lcd_text12( 96, 18, "ȡ��", 4, ( pos & 0x01 ) * 2 + 1 );         /*SET=1 INVERT=3*/
                }
            }
        }
        else
        {
            info = (INFO_ONDEMAND *)( ptr_info_ondemand + 64 * pos );
            sprintf( buf, "%02d.%s", pos + 1, info->body );
            lcd_text12( 0, 4, buf, strlen( buf ), 1 );         						/*SET=1 INVERT=3*/
            lcd_text12( 16, 18, "�㲥", 4, ( info_is_set & 0x01 ) * 2 + 1 );         /*SET=1 INVERT=3*/
            lcd_text12( 80, 18, "ȡ��", 4, 3 - ( info_is_set & 0x01 ) * 2  );         /*SET=1 INVERT=3*/
        }
    }
    lcd_update_all( );
}

/**/
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
    rt_free( ptr_info_ondemand );
    //ptr_info_ondemand = RT_NULL;
    ptr_info_ondemand	= jt808_info_ondemand_get( &count );
    rt_kprintf( "count=%d\n", count );
    pos = 0;
    info_is_set = 0;
    seting_info = 0;
    display( );
}

/**/
static void keypress( unsigned int key )
{
    uint8_t buf[32];
    INFO_ONDEMAND	 *info;
    if( count == 0 ) /*û�м�¼,���������*/
    {
        pMenuItem = &Menu_2_InforCheck;
        pMenuItem->show( );
    }
    switch( key )
    {
    case KEY_MENU:
        if( seting_info )
        {
            seting_info = 0;
            display( );
        }
        else
        {
            if( ptr_info_ondemand != RT_NULL )
            {
                rt_free( ptr_info_ondemand );
                ptr_info_ondemand = RT_NULL;
            }
            pMenuItem = &Menu_2_InforCheck;
            pMenuItem->show( );
        }
        break;
    case KEY_OK: /*�¼�����*/
        if(count)
        {
            info = (INFO_ONDEMAND *)( ptr_info_ondemand + 64 * pos );
            if(seting_info == 0)
            {
                seting_info = 1;
                info_is_set = info->st;
            }
            else
            {
                seting_info = 0;
                if(info->st != info_is_set)
                {
                    info->st = info_is_set;
                    jt808_info_ondemand_put(ptr_info_ondemand, count * sizeof(INFO_ONDEMAND));
                }

                buf[0]	= ( (INFO_ONDEMAND *)( ptr_info_ondemand + pos * 64 ) )->type;
                buf[1]	= ( (INFO_ONDEMAND *)( ptr_info_ondemand + pos * 64 ) )->st;
                jt808_tx(1, 0x0303, buf, 2 );
            }
            display( );
        }
        break;
    case KEY_UP:
        if(seting_info == 0)
        {
            if( pos )
            {
                pos--;
            }
        }
        else
        {
            info_is_set ^= 0x01;
        }
        display( );
        break;
    case KEY_DOWN:

        if(seting_info == 0)
        {
            if(( pos < ( count - 1 ) ) && (count))
            {
                pos++;
            }
        }
        else
        {
            info_is_set ^= 0x01;
        }
        display( );
        break;
    }
}

/*����Ƿ�ص�������*/
static void timetick( unsigned int tick )
{
    if( ( tick - pMenuItem->tick ) >= RT_TICK_PER_SECOND * 30 )
    {
        if( ptr_info_ondemand != RT_NULL )
        {
            rt_free( ptr_info_ondemand );
            ptr_info_ondemand = RT_NULL;
        }
        pMenuItem = &Menu_1_Idle;
        pMenuItem->show( );
    }
}

MENUITEM Menu_2_7_RequestProgram =
{
    "��Ϣ�㲥�鿴",
    12,			   0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

#else
unsigned char	Menu_dianbo			= 0;
unsigned char	dianbo_scree		= 1;
unsigned char	MSG_TypeToCenter	= 0; //���͸����ĵ����


/*
   typedef struct _MSG_BROADCAST
   {
   unsigned char	 INFO_TYPE;     //	��Ϣ����
   unsigned int     INFO_LEN;		//	��Ϣ����
   unsigned char	 INFO_PlyCancel; // �㲥/ȡ����־	   0 ȡ��  1  �㲥
   unsigned char	 INFO_SDFlag;	 //  ���ͱ�־λ
   unsigned char	 INFO_Effective; //  ��ʾ�Ƿ���Ч	1 ��ʾ��Ч	  0  ��ʾ��Ч
   unsigned char	 INFO_STR[30];	//	��Ϣ����
   }MSG_BRODCAST;

   MSG_BRODCAST	 MSG_Obj;	 // ��Ϣ�㲥
   MSG_BRODCAST	 MSG_Obj_8[8];	// ��Ϣ�㲥

 */
void SenddianboMeun( unsigned char screen, unsigned char SendOK )
{
#if NEED_TODO
    if( SendOK == 1 )
    {
        MSG_Obj_8[screen - 1].INFO_TYPE = screen;
        if( MSG_Obj_8[screen - 1].INFO_Effective )
        {
            lcd_fill( 0 );
            lcd_text12( 30, 3, (char *)MSG_Obj_8[screen - 1].INFO_STR, MSG_Obj_8[screen - 1].INFO_LEN, LCD_MODE_INVERT );
            if( MSG_Obj_8[screen - 1].INFO_PlyCancel == 1 )
            {
                lcd_text12( 30, 19, "�㲥�ɹ�", 8, LCD_MODE_INVERT );
            }
            else if( MSG_Obj_8[screen - 1].INFO_PlyCancel == 0 )
            {
                lcd_text12( 30, 19, "ȡ���ɹ�", 8, LCD_MODE_INVERT );
            }
            lcd_update_all( );
        }

        MSG_TypeToCenter = screen; //���͸����ĵ㲥��Ϣ�����
    }
    else
    {
        if( ( screen >= 1 ) && ( screen <= 8 ) )
        {
            if( MSG_Obj_8[screen - 1].INFO_Effective )
            {
                lcd_fill( 0 );
                lcd_text12( 15, 5, (char *)MSG_Obj_8[screen - 1].INFO_STR, MSG_Obj_8[screen - 1].INFO_LEN, LCD_MODE_INVERT );
                if( MSG_Obj_8[screen - 1].INFO_PlyCancel )
                {
                    lcd_text12( 15, 20, "�㲥", 4, LCD_MODE_INVERT );
                    lcd_text12( 80, 20, "ȡ��", 4, LCD_MODE_SET );
                }
                else
                {
                    lcd_text12( 15, 20, "�㲥", 4, LCD_MODE_SET );
                    lcd_text12( 80, 20, "ȡ��", 4, LCD_MODE_INVERT );
                }
                lcd_update_all( );
            }
        }
    }
#endif
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
void Dis_dianbo( unsigned char screen )
{
#if NEED_TODO
    switch( screen )
    {
    case 1:
        lcd_fill( 0 );
        lcd_text12( 0, 0, "1.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[0].INFO_Effective )
        {
            lcd_text12( 15, 0, (char *)MSG_Obj_8[0].INFO_STR, MSG_Obj_8[0].INFO_LEN, LCD_MODE_INVERT );
        }
        else
        {
            lcd_text12( 15, 0, "��", 2, LCD_MODE_INVERT );
        }

        lcd_text12( 0, 16, "2.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[1].INFO_Effective )
        {
            lcd_text12( 15, 16, (char *)MSG_Obj_8[1].INFO_STR, MSG_Obj_8[1].INFO_LEN, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 15, 16, "��", 2, LCD_MODE_SET );
        }

        if( MSG_Obj_8[0].INFO_Effective )
        {
            if( MSG_Obj_8[0].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 0, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 0, "ȡ��", 4, LCD_MODE_INVERT );
            }

            if( MSG_Obj_8[1].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 16, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 16, "ȡ��", 4, LCD_MODE_INVERT );
            }
        }
        lcd_update_all( );
        break;
    case 2:
        lcd_fill( 0 );
        lcd_text12( 0, 0, "1.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[0].INFO_Effective )
        {
            lcd_text12( 15, 0, (char *)MSG_Obj_8[0].INFO_STR, MSG_Obj_8[0].INFO_LEN, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 15, 0, "��", 2, LCD_MODE_SET );
        }
        lcd_text12( 0, 16, "2.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[1].INFO_Effective )
        {
            lcd_text12( 15, 16, (char *)MSG_Obj_8[1].INFO_STR, MSG_Obj_8[1].INFO_LEN, LCD_MODE_INVERT );
        }
        else
        {
            lcd_text12( 15, 16, "��", 2, LCD_MODE_INVERT );
        }
        if( MSG_Obj_8[1].INFO_Effective )
        {
            if( MSG_Obj_8[0].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 0, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 0, "ȡ��", 4, LCD_MODE_INVERT );
            }

            if( MSG_Obj_8[1].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 16, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 16, "ȡ��", 4, LCD_MODE_INVERT );
            }
        }
        lcd_update_all( );
        break;
    case 3:
        lcd_fill( 0 );
        lcd_text12( 0, 0, "3.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[2].INFO_Effective )
        {
            lcd_text12( 15, 0, (char *)MSG_Obj_8[2].INFO_STR, MSG_Obj_8[2].INFO_LEN, LCD_MODE_INVERT );
        }
        else
        {
            lcd_text12( 15, 0, "��", 2, LCD_MODE_INVERT );
        }
        lcd_text12( 0, 16, "4.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[3].INFO_Effective )
        {
            lcd_text12( 15, 16, (char *)MSG_Obj_8[3].INFO_STR, MSG_Obj_8[3].INFO_LEN, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 15, 16, "��", 2, LCD_MODE_SET );
        }
        if( MSG_Obj_8[2].INFO_Effective )
        {
            if( MSG_Obj_8[2].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 0, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 0, "ȡ��", 4, LCD_MODE_INVERT );
            }

            if( MSG_Obj_8[3].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 16, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 16, "ȡ��", 4, LCD_MODE_INVERT );
            }
        }
        lcd_update_all( );
        break;
    case 4:
        lcd_fill( 0 );
        lcd_text12( 0, 0, "3.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[2].INFO_Effective )
        {
            lcd_text12( 15, 0, (char *)MSG_Obj_8[2].INFO_STR, MSG_Obj_8[2].INFO_LEN, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 15, 0, "��", 2, LCD_MODE_SET );
        }
        lcd_text12( 0, 16, "4.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[3].INFO_Effective )
        {
            lcd_text12( 15, 16, (char *)MSG_Obj_8[3].INFO_STR, MSG_Obj_8[3].INFO_LEN, LCD_MODE_INVERT );
        }
        else
        {
            lcd_text12( 15, 16, "��", 2, LCD_MODE_INVERT );
        }
        if( MSG_Obj_8[3].INFO_Effective )
        {
            if( MSG_Obj_8[2].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 0, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 0, "ȡ��", 4, LCD_MODE_INVERT );
            }

            if( MSG_Obj_8[3].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 16, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 16, "ȡ��", 4, LCD_MODE_INVERT );
            }
        }
        lcd_update_all( );
        break;
    case 5:
        lcd_fill( 0 );
        lcd_text12( 0, 0, "5.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[4].INFO_Effective )
        {
            lcd_text12( 15, 0, (char *)MSG_Obj_8[4].INFO_STR, MSG_Obj_8[4].INFO_LEN, LCD_MODE_INVERT );
        }
        else
        {
            lcd_text12( 15, 0, "��", 2, LCD_MODE_INVERT );
        }
        lcd_text12( 0, 16, "6.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[5].INFO_Effective )
        {
            lcd_text12( 15, 16, (char *)MSG_Obj_8[5].INFO_STR, MSG_Obj_8[5].INFO_LEN, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 15, 16, "��", 2, LCD_MODE_SET );
        }
        if( MSG_Obj_8[4].INFO_Effective )
        {
            if( MSG_Obj_8[4].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 0, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 0, "ȡ��", 4, LCD_MODE_INVERT );
            }

            if( MSG_Obj_8[5].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 16, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 16, "ȡ��", 4, LCD_MODE_INVERT );
            }
        }
        lcd_update_all( );
        break;
    case 6:
        lcd_fill( 0 );
        lcd_text12( 0, 0, "5.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[4].INFO_Effective )
        {
            lcd_text12( 15, 0, (char *)MSG_Obj_8[4].INFO_STR, MSG_Obj_8[4].INFO_LEN, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 15, 0, "��", 2, LCD_MODE_SET );
        }
        lcd_text12( 0, 16, "6.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[5].INFO_Effective )
        {
            lcd_text12( 15, 16, (char *)MSG_Obj_8[5].INFO_STR, MSG_Obj_8[5].INFO_LEN, LCD_MODE_INVERT );
        }
        else
        {
            lcd_text12( 15, 16, "��", 2, LCD_MODE_INVERT );
        }
        if( MSG_Obj_8[5].INFO_Effective )
        {
            if( MSG_Obj_8[4].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 0, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 0, "ȡ��", 4, LCD_MODE_INVERT );
            }

            if( MSG_Obj_8[5].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 16, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 16, "ȡ��", 4, LCD_MODE_INVERT );
            }
        }
        lcd_update_all( );
        break;
    case 7:
        lcd_fill( 0 );
        lcd_text12( 0, 0, "7.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[6].INFO_Effective )
        {
            lcd_text12( 15, 0, (char *)MSG_Obj_8[6].INFO_STR, MSG_Obj_8[6].INFO_LEN, LCD_MODE_INVERT );
        }
        else
        {
            lcd_text12( 15, 0, "��", 2, LCD_MODE_INVERT );
        }
        lcd_text12( 0, 16, "8.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[7].INFO_Effective )
        {
            lcd_text12( 15, 16, (char *)MSG_Obj_8[7].INFO_STR, MSG_Obj_8[7].INFO_LEN, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 15, 16, "��", 2, LCD_MODE_SET );
        }
        if( MSG_Obj_8[6].INFO_Effective )
        {
            if( MSG_Obj_8[6].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 0, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 0, "ȡ��", 4, LCD_MODE_INVERT );
            }

            if( MSG_Obj_8[7].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 16, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 16, "ȡ��", 4, LCD_MODE_INVERT );
            }
        }
        lcd_update_all( );
        break;
    case 8:
        lcd_fill( 0 );
        lcd_text12( 0, 0, "7.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[6].INFO_Effective )
        {
            lcd_text12( 15, 0, (char *)MSG_Obj_8[6].INFO_STR, MSG_Obj_8[6].INFO_LEN, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 15, 0, "��", 2, LCD_MODE_SET );
        }
        lcd_text12( 0, 16, "8.", 2, LCD_MODE_SET );
        if( MSG_Obj_8[7].INFO_Effective )
        {
            lcd_text12( 15, 16, (char *)MSG_Obj_8[7].INFO_STR, MSG_Obj_8[7].INFO_LEN, LCD_MODE_INVERT );
        }
        else
        {
            lcd_text12( 15, 16, "��", 2, LCD_MODE_INVERT );
        }
        if( MSG_Obj_8[7].INFO_Effective )
        {
            if( MSG_Obj_8[6].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 0, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 0, "ȡ��", 4, LCD_MODE_INVERT );
            }

            if( MSG_Obj_8[7].INFO_PlyCancel == 1 )
            {
                lcd_text12( 95, 16, "�㲥", 4, LCD_MODE_INVERT );
            }
            else
            {
                lcd_text12( 95, 16, "ȡ��", 4, LCD_MODE_INVERT );
            }
        }
        lcd_update_all( );
        break;
    default:
        break;
    }
#endif
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
    //u8 i=0;
    //����8����Ϣ���ж���ʾ
#if NEED_TODO
    MSG_BroadCast_Read( );
#endif
    pMenuItem->tick = rt_tick_get( );

    Dis_dianbo( 1 );
    Menu_dianbo = 1;
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
    //u8 result=0;
    switch( key )
    {
    case KEY_MENU:
        pMenuItem = &Menu_2_InforCheck;
        pMenuItem->show( );
        CounterBack = 0;

        Menu_dianbo			= 0;
        dianbo_scree		= 1;
        MSG_TypeToCenter	= 0; //���͸����ĵ����
        break;
    case KEY_OK:
        if( Menu_dianbo == 1 )
        {
            Menu_dianbo = 2;
            //��ѡ�е����������ʾ���͵Ľ���
            SenddianboMeun( dianbo_scree, 0 );
        }
        else if( Menu_dianbo == 2 )
        {
            //SD_ACKflag.f_MsgBroadCast_0303H=1;


            /*MSG_Obj.INFO_TYPE=MSG_Obj_8[dianbo_scree-1].INFO_TYPE;
               MSG_Obj.INFO_PlyCancel=MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel;*/

            //--- �������µ�״̬ -----------
            //DF_WriteFlash(DF_Msg_Page+dianbo_scree-1, 0, (u8*)&MSG_Obj_8[dianbo_scree-1], sizeof(MSG_Obj_8[dianbo_scree-1]));

            //���ͳ�ȥ�Ľ���
            SenddianboMeun( dianbo_scree, 1 );

            Menu_dianbo		= 3;    //��ȷ�Ϸ�����Ϣ�鿴����
            dianbo_scree	= 1;
        }
        else if( Menu_dianbo == 3 )
        {
            Menu_dianbo		= 0;    //   1
            dianbo_scree	= 1;
        }
        break;
    case KEY_UP:
        if( Menu_dianbo == 1 )
        {
            dianbo_scree--;
            if( dianbo_scree <= 1 )
            {
                dianbo_scree = 1;
            }
            Dis_dianbo( dianbo_scree );
        }
        else if( Menu_dianbo == 2 )
        {
            //bitter:				  MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel=1;//�㲥
            //bitter:				  MSG_Obj_8[dianbo_scree-1].INFO_TYPE=1;
            SenddianboMeun( dianbo_scree, 0 );
        }
        break;
    case KEY_DOWN:
        if( Menu_dianbo == 1 )
        {
            dianbo_scree++;
            if( dianbo_scree >= 8 )
            {
                dianbo_scree = 8;
            }
            Dis_dianbo( dianbo_scree );
        }
        else if( Menu_dianbo == 2 )
        {
            //bitter:				  MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel=0;//ȡ��
            //bitter:				  MSG_Obj_8[dianbo_scree-1].INFO_TYPE=1;// 0
            SenddianboMeun( dianbo_scree, 0 );
        }
        break;
    }
}

MENUITEM Menu_2_7_RequestProgram =
{
    "��Ϣ�㲥�鿴",
    12,				  0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};
#endif

/************************************** The End Of File **************************************/
