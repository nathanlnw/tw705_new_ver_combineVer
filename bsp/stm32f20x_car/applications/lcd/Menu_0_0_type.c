#include  <string.h>
#include "Menu_Include.h"
//#include "Lcd.h"
#include "LCD_Driver.h"

static u8 type_selc_screen = 0;
static u8 type_selc_flag = 0;
static u8 menu1 = 0;		//主界面
static u8 menu2 = 0;		//第二层界面
static u8 menu3 = 0;		//第三层界面
static u8 set_type_ok = 0;	//非0表示设置OK

const char *car_type_str[] =
{
    "客运车",
    "旅游车",
    "危品车"

};

static void msg( void *p)
{

}


static void CarType1(unsigned char type_Sle, unsigned char sel)
{
    if(sel == 0)
    {
        lcd_text12(0, 10, "客运车 旅游车 危品车", 20, LCD_MODE_SET);
    }
    switch(type_Sle)
    {
    case 1:
        if(sel == 0)
        {
            lcd_text12(0, 10, "客运车", 6, LCD_MODE_INVERT);
        }
        else
            lcd_text12(12, 10, "车辆类型:客运车", 15, LCD_MODE_SET);
        break;
    case 2:
        if(sel == 0)
        {
            lcd_text12(7 * 6, 10, "旅游车", 6, LCD_MODE_INVERT);
        }
        else
            lcd_text12(12, 10, "车辆类型:旅游车", 15, LCD_MODE_SET);
        break;
    case 3:
        if(sel == 0)
        {
            lcd_text12(14 * 6, 10, "危品车", 6, LCD_MODE_INVERT);
        }
        else
            lcd_text12(12, 10, "车辆类型:危品车", 15, LCD_MODE_SET);
        break;
    }
}


static void type_disp(void)
{
    unsigned char i = 0;
    uint8_t play_line;
    uint8_t first_line;
    char buf[64];
    uint8_t lcd_mode;
    lcd_fill(0);
    if(menu2 == 0)
    {
        lcd_text12(0, 3, "两客一危", 8, ( menu1 & 0x01 ) * 2 + 1);		///1
        lcd_text12(0, 18, "货运",    4, 3 - ( menu1 & 0x01 ) * 2);		///2
    }
    else
    {
        if(set_type_ok)
        {
            lcd_text12(12, 3, "车辆类型选择完毕", 16, LCD_MODE_SET);
            lcd_text12(6, 18, "按确认键设置下一项", 18, LCD_MODE_SET);
        }
        else
        {
            switch(menu1)
            {
            case 1:
            {
                CarType1(menu2, menu3);
                break;
            }
            case 2:
            {
                lcd_text12(15, 3, "车辆类型:货运车", 15, LCD_MODE_SET);
                lcd_text12(6, 18, "按确认键设置下一项", 18, LCD_MODE_SET);
                break;
            }
            default :
            {
                break;
            }
            }
        }
    }
    lcd_update_all();
}



static void show(void)
{
    menu1 = 1;
    menu2 = 0;
    menu3 = 0;
    set_type_ok = 0;
    type_disp();
    memset(Menu_VechileType, 0, sizeof(Menu_VechileType));
}



static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        pMenuItem = &Menu_0_loggingin;
        pMenuItem->show();
        return;
        break;
    case KEY_OK:
        if(set_type_ok)
        {
            jt808_param.id_0xF00D = set_type_ok;
            set_type_ok = 0;
            if(jt808_param.id_0xF00D == 1) //两客一危
            {
                memset(jt808_param.id_0x0013, 0, sizeof(jt808_param.id_0x0013));
                strcpy(jt808_param.id_0x0013, "60.28.50.210");
                memcpy(jt808_param.id_0x0017, jt808_param.id_0x0013, sizeof(jt808_param.id_0x0013));
                jt808_param.id_0x0018 = 9131;
                jt808_param.id_0xF031 = 9131;
            }
            else
            {
                jt808_param.id_0xF00D = 2;
                memset(jt808_param.id_0x0013, 0, sizeof(jt808_param.id_0x0013));
                memset(jt808_param.id_0x0017, 0, sizeof(jt808_param.id_0x0017));
                strcpy(jt808_param.id_0x0013, "jt1.gghypt.net");
                strcpy(jt808_param.id_0x0017, "jt2.gghypt.net");
                jt808_param.id_0x0018 = 7008;
                jt808_param.id_0xF031 = 7008;
            }
            memset(jt808_param.id_0xF00A, 0, sizeof(jt808_param.id_0xF00A));
            memcpy(jt808_param.id_0xF00A, Menu_VechileType, strlen((const char *)Menu_VechileType));
            param_save(1);
            CarSet_0_counter = 1;
            pMenuItem = &Menu_0_loggingin;
            pMenuItem->show();
            return;
        }
        else if(menu1 == 1)
        {
            if(menu2 == 0)
            {
                menu2 = 1;
            }
            else
            {
                if(menu3 == 0)
                {
                    menu3 = 1;
                }
                else
                {
                    memcpy(Menu_VechileType, car_type_str[menu2 - 1], 6);
                    set_type_ok = 1;
                }
            }
        }
        else
        {
            menu1 = 2;
            if(menu2 == 0)
            {
                menu2 = 1;
            }
            else
            {
                memcpy(Menu_VechileType, "货运车", 6);
                set_type_ok = 2;
            }
        }
        break;
    case KEY_UP:
        if(menu2 == 0)
        {
            menu1--;
            if((menu1 == 0) || (menu1 > 2))
                menu1 = 2;
        }
        else
        {
            if(menu1 == 1)
            {
                if(menu3 == 0)
                {
                    menu2--;
                    if((menu2 == 0) || (menu2 > 2))
                        menu2 = 3;
                }
            }
            else
            {
                ;
            }
        }
        break;
    case KEY_DOWN:
        if(menu2 == 0)
        {
            menu1++;
            if(menu1 > 2)
                menu1 = 1;
        }
        else
        {
            if(menu1 == 1)
            {
                if(menu3 == 0)
                {
                    menu2++;
                    if(menu2 > 3)
                        menu2 = 1;
                }
            }
            else
            {
                ;
            }
        }
        break;
    }
    type_disp();
}


static void timetick(unsigned int systick)
{

}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_0_0_type =
{
    "车辆类型设置",
    12, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};




