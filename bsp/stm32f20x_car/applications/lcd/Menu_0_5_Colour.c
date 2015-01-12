#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

static uint8_t menu = 0;
uint8_t col_num = 0;


unsigned char *car_col[] =
{
    "蓝色",
    "黄色",
    "黑色",
    "白色",
    "其他",
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
        sprintf(buf, "车牌颜色:%s", car_col[col_num]);
        lcd_text12(20, 10, (char *)buf, strlen(buf), LCD_MODE_SET);
    }
    else
    {
        col_num_get();
        if(1 == menu)
        {
            sprintf(buf, "车牌颜色:%s", car_col[col_num]);
            lcd_text12(20, 3, (char *)buf, strlen(buf), LCD_MODE_SET);
            if(device_unlock)
            {
                lcd_text12(12, 18, "车牌颜色设置完成", 16, LCD_MODE_SET);
            }
            else
            {
                lcd_text12(12, 18, "按确认键查看信息", 16, LCD_MODE_SET);
            }
        }
        else if(2 == menu)
        {
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "无牌照", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, jt808_param.id_0x0083, 8, LCD_MODE_SET);
            lcd_text12(54, 0, jt808_param.id_0xF00A, strlen(jt808_param.id_0xF00A), LCD_MODE_SET);
            //====  车牌号未设置=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, "0", 1, LCD_MODE_SET);
            else
            {
                lcd_text12(96, 0, car_col[col_num], 4, LCD_MODE_SET);
            }

            lcd_text12(0, 12, "SIM卡号", 7, LCD_MODE_SET);
            lcd_text12(43, 12, jt808_param.id_0xF006, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "取消", 4, LCD_MODE_SET);
            lcd_text12(72, 23, "确定", 4, LCD_MODE_INVERT);
        }
        else if(3 == menu)
        {
            lcd_text12(18, 3, "保存已设置信息", 14, LCD_MODE_SET);
            lcd_text12(0, 18, "按菜单键进入待机界面", 20, LCD_MODE_SET);
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
            // 车牌颜色
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


unsigned char car_col[13] = {"车牌颜色:黄色"};

void car_col_fun(u8 par)
{
    //车牌颜色编码表
    if(par == 1)
        memcpy(Menu_VecLogoColor, "蓝色", 4);   //   1
    else if(par == 2)
        memcpy(Menu_VecLogoColor, "黄色", 4);  //   2
    else if(par == 3)
        memcpy(Menu_VecLogoColor, "黑色", 4);   //   3
    else if(par == 4)
        memcpy(Menu_VecLogoColor, "白色", 4);  //   4
    else if(par == 5)
    {
        memcpy(Menu_VecLogoColor, "其他", 4);
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
                // 车牌颜色
                if(jt808_param.id_0xF00E == 1)
                    jt808_param.id_0x0084 = 0;
                else
                    jt808_param.id_0x0084 = Menu_color_num;
                param_save(1);

                lcd_fill(0);
                lcd_text12(12, 3, "车牌颜色设置完成", 16, LCD_MODE_SET);
                lcd_text12(24, 18, "按菜单键返回", 12, LCD_MODE_SET);
                lcd_update_all();
                rt_kprintf("\r\n车牌颜色设置完成，按菜单键返回，%d", jt808_param.id_0x0084);
            }
            else
            {
                col_screen = 2;
                CarSet_0_counter = 0; //
                menu_color_flag = 1; //车牌颜色设置完成
                lcd_fill(0);
                lcd_text12(20, 3, (char *)car_col, 13, LCD_MODE_SET);
                lcd_text12(12, 18, "按确认键查看信息", 16, LCD_MODE_SET);
                lcd_update_all();
            }
        }
        else if(col_screen == 2)
        {
            menu_color_flag = 0;

            col_screen = 3;
            comfirmation_flag = 1; //保存设置信息标志
            lcd_fill(0);
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "无牌照", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  车牌号未设置=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM卡号", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "确定", 4, LCD_MODE_INVERT);
            lcd_text12(72, 23, "取消", 4, LCD_MODE_SET);
            lcd_update_all();
        }
        else if(comfirmation_flag == 1)
        {
            col_screen = 0;
            comfirmation_flag = 4;
            //保存设置的信息
            lcd_fill(0);
            lcd_text12(18, 3, "保存已设置信息", 14, LCD_MODE_SET);
            lcd_text12(0, 18, "按菜单键进入待机界面", 20, LCD_MODE_SET);
            lcd_update_all();

            // 车牌颜色
            if(jt808_param.id_0xF00E == 1)
                jt808_param.id_0x0084 = 0;
            else
                jt808_param.id_0x0084 = Menu_color_num;
            //车辆设置完成
            jt808_param.id_0xF00F = 1;   //  输入界面为0
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
            lcd_text12(6, 3, "请确认是否重新设置", 18, LCD_MODE_SET);
            lcd_text12(12, 18, "按确认键重新设置", 16, LCD_MODE_SET);
            lcd_update_all();
        }
        else if(comfirmation_flag == 3)
        {
            col_screen = 0;
            comfirmation_flag = 0;
            //重新设置
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
                lcd_text12(0, 0, "无牌照", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  车牌号未设置=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM卡号", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "确定", 4, LCD_MODE_INVERT);
            lcd_text12(72, 23, "取消", 4, LCD_MODE_SET);
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
                lcd_text12(0, 0, "无牌照", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  车牌号未设置=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM卡号", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "确定", 4, LCD_MODE_SET);
            lcd_text12(72, 23, "取消", 4, LCD_MODE_INVERT);
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
                // 车牌颜色
                if(jt808_param.id_0xF00E == 1)
                    jt808_param.id_0x0084 = 0;
                else
                    jt808_param.id_0x0084 = Menu_color_num;
                param_save(1);

                lcd_fill(0);
                lcd_text12(12, 3, "车牌颜色设置完成", 16, LCD_MODE_SET);
                lcd_text12(24, 18, "按菜单键返回", 12, LCD_MODE_SET);
                lcd_update_all();
                rt_kprintf("\r\n车牌颜色设置完成，按菜单键返回，%d", jt808_param.id_0x0084);
            }
            else
            {
                col_screen = 2;
                CarSet_0_counter = 0; //
                menu_color_flag = 1; //车牌颜色设置完成
                lcd_fill(0);
                lcd_text12(20, 3, (char *)car_col, 13, LCD_MODE_SET);
                lcd_text12(12, 18, "按确认键查看信息", 16, LCD_MODE_SET);
                lcd_update_all();
            }
        }
        else if(col_screen == 2)
        {
            menu_color_flag = 0;

            col_screen = 3;
            comfirmation_flag = 1; //保存设置信息标志
            lcd_fill(0);
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(0, 0, "无牌照", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  车牌号未设置=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM卡号", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "确定", 4, LCD_MODE_INVERT);
            lcd_text12(72, 23, "取消", 4, LCD_MODE_SET);
            lcd_update_all();
        }
        else if(comfirmation_flag == 1)
        {
            col_screen = 0;
            comfirmation_flag = 4;
            //保存设置的信息
            lcd_fill(0);
            lcd_text12(18, 3, "保存已设置信息", 14, LCD_MODE_SET);
            lcd_text12(0, 18, "按菜单键进入待机界面", 20, LCD_MODE_SET);
            lcd_update_all();

            // 车牌颜色
            if(jt808_param.id_0xF00E == 1)
                jt808_param.id_0x0084 = 0;
            else
                jt808_param.id_0x0084 = Menu_color_num;
            //车辆设置完成
            jt808_param.id_0xF00F = 1;   //  输入界面为0
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
            lcd_text12(6, 3, "请确认是否重新设置", 18, LCD_MODE_SET);
            lcd_text12(12, 18, "按确认键重新设置", 16, LCD_MODE_SET);
            lcd_update_all();
        }
        else if(comfirmation_flag == 3)
        {
            col_screen = 0;
            comfirmation_flag = 0;
            //重新设置
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
                lcd_text12(0, 0, "无牌照", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  车牌号未设置=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM卡号", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "确定", 4, LCD_MODE_INVERT);
            lcd_text12(72, 23, "取消", 4, LCD_MODE_SET);
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
                lcd_text12(0, 0, "无牌照", 6, LCD_MODE_SET);
            else
                lcd_text12(0, 0, (char *)Menu_Car_license, 8, LCD_MODE_SET);
            lcd_text12(54, 0, (char *)Menu_VechileType, 6, LCD_MODE_SET);
            //====  车牌号未设置=====
            if(jt808_param.id_0xF00E == 1)
                lcd_text12(96, 0, (char *)"0", 1, LCD_MODE_SET);
            else
                lcd_text12(96, 0, (char *)Menu_VecLogoColor, 4, LCD_MODE_SET);

            lcd_text12(0, 12, "SIM卡号", 7, LCD_MODE_SET);
            lcd_text12(43, 12, (char *)Menu_sim_Code, 12, LCD_MODE_SET);
            lcd_text12(24, 23, "确定", 4, LCD_MODE_SET);
            lcd_text12(72, 23, "取消", 4, LCD_MODE_INVERT);
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
    "车牌颜色输入",
    12, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};


