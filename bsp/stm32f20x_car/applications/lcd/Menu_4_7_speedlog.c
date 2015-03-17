#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "LCD_Driver.h"
#include "GB_vdr.h"

static u8 Dis_num = 0;
static u8 Dis_maxNum = 0;
static u8 Dis_status[16] = {"1.速度状态:正常"};
static u8 Dis_start_time[25] = {"14-06-02 10:20:15"};
static u8 Dis_end_time[25]  = {"14-06-02 15:18:32"};
static u8 get_indexnum = 0;
static u8 H15_reg[133];


static void  disp_record_15h(u8 record_num)
{
    u16  retnvlaue = 0;

    retnvlaue = gb_vdr_get_15h(get_indexnum - record_num, H15_reg);
    if(H15_reg[0] == 0x01)
        memcpy(Dis_status + 11, "正常", 4);
    else
        memcpy(Dis_status + 11, "异常", 4);
    sprintf(Dis_start_time, "st:%2X-%2X-%2X %2X:%2X:%2X", H15_reg[1], H15_reg[2], H15_reg[3], H15_reg[4], H15_reg[5], H15_reg[6]);
    sprintf(Dis_end_time, "ed:%2X-%2X-%2X %2X:%2X:%2X", H15_reg[7], H15_reg[8], H15_reg[9], H15_reg[10], H15_reg[11], H15_reg[12]);
    lcd_fill(0);
    if(retnvlaue)
    {
        lcd_text12(0, 0, Dis_status, strlen(Dis_status), LCD_MODE_SET);
        lcd_text12(0, 12, Dis_start_time, strlen(Dis_start_time), LCD_MODE_SET);
        lcd_text12(0, 22, Dis_end_time, strlen(Dis_end_time), LCD_MODE_SET);
    }
    else
        lcd_text12(0, 10, "无速度状态日志记录", 18, LCD_MODE_SET);
    lcd_update_all();
}

static void msg( void *p)
{
}
static void show(void)
{
    u8  read_flag = 0;

    //0.  --    get  usefull   record num
    if(Vdr_Wr_Rd_Offset.V_15H_Write >= 1)	//没存满
        get_indexnum = Vdr_Wr_Rd_Offset.V_15H_Write - 1;
    else
        get_indexnum = 0;

    //  1.   flag  state
    if(Vdr_Wr_Rd_Offset.V_15H_Write >= 1) //没存满
    {
        get_indexnum = Vdr_Wr_Rd_Offset.V_15H_Write - 1;
        read_flag = 1;
    }
    else if(Vdr_Wr_Rd_Offset.V_15H_Write == 1)
    {
        get_indexnum = 0;
        read_flag = 1;
    }
    else
        get_indexnum = 0;

    //   2. read
    if(read_flag)
    {
        Dis_num = 0;
        Dis_status[0] = Dis_num + 1 + '0';
        disp_record_15h(Dis_num);
    }
    else
    {
        Dis_num = 0;
        lcd_fill(0);
        lcd_text12(0, 10, "无速度状态日志记录", 18, LCD_MODE_SET);
        lcd_update_all();
    }

}

//字节       数据内容         数据内容
// 1         2-7(bcd)         8-13(bcd)
//速度状态   开始时间         结束时间
//01正常
//02异常
static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        Dis_num = 0;
        pMenuItem = &Menu_4_recorder;
        pMenuItem->show();
        break;
    case KEY_OK:
        break;
    case KEY_UP:
        if(get_indexnum)
        {
            if(Dis_num == 0)
                Dis_num = Dis_maxNum;
            else
                Dis_num--;
            Dis_status[0] = Dis_num + 1 + '0';
            disp_record_15h(Dis_num);
        }
        break;
    case KEY_DOWN:
        if(get_indexnum)
        {
            if(Dis_num > (Dis_maxNum - 1))
                Dis_num = 0;
            else
                Dis_num++;

            Dis_status[0] = Dis_num + 1 + '0';
            disp_record_15h(Dis_num);
        }
        break;
    }
}




static void timetick(unsigned int systick)
{

    CounterBack++;
    if(CounterBack != MaxBankIdleTime)
        return;
    pMenuItem = &Menu_1_Idle;
    pMenuItem->show();
    CounterBack = 0;

}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_4_7_speedlog =
{
    "速度状态日志",
    12, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

