#include "Menu_Include.h"
//#include "App_moduleConfig.h"
#include "LCD_Driver.h"


#define  AuxDns_width1  6


static u8 AuxDns_SetFlag = 1;
static u8 AuxDns_SetCounter_0 = 0, AuxDns_SetCounter_1 = 0, AuxDns_SetCounter_2 = 0;
static u8 AuxDns_SetComp = 0; //辅域名设置完成1
static u8 AuxDns_AffirmCancel = 0; // 1:确认 2:取消

static u8 AuxDns_Type_flag = 0; //区分组的选择和组内选择
static u8 AuxDns_Type_Counter = 0; //  0: 数字    1:A-M         2:N-Z

unsigned char select_AuxDns[] = {0x0C, 0x06, 0xFF, 0x06, 0x0C};
unsigned char ABC_AuxDns_0_9[13][1] = {".", ":", "#", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
unsigned char ABC_AuxDns_A_M[13][1] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m"};
unsigned char ABC_AuxDns_N_Z[13][1] = {"n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};


DECL_BMP(8, 5, select_AuxDns);

void AuxDns_Type_Sel( u8 par)
{
    lcd_fill(0);
    lcd_text12(0, 3, (char *)Menu_AuxDns, AuxDns_SetFlag - 1, LCD_MODE_SET);
    lcd_text12(20, 18, ".-9  a-m  n-z", 13, LCD_MODE_SET);
    if(par == 0)
        lcd_text12(20, 18, ".-9", 3, LCD_MODE_INVERT);
    else if(par == 1)
        lcd_text12(20 + 5 * 6, 18, "a-m", 3, LCD_MODE_INVERT);
    else if(par == 2)
        lcd_text12(20 + 10 * 6, 18, "n-z", 3, LCD_MODE_INVERT);
    lcd_update_all();
}

void AuxDns_Set(u8 par, u8 type1_2)
{
    lcd_fill(0);
    lcd_text12(0, 3, (char *)Menu_AuxDns, AuxDns_SetFlag - 1, LCD_MODE_SET);

    if(type1_2 == 1)
    {
        lcd_bitmap(par * AuxDns_width1, 14, &BMP_select_AuxDns, LCD_MODE_SET);
        lcd_text12(0, 19, ".:#0123456789", 13, LCD_MODE_SET);
    }
    else if(type1_2 == 2)
    {
        lcd_bitmap(par * AuxDns_width1, 14, &BMP_select_AuxDns, LCD_MODE_SET);
        lcd_text12(0, 19, "abcdefghijklm", 13, LCD_MODE_SET);
    }
    else if(type1_2 == 3)
    {
        lcd_bitmap(par * AuxDns_width1, 14, &BMP_select_AuxDns, LCD_MODE_SET);
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
    AuxDns_Type_Counter = 0;
    AuxDns_Type_Sel(AuxDns_Type_Counter);
    AuxDns_Type_flag = 1;
    //rt_kprintf("\r\n选择要输入的类型");

}


static void keypress(unsigned int key)
{

    u8  Dnstr[20], i = 0;

    switch(key)
    {
    case KEY_MENU:

        AuxDns_SetFlag = 1;
        AuxDns_SetCounter_0 = 0;
        AuxDns_SetCounter_1 = 0;
        AuxDns_SetCounter_2 = 0;
        AuxDns_SetComp = 0; //主域名设置完成1
        AuxDns_AffirmCancel = 0; // 1:确认 2:取消

        AuxDns_Type_flag = 0; //区分组的选择和组内选择
        AuxDns_Type_Counter = 0; //  0: 数字    1:A-M         2:N-Z


        memset(Menu_AuxDns, 0, sizeof(Menu_AuxDns));
        pMenuItem = &Menu_8_SetDNS;
        pMenuItem->show();
        break;
    case KEY_OK:
        if((AuxDns_Type_flag == 1) && (AuxDns_SetComp == 0))
        {
            AuxDns_Type_flag = 2;
            if((AuxDns_SetFlag >= 1) && (AuxDns_SetFlag <= 20))
            {
                if(AuxDns_Type_Counter == 0)
                    AuxDns_Set(AuxDns_SetCounter_0, 1);
                else if(AuxDns_Type_Counter == 1)
                    AuxDns_Set(AuxDns_SetCounter_1, 2);
                else if(AuxDns_Type_Counter == 2)
                    AuxDns_Set(AuxDns_SetCounter_2, 3);
                //rt_kprintf("\r\n第%d组",AuxDns_Type_Counter);
            }
        }
        else if((AuxDns_Type_flag == 2) && (AuxDns_SetComp == 0))
        {
            AuxDns_Type_flag = 3;
            if((AuxDns_SetFlag >= 1) && (AuxDns_SetFlag <= 20))
            {
                if(ABC_AuxDns_0_9[AuxDns_SetCounter_0][0] == 0x23)
                {
                    rt_kprintf("\r\n*********************辅域名输入完成******************");
#if 0
                    //备用dns及端口输入完成
                    //  -----------  格式判断 -----------   是单域名还是域名+ 端口---
                    // 1.step  find :
                    for(i = 0; i < strlen((const char *)Menu_AuxDns); i++)
                    {
                        if(Menu_AuxDns[i] == ':')  break;
                    }
                    // 2. judge   resualt
                    if(i == strlen((const char *)Menu_AuxDns))			// 只有域名
                        dnsr_aux(Menu_AuxDns);    // 使能设置
                    else   // 域名+port
                    {
                        memset(Dnstr, 0, sizeof(Dnstr));
                        memcpy(Dnstr, Menu_AuxDns, i);
                        dnsr_main(Dnstr);
                        //rt_kprintf("\r\n 域名part %s",Dnstr);
                        port_main(Menu_AuxDns + i + 1);
                    }
#endif
                    //-------------------------------------------------------------------------------
                    Menu_AuxDns[AuxDns_SetFlag - 1] = ABC_AuxDns_0_9[AuxDns_SetCounter_0][0];
                    AuxDns_SetComp = 1;
                    AuxDns_AffirmCancel = 1;
                    lcd_fill(0);
                    lcd_text12(0, 3, (char *)Menu_AuxDns, AuxDns_SetFlag, LCD_MODE_SET);
                    lcd_text12(24, 18, "确认    取消", 12, LCD_MODE_SET);
                    lcd_text12(24, 18, "确认", 4, LCD_MODE_INVERT);
                    lcd_update_all();
                }
                else
                {
                    if(AuxDns_Type_Counter == 0)
                    {
                        Menu_AuxDns[AuxDns_SetFlag - 1] = ABC_AuxDns_0_9[AuxDns_SetCounter_0][0];
                        AuxDns_SetFlag++;
                        AuxDns_Set(AuxDns_SetCounter_0, 1);
                        //rt_kprintf("\r\n(0_9选择)=%d",AuxDns_SetCounter_0);
                    }
                    else if(AuxDns_Type_Counter == 1)
                    {
                        Menu_AuxDns[AuxDns_SetFlag - 1] = ABC_AuxDns_A_M[AuxDns_SetCounter_1][0];
                        AuxDns_SetFlag++;
                        AuxDns_Set(AuxDns_SetCounter_1, 2);
                        //rt_kprintf("\r\n(A_M选择)=%d",AuxDns_SetCounter_1);
                    }
                    else if(AuxDns_Type_Counter == 2)
                    {
                        Menu_AuxDns[AuxDns_SetFlag - 1] = ABC_AuxDns_N_Z[AuxDns_SetCounter_2][0];
                        AuxDns_SetFlag++;
                        AuxDns_Set(AuxDns_SetCounter_2, 3);
                        //rt_kprintf("\r\n(N_Z选择)=%d",AuxDns_SetCounter_2);
                    }
                    if(AuxDns_Type_flag == 3)
                    {
                        AuxDns_Type_flag = 1;
                        AuxDns_SetCounter_0 = 0;
                        AuxDns_SetCounter_1 = 0;
                        AuxDns_SetCounter_2 = 0;

                        AuxDns_Type_Sel(AuxDns_Type_Counter);
                        //rt_kprintf("\r\n重新选组(1_2_3)=%d",AuxDns_Type_Counter);
                    }
                }
            }
        }
        else if(AuxDns_SetComp == 1)
        {
            if(AuxDns_AffirmCancel == 1) //确认
            {
                /*MainDns_AffirmCancel=3;
                MainDns_SetComp=3;

                MainDns_SetFlag=0;
                MainDns_SetComp=0;*/

                AuxDns_SetFlag = 1;
                AuxDns_SetCounter_0 = 0;
                AuxDns_SetCounter_1 = 0;
                AuxDns_SetCounter_2 = 0;
                AuxDns_SetComp = 0; //主域名设置完成1
                AuxDns_AffirmCancel = 0; // 1:确认 2:取消

                AuxDns_Type_flag = 0; //区分组的选择和组内选择
                AuxDns_Type_Counter = 0; //  0: 数字    1:A-M         2:N-Z


                lcd_fill(0);
                lcd_text12(18, 3, "辅域名设置完成", 14, LCD_MODE_SET);
                lcd_text12(24, 18, "按菜单键退出", 12, LCD_MODE_SET);
                lcd_update_all();
                //rt_kprintf("\r\n辅域名%s",Menu_AuxDns);


                //添加相应的操作，辅域名存在Menu_AuxDns[]中
                //格式【jt2.gghypt.net:7008#】
                //:    域名和端口的分隔符，#  设置完成结束符



            }
            else if(AuxDns_AffirmCancel == 2) //取消
            {
                AuxDns_SetComp = 2;
                AuxDns_AffirmCancel = 4; //重新设置主域名
                lcd_fill(0);
                lcd_text12(12, 10, "按确认键重新设置", 16, LCD_MODE_SET);
                lcd_update_all();

            }
        }

        else if(AuxDns_SetComp == 2)
        {
            //rt_kprintf("\r\n  选择取消 AuxDns_AffirmCancel=%d",AuxDns_AffirmCancel);


            if(AuxDns_AffirmCancel == 4)
            {
                memset(Menu_AuxDns, 0, sizeof(Menu_AuxDns));

                //rt_kprintf("\r\n重新设置开始");

                AuxDns_SetFlag = 1;
                AuxDns_SetCounter_0 = 0;
                AuxDns_SetCounter_1 = 0;
                AuxDns_SetCounter_2 = 0;
                AuxDns_SetComp = 0; //主域名设置完成1
                AuxDns_AffirmCancel = 0; // 1:确认 2:取消

                AuxDns_Type_flag = 0; //区分组的选择和组内选择
                AuxDns_Type_Counter = 0; //  0: 数字    1:A-M         2:N-Z


                pMenuItem = &Menu_8_SetDNS;
                pMenuItem->show();
            }
        }
        break;
    case KEY_UP:
        if(AuxDns_Type_flag == 1) //选择是0-9  A-M  N-Z
        {
            //
            if(AuxDns_Type_Counter == 0)
                AuxDns_Type_Counter = 2;
            else if(AuxDns_Type_Counter >= 1)
                AuxDns_Type_Counter--;
            AuxDns_Type_Sel(AuxDns_Type_Counter);
            //rt_kprintf("\r\n(  up)AuxDns_Type_Counter=%d",AuxDns_Type_Counter);

        }
        else if(AuxDns_Type_flag == 2) //组内选择
        {
            if((AuxDns_SetFlag >= 1) && (AuxDns_SetFlag <= 20) && (AuxDns_SetComp == 0))
            {
                if(AuxDns_Type_Counter == 0)
                {
                    if(AuxDns_SetCounter_0 == 0)
                        AuxDns_SetCounter_0 = 12;
                    else
                        AuxDns_SetCounter_0--;

                    AuxDns_Set(AuxDns_SetCounter_0, 1);
                }
                else if(AuxDns_Type_Counter == 1)
                {
                    if(AuxDns_SetCounter_1 == 0)
                        AuxDns_SetCounter_1 = 12;
                    else
                        AuxDns_SetCounter_1--;

                    AuxDns_Set(AuxDns_SetCounter_1, 2);
                }
                else if(AuxDns_Type_Counter == 2)
                {
                    if(AuxDns_SetCounter_2 == 0)
                        AuxDns_SetCounter_2 = 12;
                    else
                        AuxDns_SetCounter_2--;

                    AuxDns_Set(AuxDns_SetCounter_2, 3);
                }
            }
        }
        else if(AuxDns_SetComp == 1)
        {
            AuxDns_AffirmCancel = 1; //确认
            lcd_fill(0);
            lcd_text12(0, 3, (char *)Menu_AuxDns, AuxDns_SetFlag, LCD_MODE_SET);
            lcd_text12(24, 18, "确认    取消", 12, LCD_MODE_SET);
            lcd_text12(24, 18, "确认", 4, LCD_MODE_INVERT);
            lcd_update_all();
        }
        break;
    case KEY_DOWN:
        if(AuxDns_Type_flag == 1) //选择是0-9  A-M  N-Z
        {
            AuxDns_Type_Counter++;
            if(AuxDns_Type_Counter > 2)
                AuxDns_Type_Counter = 0;
            AuxDns_Type_Sel(AuxDns_Type_Counter);
            //	rt_kprintf("\r\n(down)AuxDns_Type_Counter=%d",AuxDns_Type_Counter);
        }
        else if(AuxDns_Type_flag == 2) //组内选择
        {
            if((AuxDns_SetFlag >= 1) && (AuxDns_SetFlag <= 20) && (AuxDns_SetComp == 0))
            {
                if(AuxDns_Type_Counter == 0)
                {
                    AuxDns_SetCounter_0++;
                    if(AuxDns_SetCounter_0 > 12)
                        AuxDns_SetCounter_0 = 0;

                    AuxDns_Set(AuxDns_SetCounter_0, 1);
                }
                else if(AuxDns_Type_Counter == 1)
                {
                    AuxDns_SetCounter_1++;
                    if(AuxDns_SetCounter_1 > 12)
                        AuxDns_SetCounter_1 = 0;

                    AuxDns_Set(AuxDns_SetCounter_1, 2);
                }
                else if(AuxDns_Type_Counter == 2)
                {
                    AuxDns_SetCounter_2++;
                    if(AuxDns_SetCounter_2 > 12)
                        AuxDns_SetCounter_2 = 0;

                    AuxDns_Set(AuxDns_SetCounter_2, 3);
                }
            }
        }

        else if(AuxDns_SetComp == 1)
        {
            AuxDns_AffirmCancel = 2; //取消
            lcd_fill(0);
            lcd_text12(0, 3, (char *)Menu_AuxDns, AuxDns_SetFlag, LCD_MODE_SET);
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
MENUITEM	Menu_8_2_AuxDnsPort =
{
    "辅域名端口设置",
    14, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

