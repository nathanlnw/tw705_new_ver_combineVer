#include "Menu_Include.h"
//#include "App_moduleConfig.h"
#include "sed1520.h"

#define  AuxIP_width1  6


static u8 AuxIp_SetFlag = 1;
static u8 AuxIp_SetCounter = 0;


unsigned char select_AuxIp[] = {0x0C, 0x06, 0xFF, 0x06, 0x0C};
unsigned char ABC_AuxIp[10][1] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};


DECL_BMP(8, 5, select_AuxIp);


void AuxIp_Set(u8 par)
{
    lcd_fill(0);
    lcd_text12(0, 3, (char *)Menu_AuxIp, 20, LCD_MODE_SET);
    lcd_bitmap(par * AuxIP_width1, 14, &BMP_select_AuxIp, LCD_MODE_SET);
    lcd_text12(0, 19, "0123456789", 10, LCD_MODE_SET);
    lcd_update_all();
}

static void msg( void *p)
{

}
static void show(void)
{
    CounterBack = 0;
    AuxIp_Set(AuxIp_SetCounter);
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        //memset(Menu_AuxIp,0,sizeof(Menu_AuxIp));
        AuxIp_SetFlag = 1;
        AuxIp_SetCounter = 0;
        memcpy(Menu_AuxIp, "   .   .   .   :    ", 20);

        pMenuItem = &Menu_8_SetDNS;
        pMenuItem->show();
        break;
    case KEY_OK:
        if((AuxIp_SetFlag >= 1) && (AuxIp_SetFlag <= 16))
        {
            if((AuxIp_SetFlag >= 1) && (AuxIp_SetFlag <= 3))
                Menu_AuxIp[AuxIp_SetFlag - 1] = ABC_AuxIp[AuxIp_SetCounter][0];
            else if((AuxIp_SetFlag >= 4) && (AuxIp_SetFlag <= 6))
                Menu_AuxIp[AuxIp_SetFlag] = ABC_AuxIp[AuxIp_SetCounter][0];
            else if((AuxIp_SetFlag >= 7) && (AuxIp_SetFlag <= 9))
                Menu_AuxIp[AuxIp_SetFlag + 1] = ABC_AuxIp[AuxIp_SetCounter][0];
            else if((AuxIp_SetFlag >= 10) && (AuxIp_SetFlag <= 12))
                Menu_AuxIp[AuxIp_SetFlag + 2] = ABC_AuxIp[AuxIp_SetCounter][0];
            else
                Menu_AuxIp[AuxIp_SetFlag + 3] = ABC_AuxIp[AuxIp_SetCounter][0];

            rt_kprintf("\r\n AuxIp_SetFlag=%d,%s", AuxIp_SetFlag, Menu_AuxIp);
            AuxIp_SetFlag++;
            AuxIp_SetCounter = 0;
            AuxIp_Set(0);
        }
        else if(AuxIp_SetFlag == 17)
        {
            //rt_kprintf("\r\n AuxIp_SetFlag=%d,%s",AuxIp_SetFlag,Menu_AuxIp);
            AuxIp_SetFlag = 18;
            lcd_fill(0);
            lcd_text12(0, 5, (char *)Menu_AuxIp, 20, LCD_MODE_SET);
            lcd_text12(10, 19, "辅IP PORT设置完成", 17, LCD_MODE_SET);
            lcd_update_all();
            //rt_kprintf("\r\n AuxIp_SetFlag=%d,%s(ok--)",AuxIp_SetFlag,Menu_AuxIp);


            //添加相应的操作，辅IP 存在Menu_AuxIp[]中
            //格式【xxx.xxx.xxx.xxx:xxxx】

            //---------- IP   地址输入完成-------------
            //Socket_aux_Set(Menu_AuxIp);

        }
        else if(AuxIp_SetFlag == 18)
        {
            //rt_kprintf("\r\n AuxIp_SetFlag=%d,%s(return)",AuxIp_SetFlag,Menu_AuxIp);
            AuxIp_SetFlag = 1;

            pMenuItem = &Menu_8_SetDNS;
            pMenuItem->show();
        }

        break;
    case KEY_UP:
        if((AuxIp_SetFlag >= 1) && (AuxIp_SetFlag <= 16))
        {
            if(AuxIp_SetCounter == 0)
                AuxIp_SetCounter = 9;
            else if(AuxIp_SetCounter >= 1)
                AuxIp_SetCounter--;

            AuxIp_Set(AuxIp_SetCounter);
        }
        break;
    case KEY_DOWN:
        if((AuxIp_SetFlag >= 1) && (AuxIp_SetFlag <= 16))
        {
            AuxIp_SetCounter++;
            if(AuxIp_SetCounter > 9)
                AuxIp_SetCounter = 0;

            AuxIp_Set(AuxIp_SetCounter);
        }
        break;
    }
}


static void timetick(unsigned int systick)
{

}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_8_4_AuxIpPort =
{
    "辅IP PORT 设置",
    14, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

