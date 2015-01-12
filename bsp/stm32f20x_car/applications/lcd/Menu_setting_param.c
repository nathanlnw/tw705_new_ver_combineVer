#include  <string.h>
#include "Menu_Include.h"
//#include "Lcd.h"
#include "sed1520.h"


static u8 menu1 = 0;		//主界面
static u8 menu2 = 0;		//第二层界面
static u8 menu3 = 0;		//第三层界面
static u8 set_type_ok = 0;	//非0表示设置OK
static char 	*param_name;				///当前正在处理的参数的名称
static uint8_t 	param_len;					///当前正在处理的参数的长度
static uint8_t 	param_type;					///当前正在处理的参数的类型
static uint8_t	param_is_num;				///该数据为1时表示为纯数字格式的字符串
static uint8_t *param_data;					///当前正在处理的参数的数据
static char 	param_in_val[36];			///当前正在处理的参数的值
static char 	param_out_val[36];			///当前正在处理的参数的值
static uint8_t (*cb_param_set)(char *val, uint8_t len);
static MENUITEM	*Menu_last;


static uint8_t  SetFlag;
static uint8_t	param_change_num_select;	///修改当前正在处理的参数中的某一个的值的，修改位被选择
static uint8_t 	param_change_num;			///修改当前正在处理的参数中的某一个的值的，0xFF表示无效
static uint8_t 	param_input_num;			///输入当前正在处理的参数中的某一个的值的

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
  *函数名称:uint8_t debug_write(char * str)
  *功能描述:输出调试信息到液晶屏，但是该信息只有在界面"Menu_5_9_debug"中才显示
  *输	入:		name	:参数名称
  				data	:参数数据
  				type	:参数类型
  				len		:需要保存的参数长度
  				is_num	:该数据为1时表示为纯数字格式的字符串
  				cb_res	:处理结果回调函数，主要用来保存参数
  *输	出: none
  *返 回 值:uint8_t.	0表示没有将信息写入显示buf，可能是停止刷新；1表示正常写入
  *作	者:白养民
  *创建日期:2014-10-30
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t lcd_set_param(char *name, uint8_t *data, uint8_t type, uint8_t len , uint8_t is_num , uint8_t (*cb_res)())
{
    uint16_t	i, j;
    uint32_t 	u32data;

    if(name)
        param_name	= name;
    else
        param_name 	= "参数";
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
        ///将多余的部分清除掉
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
        lcd_text12(10, 3, "按确定键修改参数", 16, LCD_MODE_SET);		///1
        lcd_text12((128 - strlen(param_in_val) * 6) / 2, 18, param_in_val, strlen(param_in_val), LCD_MODE_SET);		///2
    }
    else
    {
        ///设置完成了
        if(set_type_ok)
        {
            lcd_text12((128 - strlen(param_name) * 6) / 2, 3, param_name, strlen(param_name), LCD_MODE_SET);		///1
            lcd_text12(26, 18, "设置完成!", 9, LCD_MODE_SET);		///1
        }
        ///设置中
        ///纯数字格式
        else if(param_is_num)
        {
            //显示上面一行输入中的参数
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
            ///下面一行正在选择的参数
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
        ///字母数字格式
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
            ///纯数字格式
            if(param_is_num)
            {
                if(param_change_num == 0xFF)		///正在修改参数
                {
                    param_change_num = 0;
                }
                else							///正在输入参数
                {
                    menu2 = 0;
                    param_change_num = 0xFF;
                    param_change_num_select = 0;
                }
            }
            ///字母数字格式
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
            ///纯数字格式
            if(param_is_num)
            {
                if(param_change_num != 0xFF)		///正在修改参数
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
                else							///正在输入参数
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
            ///字母数字格式
            else
            {
                ;
            }
        }
        break;
    case KEY_UP:
        if(menu2)
        {
            ///纯数字格式
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
            ///字母数字格式
            else
            {
            }
        }
        break;
    case KEY_DOWN:
        if(menu2)
        {
            ///纯数字格式
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
            ///字母数字格式
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
    "参数修改界面",
    12, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};




