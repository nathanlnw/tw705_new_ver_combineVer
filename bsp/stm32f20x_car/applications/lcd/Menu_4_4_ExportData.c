#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "LCD_Driver.h"
#include "usbh_usr.h"
#include "jt808_GB19056.h"


static uint8_t menu = 0;
uint8_t usb_export_status = 0;		///0,表示没有检测到U盘，无法导出；1表示正常导出数据，66表示导出成功


static void dis_pro(void)
{
    lcd_fill(0);
    if(usb_export_status == 66)
    {
        lcd_text12(12, 10, "USB数据导出完成!", 16, LCD_MODE_SET);
    }
    else
    {
        if( 0 == menu )
        {
            lcd_text12(15, 3, "USB导出所有数据", 15, LCD_MODE_SET);
            lcd_text12(20, 18, "取消", 4, LCD_MODE_SET);
            lcd_text12(75, 18, "确定", 4, LCD_MODE_INVERT);
        }
        else if(1 == menu)
        {
            if(usb_export_status == 1)
                lcd_text12(12, 10, "USB数据导出中...", 16, LCD_MODE_SET);
            else if(usb_export_status != 66)
                lcd_text12(24, 10, "没有检测到U盘", 13, LCD_MODE_SET);
        }
    }
    lcd_update_all();
}


static void show(void)
{
    //usb_export_status 	= 0;
    pMenuItem->tick = rt_tick_get( );
    menu		= 0;
    dis_pro();
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        usb_export_status = 0;
        pMenuItem = &Menu_4_recorder;
        pMenuItem->show();
        return;
    case KEY_OK:
        if(menu < 1)
            menu++;
        if(menu == 1)
        {
            if(USB_Disk_RunStatus() == USB_FIND)
            {
                if(usb_export_status == 0)
                {
                    usb_export_status = 1;
                    gb_usb_out(0xFE);// OUTPUT ALL info
                }
            }
            else
            {
                usb_export_status = 0;
            }
        }
        break;
    case KEY_UP:
    case KEY_DOWN:
        break;
    }
    dis_pro();
}

static void msg( void *p)
{

}

#if 0
u8 export_screen = 0;
u8 export_UsbCom_flag = 0;
u8 export_UsbCom_num = 0;
u8 export_UsbCom_0015 = 0;

static void sel_usb_com(u8 par)
{
    lcd_fill(0);
    lcd_text12(0, 3, "1.USB数据导出", 13, LCD_MODE_SET);
    lcd_text12(0, 18, "2.串口数据导出", 14, LCD_MODE_SET);
    if(par == 1)
        lcd_text12(0, 3, "1.USB数据导出", 13, LCD_MODE_INVERT);
    else
        lcd_text12(0, 18, "2.串口数据导出", 14, LCD_MODE_INVERT);
    lcd_update_all();
}

static void data_export(u8 par)  //  1:usb   2:com
{
    lcd_fill(0);
    lcd_text12(0, 3, "1.所有数据2.推荐数据", 20, LCD_MODE_SET);
    lcd_text12(0, 18, "3.指定数据(00H-15H)", 19, LCD_MODE_SET);
    if(par == 1)
        lcd_text12(0, 3, "1.所有数据", 10, LCD_MODE_INVERT);
    else if(par == 2)
        lcd_text12(60, 3, "2.推荐数据", 10, LCD_MODE_INVERT);
    else if(par == 3)
        lcd_text12(0, 18, "3.指定数据(00H-15H)", 19, LCD_MODE_INVERT);
    lcd_update_all();
}


static void data_00H_15h(u8 par)
{
    lcd_fill(0);
    switch(par)
    {
    case 0:
        lcd_text12(0, 3, "00H:执行标准版本", 16, LCD_MODE_INVERT);
        lcd_text12(0, 18, "01H:当前驾驶人信息", 18, LCD_MODE_SET);
        break;
    case 1:
        lcd_text12(0, 3, "00H:执行标准版本", 16, LCD_MODE_SET);
        lcd_text12(0, 18, "01H:当前驾驶人信息", 18, LCD_MODE_INVERT);
        break;
    case 2:
        lcd_text12(0, 3, "02H:记录仪实时时间", 18, LCD_MODE_INVERT);
        lcd_text12(0, 18, "03H:累计行驶里程", 16, LCD_MODE_SET);
        break;
    case 3:
        lcd_text12(0, 3, "02H:记录仪实时时间", 18, LCD_MODE_SET);
        lcd_text12(0, 18, "03H:累计行驶里程", 16, LCD_MODE_INVERT);
        break;
    case 4:
        lcd_text12(0, 3, "04H:记录仪脉冲系数", 18, LCD_MODE_INVERT);
        lcd_text12(0, 18, "05H:车辆信息", 12, LCD_MODE_SET);
        break;
    case 5:
        lcd_text12(0, 3, "04H:记录仪脉冲系数", 18, LCD_MODE_SET);
        lcd_text12(0, 18, "05H:车辆信息", 12, LCD_MODE_INVERT);
        break;
    case 6:
        lcd_text12(0, 3, "06H:状态信号配置信息", 20, LCD_MODE_INVERT);
        lcd_text12(0, 18, "07H:唯一性编号", 14, LCD_MODE_SET);
        break;
    case 7:
        lcd_text12(0, 3, "06H:状态信号配置信息", 20, LCD_MODE_SET);
        lcd_text12(0, 18, "07H:唯一性编号", 14, LCD_MODE_INVERT);
        break;
    case 8:
        lcd_text12(0, 3, "08H:指定行驶速度记录", 20, LCD_MODE_INVERT);
        lcd_text12(0, 18, "09H:指定位置信息记录", 20, LCD_MODE_SET);
        break;
    case 9:
        lcd_text12(0, 3, "08H:指定行驶速度记录", 20, LCD_MODE_SET);
        lcd_text12(0, 18, "09H:指定位置信息记录", 20, LCD_MODE_INVERT);
        break;
    case 10:
        lcd_text12(0, 3, "10H:指定事故疑点记录", 20, LCD_MODE_INVERT);
        lcd_text12(0, 18, "11H:指定超时驾驶记录", 20, LCD_MODE_SET);
        break;
    case 11:
        lcd_text12(0, 3, "10H:指定事故疑点记录", 20, LCD_MODE_SET);
        lcd_text12(0, 18, "11H:指定超时驾驶记录", 20, LCD_MODE_INVERT);
        break;
    case 12:
        lcd_text12(0, 3, "12H:指定驾驶人身份", 18, LCD_MODE_INVERT);
        lcd_text12(0, 18, "13H:指定外部供电记录", 20, LCD_MODE_SET);
        break;
    case 13:
        lcd_text12(0, 3, "12H:指定驾驶人身份", 18, LCD_MODE_SET);
        lcd_text12(0, 18, "13H:指定外部供电记录", 20, LCD_MODE_INVERT);
        break;
    case 14:
        lcd_text12(0, 3, "14H:指定参数修改记录", 20, LCD_MODE_INVERT);
        lcd_text12(0, 18, "15H:指定速度状态日志", 20, LCD_MODE_SET);
        break;
    case 15:
        lcd_text12(0, 3, "14H:指定参数修改记录", 20, LCD_MODE_SET);
        lcd_text12(0, 18, "15H:指定速度状态日志", 20, LCD_MODE_INVERT);
        break;
    }
    lcd_update_all();
}
static void msg( void *p)
{
}
static void show(void)
{
    export_UsbCom_flag = 1;
    export_UsbCom_num = 1;
    sel_usb_com(1);
}



static void keypress(unsigned int key)
{
    u8 temp[21];
    switch(key)
    {
    case KEY_MENU:
        CounterBack = 0;
        export_screen = 0;
        export_UsbCom_flag = 0;
        export_UsbCom_num = 0;

        pMenuItem = &Menu_4_recorder;
        pMenuItem->show();
        break;
    case KEY_OK:
        if(export_screen == 0)
        {
            export_screen = 1; //
            if(export_UsbCom_flag == 1)
            {
                if(USB_Disk_RunStatus() == USB_FIND)
                {
                    lcd_fill(0);
                    lcd_text12(0, 10, "USB所有数据导出  ...", 20, LCD_MODE_SET);
                    lcd_update_all();
                    gb_usb_out(0xFE);// OUTPUT ALL info
                    // rt_kprintf("\r\n USB 导出所有数据\r\n");
                }
                else
                {
                    lcd_fill(0);
                    lcd_text12(0, 10, "没有检测到U盘", 13, LCD_MODE_SET);
                    lcd_update_all();
                    // rt_kprintf("\r\n 没有检测到U盘\r\n");
                }
            }
            else
                data_export(1);
        }
        else if(export_screen == 1)
        {
            /*if(export_UsbCom_flag==1)//USB数据导出
            	{
            	lcd_fill(0);
            	if(export_UsbCom_num==1)
            		lcd_text12(0, 10,"USB所有数据导出2  ...",21, LCD_MODE_SET);
            	else if(export_UsbCom_num==2)
            		lcd_text12(0, 10,"USB推荐数据导出  ...",20, LCD_MODE_SET);
            	else if(export_UsbCom_num==3)
            		{
            		export_screen=2;//选择00H-15H
            		export_UsbCom_0015=0;
                    data_00H_15h(0);
            		}
            	lcd_update_all();
            	}
            else*/
            if(export_UsbCom_flag == 2) //串口数据导出
            {
                lcd_fill(0);
                if(export_UsbCom_num == 1)
                    lcd_text12(0, 10, "串口所有数据导出 ...", 16, LCD_MODE_SET);
                else if(export_UsbCom_num == 2)
                    lcd_text12(0, 10, "串口推荐数据导出 ...", 16, LCD_MODE_SET);
                else if(export_UsbCom_num == 3)
                {
                    export_screen = 2; //选择00H-15H
                    export_UsbCom_0015 = 0;
                    data_00H_15h(0);
                }
                lcd_update_all();
            }
        }
        else if(export_screen == 2)
        {
            if(export_UsbCom_flag == 1)
                memcpy(temp, "USB 指定数据导出:", 17);
            else
                memcpy(temp, "串口指定数据导出:", 17);
            temp[17] = export_UsbCom_0015 / 10 + '0';
            temp[18] = export_UsbCom_0015 % 10 + '0';
            temp[19] = 'H';
            lcd_fill(0);
            lcd_text12(0, 10, temp, 20, LCD_MODE_SET);
            lcd_update_all();
        }
        break;
    case KEY_UP:
        if(export_screen == 0)
        {
            export_UsbCom_flag = 1;
            sel_usb_com(1);//USB 数据导出
        }
        else if(export_screen == 1)
        {
            if(export_UsbCom_num <= 1)
                export_UsbCom_num = 3;
            else
                export_UsbCom_num--;
            data_export(export_UsbCom_num);
        }
        else if(export_screen == 2) //  指定数据类型选择标志
        {
            if(export_UsbCom_0015 <= 0)
                export_UsbCom_0015 = 15;
            else
                export_UsbCom_0015--;
            data_00H_15h(export_UsbCom_0015);
        }
        break;
    case KEY_DOWN:
        if(export_screen == 0)
        {
            export_UsbCom_flag = 2;
            sel_usb_com(2); //
        }
        else if(export_screen == 1)
        {
            if(export_UsbCom_num >= 3)
                export_UsbCom_num = 1;
            else
                export_UsbCom_num++;
            data_export(export_UsbCom_num);
        }
        else if(export_screen == 2) //  指定数据类型选择标志
        {
            if(export_UsbCom_0015 >= 15)
                export_UsbCom_0015 = 0;
            else
                export_UsbCom_0015++;
            data_00H_15h(export_UsbCom_0015);
        }
        break;
    }
}


#endif

static void timetick(unsigned int systick)
{
    static uint8_t last_status = 0;
    if(last_status != usb_export_status)
    {
        dis_pro();
    }
    CounterBack++;
    if(CounterBack < MaxBankIdleTime)
        return;
    if(GB19056.usb_write_step != 0) //GB_USB_OUT_idle
    {
        usb_export_status = 0;
        pMenuItem = &Menu_1_Idle;
        pMenuItem->show();
        CounterBack = 0;
    }

}

MENUITEM	Menu_4_4_ExportData =
{
    "记录仪数据导出",
    14, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

