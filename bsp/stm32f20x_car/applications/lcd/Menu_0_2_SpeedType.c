#include  <string.h>
#include "Menu_Include.h"
//#include "Lcd.h"
#include "sed1520.h"

static u8 menu1 = 0;		//主界面
static u8 menu2 = 0;		//第二层界面
static u8 set_type_ok = 0;	//非0表示设置OK



static void msg( void *p)
{

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
        lcd_text12(0, 3, "脉冲速度", 8 , ( menu1 & 0x01 ) * 2 + 1);		///1
        lcd_text12(0, 18, "定位速度", 8 , 3 - ( menu1 & 0x01 ) * 2);		///2
    }
    else
    {
        lcd_text12(12, 3, "车辆速度选择完毕", 16, LCD_MODE_SET);
        lcd_text12(6, 18, "按确认键设置下一项", 18, LCD_MODE_SET);
    }
    lcd_update_all();
}



static void show(void)
{
    if(jt808_param.id_0xF043 == 1)
        menu1 = 2;
    else
        menu1 = 1;

    menu2 = 0;
    type_disp();
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
        if(menu2)
        {
            jt808_param.id_0xF043 = menu1 - 1;
            param_save(1);
            CarSet_0_counter = 3;
            pMenuItem = &Menu_0_loggingin;
            pMenuItem->show();
            return;
        }
        else if(menu2 == 0)
        {
            menu2 = 1;
        }
        break;
    case KEY_UP:
        if(menu2 == 0)
        {
            menu1--;
            if((menu1 == 0) || (menu1 > 2))
                menu1 = 2;
        }
        break;
    case KEY_DOWN:
        if(menu2 == 0)
        {
            menu1++;
            if(menu1 > 2)
                menu1 = 1;
        }
        break;
    }
    type_disp();
}


static void timetick(unsigned int systick)
{

}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_0_2_SpeedType =
{
    "速度类型设置",
    12, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};





