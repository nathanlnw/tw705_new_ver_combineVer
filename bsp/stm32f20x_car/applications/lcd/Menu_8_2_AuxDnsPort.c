#include "Menu_Include.h"
//#include "App_moduleConfig.h"
#include "LCD_Driver.h"


#define  AuxDns_width1  6


static u8 AuxDns_SetFlag = 1;
static u8 AuxDns_SetCounter_0 = 0, AuxDns_SetCounter_1 = 0, AuxDns_SetCounter_2 = 0;
static u8 AuxDns_SetComp = 0; //�������������1
static u8 AuxDns_AffirmCancel = 0; // 1:ȷ�� 2:ȡ��

static u8 AuxDns_Type_flag = 0; //�������ѡ�������ѡ��
static u8 AuxDns_Type_Counter = 0; //  0: ����    1:A-M         2:N-Z

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
    //rt_kprintf("\r\nѡ��Ҫ���������");

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
        AuxDns_SetComp = 0; //�������������1
        AuxDns_AffirmCancel = 0; // 1:ȷ�� 2:ȡ��

        AuxDns_Type_flag = 0; //�������ѡ�������ѡ��
        AuxDns_Type_Counter = 0; //  0: ����    1:A-M         2:N-Z


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
                //rt_kprintf("\r\n��%d��",AuxDns_Type_Counter);
            }
        }
        else if((AuxDns_Type_flag == 2) && (AuxDns_SetComp == 0))
        {
            AuxDns_Type_flag = 3;
            if((AuxDns_SetFlag >= 1) && (AuxDns_SetFlag <= 20))
            {
                if(ABC_AuxDns_0_9[AuxDns_SetCounter_0][0] == 0x23)
                {
                    rt_kprintf("\r\n*********************�������������******************");
#if 0
                    //����dns���˿��������
                    //  -----------  ��ʽ�ж� -----------   �ǵ�������������+ �˿�---
                    // 1.step  find :
                    for(i = 0; i < strlen((const char *)Menu_AuxDns); i++)
                    {
                        if(Menu_AuxDns[i] == ':')  break;
                    }
                    // 2. judge   resualt
                    if(i == strlen((const char *)Menu_AuxDns))			// ֻ������
                        dnsr_aux(Menu_AuxDns);    // ʹ������
                    else   // ����+port
                    {
                        memset(Dnstr, 0, sizeof(Dnstr));
                        memcpy(Dnstr, Menu_AuxDns, i);
                        dnsr_main(Dnstr);
                        //rt_kprintf("\r\n ����part %s",Dnstr);
                        port_main(Menu_AuxDns + i + 1);
                    }
#endif
                    //-------------------------------------------------------------------------------
                    Menu_AuxDns[AuxDns_SetFlag - 1] = ABC_AuxDns_0_9[AuxDns_SetCounter_0][0];
                    AuxDns_SetComp = 1;
                    AuxDns_AffirmCancel = 1;
                    lcd_fill(0);
                    lcd_text12(0, 3, (char *)Menu_AuxDns, AuxDns_SetFlag, LCD_MODE_SET);
                    lcd_text12(24, 18, "ȷ��    ȡ��", 12, LCD_MODE_SET);
                    lcd_text12(24, 18, "ȷ��", 4, LCD_MODE_INVERT);
                    lcd_update_all();
                }
                else
                {
                    if(AuxDns_Type_Counter == 0)
                    {
                        Menu_AuxDns[AuxDns_SetFlag - 1] = ABC_AuxDns_0_9[AuxDns_SetCounter_0][0];
                        AuxDns_SetFlag++;
                        AuxDns_Set(AuxDns_SetCounter_0, 1);
                        //rt_kprintf("\r\n(0_9ѡ��)=%d",AuxDns_SetCounter_0);
                    }
                    else if(AuxDns_Type_Counter == 1)
                    {
                        Menu_AuxDns[AuxDns_SetFlag - 1] = ABC_AuxDns_A_M[AuxDns_SetCounter_1][0];
                        AuxDns_SetFlag++;
                        AuxDns_Set(AuxDns_SetCounter_1, 2);
                        //rt_kprintf("\r\n(A_Mѡ��)=%d",AuxDns_SetCounter_1);
                    }
                    else if(AuxDns_Type_Counter == 2)
                    {
                        Menu_AuxDns[AuxDns_SetFlag - 1] = ABC_AuxDns_N_Z[AuxDns_SetCounter_2][0];
                        AuxDns_SetFlag++;
                        AuxDns_Set(AuxDns_SetCounter_2, 3);
                        //rt_kprintf("\r\n(N_Zѡ��)=%d",AuxDns_SetCounter_2);
                    }
                    if(AuxDns_Type_flag == 3)
                    {
                        AuxDns_Type_flag = 1;
                        AuxDns_SetCounter_0 = 0;
                        AuxDns_SetCounter_1 = 0;
                        AuxDns_SetCounter_2 = 0;

                        AuxDns_Type_Sel(AuxDns_Type_Counter);
                        //rt_kprintf("\r\n����ѡ��(1_2_3)=%d",AuxDns_Type_Counter);
                    }
                }
            }
        }
        else if(AuxDns_SetComp == 1)
        {
            if(AuxDns_AffirmCancel == 1) //ȷ��
            {
                /*MainDns_AffirmCancel=3;
                MainDns_SetComp=3;

                MainDns_SetFlag=0;
                MainDns_SetComp=0;*/

                AuxDns_SetFlag = 1;
                AuxDns_SetCounter_0 = 0;
                AuxDns_SetCounter_1 = 0;
                AuxDns_SetCounter_2 = 0;
                AuxDns_SetComp = 0; //�������������1
                AuxDns_AffirmCancel = 0; // 1:ȷ�� 2:ȡ��

                AuxDns_Type_flag = 0; //�������ѡ�������ѡ��
                AuxDns_Type_Counter = 0; //  0: ����    1:A-M         2:N-Z


                lcd_fill(0);
                lcd_text12(18, 3, "�������������", 14, LCD_MODE_SET);
                lcd_text12(24, 18, "���˵����˳�", 12, LCD_MODE_SET);
                lcd_update_all();
                //rt_kprintf("\r\n������%s",Menu_AuxDns);


                //�����Ӧ�Ĳ���������������Menu_AuxDns[]��
                //��ʽ��jt2.gghypt.net:7008#��
                //:    �����Ͷ˿ڵķָ�����#  ������ɽ�����



            }
            else if(AuxDns_AffirmCancel == 2) //ȡ��
            {
                AuxDns_SetComp = 2;
                AuxDns_AffirmCancel = 4; //��������������
                lcd_fill(0);
                lcd_text12(12, 10, "��ȷ�ϼ���������", 16, LCD_MODE_SET);
                lcd_update_all();

            }
        }

        else if(AuxDns_SetComp == 2)
        {
            //rt_kprintf("\r\n  ѡ��ȡ�� AuxDns_AffirmCancel=%d",AuxDns_AffirmCancel);


            if(AuxDns_AffirmCancel == 4)
            {
                memset(Menu_AuxDns, 0, sizeof(Menu_AuxDns));

                //rt_kprintf("\r\n�������ÿ�ʼ");

                AuxDns_SetFlag = 1;
                AuxDns_SetCounter_0 = 0;
                AuxDns_SetCounter_1 = 0;
                AuxDns_SetCounter_2 = 0;
                AuxDns_SetComp = 0; //�������������1
                AuxDns_AffirmCancel = 0; // 1:ȷ�� 2:ȡ��

                AuxDns_Type_flag = 0; //�������ѡ�������ѡ��
                AuxDns_Type_Counter = 0; //  0: ����    1:A-M         2:N-Z


                pMenuItem = &Menu_8_SetDNS;
                pMenuItem->show();
            }
        }
        break;
    case KEY_UP:
        if(AuxDns_Type_flag == 1) //ѡ����0-9  A-M  N-Z
        {
            //
            if(AuxDns_Type_Counter == 0)
                AuxDns_Type_Counter = 2;
            else if(AuxDns_Type_Counter >= 1)
                AuxDns_Type_Counter--;
            AuxDns_Type_Sel(AuxDns_Type_Counter);
            //rt_kprintf("\r\n(  up)AuxDns_Type_Counter=%d",AuxDns_Type_Counter);

        }
        else if(AuxDns_Type_flag == 2) //����ѡ��
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
            AuxDns_AffirmCancel = 1; //ȷ��
            lcd_fill(0);
            lcd_text12(0, 3, (char *)Menu_AuxDns, AuxDns_SetFlag, LCD_MODE_SET);
            lcd_text12(24, 18, "ȷ��    ȡ��", 12, LCD_MODE_SET);
            lcd_text12(24, 18, "ȷ��", 4, LCD_MODE_INVERT);
            lcd_update_all();
        }
        break;
    case KEY_DOWN:
        if(AuxDns_Type_flag == 1) //ѡ����0-9  A-M  N-Z
        {
            AuxDns_Type_Counter++;
            if(AuxDns_Type_Counter > 2)
                AuxDns_Type_Counter = 0;
            AuxDns_Type_Sel(AuxDns_Type_Counter);
            //	rt_kprintf("\r\n(down)AuxDns_Type_Counter=%d",AuxDns_Type_Counter);
        }
        else if(AuxDns_Type_flag == 2) //����ѡ��
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
            AuxDns_AffirmCancel = 2; //ȡ��
            lcd_fill(0);
            lcd_text12(0, 3, (char *)Menu_AuxDns, AuxDns_SetFlag, LCD_MODE_SET);
            lcd_text12(24, 18, "ȷ��    ȡ��", 12, LCD_MODE_SET);
            lcd_text12(72, 18, "ȡ��", 4, LCD_MODE_INVERT);
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
    "�������˿�����",
    14, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

