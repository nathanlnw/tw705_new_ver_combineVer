#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"


unsigned char CarType_counter = 0;
unsigned char CarType_Type = 0;


void CarType(unsigned char type_Sle, unsigned char sel)
{
    switch(type_Sle)
    {
    case 1:
        lcd_fill(0);
        if(sel == 0)
        {
            lcd_text12(0, 10, "���˳� ���γ� ΣƷ��", 20, LCD_MODE_SET);
            lcd_text12(0, 10, "���˳�", 6, LCD_MODE_INVERT);
        }
        else
            lcd_text12(12, 10, "��������:���˳�", 15, LCD_MODE_SET);
        lcd_update_all();
        break;
    case 2:
        lcd_fill(0);
        if(sel == 0)
        {
            lcd_text12(0, 10, "���˳� ���γ� ΣƷ��", 20, LCD_MODE_SET);
            lcd_text12(7 * 6, 10, "���γ�", 6, LCD_MODE_INVERT);
        }
        else
            lcd_text12(12, 10, "��������:���γ�", 15, LCD_MODE_SET);
        lcd_update_all();
        break;
    case 3:
        lcd_fill(0);
        if(sel == 0)
        {
            lcd_text12(0, 10, "���˳� ���γ� ΣƷ��", 20, LCD_MODE_SET);
            lcd_text12(14 * 6, 10, "ΣƷ��", 6, LCD_MODE_INVERT);
        }
        else
            lcd_text12(12, 10, "��������:ΣƷ��", 15, LCD_MODE_SET);
        lcd_update_all();
        break;


        break;
    }
}

static void msg( void *p)
{

}
static void show(void)
{
    CounterBack = 0;
    if(jt808_param.id_0xF00D == 1)
    {
        lcd_fill(0);
        lcd_text12(24, 3, "��������ѡ��", 12, LCD_MODE_SET);
        lcd_text12(0, 19, "��ȷ�ϼ�ѡ��������", 20, LCD_MODE_SET);
        lcd_update_all();

        CarType_counter = 1;
        CarType_Type = 1;

        CarType(CarType_counter, 0);
    }
    else
    {
        memcpy(Menu_VechileType, "���˳�", 6);
        lcd_fill(0);
        lcd_text12(15, 3, "��������:���˳�", 15, LCD_MODE_SET);
        lcd_text12(6, 18, "��ȷ�ϼ�������һ��", 18, LCD_MODE_SET);
        lcd_update_all();
    }
    //--printf("\r\n��������ѡ�� = %d",CarType_counter);
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:

        pMenuItem = &Menu_0_loggingin;
        pMenuItem->show();
        CarType_counter = 0;
        CarType_Type = 0;
        break;
    case KEY_OK:
        if(jt808_param.id_0xF00D == 1)
        {
            if(CarType_Type == 1)
            {
                CarType_Type = 2;
                CarType(CarType_counter, 1);
                //printf("\r\nCarType_Type = %d",CarType_Type);
            }
            else if(CarType_Type == 2)
            {
                //д�복������
                if((CarType_counter >= 1) && (CarType_counter <= 8))
                    memset(Menu_VechileType, 0, sizeof(Menu_VechileType));

                if(CarType_counter == 1)
                    memcpy(Menu_VechileType, "���˳�", 6);
                else if(CarType_counter == 2)
                    memcpy(Menu_VechileType, "���γ�", 6);
                else if(CarType_counter == 3)
                    memcpy(Menu_VechileType, "ΣƷ��", 6);

                CarType_Type = 3;
                // ��������
                if(MENU_set_carinfor_flag == 1)
                {
                    rt_kprintf("\r\n��������������ɣ����˵������أ�%s", Menu_VechileType);

                    memset(jt808_param.id_0xF00A, 0, sizeof(jt808_param.id_0xF00A));
                    memcpy(jt808_param.id_0xF00A, Menu_VechileType, strlen((const char *)Menu_VechileType));
                    param_save(1);
                }
                lcd_fill(0);
                lcd_text12(12, 3, "��������ѡ�����", 16, LCD_MODE_SET);
                lcd_text12(6, 18, "��ȷ�ϼ�������һ��", 18, LCD_MODE_SET);
                lcd_update_all();
            }
            else if(CarType_Type == 3)
            {
                CarSet_0_counter = 3; //���õ�3��
                pMenuItem = &Menu_0_loggingin;
                pMenuItem->show();

                CarType_counter = 0;
                CarType_Type = 0;
            }
        }
        else if(jt808_param.id_0xF00D == 2)
        {
            CarSet_0_counter = 3; //���õ�3��
            pMenuItem = &Menu_0_loggingin;
            pMenuItem->show();

            CarType_counter = 0;
            CarType_Type = 0;
        }

        break;
    case KEY_UP:
        if(	CarType_Type == 1)
        {
            if(CarType_counter == 1)
                CarType_counter = 3;
            else
                CarType_counter--;
            //printf("\r\n  up  ��������ѡ�� = %d",CarType_counter);
            CarType(CarType_counter, 0);
        }
        break;
    case KEY_DOWN:
        if(	CarType_Type == 1)
        {
            if(CarType_counter >= 3)
                CarType_counter = 1;
            else
                CarType_counter++;

            //printf("\r\n down ��������ѡ�� = %d",CarType_counter);
            CarType(CarType_counter, 0);
        }
        break;
    }
}


static void timetick(unsigned int systick)
{

}


MENUITEM	Menu_0_2_CarType =
{
    "��������ѡ��",
    12, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};


