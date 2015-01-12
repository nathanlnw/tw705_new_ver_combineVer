#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

static uint8_t menu = 0;
uint8_t col_num = 0;


unsigned char *car_col[] =
{
    "��ɫ",
    "��ɫ",
    "��ɫ",
    "��ɫ",
    "����",
};

static void col_num_get(void)
{
    if((jt808_param.id_0x0084 <= 4) && (jt808_param.id_0x0084))
    {
        col_num = jt808_param.id_0x0084 - 1;
    }
    else
    {
        col_num = 5;
    }
}


static void dis_pro(void)
{
    char buf[22];

    lcd_fill(0);
    if( 0 == menu )
    {
        sprintf(buf, "������ɫ:%s", car_col[col_num]);
        lcd_text12(20, 10, (char *)buf, strlen(buf), LCD_MODE_SET);
    }
    else
    {
        col_num_get();
        if(1 == menu)
        {
            sprintf(buf, "������ɫ:%s", car_col[col_num]);
            lcd_text12(20, 3, (char *)buf, strlen(buf), LCD_MODE_SET);
            if(device_unlock)
            {
                lcd_text12(12, 18, "������ɫ�������", 16, LCD_MODE_SET);
            }
            else
            {
                lcd_text12(12, 18, "��ȷ�ϼ��鿴��Ϣ", 16, LCD_MODE_SET);
            }
        }
        else if(2 == menu)
        {
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "������", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, jt808_param.id_0x0083, 8, LCD_MODE_SET);
            lcd_text12(54, 0, jt808_param.id_0xF00A, strlen(jt808_param.id_0xF00A), LCD_MODE_SET);
            //====  ���ƺ�δ����=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, "0", 1, LCD_MODE_SET);
            else
            {
                lcd_text12(96, 0, car_col[col_num], 4, LCD_MODE_SET);
            }

            lcd_text12(0, 12, "SIM����", 7, LCD_MODE_SET);
            lcd_text12(43, 12, jt808_param.id_0xF006, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "ȡ��", 4, LCD_MODE_SET);
            lcd_text12(72, 23, "ȷ��", 4, LCD_MODE_INVERT);
        }
        else if(3 == menu)
        {
            lcd_text12(18, 3, "������������Ϣ", 14, LCD_MODE_SET);
            lcd_text12(0, 18, "���˵��������������", 20, LCD_MODE_SET);
            rt_thread_delay(RT_TICK_PER_SECOND * 2);
        }
    }
    lcd_update_all();
}


static void show(void)
{
    menu = 0;
    col_num_get();
    dis_pro();
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        pMenuItem = &Menu_0_loggingin;
        pMenuItem->show();
        return;
    case KEY_OK:
        if(menu < 3)
            menu++;
        if(menu == 1)
        {
            // ������ɫ
            if(jt808_param.id_0xF00E == 1)
                jt808_param.id_0x0084 = 0;
            else
            {
                if(col_num < 4)
                    jt808_param.id_0x0084 = col_num + 1;
                else
                    jt808_param.id_0x0084 = 9;

            }
            param_save(1);
        }
        else if(menu == 2)
        {
            if(device_unlock)
            {
                pMenuItem = &Menu_0_loggingin;
                pMenuItem->show();
                return;
            }
        }
        else if(menu == 3)
        {
            jt808_param.id_0xF00F = 1;
            param_save(1);
        }
        break;
    case KEY_UP:
        if(	menu == 0)
        {
            if( col_num == 0 )
            {
                col_num = 5;
            }
            col_num--;
        }
        break;
    case KEY_DOWN:
        if(	menu == 0)
        {
            col_num++;
            if( col_num >= 5 )
            {
                col_num = 0;
            }
        }
        break;
    }
    dis_pro();
}

static void msg( void *p)
{

}

#if 0
u8 comfirmation_flag = 0;
u8 col_screen = 0;
u8 CarBrandCol_Cou = 1;


unsigned char car_col[13] = {"������ɫ:��ɫ"};

void car_col_fun(u8 par)
{
    //������ɫ�����
    if(par == 1)
        memcpy(Menu_VecLogoColor, "��ɫ", 4);   //   1
    else if(par == 2)
        memcpy(Menu_VecLogoColor, "��ɫ", 4);  //   2
    else if(par == 3)
        memcpy(Menu_VecLogoColor, "��ɫ", 4);   //   3
    else if(par == 4)
        memcpy(Menu_VecLogoColor, "��ɫ", 4);  //   4
    else if(par == 5)
    {
        memcpy(Menu_VecLogoColor, "����", 4);
        par = 9;
    } //   9

    Menu_color_num = par;

    memcpy(car_col + 9, Menu_VecLogoColor, 4);
    lcd_fill(0);
    lcd_text12(20, 10, (char *)car_col, 13, LCD_MODE_SET);
    lcd_update_all();
}


static void msg( void *p)
{

}
static void show(void)
{
    CounterBack = 0;
    col_screen = 1;
    car_col_fun(2);// old 1
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        pMenuItem = &Menu_0_loggingin;
        pMenuItem->show();
        col_screen = 0;
        CarBrandCol_Cou = 1;
        comfirmation_flag = 0;
        break;
    case KEY_OK:
        if(col_screen == 1)
        {
            if(MENU_set_carinfor_flag == 1)
            {
                comfirmation_flag = 4;
                // ������ɫ
                if(jt808_param.id_0xF00E == 1)
                    jt808_param.id_0x0084 = 0;
                else
                    jt808_param.id_0x0084 = Menu_color_num;
                param_save(1);

                lcd_fill(0);
                lcd_text12(12, 3, "������ɫ�������", 16, LCD_MODE_SET);
                lcd_text12(24, 18, "���˵�������", 12, LCD_MODE_SET);
                lcd_update_all();
                rt_kprintf("\r\n������ɫ������ɣ����˵������أ�%d", jt808_param.id_0x0084);
            }
            else
            {
                col_screen = 2;
                CarSet_0_counter = 0; //
                menu_color_flag = 1; //������ɫ�������
                lcd_fill(0);
                lcd_text12(20, 3, (char *)car_col, 13, LCD_MODE_SET);
                lcd_text12(12, 18, "��ȷ�ϼ��鿴��Ϣ", 16, LCD_MODE_SET);
                lcd_update_all();
            }
        }
        else if(col_screen == 2)
        {
            menu_color_flag = 0;

            col_screen = 3;
            comfirmation_flag = 1; //����������Ϣ��־
            lcd_fill(0);
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "������", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  ���ƺ�δ����=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM����", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "ȷ��", 4, LCD_MODE_INVERT);
            lcd_text12(72, 23, "ȡ��", 4, LCD_MODE_SET);
            lcd_update_all();
        }
        else if(comfirmation_flag == 1)
        {
            col_screen = 0;
            comfirmation_flag = 4;
            //�������õ���Ϣ
            lcd_fill(0);
            lcd_text12(18, 3, "������������Ϣ", 14, LCD_MODE_SET);
            lcd_text12(0, 18, "���˵��������������", 20, LCD_MODE_SET);
            lcd_update_all();

            // ������ɫ
            if(jt808_param.id_0xF00E == 1)
                jt808_param.id_0x0084 = 0;
            else
                jt808_param.id_0x0084 = Menu_color_num;
            //�����������
            jt808_param.id_0xF00F = 1;   //  �������Ϊ0
            memset(jt808_param.id_0xF003, 0, sizeof(jt808_param.id_0xF003));
            if(get_sock_state(0))
                jt808_state = JT808_REGISTER;
            //------------------------------------------------------------------------------------
            param_save(1);
        }
        else if(comfirmation_flag == 2)
        {
            col_screen = 0;
            comfirmation_flag = 3;
            lcd_fill(0);
            lcd_text12(6, 3, "��ȷ���Ƿ���������", 18, LCD_MODE_SET);
            lcd_text12(12, 18, "��ȷ�ϼ���������", 16, LCD_MODE_SET);
            lcd_update_all();
        }
        else if(comfirmation_flag == 3)
        {
            col_screen = 0;
            comfirmation_flag = 0;
            //��������
            pMenuItem = &Menu_0_loggingin;
            pMenuItem->show();

            comfirmation_flag = 0;
            col_screen = 0;
            CarBrandCol_Cou = 1;
        }

        break;
    case KEY_UP:
        if(col_screen == 1)
        {
            CarBrandCol_Cou--;
            if(CarBrandCol_Cou < 1)
                CarBrandCol_Cou = 5;
            car_col_fun(CarBrandCol_Cou);
        }
        else if(col_screen == 3)
        {
            comfirmation_flag = 1;

            lcd_fill(0);
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "������", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  ���ƺ�δ����=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM����", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "ȷ��", 4, LCD_MODE_INVERT);
            lcd_text12(72, 23, "ȡ��", 4, LCD_MODE_SET);
            lcd_update_all();
        }

        break;
    case KEY_DOWN:
        if(col_screen == 1)
        {
            CarBrandCol_Cou++;
            if(CarBrandCol_Cou > 5)
                CarBrandCol_Cou = 1;
            car_col_fun(CarBrandCol_Cou);
        }
        else if(col_screen == 3)
        {
            comfirmation_flag = 2;
            lcd_fill(0);
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "������", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  ���ƺ�δ����=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM����", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "ȷ��", 4, LCD_MODE_SET);
            lcd_text12(72, 23, "ȡ��", 4, LCD_MODE_INVERT);
            lcd_update_all();
        }

        break;
    }
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        pMenuItem = &Menu_0_loggingin;
        pMenuItem->show();
        col_screen = 0;
        CarBrandCol_Cou = 1;
        comfirmation_flag = 0;
        break;
    case KEY_OK:
        if(col_screen == 1)
        {
            if(MENU_set_carinfor_flag == 1)
            {
                comfirmation_flag = 4;
                // ������ɫ
                if(jt808_param.id_0xF00E == 1)
                    jt808_param.id_0x0084 = 0;
                else
                    jt808_param.id_0x0084 = Menu_color_num;
                param_save(1);

                lcd_fill(0);
                lcd_text12(12, 3, "������ɫ�������", 16, LCD_MODE_SET);
                lcd_text12(24, 18, "���˵�������", 12, LCD_MODE_SET);
                lcd_update_all();
                rt_kprintf("\r\n������ɫ������ɣ����˵������أ�%d", jt808_param.id_0x0084);
            }
            else
            {
                col_screen = 2;
                CarSet_0_counter = 0; //
                menu_color_flag = 1; //������ɫ�������
                lcd_fill(0);
                lcd_text12(20, 3, (char *)car_col, 13, LCD_MODE_SET);
                lcd_text12(12, 18, "��ȷ�ϼ��鿴��Ϣ", 16, LCD_MODE_SET);
                lcd_update_all();
            }
        }
        else if(col_screen == 2)
        {
            menu_color_flag = 0;

            col_screen = 3;
            comfirmation_flag = 1; //����������Ϣ��־
            lcd_fill(0);
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "������", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  ���ƺ�δ����=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM����", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "ȷ��", 4, LCD_MODE_INVERT);
            lcd_text12(72, 23, "ȡ��", 4, LCD_MODE_SET);
            lcd_update_all();
        }
        else if(comfirmation_flag == 1)
        {
            col_screen = 0;
            comfirmation_flag = 4;
            //�������õ���Ϣ
            lcd_fill(0);
            lcd_text12(18, 3, "������������Ϣ", 14, LCD_MODE_SET);
            lcd_text12(0, 18, "���˵��������������", 20, LCD_MODE_SET);
            lcd_update_all();

            // ������ɫ
            if(jt808_param.id_0xF00E == 1)
                jt808_param.id_0x0084 = 0;
            else
                jt808_param.id_0x0084 = Menu_color_num;
            //�����������
            jt808_param.id_0xF00F = 1;   //  �������Ϊ0
            memset(jt808_param.id_0xF003, 0, sizeof(jt808_param.id_0xF003));
            if(get_sock_state(0))
                jt808_state = JT808_REGISTER;
            //------------------------------------------------------------------------------------
            param_save(1);
        }
        else if(comfirmation_flag == 2)
        {
            col_screen = 0;
            comfirmation_flag = 3;
            lcd_fill(0);
            lcd_text12(6, 3, "��ȷ���Ƿ���������", 18, LCD_MODE_SET);
            lcd_text12(12, 18, "��ȷ�ϼ���������", 16, LCD_MODE_SET);
            lcd_update_all();
        }
        else if(comfirmation_flag == 3)
        {
            col_screen = 0;
            comfirmation_flag = 0;
            //��������
            pMenuItem = &Menu_0_loggingin;
            pMenuItem->show();

            comfirmation_flag = 0;
            col_screen = 0;
            CarBrandCol_Cou = 1;
        }

        break;
    case KEY_UP:
        if(col_screen == 1)
        {
            CarBrandCol_Cou--;
            if(CarBrandCol_Cou < 1)
                CarBrandCol_Cou = 5;
            car_col_fun(CarBrandCol_Cou);
        }
        else if(col_screen == 3)
        {
            comfirmation_flag = 1;

            lcd_fill(0);
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "������", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  ���ƺ�δ����=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM����", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "ȷ��", 4, LCD_MODE_INVERT);
            lcd_text12(72, 23, "ȡ��", 4, LCD_MODE_SET);
            lcd_update_all();
        }

        break;
    case KEY_DOWN:
        if(col_screen == 1)
        {
            CarBrandCol_Cou++;
            if(CarBrandCol_Cou > 5)
                CarBrandCol_Cou = 1;
            car_col_fun(CarBrandCol_Cou);
        }
        else if(col_screen == 3)
        {
            comfirmation_flag = 2;
            lcd_fill(0);
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "������", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  ���ƺ�δ����=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM����", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "ȷ��", 4, LCD_MODE_SET);
            lcd_text12(72, 23, "ȡ��", 4, LCD_MODE_INVERT);
            lcd_update_all();
        }

        break;
    }
}
#endif

static void timetick(unsigned int systick)
{

}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_0_Colour =
{
    "������ɫ����",
    12, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};


