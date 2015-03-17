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
#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "LCD_Driver.h"


static uint8_t menu = 0;
static uint8_t menu1 = 0;
uint8_t IC_card_showinfo = 0;		///0,表示正常显示；1表示直接显示驾驶员信息

static void driver_IC_Card(u8 par)
{
    u8  reg_str[32];
    //lcd_fill(0);
    if(par == 1)
    {
        lcd_text12(0, 3, "驾驶员姓名:", 11, LCD_MODE_SET);
        lcd_text12(45, 18, (char *)jt808_param.id_0xF008, strlen((char *)jt808_param.id_0xF008), LCD_MODE_SET);
    }
    else if(par == 2)
    {
        lcd_text12(0, 3, "机动车驾驶证号码:", 17, LCD_MODE_SET);
        lcd_text12(6, 18, (char *)jt808_param.id_0xF009, 18, LCD_MODE_SET);
    }
    else if(par == 3)
    {
        lcd_text12(0, 3, "驾驶证有效期:", 13, LCD_MODE_SET);
        memset(reg_str, 0, sizeof(reg_str));
        sprintf(reg_str, "%02X%02X-%02X-%02X", (ic_card_para.IC_Card_valitidy[0]), (ic_card_para.IC_Card_valitidy[1]), (ic_card_para.IC_Card_valitidy[2]), (ic_card_para.IC_Card_valitidy[3]));
        lcd_text12(0, 18, reg_str, 10, LCD_MODE_SET);
    }
    else if(par == 4)
    {
        lcd_text12(0, 3, "从业资格证号:", 13, LCD_MODE_SET);
        lcd_text12(6, 19, (char *)jt808_param.id_0xF00B, strlen((char *)jt808_param.id_0xF008), LCD_MODE_SET);
    }
    //lcd_update_all();
}


static void dis_pro(void)
{
    lcd_fill(0);
    if( 0 == menu1 )
    {
        lcd_text12( 0, 3, "1.驾驶员信息查看", 16, 3 - (menu * 2) );  	// SET=1 INVERT=3
        lcd_text12( 0, 19, "2.驾驶员信息发送", 16, 1 + (menu * 2) );		// SET=1 INVERT=3
    }
    else
    {
        if(menu == 0)
        {
            driver_IC_Card(menu1);
        }
        else
        {
            if( menu1 == 1 )
            {
                lcd_text12( 0, 10, "按确认发送驾驶员信息", 20, LCD_MODE_SET );
            }
            else if( menu1 == 2 )
            {
                lcd_text12( 5, 10, "驾驶员信息发送中..", 18, LCD_MODE_SET );
            }
            else if( menu1 == 3 )
            {
                if(get_sock_state(0) == 2)
                    lcd_text12( 5, 10, "驾驶员信息已经发送", 18, LCD_MODE_SET );
                else
                    lcd_text12( 5, 10, "驾驶员信息发送失败", 18, LCD_MODE_SET );
            }
        }
    }
    lcd_update_all();
}


static void msg( void *p)
{

}


static void show(void)
{
    pMenuItem->tick = rt_tick_get( );
    if(IC_card_showinfo == 1)
    {
        menu = 0;
        menu1 = 1;
    }
    else
    {
        menu = 0;
        menu1 = 0;
    }
    dis_pro();
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        IC_card_showinfo = 0;
        if(menu1)
        {
            menu1 = 0;
        }
        else
        {
            pMenuItem = &Menu_4_recorder;
            pMenuItem->show();
            return;
        }
        break;
    case KEY_OK:
        if( 0 == menu1 )
        {
            menu1 = 1;
        }
        else
        {
            if(menu == 1)
            {
                if(menu1 < 3)
                    ++menu1;
                if(menu1 == 2)
                {
                    IC_CARD_jt808_0x0702_ex(mast_socket->linkno, 0, 1);
                    menu1++;
                }
            }
        }
        break;
    case KEY_UP:
        if( 0 == menu1 )
        {
            menu ^= BIT(0);
        }
        else
        {
            if(menu == 0)
            {
                if(menu1 == 1)
                    menu1 = 4;
                else
                    menu1--;
            }
        }
        break;
    case KEY_DOWN:
        if( 0 == menu1 )
        {
            menu ^= BIT(0);
        }
        else
        {
            if(menu == 0)
            {
                if(menu1 >= 4)
                    menu1 = 1;
                else
                    menu1++;
            }
        }
        break;
    }
    dis_pro();
}


MENUITEM Menu_2_5_DriverInfor =
{
    "驾驶员信息查看",
    14, 			  0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};


#if 0

/*
   static unsigned char Jiayuan_screen=0;  //  显示查看/发送界面   =1时选择是查看还是发送界面
   static unsigned char CheckJiayuanFlag=0;//  1:进入显示驾驶员信息   2:进入发送驾驶员信息
   static unsigned char Jiayuan_1_2=0;     // 0:表示在查看界面   1:表示在发送界面
 */
typedef struct _DIS_DIRVER_INFOR
{
    unsigned char	DIS_SELECT_check_send;
    unsigned char	DIS_ENTER_check_send;
    unsigned char	DIS_SHOW_check_send;
    unsigned int	send_card_info_counter;			//发送驾驶员信息总时长，超过一定时长及判断发送失败
} DIS_DIRVER_INFOR;

DIS_DIRVER_INFOR DIS_DRIVER_inform_temp;

//驾驶员代码
void Display_jiayuan( unsigned char NameCode )
{
    //unsigned char i=0;
    lcd_fill( 0 );
    if( NameCode == 1 )
    {
        lcd_text12( 30, 3, "驾驶员姓名", 10, LCD_MODE_SET );
        lcd_text12( 42, 19, jt808_param.id_0xF008, strlen( jt808_param.id_0xF008 ), LCD_MODE_SET );
        //lcd_text12(48,19,(char *)Driver_Info.DriveName,strlen(Driver_Info.DriveName),LCD_MODE_SET);
    }
    else
    {
        lcd_text12( 30, 3, "驾驶证号码", 10, LCD_MODE_SET );
        lcd_text12( 0, 19, jt808_param.id_0xF009, strlen( jt808_param.id_0xF009 ), LCD_MODE_SET );
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
static void Dis_DriverInfor( unsigned char type, unsigned char disscreen )
{
    lcd_fill( 0 );
    if( type == 1 )
    {
        if( disscreen == 1 )
        {
            lcd_text12( 0, 3, "1.驾驶员信息查看", 16, LCD_MODE_INVERT );
            lcd_text12( 0, 19, "2.驾驶员信息发送", 16, LCD_MODE_SET );
        }
        else if( disscreen == 2 )
        {
            lcd_text12( 0, 3, "1.驾驶员信息查看", 16, LCD_MODE_SET );
            lcd_text12( 0, 19, "2.驾驶员信息发送", 16, LCD_MODE_INVERT );
        }
    }
    else if( type == 2 )
    {
        if( disscreen == 1 )
        {
            lcd_text12( 0, 10, "按确认发送驾驶员信息", 20, LCD_MODE_SET );
        }
        else if( disscreen == 3 )
        {
            lcd_text12( 5, 10, "驾驶员信息发送中", 18, LCD_MODE_SET );
        }
        else if( disscreen == 2 )
        {
            if(get_sock_state(0) == 2)
                lcd_text12( 5, 10, "驾驶员信息发送成功", 18, LCD_MODE_SET );
            else
                lcd_text12( 5, 10, "驾驶员信息发送失败", 18, LCD_MODE_SET );
        }
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

static void timetick( unsigned int tick )
{
    if(DIS_DRIVER_inform_temp.send_card_info_counter)
    {
        DIS_DRIVER_inform_temp.send_card_info_counter++;
    }
    timetick_default(tick);
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

    Dis_DriverInfor( 1, 1 );
    DIS_DRIVER_inform_temp.DIS_SELECT_check_send	= 1;
    DIS_DRIVER_inform_temp.DIS_ENTER_check_send		= 1;
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
        pMenuItem = &Menu_2_InforCheck;
        pMenuItem->show( );

        memset( &DIS_DRIVER_inform_temp, 0, sizeof( DIS_DRIVER_inform_temp ) );
        break;
    case KEY_OK:
        if( DIS_DRIVER_inform_temp.DIS_ENTER_check_send == 1 )
        {
            DIS_DRIVER_inform_temp.DIS_ENTER_check_send		= 2;
            DIS_DRIVER_inform_temp.DIS_SELECT_check_send	= 0;            //查看或者发送已经选好

            if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 0 )           //进入查看驾驶员信息界面
            {
                Display_jiayuan( 1 );
            }
            else if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 1 )      //进入发送驾驶员信息界面
            {
                Dis_DriverInfor( 2, 1 );
            }
        }
        else if( DIS_DRIVER_inform_temp.DIS_ENTER_check_send == 2 )
        {
            DIS_DRIVER_inform_temp.DIS_ENTER_check_send = 3;
            if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 0 )           //返回查看和发送界面
            {
                Dis_DriverInfor( 1, 1 );
                DIS_DRIVER_inform_temp.DIS_SELECT_check_send	= 1;
                DIS_DRIVER_inform_temp.DIS_ENTER_check_send		= 1;
            }
            else if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 1 )      //提示发送成功
            {
                Dis_DriverInfor( 2, 2 );
                IC_CARD_jt808_0x0702_ex(mast_socket->linkno, 0, 1);
                //SD_ACKflag.f_DriverInfoSD_0702H=1;
                DIS_DRIVER_inform_temp.DIS_ENTER_check_send		= 0;        //    1
                DIS_DRIVER_inform_temp.DIS_SELECT_check_send	= 0;
                DIS_DRIVER_inform_temp.DIS_SHOW_check_send		= 0;
            }
        }
        break;
    case KEY_UP:
        if( DIS_DRIVER_inform_temp.DIS_ENTER_check_send == 2 )
        {
            if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 0 )           //查看
            {
                Display_jiayuan( 1 );
            }
            else if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 1 )      //发送
            {
                Dis_DriverInfor( 2, 1 );
            }
        }
        else if( DIS_DRIVER_inform_temp.DIS_SELECT_check_send == 1 )        //选择进入查看或者发送
        {
            DIS_DRIVER_inform_temp.DIS_SHOW_check_send = 0;
            Dis_DriverInfor( 1, 1 );
        }
        break;
    case KEY_DOWN:
        if( DIS_DRIVER_inform_temp.DIS_ENTER_check_send == 2 )
        {
            if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 0 )           //查看
            {
                Display_jiayuan( 2 );
            }
            else if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 1 )      //发送
            {
                Dis_DriverInfor( 2, 1 );
            }
        }
        else if( DIS_DRIVER_inform_temp.DIS_SELECT_check_send == 1 )        //选择进入查看或者发送
        {
            DIS_DRIVER_inform_temp.DIS_SHOW_check_send = 1;
            Dis_DriverInfor( 1, 2 );
        }
        break;
    }
}


MENUITEM Menu_2_5_DriverInfor =
{
    "驾驶员信息查看",
    14,				  0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};
#endif
/************************************** The End Of File **************************************/
