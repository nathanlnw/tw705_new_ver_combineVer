#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "sed1520.h"

u8 IC_card_num = 0;

static void driver_IC_Card(u8 par)
{
    u8  reg_str[32];
    lcd_fill(0);
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
    lcd_update_all();
}

static void msg( void *p)
{
}
static void show(void)
{
    IC_card_num = 1;
    driver_IC_Card(IC_card_num);
}



static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        IC_card_num = 0;
        CounterBack = 0;
        pMenuItem = &Menu_4_recorder;
        pMenuItem->show();
        break;
    case KEY_OK:
        break;
    case KEY_UP:
        if(IC_card_num == 1)
            IC_card_num = 4;
        else
            IC_card_num--;
        driver_IC_Card(IC_card_num);
        break;
    case KEY_DOWN:
        if(IC_card_num >= 4)
            IC_card_num = 1;
        else
            IC_card_num++;
        driver_IC_Card(IC_card_num);
        break;
    }
}




static void timetick(unsigned int systick)
{

    CounterBack++;
    if(CounterBack != MaxBankIdleTime)
        return;
    pMenuItem = &Menu_1_Idle;
    pMenuItem->show();
    CounterBack = 0;

}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_4_5_ICcard =
{
    "驾驶员信息",
    10, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

