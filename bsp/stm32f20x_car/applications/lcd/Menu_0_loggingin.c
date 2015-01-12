#include "Menu_Include.h"
#include "sed1520.h"

static const uint8_t	menu_num = 6;
#define  DIS_Dur_width_inter 11


static MENUITEM	Menu_3 =
{
    "0+SIM卡号输入",
    13, 0,
    (void *)0,
    (void *)0,
    (void *)0,
    (void *)0
};




static PMENUITEM		psubmenu[menu_num] =
{
    &Menu_0_0_type,
    &Menu_0_1_license,
    &Menu_0_2_SpeedType,
    &Menu_3,//&Menu_0_3_Sim,
    &Menu_0_4_vin,
    //&Menu_0_6_PulseCoefficient,
    &Menu_0_Colour,
};



uint8_t cb_param_set_SIMID(char *val, uint8_t len)
{
    uint16_t	i, j;
    uint32_t 	u32data;
    uint8_t 	ret = 0;
    if(len <= 12)
    {
        memset(jt808_param.id_0xF006, 0, sizeof(jt808_param.id_0xF006));
        strncpy( jt808_param.id_0xF006, (char *)val, len );
        memcpy(Menu_sim_Code, jt808_param.id_0xF006, 12);
        param_save(1);
        ret = 1;
    }
    CarSet_0_counter = 4;
    return ret;
}


/*
uint8_t cb_param_set_Pulse(char * val,uint8_t len)
{
	uint16_t	i,j;
	uint32_t 	u32data;
	uint8_t 	ret = 0;

	if( sscanf(val,"%d",&u32data) == 1)
	{
		jt808_param.id_0xF032 =  u32data;
		param_save(1);
		ret = 1;
	}
	CarSet_0_counter = 6;
	return ret;
}
*/


/**/
void CarSet_0_fun( u8 set_type )
{
    unsigned char i = 0;

    lcd_fill( 0 );
    lcd_text12( 0, 3, "注册", 4, LCD_MODE_SET);
    lcd_text12( 0, 17, "输入", 4, LCD_MODE_SET);
    for( i = 0; i < menu_num; i++ )
    {
        lcd_bitmap( 30 + i * DIS_Dur_width_inter, 5, &BMP_noselect_log, LCD_MODE_SET );
    }
    lcd_bitmap( 30 + set_type * DIS_Dur_width_inter, 5, &BMP_select_log, LCD_MODE_SET );
    lcd_text12( 30, 19, (char *)( psubmenu[set_type]->caption ), psubmenu[set_type]->len, LCD_MODE_SET );
    lcd_update_all( );
}

void CarSet_0_fun_old(u8 set_type)
{
    u8 i = 0;

    lcd_fill(0);
    lcd_text12( 0, 3, "注册", 4, LCD_MODE_SET);
    lcd_text12( 0, 17, "输入", 4, LCD_MODE_SET);

    switch(set_type)
    {
        //for(i=0;i<=5;i++)
        //lcd_bitmap(35+i*12, 5, &BMP_noselect_log ,LCD_MODE_SET);
        //lcd_bitmap(35+set_type*12, 5, &BMP_select_log ,LCD_MODE_INVERT);
    case 0:
        for(i = 0; i <= 5; i++)
            lcd_bitmap(35 + i * 12, 5, &BMP_noselect_log , LCD_MODE_SET);
        lcd_bitmap(35, 5, &BMP_select_log , LCD_MODE_SET);
        lcd_text12(30, 19, "车辆类型设置", 12, LCD_MODE_INVERT);
        break;
    case 1:
        for(i = 0; i <= 5; i++)
            lcd_bitmap(35 + i * 12, 5, &BMP_noselect_log , LCD_MODE_SET);
        lcd_bitmap(47, 5, &BMP_select_log , LCD_MODE_SET);
        lcd_text12(30, 19, "车牌号输入", 10, LCD_MODE_INVERT);
        break;
    case 2:
        for(i = 0; i <= 5; i++)
            lcd_bitmap(35 + i * 12, 5, &BMP_noselect_log , LCD_MODE_SET);
        lcd_bitmap(59, 5, &BMP_select_log , LCD_MODE_SET);
        lcd_text12(30, 19, "车辆类型选择", 12, LCD_MODE_INVERT);
        break;
    case 3:
        for(i = 0; i <= 5; i++)
            lcd_bitmap(35 + i * 12, 5, &BMP_noselect_log , LCD_MODE_SET);
        lcd_bitmap(71, 5, &BMP_select_log , LCD_MODE_SET);
        lcd_text12(30, 19, "0+SIM卡号输入", 13, LCD_MODE_INVERT);
        break;
    case 4:
        for(i = 0; i <= 5; i++)
            lcd_bitmap(35 + i * 12, 5, &BMP_noselect_log , LCD_MODE_SET);
        lcd_bitmap(83, 5, &BMP_select_log , LCD_MODE_SET);
        lcd_text12(30, 19, "17位VIN输入", 11, LCD_MODE_INVERT);
        break;
    case 5:
        for(i = 0; i <= 5; i++)
            lcd_bitmap(35 + i * 12, 5, &BMP_noselect_log , LCD_MODE_SET);
        lcd_bitmap(95, 5, &BMP_select_log , LCD_MODE_SET);
        lcd_text12(30, 19, "车牌颜色输入", 12, LCD_MODE_INVERT);
        break;
    case 6:
        for(i = 0; i <= 5; i++)
            lcd_bitmap(35 + i * 12, 5, &BMP_noselect_log , LCD_MODE_SET);
        lcd_bitmap(95, 5, &BMP_select_log , LCD_MODE_SET);
        lcd_text12(30, 19, "车牌颜色输入", 12, LCD_MODE_INVERT);
        break;
    }
    lcd_update_all();
}
static void msg( void *p)
{

}
static void show(void)
{
    CounterBack = 0;
    CarSet_0_fun(CarSet_0_counter);
}


static void keypress(unsigned int key)
{
    switch(key)
    {
    case KEY_MENU:
        if((menu_color_flag) || (MENU_set_carinfor_flag == 1))
        {
            menu_color_flag = 0;

            pMenuItem = &Menu_1_Idle; //进入信息查看界面
            pMenuItem->show();
        }
        else
        {
            CarSet_0_counter = 0;
            pMenuItem = &Menu_0_0_password;//Menu_0_loggingin;//
            pMenuItem->show( );
        }
        break;
    case KEY_OK:
        if(3 == CarSet_0_counter)
        {
            lcd_set_param("0+SIM卡号设置", (uint8_t *)&jt808_param.id_0xF006, TYPE_STR, 12, 1, cb_param_set_SIMID);
        }
        else
        {
            pMenuItem = psubmenu[CarSet_0_counter];         //鉴权注册
            pMenuItem->show( );
        }
        break;
        /*
        case KEY_OK:
        if(CarSet_0_counter==0)
           	{
        	pMenuItem=&Menu_0_0_type;//车牌号输入
             pMenuItem->show();
           	}
        else if(CarSet_0_counter==1)
           	{
        	pMenuItem=&Menu_0_1_license;//车牌号输入
             pMenuItem->show();
           	}
        else if(CarSet_0_counter==2)
        	{
        	pMenuItem=&Menu_0_2_CarType;//type
            pMenuItem->show();
        	}
        else if(CarSet_0_counter==3)
        	{
        	  pMenuItem=&Menu_0_3_Sim;//sim 卡号
              pMenuItem->show();
        	}
        else if(CarSet_0_counter==4)
        	{
        	   pMenuItem=&Menu_0_4_vin;//vin
               pMenuItem->show();
        	}
        else if(CarSet_0_counter==5)
        	{
        	pMenuItem=&Menu_0_5_Colour;//颜色
              pMenuItem->show();
        	}
        break;
        */
    case KEY_UP:
        if(MENU_set_carinfor_flag == 1)
        {
            if(CarSet_0_counter == 0)
                CarSet_0_counter = menu_num;
            CarSet_0_counter--;

            CarSet_0_fun(CarSet_0_counter);
        }
        break;
    case KEY_DOWN:
        if(MENU_set_carinfor_flag == 1)
        {
            CarSet_0_counter++;
            if(CarSet_0_counter >= menu_num)
                CarSet_0_counter = 0;

            CarSet_0_fun(CarSet_0_counter);
        }
        break;
    }
}


static void timetick(unsigned int systick)
{
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_0_loggingin =
{
    "车辆设置",
    8, 0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

