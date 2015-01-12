
#include "Menu_Include.h"
#include "sed1520.h"


u8 Dis_screen_6_flag = 0;

static void msg( void *p)
{

}
static void show(void)
{
    if(Dis_screen_6_flag == 0)
    {
        Dis_screen_6_flag = 1;
        lcd_fill(0);
        lcd_text12(0, 3, "����", 4, LCD_MODE_SET);
        lcd_text12(0, 17, "����", 4, LCD_MODE_SET);
        lcd_text12(42, 3, "������Ϣ����", 12, LCD_MODE_SET);
        lcd_text12(27, 17, "�밴ȷ����������", 16, LCD_MODE_SET);
        lcd_update_all();
    }
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        //�����˳�
        Password_correctFlag = 1;

        Dis_screen_6_flag = 0;
        pMenuItem = &Menu_1_menu;
        pMenuItem->show();
        break;
    case KEY_OK:
        if(Dis_screen_6_flag == 1)
        {
            //���õ������Ϣ��־
            MENU_set_carinfor_flag = 1;
            Dis_screen_6_flag = 0;
            if(ic_card_para.administrator_card == 1)
            {
                pMenuItem = &Menu_0_loggingin;
                pMenuItem->show( );
            }
            else
            {
                pMenuItem = &Menu_0_0_password;
                pMenuItem->show( );
            }
        }
        break;
    case KEY_UP:

        break;
    case KEY_DOWN:

        break;
    }
}

static void timetick(unsigned int systick)
{
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_6_SetInfor =
{
    "������Ϣ����",
    12, 0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};

