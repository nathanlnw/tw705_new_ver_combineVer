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
#ifndef SED1520_H_
#define SED1520_H_

// define to disable busy-delays (useful for simulation)
//// #define LCD_SIM (1)
// define to enable additional debugging functions
//// #define LCD_DEBUG (1)

#include <stdint.h>
#include "menu_include.h"

/* draw modes */
#define LCD_MODE_CLEAR	0
#define LCD_MODE_SET	1
#define LCD_MODE_XOR	2
#define LCD_MODE_INVERT 3

/* command function equates for ST7565R LCD Display Controller */
#define LCD_DISP_OFF       0xAE	/* turn LCD panel OFF */
#define LCD_DISP_ON        0xAF	/* turn LCD panel ON */
#define LCD_SET_LINE       0xC0	/* set line for COM0 (4 lsbs = ST3:ST2:ST1:ST0) */
#define LCD_SET_PAGE_ST7565R       0xB0	/* set page address (2 lsbs = P1:P0) */
#define LCD_SET_PAGE_SED1520       0xB8	/* set page address (2 lsbs = P1:P0) */

#define LCD_SET_COLH       0x10 /*  列地址高字节*/
#define LCD_SET_COL        0x00	/* set column address (6 lsbs = Y4:Y4:Y3:Y2:Y1:Y0) */
#define LCD_SET_ADC_NOR    0xA0	/* ADC set for normal direction */
#define LCD_SET_ADC_REV    0xA1	/* ADC set for reverse direction */
#define LCD_STATIC_OFF     0xA6	/* normal drive */
#define LCD_STATIC_ON      0xA7	/* static drive (power save) */
#define LCD_DISPALL_NORMAL 0xA4 /*0:disp all points  normal --new lcd*/
#define LCD_DISPALL_ENABLE 0xA5 /*1:disp all points  ON---new lcd*/
#define LCD_BIAS_1D9       0xA2 /* bias    1/9 bias*/
#define LCD_BIAS_1D7       0xA3 /* bias    1/7 bias*/
#define LCD_OUTPUT_NORMAL  0xC0 /* output  scann  direction   0:  normal direction--new lcd*/
#define LCD_OUTPUT_RVS     0xC8 /* output  scann  direction   1:  reverse direction--new lcd*/
#define LCD_SET_MODIFY     0xE0	/* start read-modify-write mode */
#define LCD_CLR_MODIFY     0xEE	/* end read-modify-write mode */
#define LCD_RESET          0xE2	/* soft reset command */

/* LCD screen and bitmap image array consants */
#define LCD_X_BYTES 122
#define LCD_Y_BYTES 4
#define SCRN_LEFT	0
#define SCRN_TOP	0
#define SCRN_RIGHT	121
#define SCRN_BOTTOM 31

#define LCD_STARTCOL_REVERSE 19

#define Serial74HC_Control(i,bit)			{Control_74HC_1=(u8)i ? (Control_74HC_1|BIT(bit))  : (Control_74HC_1&~(BIT(bit))) ;ControlBitShift(Control_74HC_1);} //Q_bit
#define Serial74HC_Control_NoChange(i,bit)	Control_74HC_1=(u8)i ? (Control_74HC_1|BIT(bit))  : (Control_74HC_1&~(BIT(bit)))	//Q_bit


#define LCD_RST0(i)   	Serial74HC_Control(i,0)
#define LCD_E1(i)   	Serial74HC_Control_NoChange(i,1)
#define LCD_E2(i)   	Serial74HC_Control_NoChange(i,2)
// new lcd
#define LCD_RD(i)   	Serial74HC_Control_NoChange(i,2)
#define LCD_CS(i)   	Serial74HC_Control_NoChange(i,1)



#define LCD_RW(i)   	Serial74HC_Control(i,3)
#define LCD_A0(i)   	Serial74HC_Control(i,4)
#define Printer_3v3(i)  Serial74HC_Control_NoChange(i,5)
#define LCD_BL(i)   	Serial74HC_Control(i,6)
#define LCD_BL_2(i)   	Serial74HC_Control_NoChange(i,6)
#define RELAY_SET(i)   	Serial74HC_Control_NoChange(i,7)


extern uint8_t	Control_74HC_1_Used;	///串并转化芯片1的最后一次有效的输出数据，用于LCD和其他控制相关
extern uint8_t	Control_74HC_1;			///串并转化芯片1输出数据(不一定输出数据)，用于LCD和其他控制相关
extern uint8_t	Control_74HC_2;			///串并转化芯片2输出数据，用于LCD数据


void lcd_init( void );


void lcd_fill( const unsigned char pattern );


void lcd_update( const unsigned char top, const unsigned char bottom );


void lcd_update_all( void );


void lcd_bitmap( const uint8_t left, const uint8_t top, IMG_DEF *img_ptr, const uint8_t mode );


void lcd_text12( char left, char top, const char *pinfo, char len, const char mode );
void lcd_asc0608( char left, char top, char *p, const char mode );




extern void Lcd_hardInit_timer(void);

#endif
/************************************** The End Of File **************************************/
