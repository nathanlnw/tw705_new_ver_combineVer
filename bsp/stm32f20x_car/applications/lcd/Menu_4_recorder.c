#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "sed1520.h"

static const uint8_t 	menu_num = 8;

#define  DIS_Dur_width_inter 10

static unsigned char menu_pos = 0;

static PMENUITEM psubmenu[menu_num] =
{
    &Menu_4_1_pilao,     			//��ʱ��ʻ��¼		1
    &Menu_2_4_CarInfor,  			//������Ϣ			2
    //&Menu_4_5_ICcard,    		//��ʻԱ��Ϣ		2
    &Menu_2_5_DriverInfor,   	//��ʻԱ��Ϣ		2
    &Menu_4_7_speedlog,  			//�ٶ���־��Ϣ		1
    &Menu_4_4_ExportData,			//��¼�����ݵ���	1
    &Menu_4_3_PulseCoefficient,	//����ϵ��			2
    &Menu_2_6_Mileage,   		//��̲鿴			2
    &Menu_4_2_chaosu,   		//���ٲ鿴			2
};


static void menuswitch(void)
{
    unsigned char i = 0;

    lcd_fill(0);
    lcd_text12(0, 3, "��¼��", 6, LCD_MODE_SET);
    lcd_text12(6, 17, "��Ϣ", 4, LCD_MODE_SET);
    for(i = 0; i < menu_num; i++)
        lcd_bitmap(40 + i * DIS_Dur_width_inter, 5, &BMP_noselect_log, LCD_MODE_SET);
    lcd_bitmap(40 + menu_pos * DIS_Dur_width_inter, 5, &BMP_select_log, LCD_MODE_SET);
    lcd_text12(37, 19, (char *)(psubmenu[menu_pos]->caption), psubmenu[menu_pos]->len, LCD_MODE_SET);
    lcd_update_all();
}


static void msg( void *p)
{
}
static void show(void)
{
    pMenuItem->tick = rt_tick_get( );
    if(( menu_pos > menu_num ) || (menu_first_in & BIT(3)))
    {
        menu_pos = 0;
    }
    menu_first_in &= ~(BIT(3));
    menuswitch();
}

/**/
static void keypress( unsigned int key )
{
    switch( key )
    {
    case KEY_MENU:
        menu_pos = 0;
        pMenuItem	= &Menu_1_menu;    //
        pMenuItem->show( );
        break;
    case KEY_OK:
        pMenuItem = psubmenu[menu_pos];
        pMenuItem->show( );
        break;
    case KEY_UP:
        if( menu_pos == 0 )
        {
            menu_pos = menu_num;
        }
        menu_pos--;
        menuswitch( );
        break;
    case KEY_DOWN:
        menu_pos++;
        if( menu_pos >= menu_num )
        {
            menu_pos = 0;
        }
        menuswitch( );
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
MENUITEM	Menu_4_recorder =
{
    "��¼����Ϣ",
    10, 0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};



