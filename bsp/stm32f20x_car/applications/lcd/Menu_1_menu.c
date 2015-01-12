#include "Menu_Include.h"
#include "sed1520.h"

#if 0
#define MENU1_MENU_NUM		6

u8 menu_flag = 1;					///主菜单标记
static const char *menu_Str[] =
{
    "1.记录仪信息",
    "2.信息查看",
    "3.信息交互",
    "4.其他信息",
    "5.车辆设置",
    "6.网络设置",
};

static void menu_dis(void)
{
    uint8_t i, mode;
    uint8_t current_flag, last_flag;
    uint8_t flag = (menu_flag - 1) % MENU1_MENU_NUM;
    current_flag = flag & 0xFE;

    lcd_fill(0);
    for(i = 0; i < 2; i++)
    {
        if(current_flag == flag)
            mode = LCD_MODE_INVERT;
        else
            mode = LCD_MODE_SET;
        lcd_text12(0, i * 15 + 3, menu_Str[current_flag], strlen(menu_Str[current_flag]), mode);
        ++current_flag;
        if(current_flag + 1 > MENU1_MENU_NUM)
            break;
    }
    lcd_update_all();
}

static void msg( void *p)
{

}
static void show(void)
{
    menu_flag = menu_flag % (MENU1_MENU_NUM + 1);
    if(!menu_flag)
        menu_flag = 1;
    menu_dis();
}

static void keypress(unsigned int key)
{

    switch(key)
    {
    case KEY_MENU:
        pMenuItem = &Menu_1_Idle;
        pMenuItem->show();
        menu_flag = 1;
        break;
    case KEY_OK:
        if(menu_flag == 1)
        {
            pMenuItem = &Menu_4_recorder;
        }
        else if(menu_flag == 2)
        {
            pMenuItem = &Menu_2_InforCheck;
        }
        else if(menu_flag == 3)
        {
            pMenuItem = &Menu_3_InforInteract;
        }
        else if(menu_flag == 4)
        {
            pMenuItem = &Menu_5_other;
        }
        else if(menu_flag == 5)
        {
            pMenuItem = &Menu_6_SetInfor;
        }
        else if(menu_flag == 6)
        {
            pMenuItem = &Menu_8_SetDNS;
        }
        pMenuItem->show();
        break;
    case KEY_UP:
        if(menu_flag == 1)
            menu_flag = MENU1_MENU_NUM;
        else
            menu_flag--;
        menu_dis();
        break;
    case KEY_DOWN:
        if(menu_flag >= MENU1_MENU_NUM)
            menu_flag = 1;
        else
            menu_flag++;
        menu_dis();
        break;
    }
}
#endif
static const uint8_t 	menu_num = 6;
static unsigned char	menu_pos = 0;


static PMENUITEM psubmenu[menu_num] =
{
    &Menu_4_recorder,   		//
    &Menu_2_InforCheck,     	//
    &Menu_3_InforInteract,   	//
    &Menu_5_other,         		//
    &Menu_6_SetInfor,      		//
    &Menu_8_SetDNS,       	//
};


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void menuswitch( void )
{
    uint8_t i, mode;
    uint8_t current_flag, last_flag;
    uint8_t flag = menu_pos % menu_num;
    char buf[20];
    current_flag = flag & 0xFE;

    lcd_fill(0);
    for(i = 0; i < 2; i++)
    {
        if(current_flag == flag)
            mode = LCD_MODE_INVERT;
        else
            mode = LCD_MODE_SET;
        sprintf(buf, "%d.%s", current_flag + 1, (char *)( psubmenu[current_flag]->caption ));
        lcd_text12(0, i * 15 + 3, buf, strlen(buf), mode);
        ++current_flag;
        if(current_flag + 1 > menu_num)
            break;
    }
    lcd_update_all();
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void msg( void *p )
{
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void show( void )
{
    pMenuItem->tick = rt_tick_get();
    if(( menu_pos > menu_num ) || (menu_first_in & BIT(0)))
    {
        menu_pos = 0;
    }
    menu_first_in &= ~(BIT(0));
    menuswitch( );
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void keypress( unsigned int key )
{
    switch( key )
    {
    case KEY_MENU:
        menu_pos = 0;
        pMenuItem	= &Menu_1_Idle; //;
        pMenuItem->show( );
        break;
    case KEY_OK:
        pMenuItem = psubmenu[menu_pos];
        pMenuItem->show( );
        break;
    case KEY_UP:
        if( menu_pos == 0 )
        {
            menu_pos = menu_num;
        }
        menu_pos--;
        menuswitch( );
        break;
    case KEY_DOWN:
        menu_pos++;
        if( menu_pos >= menu_num )
        {
            menu_pos = 0;
        }
        menuswitch( );
        break;
    }
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_1_menu =
{
    "主菜单",
    6,		0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};

