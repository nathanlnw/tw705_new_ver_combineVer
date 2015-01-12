#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

#define  pass_width1  6
#define  ENTER_CODE  "001100"      // NX  密码
//#define  ENTER_CODE  "000000"      // NX  密码


u8 set_car_codetype = 0;
//char PassWord[10];
u8 password_SetFlag = 1, password_Counter = 0;
u8 password_icon[] = {0x0C, 0x06, 0xFF, 0x06, 0x0C};

DECL_BMP(8, 5, password_icon);


void password_Set(u8 par)
{
    char tempbuf[64];
    memcpy(tempbuf, PassWord, 10);
    lcd_fill(0);
    lcd_text12(0, 3, "请输入6位密码:", 14, LCD_MODE_SET);
    if(password_SetFlag > 1)
        lcd_text12(84, 3, tempbuf, password_SetFlag - 1, LCD_MODE_SET); //-1+14
    lcd_bitmap(par * pass_width1, 14, &BMP_password_icon, LCD_MODE_SET);
    lcd_text12(0, 19, "0123456789", 10, LCD_MODE_SET);
    lcd_text12(63, 20, "TJ.Combine", 10, LCD_MODE_SET); // 天津公共货运平台
    lcd_update_all();
}

static void msg( void *p)
{

}
static void show(void)
{
    if(device_unlock)
    {
        MENU_set_carinfor_flag = 1;
    }
    else
    {
        MENU_set_carinfor_flag = 0;
    }
    CounterBack = 0;
    set_car_codetype = 0;
    password_SetFlag = 1;
    password_Counter = 0;
    Password_correctFlag = 0;
    memset(PassWord, 0, sizeof(PassWord));
    password_Set(password_Counter);
    //rt_kprintf("\r\nshow:password=%s,password_SetFlag=%d,password_Counter=%d\r\n",PassWord,password_SetFlag,password_Counter);
    rt_kprintf("\r\n---1");
}


static void keypress(unsigned int key)
{
    char buf[32];
    CounterBack = 0;
    switch(key)
    {
    case KEY_MENU:

        if((Password_correctFlag == 1) && (1 == set_car_codetype))
        {
            if(set_car_codetype == 1)
            {
                set_car_codetype = 0;
                CarSet_0_counter = 0; //设置第1项
                pMenuItem = &Menu_0_loggingin;
            }
            else if(NET_SET_FLAG == 1)
            {
                pMenuItem = &Menu_8_SetDNS;
            }
            pMenuItem->show();
            memset(PassWord, 0, sizeof(PassWord));
            password_SetFlag = 1;
            password_Counter = 0;
            rt_kprintf("\r\n---2");
        }
        else if(device_unlock)
        {
            pMenuItem = &Menu_1_Idle;
            pMenuItem->show();
        }
        break;
    case KEY_OK:
        if((password_SetFlag >= 1) && (password_SetFlag <= 6))
        {
            PassWord[password_SetFlag - 1] = password_Counter + '0';
            //rt_kprintf("\r\n已设置密码=%s",password_Code);
            //rt_kprintf("\r\npassword_code_1=%d,%d,%d,%d,%d,%d,",PassWord[0],PassWord[1],PassWord[2],PassWord[3],PassWord[4],PassWord[5]);
            password_SetFlag++;
            password_Set(0);
            //rt_kprintf("\r\npassword_code_2=%d,%d,%d,%d,%d,%d,",PassWord[0],PassWord[1],PassWord[2],PassWord[3],PassWord[4],PassWord[5]);
            //rt_kprintf("\r\nok:密码 =%s,password_SetFlag=%d,password_Counter=%d\r\n",PassWord,password_SetFlag,password_Counter);
            password_Counter = 0;
        }
        else if(password_SetFlag == 7)
        {
            if((strncmp((char *)PassWord, ENTER_CODE, 6) == 0) && (!NET_SET_FLAG))
            {
                password_SetFlag = 8;
                Password_correctFlag = 1;
                set_car_codetype = 1;

                lcd_fill(0);
                lcd_text12(36, 3, "密码正确", 8, LCD_MODE_SET);
                lcd_text12(0, 19, "按菜单键进入设置信息", 20, LCD_MODE_SET);
                lcd_update_all();
            }
            else if((strncmp((char *)PassWord, "001100", 6) == 0) || (strncmp((char *)PassWord, "001101", 6) == 0))
            {
                NET_SET_FLAG = 2; //密码正确，可以开始设置网络信息了
                pMenuItem = &Menu_8_SetDNS;
                pMenuItem->show();
                rt_kprintf("\r\n密码:%s,开始DNS设置", PassWord);
            }
            else if(ic_card_para.administrator_card == 1)
            {
                password_SetFlag = 8;
                Password_correctFlag = 1;
                set_car_codetype = 1;

                lcd_fill(0);
                lcd_text12(30, 3, "管理员操作", 10, LCD_MODE_SET);
                lcd_text12(0, 19, "按菜单键进入设置信息", 20, LCD_MODE_SET);
                lcd_update_all();
            }
            else if(strncmp((char *)PassWord, "112233", 6) == 0)
            {
                password_SetFlag = 9;
                /*
                if((PassWord[5] > '0')&&(PassWord[5] <= '3'))
                	{
                	lcd_fill(0);
                	lcd_text12(36,3,"清空数据",8,LCD_MODE_SET);
                	memset(buf,0,sizeof(buf));
                	if(( PassWord[5]-'0' )& BIT(0))
                		{
                		strcat(buf,"参数.");
                		}
                	if(( PassWord[5]-'0' )& BIT(1))
                		{
                		strcat(buf,"数据.");
                		}
                	lcd_text12(12,19,buf,strlen(buf),LCD_MODE_SET);
                	lcd_update_all();
                	rt_thread_delay( RT_TICK_PER_SECOND * 2 );
                	factory_ex(PassWord[5]-'0',0);
                	pMenuItem=&Menu_0_0_password;
                	pMenuItem->show();
                	}
                	*/
                pMenuItem = &Menu_param_set_01;
                pMenuItem->show();
            }
            else
            {
                password_SetFlag = 9;
                lcd_fill(0);
                lcd_text12(36, 3, "密码错误", 8, LCD_MODE_SET);
                lcd_text12(12, 19, "按确认键重新设置", 16, LCD_MODE_SET);
                lcd_update_all();
            }
        }
        else if(password_SetFlag == 9)
        {
            pMenuItem = &Menu_0_0_password;
            pMenuItem->show();
        }

        break;
    case KEY_UP:
        if((password_SetFlag >= 1) && (password_SetFlag <= 6))
        {
            if(password_Counter == 0)
                password_Counter = 9;
            else if(password_Counter >= 1)
                password_Counter--;
            password_Set(password_Counter);
        }
        //rt_kprintf("\r\n  up:password=%s,password_SetFlag=%d,password_Counter=%d\r\n",password_Code,password_SetFlag,password_Counter);
        break;
    case KEY_DOWN:
        if((password_SetFlag >= 1) && (password_SetFlag <= 6))
        {
            password_Counter++;
            if(password_Counter > 9)
                password_Counter = 0;
            password_Set(password_Counter);
        }
        //rt_kprintf("\r\ndown:password=%s,password_SetFlag=%d,password_Counter=%d\r\n",password_Code,password_SetFlag,password_Counter);
        break;
    }
}


static void timetick(unsigned int systick)
{

    CounterBack++;
    if(CounterBack < MaxBankIdleTime)
        return;
    CounterBack = 0;

    if(device_unlock)
    {
        pMenuItem = &Menu_1_Idle;
        pMenuItem->show();
        memset(PassWord, 0, sizeof(PassWord));
        rt_kprintf("\r\n---3");
        password_SetFlag = 1;
        password_Counter = 0;
    }

}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_0_0_password =
{
    "密码设置",
    8, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};


