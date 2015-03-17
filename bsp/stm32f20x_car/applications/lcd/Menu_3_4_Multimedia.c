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

#include "camera.h"


/*
   Ҫ�ܲ鿴��¼?

   ͼƬ��¼
    ��ȡͼƬ��¼���� ����������
   �����ϴ�
   ��ǰ����
 */
static int16_t pos;

#define SCR_PHOTO_MENU				0
#define SCR_PHOTO_SELECT_ITEM		1
#define SCR_PHOTO_SELECT_DETAILED	2
#define SCR_PHOTO_SEND_0x0800		3
#define SCR_PHOTO_TAKE				4

static uint8_t	scr_mode = SCR_PHOTO_MENU; /*��ǰ��ʾ�Ľ���״̬*/

static uint8_t	*pHead		= RT_NULL;
static uint16_t pic_count	= 0;
TypeDF_PackageHead	 *phead_pro = RT_NULL;		//��ǰ���ڴ���Ķ�ý��ͼƬͷ��Ϣ
static uint8_t	cam_state		= 0;			//�ò���Ϊ1��ʾ����ok
static void display( void );

/*********************************************************************************
  *��������:void cam_ok_menu( struct _Style_Cam_Requset_Para *para,uint32_t pic_id )
  *��������:ƽ̨�·�������������Ļص�����_������Ƭ����OK
  *��	��:	para	:���մ���ṹ��
   pic_id	:ͼƬID
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static void cam_ok_menu( struct _Style_Cam_Requset_Para *para, uint32_t pic_id )
{
    static uint32_t last_id = 0;
    cam_state = 1;
    display();
#ifdef JT808_TEST_JTB
    if(last_id != pic_id)
    {
        Cam_jt808_0x0800_ex(mast_socket->linkno, pic_id, 0, 0);
        last_id = pic_id;
    }
#endif
}


/*********************************************************************************
  *��������:void Cam_takepic_ex(u16 id,u16 num,u16 space,u8 send,Style_Cam_Requset_Para trige)
  *��������:	������������
  *��	��:	id		:�����ID��ΧΪ1-15
   num		:��������
   space	:���ռ������λΪ��
   save	:�Ƿ񱣴�ͼƬ��FLASH��
   send	:������Ƿ��ϴ���1��ʾ�ϴ�
   trige	:���մ���Դ����Ϊ	Style_Cam_Requset_Para
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_Start(u8 save, u8 send )
{
    Style_Cam_Requset_Para tempPara;
    memset( &tempPara, 0, sizeof( tempPara ) );
    tempPara.Channel_ID		= 0xFF;
    tempPara.PhotoSpace		= 0;
    tempPara.PhotoTotal		= 1;
    tempPara.SavePhoto		= save;
    tempPara.SendPhoto		= send;
    tempPara.TiggerStyle	= Cam_TRIGGER_OTHER;
    tempPara.cb_response_cam_ok		= cam_ok_menu;   ///һ����Ƭ���ճɹ��ص�����
    take_pic_request( &tempPara );
}



/**/
static void display( void )
{
    uint16_t			pic_page_start; /*ÿ�ĸ�ͼƬ��¼Ϊһ��page*/
    uint8_t				buf[32], buf_time[16];
    uint16_t			i;
    MYTIME				t;
    TypeDF_PackageHead	 *pcurrhead;

    lcd_fill( 0 );
    switch( scr_mode )
    {
    case SCR_PHOTO_MENU:
        pos &= 0x01;
        lcd_text12( 5, 4, "1.ͼƬ��¼", 10, 3 - pos * 2 );
        lcd_text12( 5, 18, "2.�����ϴ�", 10, pos * 2 + 1 );
        break;
    case SCR_PHOTO_SELECT_ITEM:
        if( pic_count ) /*��ͼƬ*/
        {
            if( pos >= pic_count )
            {
                pos = 0;
            }
            if( pos < 0 )
            {
                pos = pic_count - 1;
            }
            pic_page_start = pos & 0xFFFC; /*ÿ4��1��*/
            for( i = pic_page_start; i < pic_page_start + 4; i++ )
            {
                if( i >= pic_count )
                {
                    break;
                }

                pcurrhead	= (TypeDF_PackageHead *)( pHead + i * sizeof( TypeDF_PackageHead ) );
                t			= pcurrhead->Time;

                sprintf( buf, "%02d>%02d-%02d-%02d %02d:%02d:%02d",
                         i + 1, YEAR( t ), MONTH( t ), DAY( t ), HOUR( t ), MINUTE( t ), SEC( t ));
                if( i == pos )
                {
                    lcd_asc0608( 0, 8 * ( i & 0x03 ), buf, LCD_MODE_INVERT );
                }
                else
                {
                    lcd_asc0608( 0, 8 * ( i & 0x03 ), buf, LCD_MODE_SET );
                }
            }
        }
        else  /*û��ͼƬ*/
        {
            lcd_text12( 25, 12, "û��ͼƬ��¼", 12, LCD_MODE_SET );
        }
        break;
    case SCR_PHOTO_SELECT_DETAILED:/*��ʾͼƬ��ϸ��Ϣ*/
        if( pos >= pic_count )
        {
            pos = 0;
        }
        if( pos < 0 )
        {
            pos = pic_count - 1;
        }
        phead_pro = pcurrhead = (TypeDF_PackageHead *)( pHead + pos * sizeof( TypeDF_PackageHead ) );
        t			= pcurrhead->Time;

        sprintf( buf, "%02d:%02d:%02d", HOUR( t ), MINUTE( t ), SEC( t ));
        lcd_asc0608( 0, 0, buf, LCD_MODE_SET );

        sprintf(buf, "size=%d", pcurrhead->Len - 64);
        lcd_asc0608( 64, 0, buf, LCD_MODE_SET );

        sprintf(buf, "ch=%d trig=%d sta=0x%2X", pcurrhead->Channel_ID, pcurrhead->TiggerStyle, pcurrhead->State);
        lcd_asc0608( 0, 8, buf, LCD_MODE_SET );
        lcd_text12( 0, 16, "��ȷ����������Ϣ!", 16, LCD_MODE_SET );

        break;
    case SCR_PHOTO_SEND_0x0800:
        lcd_text12( 6, 12, "ͼƬ��Ϣ������...", 17, LCD_MODE_SET );
        break;
    case SCR_PHOTO_TAKE:
        if(cam_state)
        {
            lcd_text12( 4, 12, "���ճɹ�,������...", 18, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 20, 12, "���ս�����...", 13, LCD_MODE_SET );
        }
        break;
    }
    lcd_update_all( );
}

/*�������ռ��ϴ��Ĺ���*/
static void msg( void *p )
{
    pMenuItem->tick = rt_tick_get( );
}

/**/
static void show( void )
{
    pMenuItem->tick = rt_tick_get( );
    pos				= 0;
    scr_mode		= SCR_PHOTO_MENU;
    phead_pro		= RT_NULL;
    display( );
}

/*�����������ջ��ϴ�����������ж�?*/
static void keypress( unsigned int key )
{
    switch( key )
    {
    case KEY_MENU:

        if( scr_mode == SCR_PHOTO_MENU )
        {
            pMenuItem = &Menu_3_InforInteract;
            pMenuItem->show( );
            break;
        }
        if( scr_mode == SCR_PHOTO_SELECT_ITEM ) /*���˵����˵�*/
        {
            scr_mode = SCR_PHOTO_MENU;
            rt_free( pHead );
            pHead = RT_NULL;
        }
        else if(( scr_mode == SCR_PHOTO_SELECT_DETAILED) || (scr_mode == SCR_PHOTO_SEND_0x0800)) /*���˵�������Ƭ��Ŀ��ʾ*/
        {
            scr_mode = SCR_PHOTO_SELECT_ITEM;
        }
        else if(scr_mode == SCR_PHOTO_TAKE)
        {
            scr_mode = SCR_PHOTO_MENU;
        }
        display();
        break;
    case KEY_OK:
        if( scr_mode == SCR_PHOTO_MENU )
        {
            if( pos == 0 ) /*ͼƬ��¼*/
            {
                pic_count = 0;
                rt_free(pHead);
                pHead = RT_NULL;
                pHead		= Cam_Flash_SearchPicHead( 0x00000000, 0xFFFFFFFF, 0, 0xFF, &pic_count , BIT(1));
                scr_mode	= SCR_PHOTO_SELECT_ITEM;

            }
            else  /*ͼƬ����*/
            {
                scr_mode	= SCR_PHOTO_TAKE;
                cam_state	= 0;
                if(Cam_get_state() == 0)
                {
#ifdef JT808_TEST_JTB
                    //Cam_takepic(0xFF,1,0,Cam_TRIGGER_OTHER);
                    Cam_Start(1, 0);
#else
                    //Cam_takepic(0xFF,0,1,Cam_TRIGGER_OTHER);
                    Cam_Start(0, 1);
#endif
                }
            }
        }
        else if( scr_mode == SCR_PHOTO_SELECT_ITEM )
        {
            if(pic_count)
            {
                phead_pro = RT_NULL;
                scr_mode = SCR_PHOTO_SELECT_DETAILED;
            }
        }
        else if(scr_mode == SCR_PHOTO_SELECT_DETAILED)
        {
            if(phead_pro)
            {
                scr_mode = SCR_PHOTO_SEND_0x0800;
                Cam_jt808_0x0800_ex(mast_socket->linkno, phead_pro->id, 0, 0);
            }
        }
        else if(scr_mode == SCR_PHOTO_SEND_0x0800)
        {
            ;
        }
        else if(scr_mode == SCR_PHOTO_TAKE)
        {
            scr_mode = SCR_PHOTO_MENU;
        }
        display( );
        break;
    case KEY_UP:
        pos--;
        display( );
        break;
    case KEY_DOWN:
        pos++;
        display( );
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

    MENUITEM *tmp;

    if( ( tick - pMenuItem->tick ) >= 100 * 60 )
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
        rt_free(pHead);
        pHead = RT_NULL;
        pMenuItem->show( );
    }
}


MENUITEM Menu_3_4_Multimedia =
{
    "��ý����Ϣ",
    10,				  0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/
