#include "Menu_Include.h"
//#include "App_moduleConfig.h"
#include "LCD_Driver.h"
#define  MainIP_width1  6


static u8 MainIp_SetFlag = 1;
static u8 MainIp_SetCounter = 0;


unsigned char select_MainIp[] = {0x0C, 0x06, 0xFF, 0x06, 0x0C};
unsigned char ABC_MainIp[10][1] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};


DECL_BMP(8, 5, select_MainIp);


void MainIp_Set(u8 par)
{
    lcd_fill(0);
    lcd_text12(0, 3, (char *)Menu_MainIp, 20, LCD_MODE_SET);
    lcd_bitmap(par * MainIP_width1, 14, &BMP_select_MainIp, LCD_MODE_SET);
    lcd_text12(0, 19, "0123456789", 10, LCD_MODE_SET);
    lcd_update_all();
}

static void msg( void *p)
{

}
static void show(void)
{
    CounterBack = 0;
    MainIp_Set(MainIp_SetCounter);
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        //memset(Menu_MainIp,0,sizeof(Menu_MainIp));
        MainIp_SetFlag = 1;
        MainIp_SetCounter = 0;
        memcpy(Menu_MainIp, "   .   .   .   :    ", 20);

        pMenuItem = &Menu_8_SetDNS;
        pMenuItem->show();
        break;
    case KEY_OK:
        if((MainIp_SetFlag >= 1) && (MainIp_SetFlag <= 16))
        {
            if((MainIp_SetFlag >= 1) && (MainIp_SetFlag <= 3))
                Menu_MainIp[MainIp_SetFlag - 1] = ABC_MainIp[MainIp_SetCounter][0];
            else if((MainIp_SetFlag >= 4) && (MainIp_SetFlag <= 6))
                Menu_MainIp[MainIp_SetFlag] = ABC_MainIp[MainIp_SetCounter][0];
            else if((MainIp_SetFlag >= 7) && (MainIp_SetFlag <= 9))
                Menu_MainIp[MainIp_SetFlag + 1] = ABC_MainIp[MainIp_SetCounter][0];
            else if((MainIp_SetFlag >= 10) && (MainIp_SetFlag <= 12))
                Menu_MainIp[MainIp_SetFlag + 2] = ABC_MainIp[MainIp_SetCounter][0];
            else
                Menu_MainIp[MainIp_SetFlag + 3] = ABC_MainIp[MainIp_SetCounter][0];
            //	rt_kprintf("\r\n MainIp_SetFlag=%d,%s",MainIp_SetFlag,Menu_MainIp);
            MainIp_SetFlag++;
            MainIp_SetCounter = 0;
            MainIp_Set(0);
        }
        else if(MainIp_SetFlag == 17)
        {
            //rt_kprintf("\r\n MainIp_SetFlag=%d,%s",MainIp_SetFlag,Menu_MainIp);
            MainIp_SetFlag = 18;
            lcd_fill(0);
            lcd_text12(0, 5, (char *)Menu_MainIp, 20, LCD_MODE_SET);
            lcd_text12(10, 19, "主IP PORT设置完成", 17, LCD_MODE_SET);
            lcd_update_all();
            //rt_kprintf("\r\n MainIp_SetFlag=%d,%s(ok--)",MainIp_SetFlag,Menu_MainIp);


            //添加相应的操作，主IP 存在Menu_MainIp[]中
            //格式【xxx.xxx.xxx.xxx:xxxx】
            //---------- IP   地址输入完成-------------
            //Socket_main_Set(Menu_MainIp);
            memset(jt808_param.id_0xF049, 0, sizeof(jt808_param.id_0xF049));
            memset(jt808_param.id_0xF04A, 0, sizeof(jt808_param.id_0xF04A));

            memcpy(jt808_param.id_0xF049, Menu_MainIp, 15);
            jt808_param.id_0xF04A = (Menu_MainIp[16] - '0') * 1000 + (Menu_MainIp[17] - '0') * 100 + (Menu_MainIp[18] - '0') * 10 + Menu_MainIp[19] - '0';

            param_save(1);
            rt_kprintf("\r\n         ip:%s", Menu_MainIp);
            rt_kprintf("\r\n       端口:%d", jt808_param.id_0xF04A);

        }
        else if(MainIp_SetFlag == 18)
        {
            //	rt_kprintf("\r\n MainIp_SetFlag=%d,%s(return)",MainIp_SetFlag,Menu_MainIp);
            MainIp_SetFlag = 1;

            pMenuItem = &Menu_8_SetDNS;
            pMenuItem->show();
        }

        break;
    case KEY_UP:
        if((MainIp_SetFlag >= 1) && (MainIp_SetFlag <= 16))
        {
            if(MainIp_SetCounter == 0)
                MainIp_SetCounter = 9;
            else if(MainIp_SetCounter >= 1)
                MainIp_SetCounter--;

            MainIp_Set(MainIp_SetCounter);
        }
        break;
    case KEY_DOWN:
        if((MainIp_SetFlag >= 1) && (MainIp_SetFlag <= 16))
        {
            MainIp_SetCounter++;
            if(MainIp_SetCounter > 9)
                MainIp_SetCounter = 0;

            MainIp_Set(MainIp_SetCounter);
        }
        break;
    }
}


static void timetick(unsigned int systick)
{

}
ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_8_3_MainIpPort =
{
    "主IP PORT 设置",
    14, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

