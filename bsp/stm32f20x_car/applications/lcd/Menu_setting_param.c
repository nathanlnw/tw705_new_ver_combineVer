#include  <string.h>
#include "Menu_Include.h"
//#include "Lcd.h"
#include "sed1520.h"


static u8 menu1 = 0;		//������
static u8 menu2 = 0;		//�ڶ������
static u8 menu3 = 0;		//���������
static u8 set_type_ok = 0;	//��0��ʾ����OK
static char 	*param_name;				///��ǰ���ڴ���Ĳ���������
static uint8_t 	param_len;					///��ǰ���ڴ���Ĳ����ĳ���
static uint8_t 	param_type;					///��ǰ���ڴ���Ĳ���������
static uint8_t	param_is_num;				///������Ϊ1ʱ��ʾΪ�����ָ�ʽ���ַ���
static uint8_t *param_data;					///��ǰ���ڴ���Ĳ���������
static char 	param_in_val[36];			///��ǰ���ڴ���Ĳ�����ֵ
static char 	param_out_val[36];			///��ǰ���ڴ���Ĳ�����ֵ
static uint8_t (*cb_param_set)(char *val, uint8_t len);
static MENUITEM	*Menu_last;


static uint8_t  SetFlag;
static uint8_t	param_change_num_select;	///�޸ĵ�ǰ���ڴ���Ĳ����е�ĳһ����ֵ�ģ��޸�λ��ѡ��
static uint8_t 	param_change_num;			///�޸ĵ�ǰ���ڴ���Ĳ����е�ĳһ����ֵ�ģ�0xFF��ʾ��Ч
static uint8_t 	param_input_num;			///���뵱ǰ���ڴ���Ĳ����е�ĳһ����ֵ��

static unsigned char select_num[] = {0x0C, 0x06, 0xFF, 0x06, 0x0C};

DECL_BMP(8, 5, select_num);


static void msg( void *p)
{

}

uint8_t cb_param_set_default(char *val, uint8_t len)
{
    uint16_t	i, j;
    uint32_t 	u32data;
    uint8_t 	ret = 0;
    if(param_data == 0)
        return 0;
    switch(param_type)
    {
    case TYPE_BYTE:
    case TYPE_WORD:
    case TYPE_DWORD:
        if( sscanf(val, "%d", &u32data) == 1)
        {
            if(param_type == TYPE_BYTE)
            {
                *param_data = u32data;
            }
            else if(param_type == TYPE_WORD)
            {
                *(uint16_t *)param_data = u32data;
            }
            else
            {
                *(uint32_t *)param_data = u32data;
            }
            ret = 1;
        }
        break;
    case TYPE_CAN:
        if(strlen(val) < 9)
        {
            memset(param_data, 0, 8);
            Ascii_To_Hex(param_data, val, 8);
            ret = 1;
        }
        break;
    default :
        if(len <= param_len)
        {
            memset(param_data, 0, param_len);
            strncpy( param_data, (char *)val, len );
            ret = 1;
        }
        break;
    }
    if(ret)
        param_save(1);
    return ret;
}


/*********************************************************************************
  *��������:uint8_t debug_write(char * str)
  *��������:���������Ϣ��Һ���������Ǹ���Ϣֻ���ڽ���"Menu_5_9_debug"�в���ʾ
  *��	��:		name	:��������
  				data	:��������
  				type	:��������
  				len		:��Ҫ����Ĳ�������
  				is_num	:������Ϊ1ʱ��ʾΪ�����ָ�ʽ���ַ���
  				cb_res	:�������ص���������Ҫ�����������
  *��	��: none
  *�� �� ֵ:uint8_t.	0��ʾû�н���Ϣд����ʾbuf��������ֹͣˢ�£�1��ʾ����д��
  *��	��:������
  *��������:2014-10-30
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t lcd_set_param(char *name, uint8_t *data, uint8_t type, uint8_t len , uint8_t is_num , uint8_t (*cb_res)())
{
    uint16_t	i, j;
    uint32_t 	u32data;

    if(name)
        param_name	= name;
    else
        param_name 	= "����";
    if(len > 20)
        len = 20;
    param_data = data;
    param_is_num = is_num;
    memset(param_in_val, 0, sizeof(param_in_val));
    ////////////
    switch(type)
    {
    case TYPE_BYTE:
    case TYPE_WORD:
    case TYPE_DWORD:
        if(type == TYPE_BYTE)
        {
            u32data = *data;
        }
        else if(type == TYPE_WORD)
        {
            u32data = *(uint16_t *)data;
        }
        else if(type == TYPE_DWORD)
        {
            u32data = *(uint32_t *)data;
        }
        sprintf(param_in_val, "%010d", u32data);
        ///������Ĳ��������
        if(strlen(param_in_val) > len)
        {
            j = strlen(param_in_val) - len;
            for(i = 0; i < strlen(param_in_val); i++)
            {
                param_in_val[i] = param_in_val[i + j];
            }
        }
        break;
    case TYPE_CAN:
        Hex_To_Ascii(param_in_val, data, 8);
        break;
    default :
        if(type > TYPE_CAN)
        {
            memcpy(param_in_val, data, strlen(data));
        }
        break;
    }
    ////////////
    memcpy(param_out_val, param_in_val, sizeof(param_in_val));
    param_type	= type;
    param_len	= len;
    if(cb_res)
        cb_param_set = cb_res;
    else
        cb_param_set = cb_param_set_default;
    Menu_last = pMenuItem;
    pMenuItem = &Menu_setting_param;
    pMenuItem->show();
    return 0;
}


static void param_set_disp(void)
{
    unsigned char i = 0;
    uint8_t play_line;
    uint8_t first_line;
    char buf[64];
    uint8_t lcd_mode;
    char c;
    lcd_fill(0);
    if(menu2 == 0)
    {
        lcd_text12(10, 3, "��ȷ�����޸Ĳ���", 16, LCD_MODE_SET);		///1
        lcd_text12((128 - strlen(param_in_val) * 6) / 2, 18, param_in_val, strlen(param_in_val), LCD_MODE_SET);		///2
    }
    else
    {
        ///���������
        if(set_type_ok)
        {
            lcd_text12((128 - strlen(param_name) * 6) / 2, 3, param_name, strlen(param_name), LCD_MODE_SET);		///1
            lcd_text12(26, 18, "�������!", 9, LCD_MODE_SET);		///1
        }
        ///������
        ///�����ָ�ʽ
        else if(param_is_num)
        {
            //��ʾ����һ�������еĲ���
            for(i = 0; i < param_len; i++)
            {
                if((param_out_val[i] >= '0') && (param_out_val[i] <= '9'))
                {
                    c = param_out_val[i];
                }
                else
                {
                    c = '-';
                }
                if(i == param_change_num)
                {
                    lcd_mode = LCD_MODE_INVERT;
                }
                else
                {
                    if((param_change_num == 0xFF) && (param_input_num == i ))
                        lcd_mode = LCD_MODE_INVERT;
                    else
                        lcd_mode = LCD_MODE_SET;
                }
                lcd_text12(i * 6, 1, &c, 1, lcd_mode);
            }
            ///����һ������ѡ��Ĳ���
            for(i = 0; i < 10; i++)
            {
                c = '0' + i;
                if(i == SetFlag)
                {
                    //lcd_mode = LCD_MODE_INVERT;
                    lcd_mode = LCD_MODE_SET;
                }
                else
                {
                    lcd_mode = LCD_MODE_SET;
                }
                lcd_text12(i * 6, 20, &c, 1, lcd_mode);
            }
            lcd_bitmap(SetFlag * 6, 15, &BMP_select_num, LCD_MODE_SET);
        }
        ///��ĸ���ָ�ʽ
        else
        {
            ;
        }
    }
    lcd_update_all();
}



static void show(void)
{
    menu1 = 1;
    menu2 = 0;
    menu3 = 0;
    set_type_ok = 0;
    SetFlag = 0;
    param_change_num = 0xFF;
    param_input_num = 0;
    param_change_num_select = 0;
    param_set_disp();
}



static void keypress(unsigned int key)
{
    uint8_t i;

    switch(key)
    {
    case KEY_MENU:
        if((menu2 == 0) || (set_type_ok))
        {
            pMenuItem = Menu_last;
            pMenuItem->show();
            return;
        }
        else
        {
            ///�����ָ�ʽ
            if(param_is_num)
            {
                if(param_change_num == 0xFF)		///�����޸Ĳ���
                {
                    param_change_num = 0;
                }
                else							///�����������
                {
                    menu2 = 0;
                    param_change_num = 0xFF;
                    param_change_num_select = 0;
                }
            }
            ///��ĸ���ָ�ʽ
            else
            {
                ;
            }
        }
        break;
    case KEY_OK:
        if(set_type_ok)
        {
            pMenuItem = Menu_last;
            pMenuItem->show();
            return;
        }
        else if(menu2 == 0)
        {
            menu2 = 1;
            menu3 = 0;
            param_change_num = 0xFF;
            param_change_num_select = 0;
            param_input_num = 0;
            SetFlag = 0;
        }
        else
        {
            ///�����ָ�ʽ
            if(param_is_num)
            {
                if(param_change_num != 0xFF)		///�����޸Ĳ���
                {
                    if(param_change_num_select)
                    {
                        param_out_val[param_change_num] = '0' + SetFlag;
                        param_change_num = 0xFF;
                        param_change_num_select = 0;
                    }
                    else
                    {
                        param_change_num_select = 1;
                    }
                }
                else							///�����������
                {
                    if(param_input_num < param_len)
                    {
                        param_out_val[param_input_num] = '0' + SetFlag;
                        param_input_num++;
                    }
                    else
                    {
                        set_type_ok = 1;
                        cb_param_set(param_out_val, strlen(param_out_val));
                    }
                }
                SetFlag = 0;
            }
            ///��ĸ���ָ�ʽ
            else
            {
                ;
            }
        }
        break;
    case KEY_UP:
        if(menu2)
        {
            ///�����ָ�ʽ
            if(param_is_num)
            {
                if((param_change_num != 0xFF) && (param_change_num_select == 0))
                {
                    param_change_num--;
                    if(param_change_num >= param_len)
                        param_change_num = param_len - 1;
                }
                else
                {
                    SetFlag--;
                    if(SetFlag > 9)
                        SetFlag = 9;
                }
            }
            ///��ĸ���ָ�ʽ
            else
            {
            }
        }
        break;
    case KEY_DOWN:
        if(menu2)
        {
            ///�����ָ�ʽ
            if(param_is_num)
            {
                if((param_change_num != 0xFF) && (param_change_num_select == 0))
                {
                    param_change_num++;
                    if(param_change_num >= param_len)
                        param_change_num = 0;
                }
                else
                {
                    SetFlag++;
                    if(SetFlag > 9)
                        SetFlag = 0;
                }
            }
            ///��ĸ���ָ�ʽ
            else
            {
            }
        }
        break;
    }
    param_set_disp();
}


static void timetick(unsigned int tick)
{
    MENUITEM *tmp;
    if( ( tick - pMenuItem->tick ) >= 100 * 60 )
    {
        if( Menu_last != (void *)0 )
        {
            tmp = pMenuItem->parent;
            pMenuItem->parent = (void *)0;
            pMenuItem = tmp;
            pMenuItem->show( );
        }
    }
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_setting_param =
{
    "�����޸Ľ���",
    12, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};




