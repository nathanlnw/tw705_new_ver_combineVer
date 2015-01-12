#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

#define  pulse_width2  6

static u8 pulse_sel_screen = 0; //
static u8 pulse_coefficient_num = 0; //�����ã�5λ
static u8 pulse_coefficient_set = 0; //ÿһλ��������ݣ�0-9
static u8 pulse_coefficient_arr[6];//����ϵ��

static unsigned char select_oil[] = {0x0C, 0x06, 0xFF, 0x06, 0x0C};

DECL_BMP(8, 5, select_oil);

static void pulse_Set(u8 par)
{
    lcd_fill(0);
    lcd_text12(0, 3, (char *)pulse_coefficient_arr, pulse_coefficient_num - 1, LCD_MODE_SET);
    lcd_bitmap(par * pulse_width2, 14, &BMP_select_oil, LCD_MODE_SET);
    lcd_text12(0, 19, "0123456789", 10, LCD_MODE_SET);
    lcd_update_all();
}
static void msg( void *p)
{
}

static void show(void)
{
    pulse_sel_screen = 1;
    pulse_coefficient_num = 0; //��ʼ����5λ����ϵ��
    pulse_coefficient_set = 0; //ÿһλ��������ݣ�0-9
    memset(pulse_coefficient_arr, 0, sizeof(pulse_coefficient_arr));
    lcd_fill(0);
    lcd_text12(10, 3, "������5λ�������", 17, LCD_MODE_SET);
    lcd_text12(19, 18, "��λΪƽ������", 14, LCD_MODE_SET);
    lcd_update_all();
}


static void keypress(unsigned int key)
{
    uint32_t data_u32;
    switch(key)
    {
    case KEY_MENU:
        pulse_sel_screen = 0; //
        pulse_coefficient_num = 0; //�����ã�5λ
        CounterBack = 0;
        pMenuItem = &Menu_0_loggingin;
        pMenuItem->show();
        break;

    case KEY_OK:
        if(pulse_sel_screen == 1) //����Ҫ���õ�����ϵ��
        {
            //����ϵ������
            if((pulse_coefficient_num >= 0) && (pulse_coefficient_num <= 5))
            {
                if((pulse_coefficient_set <= 9) && (pulse_coefficient_num > 0))
                {
                    pulse_coefficient_arr[pulse_coefficient_num - 1] = pulse_coefficient_set + '0';
                }
                pulse_coefficient_num++;
                pulse_coefficient_set = 0;
                pulse_Set(0);
            }
            else if(pulse_coefficient_num == 6)
            {
                pulse_coefficient_num = 7;

                lcd_fill(0);
                lcd_text12(0, 3, (char *)pulse_coefficient_arr, 5, LCD_MODE_SET);
                lcd_text12(13, 19, "��������������", 16, LCD_MODE_SET);
                lcd_update_all();
                if(sscanf(pulse_coefficient_arr, "%d", &data_u32))
                {
                    rt_kprintf("\n �������=%d ", data_u32);
                    jt808_param.id_0xF04B = data_u32;
                    param_save(1);
                }
                //��ӱ�����Ϣ

            }
            else if(pulse_coefficient_num == 7)
            {
                CarSet_0_counter = 7;
                pulse_sel_screen = 0; //

                pulse_coefficient_num = 0; //�����ã�5λ
                pulse_coefficient_set = 0; //ÿһλ��������ݣ�0-9
                memset(pulse_coefficient_arr, 0, sizeof(pulse_coefficient_arr));

                pMenuItem = &Menu_0_loggingin;
                pMenuItem->show();
            }
        }
        break;

    case KEY_UP:
        if((pulse_sel_screen == 1) && (pulse_coefficient_num)) //��������ϵ��
        {
            if(pulse_coefficient_num <= 5)
            {
                if(pulse_coefficient_set < 1)
                    pulse_coefficient_set = 9;
                else
                    pulse_coefficient_set--;
                pulse_Set(pulse_coefficient_set);
            }
        }
        break;

    case KEY_DOWN:
        if((pulse_sel_screen == 1) && (pulse_coefficient_num)) //��������ϵ��
        {
            if(pulse_coefficient_num <= 5)
            {
                if(pulse_coefficient_set == 9)
                    pulse_coefficient_set = 0;
                else
                    pulse_coefficient_set++;
                pulse_Set(pulse_coefficient_set);
            }
        }
        break;

    }
}


static void timetick(unsigned int systick)
{

    CounterBack++;
    if(CounterBack < MaxBankIdleTime)
        return;
    pMenuItem = &Menu_1_Idle;
    pMenuItem->show();
    CounterBack = 0;

}



ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_0_7_OilCoefficient =
{
    "�����������",
    12, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

