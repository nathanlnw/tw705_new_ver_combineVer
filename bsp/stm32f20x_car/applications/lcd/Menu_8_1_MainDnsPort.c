#include "Menu_Include.h"
//#include "App_moduleConfig.h"
#include "LCD_Driver.h"

#define  MainDns_width1  6

///输入格式为"域名:端口好#",井号为结束符，如"jt1.gghypt.net:7008#"

static u8 MainDns_SetFlag = 1;
static u8 MainDns_SetCounter_0 = 0, MainDns_SetCounter_1 = 0, MainDns_SetCounter_2 = 0;
static u8 MainDns_SetComp = 0; //主域名设置完成1
static u8 MainDns_AffirmCancel = 0; // 1:确认 2:取消

static u8 MainDns_Type_flag = 0; //区分组的选择和组内选择
static u8 MainDns_Type_Counter = 0; //  0: 数字    1:A-M         2:N-Z

unsigned char select_MainDns[] = {0x0C, 0x06, 0xFF, 0x06, 0x0C};
unsigned char ABC_MainDns_0_9[13][1] = {".", ":", "#", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
unsigned char ABC_MainDns_A_M[13][1] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m"};
unsigned char ABC_MainDns_N_Z[13][1] = {"n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};


DECL_BMP(8, 5, select_MainDns);

void MainDns_Type_Sel( u8 par)
{
    lcd_fill(0);
    lcd_text12(0, 3, (char *)Menu_MainDns, MainDns_SetFlag - 1, LCD_MODE_SET);
    lcd_text12(20, 18, ".-9  a-m  n-z", 13, LCD_MODE_SET);
    if(par == 0)
        lcd_text12(20, 18, ".-9", 3, LCD_MODE_INVERT);
    else if(par == 1)
        lcd_text12(20 + 5 * 6, 18, "a-m", 3, LCD_MODE_INVERT);
    else if(par == 2)
        lcd_text12(20 + 10 * 6, 18, "n-z", 3, LCD_MODE_INVERT);
    lcd_update_all();
}

void MainDns_Set(u8 par, u8 type1_2)
{
    lcd_fill(0);
    lcd_text12(0, 3, (char *)Menu_MainDns, MainDns_SetFlag - 1, LCD_MODE_SET);

    if(type1_2 == 1)
    {
        lcd_bitmap(par * MainDns_width1, 14, &BMP_select_MainDns, LCD_MODE_SET);
        lcd_text12(0, 19, ".:#0123456789", 13, LCD_MODE_SET);
    }
    else if(type1_2 == 2)
    {
        lcd_bitmap(par * MainDns_width1, 14, &BMP_select_MainDns, LCD_MODE_SET);
        lcd_text12(0, 19, "abcdefghijklm", 13, LCD_MODE_SET);
    }
    else if(type1_2 == 3)
    {
        lcd_bitmap(par * MainDns_width1, 14, &BMP_select_MainDns, LCD_MODE_SET);
        lcd_text12(0, 19, "nopqrstuvwxyz", 13, LCD_MODE_SET);
    }
    lcd_update_all();
}

static void msg( void *p)
{

}
static void show(void)
{
    CounterBack = 0;
    MainDns_Type_Counter = 0;
    MainDns_Type_Sel(MainDns_Type_Counter);
    MainDns_Type_flag = 1;
    //rt_kprintf("\r\n选择要输入的类型");

}


static void keypress(unsigned int key)
{
    u8  Dnstr[22], i = 0;

    switch(key)
    {
    case KEY_MENU:

        MainDns_SetFlag = 1;
        MainDns_SetCounter_0 = 0;
        MainDns_SetCounter_1 = 0;
        MainDns_SetCounter_2 = 0;
        MainDns_SetComp = 0; //主域名设置完成1
        MainDns_AffirmCancel = 0; // 1:确认 2:取消

        MainDns_Type_flag = 0; //区分组的选择和组内选择
        MainDns_Type_Counter = 0; //  0: 数字    1:A-M         2:N-Z


        memset(Menu_MainDns, 0, sizeof(Menu_MainDns));
        pMenuItem = &Menu_8_SetDNS;
        pMenuItem->show();
        break;
    case KEY_OK:
        if((MainDns_Type_flag == 1) && (MainDns_SetComp == 0))
        {
            MainDns_Type_flag = 2;
            if((MainDns_SetFlag >= 1) && (MainDns_SetFlag <= 20))
            {
                if(MainDns_Type_Counter == 0)
                    MainDns_Set(MainDns_SetCounter_0, 1);
                else if(MainDns_Type_Counter == 1)
                    MainDns_Set(MainDns_SetCounter_1, 2);
                else if(MainDns_Type_Counter == 2)
                    MainDns_Set(MainDns_SetCounter_2, 3);
                //rt_kprintf("\r\n第%d组",MainDns_Type_Counter);
            }
        }
        else if((MainDns_Type_flag == 2) && (MainDns_SetComp == 0))
        {
            MainDns_Type_flag = 3;
            if((MainDns_SetFlag >= 1) && (MainDns_SetFlag <= 20))
            {
                if((ABC_MainDns_0_9[MainDns_SetCounter_0][0] == 0x23) || (MainDns_SetFlag == 20)) //'#'
                {
                    if((MainDns_SetFlag == 20) && (ABC_MainDns_0_9[MainDns_SetCounter_0][0] != 0x23))
                    {
                        Menu_MainDns[MainDns_SetFlag - 1] = ABC_MainDns_0_9[MainDns_SetCounter_0][0];
                    }
                    if(DNS_Main_Aux == 1)
                        rt_kprintf("\r\n*****主域名输入完成***** %s", Menu_MainDns);
                    else if(DNS_Main_Aux == 2)
                        rt_kprintf("\r\n*****辅域名输入完成***** %s", Menu_MainDns);
                    //#if 0
                    //主dns及端口输入完成
                    //  -----------  格式判断 -----------   是单域名还是域名+ 端口---
                    // 1.step  find :
                    for(i = 0; i < strlen((const char *)Menu_MainDns); i++)
                    {
                        if(Menu_MainDns[i] == ':')  break;
                    }
                    // 2. judge   resualt
                    if(i == strlen((const char *)Menu_MainDns))			// 只有域名
                    {
                        if(DNS_Main_Aux == 1)
                        {
                            memset(jt808_param.id_0xF049, 0, sizeof(jt808_param.id_0xF049));
                            memcpy(jt808_param.id_0xF049, Menu_MainDns, i);
                            memcpy(jt808_param.id_0x0013, jt808_param.id_0xF049, sizeof(jt808_param.id_0xF049));
                            memcpy(jt808_param.id_0x0017, jt808_param.id_0xF049, sizeof(jt808_param.id_0xF049));

                            param_save(1);
                            rt_kprintf("\r\n 无端口(主) dns:%s", Menu_MainDns);
                        }
                        else
                        {
                            //在这里添加备份域名保存
                            //
                            memset(jt808_param.id_0x0017, 0, sizeof(jt808_param.id_0x0017));
                            memcpy(jt808_param.id_0x0017, Menu_MainDns, i);
                            param_save(1);
                            rt_kprintf("\r\n 无端口(辅) dns:%s", Menu_MainDns);
                        }
                    }
                    else   // 域名+port
                    {
                        if(DNS_Main_Aux == 1)
                        {
                            memset(jt808_param.id_0xF049, 0, sizeof(jt808_param.id_0xF049));
                            memset(jt808_param.id_0xF04A, 0, sizeof(jt808_param.id_0xF04A));

                            memcpy(jt808_param.id_0xF049, Menu_MainDns, i);
                            jt808_param.id_0xF04A = (Menu_MainDns[i + 1] - '0') * 1000 + (Menu_MainDns[i + 2] - '0') * 100 + (Menu_MainDns[i + 3] - '0') * 10 + Menu_MainDns[i + 4] - '0';

                            memcpy(jt808_param.id_0x0013, jt808_param.id_0xF049, sizeof(jt808_param.id_0xF049));
                            memcpy(jt808_param.id_0x0017, jt808_param.id_0xF049, sizeof(jt808_param.id_0xF049));
                            jt808_param.id_0xF031 = jt808_param.id_0x0018 = jt808_param.id_0xF04A;
                            param_save(1);
                            rt_kprintf("\r\n        dns(主):%s", Menu_MainDns);
                            rt_kprintf("\r\n       端口(主):%d", jt808_param.id_0xF04A);
                        }
                        else if(DNS_Main_Aux == 2)
                        {
                            memset(jt808_param.id_0x0017, 0, sizeof(jt808_param.id_0x0017));
                            memset(jt808_param.id_0xF031, 0, sizeof(jt808_param.id_0xF031));

                            memcpy(jt808_param.id_0x0017, Menu_MainDns, i);
                            jt808_param.id_0xF031 = (Menu_MainDns[i + 1] - '0') * 1000 + (Menu_MainDns[i + 2] - '0') * 100 + (Menu_MainDns[i + 3] - '0') * 10 + Menu_MainDns[i + 4] - '0';
                            param_save(1);
                            //在这里添加备份域名和端口保存
                            rt_kprintf("\r\n        dns(辅):%s", Menu_MainDns);
                            rt_kprintf("\r\n       端口(辅):%d", jt808_param.id_0xF031);
                        }

                    }
                    //#endif
                    //-----------------------------------------------
                    Menu_MainDns[MainDns_SetFlag - 1] = ABC_MainDns_0_9[MainDns_SetCounter_0][0];
                    //MainDns_SetFlag++;
                    MainDns_SetComp = 1;
                    MainDns_AffirmCancel = 1;
                    lcd_fill(0);
                    lcd_text12(0, 3, (char *)Menu_MainDns, MainDns_SetFlag, LCD_MODE_SET);
                    lcd_text12(24, 18, "确认    取消", 12, LCD_MODE_SET);
                    lcd_text12(24, 18, "确认", 4, LCD_MODE_INVERT);
                    lcd_update_all();
                    //rt_kprintf("\r\n*****主域名输入完成***** %s",Menu_MainDns);
                }
                else
                {
                    if(MainDns_Type_Counter == 0)
                    {
                        Menu_MainDns[MainDns_SetFlag - 1] = ABC_MainDns_0_9[MainDns_SetCounter_0][0];
                        MainDns_SetFlag++;
                        MainDns_Set(MainDns_SetCounter_0, 1);
                        //rt_kprintf("\r\n(0_9选择)=%d",MainDns_SetCounter_0);
                    }
                    else if(MainDns_Type_Counter == 1)
                    {
                        Menu_MainDns[MainDns_SetFlag - 1] = ABC_MainDns_A_M[MainDns_SetCounter_1][0];
                        MainDns_SetFlag++;
                        MainDns_Set(MainDns_SetCounter_1, 2);
                        //rt_kprintf("\r\n(A_M选择)=%d",MainDns_SetCounter_1);
                    }
                    else if(MainDns_Type_Counter == 2)
                    {
                        Menu_MainDns[MainDns_SetFlag - 1] = ABC_MainDns_N_Z[MainDns_SetCounter_2][0];
                        MainDns_SetFlag++;
                        MainDns_Set(MainDns_SetCounter_2, 3);
                        //rt_kprintf("\r\n(N_Z选择)=%d",MainDns_SetCounter_2);
                    }
                    if(MainDns_Type_flag == 3)
                    {
                        MainDns_Type_flag = 1;
                        MainDns_SetCounter_0 = 0;
                        MainDns_SetCounter_1 = 0;
                        MainDns_SetCounter_2 = 0;

                        MainDns_Type_Sel(MainDns_Type_Counter);
                        //rt_kprintf("\r\n重新选组(1_2_3)=%d",MainDns_Type_Counter);
                    }
                }
            }
        }
        else if(MainDns_SetComp == 1)
        {
            if(MainDns_AffirmCancel == 1) //确认
            {
                /*MainDns_AffirmCancel=3;
                MainDns_SetComp=3;

                MainDns_SetFlag=0;
                MainDns_SetComp=0;*/

                MainDns_SetFlag = 1;
                MainDns_SetCounter_0 = 0;
                MainDns_SetCounter_1 = 0;
                MainDns_SetCounter_2 = 0;
                MainDns_SetComp = 0; //主域名设置完成1
                MainDns_AffirmCancel = 0; // 1:确认 2:取消

                MainDns_Type_flag = 0; //区分组的选择和组内选择
                MainDns_Type_Counter = 0; //  0: 数字    1:A-M         2:N-Z

                lcd_fill(0);
                if(DNS_Main_Aux == 1)
                {
                    lcd_text12(18, 3, "主域名设置完成", 14, LCD_MODE_SET);
                    lcd_text12(24, 18, "按菜单键退出", 12, LCD_MODE_SET);
                }
                else if(DNS_Main_Aux == 2)
                {
                    lcd_text12(18, 3, "辅域名设置完成", 14, LCD_MODE_SET);
                    lcd_text12(24, 18, "按菜单键退出", 12, LCD_MODE_SET);
                }
                lcd_update_all();
                //rt_kprintf("\r\n主域名%s",Menu_MainDns);


                //添加相应的操作，主域名存在Menu_MainDns[]中
                //格式【jt1.gghypt.net:7008#】
                //:    域名和端口的分隔符，#  设置完成结束符



            }
            else if(MainDns_AffirmCancel == 2) //取消
            {
                MainDns_SetComp = 2;
                MainDns_AffirmCancel = 4; //重新设置主域名
                lcd_fill(0);
                lcd_text12(12, 10, "按确认键重新设置", 16, LCD_MODE_SET);
                lcd_update_all();

            }
        }

        else if(MainDns_SetComp == 2)
        {
            rt_kprintf("\r\n  选择取消 MainDns_AffirmCancel=%d", MainDns_AffirmCancel);


            if(MainDns_AffirmCancel == 4)
            {
                memset(Menu_MainDns, 0, sizeof(Menu_MainDns));

                //rt_kprintf("\r\n重新设置开始");

                MainDns_SetFlag = 1;
                MainDns_SetCounter_0 = 0;
                MainDns_SetCounter_1 = 0;
                MainDns_SetCounter_2 = 0;
                MainDns_SetComp = 0; //主域名设置完成1
                MainDns_AffirmCancel = 0; // 1:确认 2:取消

                MainDns_Type_flag = 0; //区分组的选择和组内选择
                MainDns_Type_Counter = 0; //  0: 数字    1:A-M         2:N-Z


                pMenuItem = &Menu_8_SetDNS;
                pMenuItem->show();
            }
        }
        break;
    case KEY_UP:
        if(MainDns_Type_flag == 1) //选择是0-9  A-M  N-Z
        {
            //
            if(MainDns_Type_Counter == 0)
                MainDns_Type_Counter = 2;
            else if(MainDns_Type_Counter >= 1)
                MainDns_Type_Counter--;
            MainDns_Type_Sel(MainDns_Type_Counter);
            //rt_kprintf("\r\n(  up)MainDns_Type_Counter=%d",MainDns_Type_Counter);

        }
        else if(MainDns_Type_flag == 2) //组内选择
        {
            if((MainDns_SetFlag >= 1) && (MainDns_SetFlag <= 20) && (MainDns_SetComp == 0))
            {
                if(MainDns_Type_Counter == 0)
                {
                    if(MainDns_SetCounter_0 == 0)
                        MainDns_SetCounter_0 = 12;
                    else
                        MainDns_SetCounter_0--;

                    MainDns_Set(MainDns_SetCounter_0, 1);
                }
                else if(MainDns_Type_Counter == 1)
                {
                    if(MainDns_SetCounter_1 == 0)
                        MainDns_SetCounter_1 = 12;
                    else
                        MainDns_SetCounter_1--;

                    MainDns_Set(MainDns_SetCounter_1, 2);
                }
                else if(MainDns_Type_Counter == 2)
                {
                    if(MainDns_SetCounter_2 == 0)
                        MainDns_SetCounter_2 = 12;
                    else
                        MainDns_SetCounter_2--;

                    MainDns_Set(MainDns_SetCounter_2, 3);
                }
            }
        }
        else if(MainDns_SetComp == 1)
        {
            MainDns_AffirmCancel = 1; //确认
            lcd_fill(0);
            lcd_text12(0, 3, (char *)Menu_MainDns, MainDns_SetFlag, LCD_MODE_SET);
            lcd_text12(24, 18, "确认    取消", 12, LCD_MODE_SET);
            lcd_text12(24, 18, "确认", 4, LCD_MODE_INVERT);
            lcd_update_all();
        }
        break;
    case KEY_DOWN:
        if(MainDns_Type_flag == 1) //选择是0-9  A-M  N-Z
        {
            MainDns_Type_Counter++;
            if(MainDns_Type_Counter > 2)
                MainDns_Type_Counter = 0;
            MainDns_Type_Sel(MainDns_Type_Counter);
            //rt_kprintf("\r\n(down)MainDns_Type_Counter=%d",MainDns_Type_Counter);
        }
        else if(MainDns_Type_flag == 2) //组内选择
        {
            if((MainDns_SetFlag >= 1) && (MainDns_SetFlag <= 20) && (MainDns_SetComp == 0))
            {
                if(MainDns_Type_Counter == 0)
                {
                    MainDns_SetCounter_0++;
                    if(MainDns_SetCounter_0 > 12)
                        MainDns_SetCounter_0 = 0;

                    MainDns_Set(MainDns_SetCounter_0, 1);
                }
                else if(MainDns_Type_Counter == 1)
                {
                    MainDns_SetCounter_1++;
                    if(MainDns_SetCounter_1 > 12)
                        MainDns_SetCounter_1 = 0;

                    MainDns_Set(MainDns_SetCounter_1, 2);
                }
                else if(MainDns_Type_Counter == 2)
                {
                    MainDns_SetCounter_2++;
                    if(MainDns_SetCounter_2 > 12)
                        MainDns_SetCounter_2 = 0;

                    MainDns_Set(MainDns_SetCounter_2, 3);
                }
            }
        }

        else if(MainDns_SetComp == 1)
        {
            MainDns_AffirmCancel = 2; //取消
            lcd_fill(0);
            lcd_text12(0, 3, (char *)Menu_MainDns, MainDns_SetFlag, LCD_MODE_SET);
            lcd_text12(24, 18, "确认    取消", 12, LCD_MODE_SET);
            lcd_text12(72, 18, "取消", 4, LCD_MODE_INVERT);
            lcd_update_all();
        }
        break;
    }
}

static void timetick(unsigned int systick)
{
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_8_1_MainDnsPort =
{
    "主域名端口设置",
    14, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

