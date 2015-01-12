#include "Menu_Include.h"




u8 version_screen = 0;
static void msg( void *p)
{

}

static version_disp(void)
{
    unsigned char i = 0;
    char buf[64];

    lcd_fill(0);
    sprintf(buf, "硬件版本:V%d.%02d", HARD_VER / 100, HARD_VER % 100);
    lcd_text12(0, 3, buf, strlen(buf), 1);
    sprintf(buf, "软件版本:V%d.%02d", SOFT_VER / 100, SOFT_VER % 100);
    lcd_text12(0, 17, buf, strlen(buf), 1);
    lcd_update_all();
}
static void show(void)
{
    pMenuItem->tick = rt_tick_get();

    version_disp();
}


static void keypress(unsigned int key)
{

    switch(key)
    {
    case KEY_MENU:
        pMenuItem = &Menu_5_other;
        pMenuItem->show();

        version_screen = 0;
        break;
    case KEY_OK:
        //version_disp();
        break;
    case KEY_UP:
        break;
    case KEY_DOWN:
        break;
    }
}



MENUITEM	Menu_5_7_Version =
{
    "版本显示",
    8, 0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};

