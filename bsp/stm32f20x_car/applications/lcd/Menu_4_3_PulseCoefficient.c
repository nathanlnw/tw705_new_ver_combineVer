#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "sed1520.h"
static void show(void)
{
    char buf[32];
    lcd_fill( 0 );
    sprintf( buf, "脉冲系数:%05d", jt808_param.id_0xF032 & 0xFFFF);
    lcd_text12( 0, 4, (char *)buf, strlen( buf ), LCD_MODE_SET );
    sprintf( buf, "脉冲速度:%03dKM/H", car_data.get_speed);
    lcd_text12( 0, 18, (char *)buf, strlen( buf ), LCD_MODE_SET );
    lcd_update_all( );

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
    if( key == KEY_MENU )
    {
        pMenuItem = &Menu_4_recorder;
        pMenuItem->show( );
    }
}


/**/
static void msg( void *p )
{
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_4_3_PulseCoefficient =
{
    "脉冲系数相关",
    12, 0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};



