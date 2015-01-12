#include "Menu_Include.h"
//#include "App_moduleConfig.h"
#include "sed1520.h"
u8 Dis_screen_8_flag = 0;

static const uint8_t	menu_num = 7;

static unsigned char menu_pos = 0;
static unsigned char menu_2 = 0;


static MENUITEM	Menu_2 =
{
    "辅域名端口设置",
    12, 0,
    (void *)0,
    (void *)0,
    (void *)0,
    (void *)0
};
static MENUITEM	Menu_3 =
{
    "车主电话设置",
    12, 0,
    (void *)0,
    (void *)0,
    (void *)0,
    (void *)0
};
static MENUITEM	Menu_4 =
{
    "车台请求解锁",
    12, 0,
    (void *)0,
    (void *)0,
    (void *)0,
    (void *)0
};
static MENUITEM	Menu_5 =
{
    "设置联网模式",
    12, 0,
    (void *)0,
    (void *)0,
    (void *)0,
    (void *)0
};
static MENUITEM	Menu_6 =
{
    "行驶记录仪模式",
    14, 0,
    (void *)0,
    (void *)0,
    (void *)0,
    (void *)0
};

static MENUITEM	Menu_7 =
{
    "脉冲系数设置",
    12, 0,
    (void *)0,
    (void *)0,
    (void *)0,
    (void *)0
};
static PMENUITEM psubmenu[menu_num] =
{
    &Menu_8_1_MainDnsPort,
    &Menu_2,
    &Menu_3,		//车主电话号码
    &Menu_4,		//"车台请求解锁"
    &Menu_5,		//"设置联网模式"
    &Menu_6,		//"行驶记录仪模式"
    &Menu_7,		//"脉冲系数设置"
    //&Menu_8_3_MainIpPort,
    //&Menu_8_4_AuxIpPort,

};




uint8_t cb_param_set_user_phone(char *val, uint8_t len)
{
    uint16_t	i, j;
    uint32_t 	u32data;
    uint8_t 	ret = 0;
    if(len <= 11)
    {
        memset(jt808_param.id_0xF014, 0, sizeof(jt808_param.id_0xF014));
        strncpy( jt808_param.id_0xF014, (char *)val, len );
        ret = 1;
        param_save(1);
    }
    CarSet_0_counter = 7;
    return ret;
}


static void menuswitch(void)
{
    unsigned char i = 0;

    //lcd_fill(0);
    lcd_text12(0, 3, "其它", 4, LCD_MODE_SET);
    lcd_text12(0, 17, "设置", 4, LCD_MODE_SET);
    for(i = 0; i < menu_num; i++)
        lcd_bitmap(30 + i * 11, 5, &BMP_noselect_log, LCD_MODE_SET);
    lcd_bitmap(30 + menu_pos * 11, 5, &BMP_select_log, LCD_MODE_SET);
    lcd_text12(30, 19, (char *)(psubmenu[menu_pos]->caption), psubmenu[menu_pos]->len, LCD_MODE_SET);
    //lcd_update_all();
}


static void display_dns(void)
{
    lcd_fill(0);
    if( NET_SET_FLAG != 2 )
    {
        menu_pos = 0;
        NET_SET_FLAG = 1;//
        lcd_text12(0, 3, "其它", 4, LCD_MODE_SET);
        lcd_text12(0, 17, "设置", 4, LCD_MODE_SET);
        lcd_text12(42, 3, "网络信息设置", 12, LCD_MODE_SET);
        lcd_text12(27, 17, "请按确认输入密码", 16, LCD_MODE_SET);
    }
    else if(NET_SET_FLAG == 2)
    {
        //网络设置选择
        if(menu_2 == 0)
        {
            menuswitch();
        }
        else
        {
            lcd_text12(12, 11, "开始发送解锁数据", 16, LCD_MODE_SET);
        }
    }
    lcd_update_all();
}


static void msg( void *p)
{

}
static void show(void)
{
    menu_2 = 0;
    rt_kprintf("\r\n设置开始 NET_SET_FLAG=%d", NET_SET_FLAG);
    display_dns();

    /*
    if((Dis_screen_8_flag==0)&&(NET_SET_FLAG!=3))
    	{
        Dis_screen_8_flag=1;
    	NET_SET_FLAG=1;//
        lcd_fill(0);
    	lcd_text12(0,3,"网络",4,LCD_MODE_SET);
        lcd_text12(0,17,"设置",4,LCD_MODE_SET);
    	lcd_text12(42,3,"网络信息设置",12,LCD_MODE_SET);
    	lcd_text12(27,17,"请按确认输入密码",16,LCD_MODE_SET);
    	lcd_update_all();
    	}
    else if(NET_SET_FLAG==3)
    	{
    	//网络设置选择
    	menuswitch();
    	Dis_screen_8_flag=2;
    	}
    	*/
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        if(menu_2 == 0)
        {
            NET_SET_FLAG = 0;
            if(device_unlock)
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
            return;
        }
        else
        {
            menu_2 = 0;
            //menuswitch();
        }
        break;
    case KEY_OK:
        if(NET_SET_FLAG == 1)
        {
            pMenuItem = &Menu_0_0_password;
            pMenuItem->show();
            return;
        }
        else
        {
            if(2 == menu_pos)
            {
                lcd_set_param(Menu_3.caption, (uint8_t *)&jt808_param.id_0xF014, TYPE_STR, 11, 1, cb_param_set_user_phone);
                return;
            }
            else if(4 == menu_pos)
            {
                lcd_set_param(Menu_5.caption, (uint8_t *)&jt808_param.id_0xF00F, TYPE_BYTE, 1, 1, 0);
                return;
            }
            else if(5 == menu_pos)
            {
                lcd_set_param(Menu_6.caption, (uint8_t *)&jt808_param.id_0xF041, TYPE_BYTE, 1, 1, 0);
                return;
            }
            else if(6 == menu_pos)
            {
                lcd_set_param(Menu_7.caption, (uint8_t *)&jt808_param.id_0xF032, TYPE_DWORD, 5, 1, 0);
            }
            else if(3 == menu_pos)
            {
                menu_2 = 1;
                jt808_tx_lock(mast_socket->linkno);
                //lcd_fill(0);
                //lcd_text12(12,11,"开始发送解锁数据",16,LCD_MODE_SET);
                //lcd_update_all();
            }
            else
            {
                DNS_Main_Aux = menu_pos + 1;
                pMenuItem = &Menu_8_1_MainDnsPort;
                pMenuItem->show();
                return;
            }
        }
        break;
    case KEY_UP:
        if(menu_pos == 0)
            menu_pos = menu_num;
        menu_pos--;
        break;
    case KEY_DOWN:
        menu_pos++;
        if(menu_pos >= menu_num)
            menu_pos = 0;
        //menuswitch();
        break;
    }
    display_dns();
}

static void timetick(unsigned int tick)
{
    if(device_unlock)
    {
        if( ( tick - pMenuItem->tick ) >= 100 * 30 )
        {
            NET_SET_FLAG = 0;
            pMenuItem = &Menu_1_Idle;
            pMenuItem->show( );
        }
    }
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_8_SetDNS =
{
    "其它设置",
    8, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

