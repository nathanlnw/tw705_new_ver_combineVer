#include "Menu_Include.h"
#include <string.h>
#include "LCD_Driver.h"


uint8_t 			debug_msg[DEBUG_MSG_NUM][DEBUG_MSG_MAX_LEN];
uint8_t				debug_msg_rd = 0;
volatile uint8_t	debug_msg_wr = 0;
uint8_t				debug_stop = 0;
static uint8_t 		debug_str_num = 0;
static uint8_t		debug_msg_new = 0;

/*********************************************************************************
  *��������:uint8_t debug_write(char * str)
  *��������:���������Ϣ��Һ���������Ǹ���Ϣֻ���ڽ���"Menu_5_9_debug"�в���ʾ
  *��	��:	str:��Ҫ��ʾ�ĵ�����Ϣ��ע�����Ϣ�����������DEBUG_MSG_MAX_LEN-1������Ľ�����ȡ������ʾ
  *��	��: none
  *�� �� ֵ:uint8_t.	0��ʾû�н���Ϣд����ʾbuf��������ֹͣˢ�£�1��ʾ����д��
  *��	��:������
  *��������:2014-10-30
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t debug_write(char *str)
{
    uint16_t	i;
    char		*p;
    uint16_t 	len = strlen(str);
    char		buf[32];

    sd_write_console(str);
    if((debug_str_num < DEBUG_MSG_NUM) || (debug_stop == 0))
    {
        if(len > DEBUG_MSG_MAX_LEN - 1)
            len = DEBUG_MSG_MAX_LEN - 1;
        if(memcmp(str, debug_msg[debug_msg_wr], len) == 0)	//���Ҫд������ݺ�֮ǰд�����ȫ��ͬ����д�롣
            return 0;
        /////////
        p = str;
        for(i = 0; i < 10; i++)
        {
            memset(buf, 0, 22);
            len = my_strncpy(buf, p, 19);
            p += len;
            if(len == 0)
                break;
            if(len > DEBUG_MSG_MAX_LEN - 1)
                len = DEBUG_MSG_MAX_LEN - 1;
            if(debug_str_num)			///����������ˣ��ͽ�Ҫд�����Ϣ����
                debug_msg_wr++;
            debug_msg_wr %= DEBUG_MSG_NUM;
            memset(debug_msg[debug_msg_wr], 0x20, DEBUG_MSG_MAX_LEN);
            my_strncpy(debug_msg[debug_msg_wr], buf, len);
            debug_msg[debug_msg_wr][DEBUG_MSG_MAX_LEN - 1] = 0;
            if(debug_stop == 0)
            {
                debug_msg_rd = debug_msg_wr;
                debug_msg_new = 1;
            }
            if(debug_str_num < DEBUG_MSG_NUM)
                debug_str_num++;
        }
        /////////
        return 1;
    }
    return 0;
}

static void msg( void *p)
{

}

static debug_disp(void)
{
    unsigned char i = 0;
    uint8_t play_line;
    uint8_t first_line;
    char buf[64];
    uint8_t lcd_mode;

    if(debug_msg_rd == 0)
    {
        play_line = DEBUG_MSG_NUM - 1;
    }
    else
    {
        play_line = debug_msg_rd - 1;
    }
    if(debug_stop == 0)
    {
        lcd_mode = LCD_MODE_SET;
    }
    else
    {
        lcd_mode = LCD_MODE_INVERT;
    }
    //lcd_fill(0);
    //��Ҫ��ʾ�������ַ��������һλ�����Ϊ0������������ʱ�򲻻Ὣ��������ݿ�������
    for(i = 0; i < DEBUG_MSG_NUM; i++)
    {
        debug_msg[i][DEBUG_MSG_MAX_LEN - 1] = 0;
    }
    if(debug_str_num < DEBUG_MSG_NUM)
        first_line = 0;
    else
        first_line = (debug_msg_wr + 1) % DEBUG_MSG_NUM;
    //���bufΪ0
    memset(buf, 0, sizeof(buf));
    //����Ҫ��ʾ���ַ�
    //sprintf(buf,":%s",debug_msg[play_line]);
    if(play_line == debug_msg_wr)
    {
        strcat(buf, debug_msg[play_line]);
        buf[19] = '>';
    }
    else
    {
        if(play_line == first_line)
            buf[0] = '>';
        strcat(buf, debug_msg[play_line]);
    }
    //��������ַ�ȫ����ʾΪ�ո������Ͳ���Ҫÿ�ζ�����
    for(i = 1; i < 20; i++)
    {
        if(buf[i] == 0)
            buf[i] = 0x20;
    }
    lcd_text12(0, 3, buf, 20, lcd_mode);

    //���bufΪ0
    memset(buf, 0, sizeof(buf));
    //����Ҫ��ʾ���ַ�
    //sprintf(buf,":%s",debug_msg[debug_msg_rd]);
    if(debug_msg_rd == debug_msg_wr)
    {
        strcat(buf, debug_msg[debug_msg_rd]);
        buf[19] = '>';
    }
    else
    {
        if(debug_msg_rd == first_line)
            buf[0] = '>';
        strcat(buf, debug_msg[debug_msg_rd]);
    }
    //��������ַ�ȫ����ʾΪ�ո������Ͳ���Ҫÿ�ζ�����
    for(i = 1; i < 20; i++)
    {
        if(buf[i] == 0)
            buf[i] = 0x20;
    }
    lcd_text12(0, 17, buf, 20, lcd_mode);
    lcd_update_all();
}
static void show(void)
{
    pMenuItem->tick = rt_tick_get();
    debug_stop = 0;
    debug_msg_rd = debug_msg_wr;
    lcd_fill(0);
    debug_disp();
}


static void keypress(unsigned int key)
{
    uint8_t temp_line = 0;
    switch(key)
    {
    case KEY_MENU:
        pMenuItem = &Menu_5_other;
        pMenuItem->show();
        return;
    case KEY_OK:
        debug_stop ^= 0x01;
        //version_disp();
        break;
    case KEY_UP:
        if(debug_str_num < DEBUG_MSG_NUM)
        {
            temp_line = 1;
        }
        else
            temp_line = (debug_msg_wr + 2) % DEBUG_MSG_NUM;
        //if((debug_stop)&&(debug_msg_rd != temp_line))
        if(debug_msg_rd != temp_line)
        {
            if((debug_msg_rd == 0) || (debug_msg_rd >= DEBUG_MSG_NUM ))
                debug_msg_rd = DEBUG_MSG_NUM - 1;
            else
                debug_msg_rd--;
        }
        break;
    case KEY_DOWN:
        //if((debug_stop)&&(debug_msg_rd != debug_msg_wr))
        if(debug_msg_rd != debug_msg_wr)
        {
            if(debug_msg_rd >= DEBUG_MSG_NUM - 1)
                debug_msg_rd = 0;
            else
                debug_msg_rd++;
        }
        break;
    }
    debug_disp();
}

/*����Ƿ�ص�������*/
void timetick_debug( unsigned int tick )
{
    MENUITEM *tmp;
    if( ( tick - pMenuItem->tick ) >= RT_TICK_PER_SECOND * 120 )
    {
        pMenuItem = &Menu_1_Idle;
        pMenuItem->show( );
    }
    else if(debug_msg_new)
    {
        debug_msg_new = 0;
        debug_disp();
    }
}



MENUITEM	Menu_5_9_debug =
{
    "������Ϣ��ʾ",
    8, 0,
    &show,
    &keypress,
    &timetick_debug,
    &msg,
    (void *)0
};


