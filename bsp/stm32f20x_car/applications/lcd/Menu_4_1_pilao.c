/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// 文件名
 * Author:			// 作者
 * Date:			// 日期
 * Description:		// 模块描述
 * Version:			// 版本信息
 * Function List:	// 主要函数及其功能
 *     1. -------
 * History:			// 历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include  <string.h>
#include "Menu_Include.h"
#include <string.h>
#include "LCD_Driver.h"


#if 1
static uint8_t	count;
static uint8_t	pos;

REC_OVERTIME rec_overtime[2];

/**/
static void pilao_display_old( void )
{
    uint8_t *p;
    uint8_t len_tel, len_man;
    char	buf[32];
    lcd_fill( 0 );
    if( count == 0 )
    {
        lcd_fill( 0 );
        lcd_text12( ( 122 - 16 * 6 ) >> 1, 14, "[无疲劳驾驶记录]", 16, LCD_MODE_SET );
    }
    else
    {
        sprintf(buf, "[%d]%02d-%02d-%02d %02d:%02d:%02d",
                pos + 1,
                rec_overtime[pos].start[0],
                rec_overtime[pos].start[1],
                rec_overtime[pos].start[2],
                rec_overtime[pos].start[3],
                rec_overtime[pos].start[4],
                rec_overtime[pos].start[5]);

        lcd_text12( 0, 0, buf, strlen( buf ), LCD_MODE_SET );

        sprintf(buf, "   %02d-%02d-%02d %02d:%02d:%02d",
                rec_overtime[pos].end[0],
                rec_overtime[pos].end[1],
                rec_overtime[pos].end[2],
                rec_overtime[pos].end[3],
                rec_overtime[pos].end[4],
                rec_overtime[pos].end[5]);

        lcd_text12( 0, 11, buf, strlen( buf ), LCD_MODE_SET );



    }
    lcd_update_all( );
}

/**/
static void pilao_display( void )
{
    uint8_t *p;
    uint8_t len_tel, len_man;
    char	buf[32];
    MYTIME		time;

    lcd_fill( 0 );
    if( count == 0 )
    {
        lcd_fill( 0 );
        lcd_text12( ( 122 - 16 * 6 ) >> 1, 14, "[无疲劳驾驶记录]", 16, LCD_MODE_SET );
    }
    else
    {
        time	= utc_to_mytime(jt808_param_bk.utc_over_time_start);
        sprintf(buf, "[%d]%02d-%02d-%02d %02d:%02d:%02d",
                pos + 1,
                YEAR( time),
                MONTH( time ),
                DAY( time ),
                HOUR( time ),
                MINUTE( time ),
                SEC( time )
               );

        lcd_text12( 0, 0, buf, strlen( buf ), LCD_MODE_SET );
        time	= utc_to_mytime(jt808_param_bk.utc_over_time_end);
        sprintf(buf, "   %02d-%02d-%02d %02d:%02d:%02d",
                YEAR( time),
                MONTH( time ),
                DAY( time ),
                HOUR( time ),
                MINUTE( time ),
                SEC( time )
               );

        lcd_text12( 0, 11, buf, strlen( buf ), LCD_MODE_SET );



    }
    lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
    pMenuItem->tick = rt_tick_get( );
    if((jt808_param_bk.utc_over_time_start != 0) && (jt808_param_bk.utc_over_time_start < jt808_param_bk.utc_over_time_end) && (jt808_param_bk.utc_over_time_end <= utc_now))
    {
        count = 1;
    }
    else
    {
        count = 0;
    }
    //count			= vdr_11_get_1(0,0xFFFFFFFF,8,rec_overtime );
    pos				= 0;
    pilao_display( );
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
        pMenuItem = &Menu_4_recorder;
        pMenuItem->show( );
        break;
    case KEY_OK:

        break;
    case KEY_UP:
        if( pos )
        {
            pos--;
            pilao_display( );
        }
        break;
    case KEY_DOWN:
        if( pos < count - 1 )
        {
            pos++;
            pilao_display( );
        }
        break;
    }
}


MENUITEM Menu_4_1_pilao =
{
    "疲劳驾驶查看",
    12,				  0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};

#else

unsigned char			DisOverSpeedS[25] = { "S: 11/02/12  15:01:02" };
unsigned char			DisOverSpeedE[25] = { "E: 11/02/12  20:21:03" };

static unsigned char	PiLaoNumScreen = 1;

static unsigned char	PiLaoSreen = 1;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void drawPiLao_1( void )
{
    unsigned char	screenNUM	= 0;
    unsigned char	Num[20]		= { "1.    驾驶证号码" };
    unsigned char	Code[19]	= { "                  " }; //

    screenNUM	= PiLaoNumScreen / 2;
    Num[0]		+= screenNUM;

    memcpy( Code, PilaoJilu[screenNUM].PCard, 18 );
    lcd_fill( 0 );
    lcd_text12( 0, 3, ( char *)Num, sizeof( Num ), LCD_MODE_SET );
    lcd_text12( 15, 20, (char *)Code, sizeof( Code ), LCD_MODE_SET );
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
static void drawPiLao_2( )
{
    unsigned char	t_s[35], t_e[35];
    unsigned char	screenNUM = 0;

    screenNUM = PiLaoNumScreen / 2;
    lcd_fill( 0 );
    rt_kprintf( (char *)t_s, "S: %02d/%02d/%02d %02d:%02d:%02d", PilaoJilu[screenNUM - 1].StartTime[0], PilaoJilu[screenNUM - 1].StartTime[1], PilaoJilu[screenNUM - 1].StartTime[2], PilaoJilu[screenNUM - 1].StartTime[3], PilaoJilu[screenNUM - 1].StartTime[4], PilaoJilu[screenNUM - 1].StartTime[5] );
    rt_kprintf( (char *)t_e, "E: %02d/%02d/%02d %02d:%02d:%02d", PilaoJilu[screenNUM - 1].EndTime[0], PilaoJilu[screenNUM - 1].EndTime[1], PilaoJilu[screenNUM - 1].EndTime[2], PilaoJilu[screenNUM - 1].EndTime[3], PilaoJilu[screenNUM - 1].EndTime[4], PilaoJilu[screenNUM - 1].EndTime[5] );
    lcd_text12( 0, 5, (char *)t_s, strlen( (char *)t_s ), LCD_MODE_SET );
    lcd_text12( 0, 19, (char *)t_s, strlen( (char *)t_e ), LCD_MODE_SET );

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
    pMenuItem->tick = rt_tick_get( );

    ErrorRecord				= 0;    //疲劳超速记录错误清0
    StartDisTiredExpspeed	= 0;
    tire_Flag				= 0;    //查看疲劳超速报警记录过程标志清0;

    lcd_fill( 0 );
    lcd_text12( 0, 10, "按确认键查看疲劳记录", 20, LCD_MODE_SET );
    lcd_update_all( );

    //读疲劳驾驶记录
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
    unsigned char	temp		= 0;
    unsigned char	tired_num	= 0;

    switch( key )
    {
    case KEY_MENU:
        CounterBack				= 0;
        ErrorRecord				= 0;    //疲劳超速记录错误清0
        StartDisTiredExpspeed	= 0;
        tire_Flag				= 0;    //查看疲劳报警记录过程标志清0;
        PiLaoSreen				= 1;

        pMenuItem = &Menu_4_InforTirExspd;
        pMenuItem->show( );
        break;
    case KEY_OK:
        if( PiLaoSreen == 1 )
        {
            PiLaoSreen = 2;
#if NEED_TODO
            tired_num = Api_DFdirectory_Query( tired_warn, 0 ); //查询当前疲劳驾驶记录数目
#endif
            if( tired_num > 0 )
            {
                tire_Flag = 1;
                //rt_kprintf("\r\n  已有  疲劳驾驶 的记录 %d 条\r\n",TiredDrv_write);
                if( tired_num >= 3 )
                {
                    ReadPiLao( 3 );
                }
                else
                {
                    ReadPiLao( tired_num );
                }

                Dis_pilao( data_tirexps );
            }
            else
            {
                tire_Flag = 2;
                //rt_kprintf("\r\n无疲劳驾驶记录,取超速驾驶记录\r\n");
            }
        }
        else if( PiLaoSreen == 2 )
        {
            PiLaoSreen = 3;
            if( tire_Flag == 3 )
            {
                tire_Flag				= 4;
                PiLaoNumScreen			= 0;
                StartDisTiredExpspeed	= 1;
                lcd_fill( 0 );
                lcd_text12( 20, 10, "按下翻逐条查看", 14, LCD_MODE_SET );
                lcd_update_all( );
            }
        }

        break;
    case KEY_UP:
        if( tire_Flag == 4 )
        {
            if( PiLaoNumScreen > 0 )
            {
                PiLaoNumScreen--;
            }
            if( PiLaoNumScreen < 1 )
            {
                PiLaoNumScreen = 1;
            }
            if( PiLaoNumScreen % 2 == 1 )
            {
                drawPiLao_1( );     //驾驶证代码
            }
            else
            {
                if( ErrorRecord == 0 )
                {
                    StartDisTiredExpspeed = 0;
                    drawPiLao_2( ); //开始时间  结束时间
                }
            }
        }
        break;
    case KEY_DOWN:
        if( tire_Flag == 4 )
        {
            PiLaoNumScreen++;


            /* if(TiredDrv_write>=3)
               temp=6;
               else
               temp=TiredDrv_write*2;*/
            if( PiLaoNumScreen >= temp )
            {
                PiLaoNumScreen = temp;
            }

            if( PiLaoNumScreen % 2 == 1 )
            {
                drawPiLao_1( );     //驾驶证代码
            }
            else
            {
                if( ErrorRecord == 0 )
                {
                    StartDisTiredExpspeed = 0;
                    drawPiLao_2( ); //开始时间  结束时间
                }
            }
        }
        break;
    }
}

MENUITEM Menu_4_1_pilao =
{
    "疲劳驾驶查看",
    12,				  0,
    &show,
    &keypress,
    &timetick_default,
    &msg,
    (void *)0
};

#endif

/************************************** The End Of File **************************************/
