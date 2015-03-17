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
#include  <stdlib.h>
#include  <stdio.h>
#include  <string.h>

#include "menu_include.h"
#include "LCD_Driver.h"
#include "printer.h"
#include "jt808_util.h"

#include "GB_vdr.h"
#include "jt808_GB19056.h"


static uint32_t lasttick;
unsigned char	dispstat = 0;
unsigned int    reset_firstset = 0; //恢复设置状态

unsigned char	gsm_g[] =
{
    0x1c,                                                                       /*[   ***  ]*/
    0x22,                                                                       /*[  *   * ]*/
    0x40,                                                                       /*[ *      ]*/
    0x40,                                                                       /*[ *      ]*/
    0x4e,                                                                       /*[ *  *** ]*/
    0x42,                                                                       /*[ *    * ]*/
    0x22,                                                                       /*[  *   * ]*/
    0x1e,                                                                       /*[   **** ]*/
};
#if 0
unsigned char	gsm_0[] =
{
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x80,                                                                       /*[*       ]*/
    0x80,                                                                       /*[*       ]*/
};

unsigned char	gsm_1[] =
{
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x20,                                                                       /*[  *     ]*/
    0x20,                                                                       /*[  *     ]*/
    0xa0,                                                                       /*[* *     ]*/
    0xa0,                                                                       /*[* *     ]*/
};

unsigned char	gsm_2[] =
{
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x08,                                                                       /*[    *   ]*/
    0x08,                                                                       /*[    *   ]*/
    0x28,                                                                       /*[  * *   ]*/
    0x28,                                                                       /*[  * *   ]*/
    0xa8,                                                                       /*[* * *   ]*/
    0xa8,                                                                       /*[* * *   ]*/
};

unsigned char	gsm_3[] =
{
    0x02,                                                                       /*[      * ]*/
    0x02,                                                                       /*[      * ]*/
    0x0a,                                                                       /*[    * * ]*/
    0x0a,                                                                       /*[    * * ]*/
    0x2a,                                                                       /*[  * * * ]*/
    0x2a,                                                                       /*[  * * * ]*/
    0xaa,                                                                       /*[* * * * ]*/
    0xaa,                                                                       /*[* * * * ]*/
};
#endif

unsigned char	gsm_0[] =
{
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x80,                                                                       /*[*       ]*/
    0x80,                                                                       /*[*       ]*/
};

unsigned char	gsm_1[] =
{
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x20,                                                                       /*[  *     ]*/
    0x20,                                                                       /*[  *     ]*/
    0xa0,                                                                       /*[* *     ]*/
};

unsigned char	gsm_2[] =
{
    0x00,                                                                       /*[        ]*/
    0x00,                                                                       /*[        ]*/
    0x08,                                                                       /*[        ]*/
    0x08,                                                                       /*[    *   ]*/
    0x08,                                                                       /*[    *   ]*/
    0x28,                                                                       /*[  * *   ]*/
    0x28,                                                                       /*[  * *   ]*/
    0xa8,                                                                       /*[* * *   ]*/
};

unsigned char	gsm_3[] =
{
    0x00,                                                                       /*[        ]*/
    0x02,                                                                       /*[      * ]*/
    0x02,                                                                       /*[      * ]*/
    0x0a,                                                                       /*[    * * ]*/
    0x0a,                                                                       /*[    * * ]*/
    0x2a,                                                                       /*[  * * * ]*/
    0x2a,                                                                       /*[  * * * ]*/
    0xaa,                                                                       /*[* * * * ]*/
};



unsigned char	link_on[] =
{
    0x08,                                                                       /*[    *   ]*/
    0x04,                                                                       /*[     *  ]*/
    0xfe,                                                                       /*[******* ]*/
    0x00,                                                                       /*[        ]*/
    0xfe,                                                                       /*[******* ]*/
    0x40,                                                                       /*[ *      ]*/
    0x20,                                                                       /*[  *     ]*/
    0x00,                                                                       /*[        ]*/
};

unsigned char	link_off[] =
{
    0x10,                                                                       /*[   *    ]*/
    0x08,                                                                       /*[    *   ]*/
    0xc6,                                                                       /*[**   ** ]*/
    0x00,                                                                       /*[        ]*/
    0xe6,                                                                       /*[***  ** ]*/
    0x10,                                                                       /*[   *    ]*/
    0x08,                                                                       /*[    *   ]*/
    0x00,                                                                       /*[        ]*/
};
/*
unsigned char	ANT_GSM[] = {
	0xFE,																		//*[******* ]
	0x92,																		//*[*  *  * ]
	0x54,																		//*[ * * *  ]
	0x38,																		//*[  ***   ]
	0x10,																		//*[   *    ]
	0x10,																		//*[   *    ]
	0x10,																		//*[   *    ]
	0x00,																		//*[        ]
	};
*/
unsigned char	num0[] = { 0x00, 0x70, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00 };    /*"0",0*/ /*"0",0*/
unsigned char	num1[] = { 0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00 };    /*"1",1*/ /*"1",1*/
unsigned char	num2[] = { 0x00, 0x78, 0x88, 0x08, 0x30, 0x40, 0xF8, 0x00 };    /*"2",2*/ /*"2",2*/
unsigned char	num3[] = { 0x00, 0xF8, 0x88, 0x30, 0x08, 0x88, 0x70, 0x00 };    /*"3",3*/ /*"3",3*/
unsigned char	num4[] = { 0x00, 0x10, 0x30, 0x50, 0x90, 0x78, 0x10, 0x00 };    /*"4",4*/ /*"4",4*/
unsigned char	num5[] = { 0x00, 0xF8, 0x80, 0xF0, 0x08, 0x88, 0x70, 0x00 };    /*"5",5*/ /*"5",5*/
unsigned char	num6[] = { 0x00, 0x70, 0x80, 0xF0, 0x88, 0x88, 0x70, 0x00 };    /*"6",6*/ /*"6",6*/
unsigned char	num7[] = { 0x00, 0xF8, 0x90, 0x10, 0x20, 0x20, 0x20, 0x00 };    /*"7",7*/ /*"7",7*/
unsigned char	num8[] = { 0x00, 0x70, 0x88, 0x70, 0x88, 0x88, 0x70, 0x00 };    /*"8",8*/ /*"8",8*/
unsigned char	num9[] = { 0x00, 0x70, 0x88, 0x88, 0x78, 0x08, 0x70, 0x00 };    /*"9",9*/ /*"9",9*/

//DECL_BMP(6,8,num0);

IMG_DEF img_num0608[10] =
{
    { 6, 8, num0 },
    { 6, 8, num1 },
    { 6, 8, num2 },
    { 6, 8, num3 },
    { 6, 8, num4 },
    { 6, 8, num5 },
    { 6, 8, num6 },
    { 6, 8, num7 },
    { 6, 8, num8 },
    { 6, 8, num9 },
};

static unsigned char	Battery[] = { 0x00, 0xFC, 0xFF, 0xFF, 0xFC, 0x00 };     //8*6
static unsigned char	NOBattery[] = { 0x04, 0x0C, 0x98, 0xB0, 0xE0, 0xF8 };   //6*6
static unsigned char	TriangleS[] = { 0x30, 0x78, 0xFC, 0xFC, 0x78, 0x30 };   //6*6
static unsigned char	TriangleK[] = { 0x30, 0x48, 0x84, 0x84, 0x48, 0x30 };   //6*6

static unsigned char	empty[] = { 0x84, 0x84, 0x84, 0x84, 0x84, 0xFC };       /*空车*/
static unsigned char	full_0[] = { 0x84, 0x84, 0x84, 0xFC, 0xFC, 0xFC };      /*半满*/
static unsigned char	full_1[] = { 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC };      /*重车*/

static unsigned char	arrow_0[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00,
    0x01, 0xF0, 0x00, 0x00, 0x7F, 0x83, 0xF8, 0x3F, 0xC0, 0x41, 0x07, 0xFC, 0x10, 0x40, 0x42, 0x0F,
    0xFE, 0x08, 0x40, 0x44, 0x00, 0x00, 0x04, 0x40, 0x49, 0x55, 0x55, 0x52, 0x40, 0x50, 0x00, 0x00,
    0x01, 0x40, 0x61, 0x00, 0x00, 0x10, 0xC0, 0x44, 0x00, 0x00, 0x04, 0x40, 0x0D, 0x00, 0x00, 0x16,
    0x00, 0x14, 0x00, 0x00, 0x05, 0x00, 0x25, 0x00, 0x00, 0x14, 0x80, 0x44, 0x00, 0x00, 0x04, 0x40,
    0x85, 0x00, 0x00, 0x14, 0x20, 0x44, 0x00, 0x00, 0x04, 0x40, 0x25, 0x00, 0x00, 0x14, 0x80, 0x14,
    0x00, 0x00, 0x05, 0x00, 0x0D, 0x00, 0x00, 0x16, 0x00, 0x44, 0x00, 0x00, 0x04, 0x40, 0x61, 0x55,
    0x55, 0x50, 0xC0, 0x50, 0x00, 0x00, 0x01, 0x40, 0x48, 0x0F, 0xFE, 0x02, 0x40, 0x44, 0x04, 0x04,
    0x04, 0x40, 0x42, 0x02, 0x08, 0x08, 0x40, 0x41, 0x01, 0x10, 0x10, 0x40, 0x7F, 0x80, 0xA0, 0x3F,
    0xC0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

unsigned char			arrow_45[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00,
    0x01, 0x10, 0x00, 0x00, 0x7F, 0x82, 0x08, 0x3F, 0xC0, 0x41, 0x04, 0x04, 0x1F, 0xC0, 0x42, 0x0F,
    0xFE, 0x0F, 0xC0, 0x44, 0x00, 0x00, 0x07, 0xC0, 0x49, 0x55, 0x55, 0x53, 0xC0, 0x50, 0x00, 0x00,
    0x01, 0xC0, 0x61, 0x00, 0x00, 0x10, 0xC0, 0x44, 0x00, 0x00, 0x04, 0x40, 0x0D, 0x00, 0x00, 0x16,
    0x00, 0x14, 0x00, 0x00, 0x05, 0x00, 0x25, 0x00, 0x00, 0x14, 0x80, 0x44, 0x00, 0x00, 0x04, 0x40,
    0x85, 0x00, 0x00, 0x14, 0x20, 0x44, 0x00, 0x00, 0x04, 0x40, 0x25, 0x00, 0x00, 0x14, 0x80, 0x14,
    0x00, 0x00, 0x05, 0x00, 0x0D, 0x00, 0x00, 0x16, 0x00, 0x44, 0x00, 0x00, 0x04, 0x40, 0x61, 0x55,
    0x55, 0x50, 0xC0, 0x50, 0x00, 0x00, 0x01, 0x40, 0x48, 0x0F, 0xFE, 0x02, 0x40, 0x44, 0x04, 0x04,
    0x04, 0x40, 0x42, 0x02, 0x08, 0x08, 0x40, 0x41, 0x01, 0x10, 0x10, 0x40, 0x7F, 0x80, 0xA0, 0x3F,
    0xC0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char			arrow_90[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00,
    0x01, 0x10, 0x00, 0x00, 0x7F, 0x82, 0x08, 0x3F, 0xC0, 0x41, 0x04, 0x04, 0x10, 0x40, 0x42, 0x0F,
    0xFE, 0x08, 0x40, 0x44, 0x00, 0x00, 0x04, 0x40, 0x49, 0x55, 0x55, 0x52, 0x40, 0x50, 0x00, 0x00,
    0x01, 0x40, 0x61, 0x00, 0x00, 0x10, 0xC0, 0x44, 0x00, 0x00, 0x04, 0x40, 0x0D, 0x00, 0x00, 0x16,
    0x00, 0x14, 0x00, 0x00, 0x07, 0x00, 0x25, 0x00, 0x00, 0x17, 0x80, 0x44, 0x00, 0x00, 0x07, 0xC0,
    0x85, 0x00, 0x00, 0x17, 0xE0, 0x44, 0x00, 0x00, 0x07, 0xC0, 0x25, 0x00, 0x00, 0x17, 0x80, 0x14,
    0x00, 0x00, 0x07, 0x00, 0x0D, 0x00, 0x00, 0x16, 0x00, 0x44, 0x00, 0x00, 0x04, 0x40, 0x61, 0x55,
    0x55, 0x50, 0xC0, 0x50, 0x00, 0x00, 0x01, 0x40, 0x48, 0x0F, 0xFE, 0x02, 0x40, 0x44, 0x04, 0x04,
    0x04, 0x40, 0x42, 0x02, 0x08, 0x08, 0x40, 0x41, 0x01, 0x10, 0x10, 0x40, 0x7F, 0x80, 0xA0, 0x3F,
    0xC0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

unsigned char			arrow_135[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00,
    0x01, 0x10, 0x00, 0x00, 0x7F, 0x82, 0x08, 0x3F, 0xC0, 0x41, 0x04, 0x04, 0x10, 0x40, 0x42, 0x0F,
    0xFE, 0x08, 0x40, 0x44, 0x00, 0x00, 0x04, 0x40, 0x49, 0x55, 0x55, 0x52, 0x40, 0x50, 0x00, 0x00,
    0x01, 0x40, 0x61, 0x00, 0x00, 0x10, 0xC0, 0x44, 0x00, 0x00, 0x04, 0x40, 0x0D, 0x00, 0x00, 0x16,
    0x00, 0x14, 0x00, 0x00, 0x05, 0x00, 0x25, 0x00, 0x00, 0x14, 0x80, 0x44, 0x00, 0x00, 0x04, 0x40,
    0x85, 0x00, 0x00, 0x14, 0x20, 0x44, 0x00, 0x00, 0x04, 0x40, 0x25, 0x00, 0x00, 0x14, 0x80, 0x14,
    0x00, 0x00, 0x05, 0x00, 0x0D, 0x00, 0x00, 0x16, 0x00, 0x44, 0x00, 0x00, 0x04, 0x40, 0x61, 0x55,
    0x55, 0x50, 0xC0, 0x50, 0x00, 0x00, 0x01, 0xC0, 0x48, 0x0F, 0xFE, 0x03, 0xC0, 0x44, 0x04, 0x04,
    0x07, 0xC0, 0x42, 0x02, 0x08, 0x0F, 0xC0, 0x41, 0x01, 0x10, 0x1F, 0xC0, 0x7F, 0x80, 0xA0, 0x3F,
    0xC0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char			arrow_180[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00,
    0x01, 0x10, 0x00, 0x00, 0x7F, 0x82, 0x08, 0x3F, 0xC0, 0x41, 0x04, 0x04, 0x10, 0x40, 0x42, 0x0F,
    0xFE, 0x08, 0x40, 0x44, 0x00, 0x00, 0x04, 0x40, 0x49, 0x55, 0x55, 0x52, 0x40, 0x50, 0x00, 0x00,
    0x01, 0x40, 0x61, 0x00, 0x00, 0x10, 0xC0, 0x44, 0x00, 0x00, 0x04, 0x40, 0x0D, 0x00, 0x00, 0x16,
    0x00, 0x14, 0x00, 0x00, 0x05, 0x00, 0x25, 0x00, 0x00, 0x14, 0x80, 0x44, 0x00, 0x00, 0x04, 0x40,
    0x85, 0x00, 0x00, 0x14, 0x20, 0x44, 0x00, 0x00, 0x04, 0x40, 0x25, 0x00, 0x00, 0x14, 0x80, 0x14,
    0x00, 0x00, 0x05, 0x00, 0x0D, 0x00, 0x00, 0x16, 0x00, 0x44, 0x00, 0x00, 0x04, 0x40, 0x61, 0x55,
    0x55, 0x50, 0xC0, 0x50, 0x00, 0x00, 0x01, 0x40, 0x48, 0x0F, 0xFE, 0x02, 0x40, 0x44, 0x07, 0xFC,
    0x04, 0x40, 0x42, 0x03, 0xF8, 0x08, 0x40, 0x41, 0x01, 0xF0, 0x10, 0x40, 0x7F, 0x80, 0xE0, 0x3F,
    0xC0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

unsigned char			arrow_225[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00,
    0x01, 0x10, 0x00, 0x00, 0x7F, 0x82, 0x08, 0x3F, 0xC0, 0x41, 0x04, 0x04, 0x10, 0x40, 0x42, 0x0F,
    0xFE, 0x08, 0x40, 0x44, 0x00, 0x00, 0x04, 0x40, 0x49, 0x55, 0x55, 0x52, 0x40, 0x50, 0x00, 0x00,
    0x01, 0x40, 0x61, 0x00, 0x00, 0x10, 0xC0, 0x44, 0x00, 0x00, 0x04, 0x40, 0x0D, 0x00, 0x00, 0x16,
    0x00, 0x14, 0x00, 0x00, 0x05, 0x00, 0x25, 0x00, 0x00, 0x14, 0x80, 0x44, 0x00, 0x00, 0x04, 0x40,
    0x85, 0x00, 0x00, 0x14, 0x20, 0x44, 0x00, 0x00, 0x04, 0x40, 0x25, 0x00, 0x00, 0x14, 0x80, 0x14,
    0x00, 0x00, 0x05, 0x00, 0x0D, 0x00, 0x00, 0x16, 0x00, 0x44, 0x00, 0x00, 0x04, 0x40, 0x61, 0x55,
    0x55, 0x50, 0xC0, 0x70, 0x00, 0x00, 0x01, 0x40, 0x78, 0x0F, 0xFE, 0x02, 0x40, 0x7C, 0x04, 0x04,
    0x04, 0x40, 0x7E, 0x02, 0x08, 0x08, 0x40, 0x7F, 0x01, 0x10, 0x10, 0x40, 0x7F, 0x80, 0xA0, 0x3F,
    0xC0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

unsigned char			arrow_270[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00,
    0x01, 0x10, 0x00, 0x00, 0x7F, 0x82, 0x08, 0x3F, 0xC0, 0x41, 0x04, 0x04, 0x10, 0x40, 0x42, 0x0F,
    0xFE, 0x08, 0x40, 0x44, 0x00, 0x00, 0x04, 0x40, 0x49, 0x55, 0x55, 0x52, 0x40, 0x50, 0x00, 0x00,
    0x01, 0x40, 0x61, 0x00, 0x00, 0x10, 0xC0, 0x44, 0x00, 0x00, 0x04, 0x40, 0x0D, 0x00, 0x00, 0x16,
    0x00, 0x1C, 0x00, 0x00, 0x05, 0x00, 0x3D, 0x00, 0x00, 0x14, 0x80, 0x7C, 0x00, 0x00, 0x04, 0x40,
    0xFD, 0x00, 0x00, 0x14, 0x20, 0x7C, 0x00, 0x00, 0x04, 0x40, 0x3D, 0x00, 0x00, 0x14, 0x80, 0x1C,
    0x00, 0x00, 0x05, 0x00, 0x0D, 0x00, 0x00, 0x16, 0x00, 0x44, 0x00, 0x00, 0x04, 0x40, 0x61, 0x55,
    0x55, 0x50, 0xC0, 0x50, 0x00, 0x00, 0x01, 0x40, 0x48, 0x0F, 0xFE, 0x02, 0x40, 0x44, 0x04, 0x04,
    0x04, 0x40, 0x42, 0x02, 0x08, 0x08, 0x40, 0x41, 0x01, 0x10, 0x10, 0x40, 0x7F, 0x80, 0xA0, 0x3F,
    0xC0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char			arrow_315[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00,
    0x01, 0x10, 0x00, 0x00, 0x7F, 0x82, 0x08, 0x3F, 0xC0, 0x7F, 0x04, 0x04, 0x10, 0x40, 0x7E, 0x0F,
    0xFE, 0x08, 0x40, 0x7C, 0x00, 0x00, 0x04, 0x40, 0x79, 0x55, 0x55, 0x52, 0x40, 0x70, 0x00, 0x00,
    0x01, 0x40, 0x61, 0x00, 0x00, 0x10, 0xC0, 0x44, 0x00, 0x00, 0x04, 0x40, 0x0D, 0x00, 0x00, 0x16,
    0x00, 0x14, 0x00, 0x00, 0x05, 0x00, 0x25, 0x00, 0x00, 0x14, 0x80, 0x44, 0x00, 0x00, 0x04, 0x40,
    0x85, 0x00, 0x00, 0x14, 0x20, 0x44, 0x00, 0x00, 0x04, 0x40, 0x25, 0x00, 0x00, 0x14, 0x80, 0x14,
    0x00, 0x00, 0x05, 0x00, 0x0D, 0x00, 0x00, 0x16, 0x00, 0x44, 0x00, 0x00, 0x04, 0x40, 0x61, 0x55,
    0x55, 0x50, 0xC0, 0x50, 0x00, 0x00, 0x01, 0x40, 0x48, 0x0F, 0xFE, 0x02, 0x40, 0x44, 0x04, 0x04,
    0x04, 0x40, 0x42, 0x02, 0x08, 0x08, 0x40, 0x41, 0x01, 0x10, 0x10, 0x40, 0x7F, 0x80, 0xA0, 0x3F,
    0xC0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

IMG_DEF					img_arrow[8] =
{
    { 35, 32, arrow_0	},
    { 35, 32, arrow_45	},
    { 35, 32, arrow_90	},
    { 35, 32, arrow_135 },
    { 35, 32, arrow_180 },
    { 35, 32, arrow_225 },
    { 35, 32, arrow_270 },
    { 35, 32, arrow_315 },
};

/*0度周围的角度 337-359 0-23作为缺省情况处理*/
static uint8_t check_arrow_index( uint16_t cog )
{
    uint8_t		i;
    uint16_t	arrow_scope[8] = { 23, 68, 113, 158, 203, 248, 293, 337 };
    for( i = 1; i < 8; i++ )
    {
        if( ( cog > arrow_scope[i - 1] ) && ( cog <= arrow_scope[i] ) )
        {
            return i;
        }
    }
    return 0;
}

//电池 是否校验特征系数的标志
DECL_BMP( 8, 6, Battery );
DECL_BMP( 6, 6, NOBattery );
DECL_BMP( 6, 6, TriangleS );
DECL_BMP( 6, 6, TriangleK );
//信号强度标志
DECL_BMP( 7, 8, gsm_g );
DECL_BMP( 7, 8, gsm_0 );
DECL_BMP( 7, 8, gsm_1 );
DECL_BMP( 7, 8, gsm_2 );
DECL_BMP( 7, 8, gsm_3 );
//连接或者在线标志
DECL_BMP( 7, 8, link_on );
DECL_BMP( 7, 8, link_off );
//空车 半满 重车
DECL_BMP( 6, 6, empty );
DECL_BMP( 6, 6, full_0 );
DECL_BMP( 6, 6, full_1 );

extern uint8_t modem_no_sim;


void lcd_text0608( char left, char top, const char *pinfo, char len, const char mode )
{
    lcd_asc0608(left, top, (char *)pinfo, mode);
}



/*
   这里显示gps时间，若gps未定位，时间停走
   可选择
   rtc时间，需要频繁读取(1秒1次)，如何判断校时
   使用自己的mytime，如何更新(使用系统的1s定时器是否准确)
 */

void Disp_Idle( void )
{
    char	*mode[] = { "  ", "BD", "GP", "GN" };
    char	buf_date[22];
    char	buf_time[22];
    char	buf_speed[20];
    uint8_t lcd_mode;

    lcd_fill( 0 );

    sprintf( buf_date, "%02d-%02d-%02d",
             YEAR( mytime_now ),
             MONTH( mytime_now ),
             DAY( mytime_now ) );

    sprintf( buf_time, "%02d:%02d:%02d",
             HOUR( mytime_now ),
             MINUTE( mytime_now ),
             SEC( mytime_now ) );
    if( jt808_param_bk.car_alarm & ( BIT_ALARM_GPS_ERR | BIT_ALARM_GPS_OPEN | BIT_ALARM_GPS_SHORT ) )    /*北斗异常，不显示*/
    {
        lcd_text0608( 0, 1, mode[0], 2, LCD_MODE_SET );
        //sprintf( buf_date, "--/--/--" );
        //sprintf( buf_time, "--:--:--" );
    }
    else                                                                                    /*定位正常显示*/
    {
        if( jt808_status & BIT_STATUS_FIXED )
        {
            lcd_text0608( 0, 1, mode[gps_status.mode], 2, LCD_MODE_SET );
        }
        else
        {
            lcd_text0608( 0, 1, mode[gps_status.mode], 2, LCD_MODE_INVERT );
        }
        lcd_bitmap( 12, 2, &img_num0608[gps_status.NoSV / 10], LCD_MODE_SET );
        lcd_bitmap( 18, 2, &img_num0608[gps_status.NoSV % 10], LCD_MODE_SET );

        lcd_bitmap( 86, 0, &img_arrow[check_arrow_index( gps_cog )], LCD_MODE_SET );
        sprintf( buf_speed, "%03d", current_speed );
        lcd_text0608( 95, 12, (char *)buf_speed, strlen( buf_speed ), LCD_MODE_SET );
    }

    lcd_text0608( 0, 11, (char *)buf_date, strlen( buf_date ), LCD_MODE_SET );
    lcd_text0608( 0, 21, (char *)buf_time, strlen( buf_time ), LCD_MODE_SET );


    if( modem_no_sim )
    {
        lcd_mode = LCD_MODE_INVERT;
    }
    else
    {
        lcd_mode = LCD_MODE_SET;
    }

    if( mast_socket->state == CONNECTED ) /*gprs连接状态*/
    {
        lcd_bitmap( 31, 1, &BMP_link_on, LCD_MODE_SET );
    }
    else
    {
        lcd_bitmap( 31, 1, &BMP_link_off, LCD_MODE_SET );
    }

    //lcd_bitmap( 30, 2, &BMP_gsm_g, lcd_mode );
    //lcd_text12( 39, 0, "G", 1, lcd_mode );
    lcd_text0608( 39, 1, "G", 1, lcd_mode );

    if((gsm_param.csq < 99) && (gsm_param.csq > 8))
    {
        if(gsm_param.csq < 16)
        {
            lcd_bitmap( 46, 0, &BMP_gsm_1, LCD_MODE_SET );
        }
        else if(gsm_param.csq < 24)
        {
            lcd_bitmap( 46, 0, &BMP_gsm_2, LCD_MODE_SET );
        }
        else
        {
            lcd_bitmap( 46, 0, &BMP_gsm_3, LCD_MODE_SET );
        }
    }
    else
    {
        lcd_bitmap( 46, 0, &BMP_gsm_0, LCD_MODE_SET );
    }
    //lcd_bitmap( 38, 1, &BMP_gsm_3, LCD_MODE_SET );


    //lcd_text12( 72, 1, "GPRS", 4, lcd_mode );


    if( jt808_param_bk.car_alarm & BIT_ALARM_LOST_PWR )
    {
        lcd_bitmap( 60, 1, &BMP_Battery, LCD_MODE_SET );
    }
    else
    {
        lcd_bitmap( 60, 1, &BMP_NOBattery, LCD_MODE_SET );
    }

#if 0
    if( JT808Conf_struct.LOAD_STATE == 1 ) //车辆载重标志
    {
        lcd_bitmap( 95, 2, &BMP_empty, LCD_MODE_SET );
    }
    else if( JT808Conf_struct.LOAD_STATE == 2 )
    {
        lcd_bitmap( 95, 2, &BMP_full_0, LCD_MODE_SET );
    }
    else if( JT808Conf_struct.LOAD_STATE == 3 )
    {
        lcd_bitmap( 95, 2, &BMP_full_1, LCD_MODE_SET );
    }

    if( ModuleStatus & 0x04 ) //电源标志
    {
        lcd_bitmap( 105, 2, &BMP_Battery, LCD_MODE_SET );
    }
    else
    {
        lcd_bitmap( 105, 2, &BMP_NOBattery, LCD_MODE_SET );
    }

    if( DF_K_adjustState ) //是否校验特征系数
    {
        lcd_bitmap( 115, 2, &BMP_TriangleS, LCD_MODE_SET );
    }
    else
    {
        lcd_bitmap( 115, 2, &BMP_TriangleK, LCD_MODE_SET );
    }
#endif

    lcd_update_all( );
}

/*
   这里显示gps时间，若gps未定位，时间停走
   可选择
   rtc时间，需要频繁读取(1秒1次)，如何判断校时
   使用自己的mytime，如何更新(使用系统的1s定时器是否准确)
 */

void Disp_Idle_old( void )
{
    char	*mode[] = { "  ", "BD", "GP", "GN" };
    char	buf_date[22];
    char	buf_time[22];
    char	buf_speed[20];
    uint8_t lcd_mode;

    lcd_fill( 0 );

    sprintf( buf_date, "%02d-%02d-%02d",
             YEAR( mytime_now ),
             MONTH( mytime_now ),
             DAY( mytime_now ) );

    sprintf( buf_time, "%02d:%02d:%02d",
             HOUR( mytime_now ),
             MINUTE( mytime_now ),
             SEC( mytime_now ) );
    if( jt808_param_bk.car_alarm & ( BIT_ALARM_GPS_ERR | BIT_ALARM_GPS_OPEN | BIT_ALARM_GPS_SHORT ) )    /*北斗异常，不显示*/
    {
        lcd_text12( 0, 0, mode[0], 2, LCD_MODE_SET );
        //sprintf( buf_date, "--/--/--" );
        //sprintf( buf_time, "--:--:--" );
    }
    else                                                                                    /*定位正常显示*/
    {
        if( jt808_status & BIT_STATUS_FIXED )
        {
            lcd_text12( 0, 0, mode[gps_status.mode], 2, LCD_MODE_SET );
        }
        else
        {
            lcd_text12( 0, 0, mode[gps_status.mode], 2, LCD_MODE_INVERT );
        }
        lcd_bitmap( 12, 1, &img_num0608[gps_status.NoSV / 10], LCD_MODE_SET );
        lcd_bitmap( 18, 1, &img_num0608[gps_status.NoSV % 10], LCD_MODE_SET );

        lcd_bitmap( 86, 0, &img_arrow[check_arrow_index( gps_cog )], LCD_MODE_SET );
        sprintf( buf_speed, "%03d", current_speed );
        lcd_text12( 95, 9, (char *)buf_speed, strlen( buf_speed ), LCD_MODE_SET );
    }

    lcd_text12( 0, 10, (char *)buf_date, strlen( buf_date ), LCD_MODE_SET );
    lcd_text12( 0, 21, (char *)buf_time, strlen( buf_time ), LCD_MODE_SET );


    //lcd_bitmap( 30, 2, &BMP_gsm_g, lcd_mode );
    lcd_text12( 31, 0, "G", 1, LCD_MODE_SET );

    if((gsm_param.csq < 99) && (gsm_param.csq > 8))
    {
        if(gsm_param.csq < 16)
        {
            lcd_bitmap( 38, 2, &BMP_gsm_1, LCD_MODE_SET );
        }
        else if(gsm_param.csq < 24)
        {
            lcd_bitmap( 38, 2, &BMP_gsm_2, LCD_MODE_SET );
        }
        else
        {
            lcd_bitmap( 38, 2, &BMP_gsm_3, LCD_MODE_SET );
        }
    }
    else
    {
        lcd_bitmap( 38, 2, &BMP_gsm_0, LCD_MODE_SET );
    }
    //lcd_bitmap( 38, 2, &BMP_gsm_3, LCD_MODE_SET );

    if( modem_no_sim )
    {
        lcd_mode = LCD_MODE_INVERT;
    }
    else
    {
        lcd_mode = LCD_MODE_SET;
    }

    lcd_text12( 48, 0, "GPRS", 4, lcd_mode );

    if( mast_socket->state == CONNECTED ) /*gprs连接状态*/
    {
        lcd_bitmap( 72, 2, &BMP_link_on, LCD_MODE_SET );
    }
    else
    {
        lcd_bitmap( 72, 2, &BMP_link_off, LCD_MODE_SET );
    }

#if 0
    if( JT808Conf_struct.LOAD_STATE == 1 ) //车辆载重标志
    {
        lcd_bitmap( 95, 2, &BMP_empty, LCD_MODE_SET );
    }
    else if( JT808Conf_struct.LOAD_STATE == 2 )
    {
        lcd_bitmap( 95, 2, &BMP_full_0, LCD_MODE_SET );
    }
    else if( JT808Conf_struct.LOAD_STATE == 3 )
    {
        lcd_bitmap( 95, 2, &BMP_full_1, LCD_MODE_SET );
    }

    if( ModuleStatus & 0x04 ) //电源标志
    {
        lcd_bitmap( 105, 2, &BMP_Battery, LCD_MODE_SET );
    }
    else
    {
        lcd_bitmap( 105, 2, &BMP_NOBattery, LCD_MODE_SET );
    }

    if( DF_K_adjustState ) //是否校验特征系数
    {
        lcd_bitmap( 115, 2, &BMP_TriangleS, LCD_MODE_SET );
    }
    else
    {
        lcd_bitmap( 115, 2, &BMP_TriangleK, LCD_MODE_SET );
    }
#endif

    lcd_update_all( );
}



/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
    reset_firstset = 0;
    Disp_Idle( );
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

#define  	enable 				1
#define  	disable    			0
#define  	other        		2
#define  	transfer    		3

    uint8_t		drive_illegalstr[120];
    u8 			datatime_str[6];
    u32  		current_u32time = 0; //  当前的时间
    u32  		old2day_u32time = 0; //  前2个日历天的时间    current-2x86400 (172800)
    u32  		read_u32time = 0;
    u32  		read_u32_ENDTime = 0; // 读取记录中结束时间
    u8			efftive_counter = 0, check_limit = 0; // check_limit	 表示需要检索记录的数目
    u8			leftminute = 0; // 当前分钟数值
    u8			find_chaoshi_record = 0;	//

    char		buf[128];
    uint8_t   	i = 0, pos, hour, minute;
    uint32_t  	data_u32;
    MYTIME		time;


    switch( key )
    {
    case KEY_MENU:
        menu_first_in = 0xFF;
        //pMenuItem = &Menu_2_InforCheck;
        pMenuItem = &Menu_1_menu;
        pMenuItem->show( );
        reset_firstset = 0;
        break;
    case KEY_UP:
        if(reset_firstset == 0)
            reset_firstset = 1;
        else if(reset_firstset == 3)
            reset_firstset = 4;
        else if(reset_firstset == 4)
            reset_firstset = 5;
        else    // add later
            reset_firstset = 0;
        break;
    case KEY_DOWN:
        if(reset_firstset == 1)
            reset_firstset = 2;
        else if(reset_firstset == 2)
            reset_firstset = 3;
        else if(reset_firstset == 5)
            reset_firstset = 6;
        else    // add later
            reset_firstset = 0;
        break;
    case KEY_OK_REPEAT:
        reset_firstset = 0;
        //打印开电
        //GPIO_SetBits( GPIOB, GPIO_Pin_6 );
        if(print_state == 0)
        {
            print_state = 1;
            sprintf( buf, "机动车号牌号码:%s\n", jt808_param.id_0x0083 );
            printer( buf );
            sprintf( buf, "机动车号牌分类:%s\n", jt808_param.id_0xF00A );
            printer( buf );
            sprintf( buf, "车辆VIN:%s\n", jt808_param.id_0xF005 );
            printer( buf );
            sprintf( buf, "驾驶员姓名:%s\n", jt808_param.id_0xF008 );
            printer( buf );
            sprintf( buf, "机动车驾驶证号码:\r\n    %s", jt808_param.id_0xF009 );
            printer( buf );
            if((VdrData.H_15[0] == 0x02) && (Limit_max_SateFlag == 0))
                printer("\r\n速度状态: 异常\r\n");
            else
                printer("\r\n速度状态: 正常\r\n");
            memset( buf, 64, 0 );
            sprintf( buf, "打印时间:20%02d-%02d-%02d %02d:%02d:%02d\n",
                     YEAR( mytime_now ),
                     MONTH( mytime_now ),
                     DAY( mytime_now ),
                     HOUR( mytime_now ),
                     MINUTE( mytime_now ),
                     SEC( mytime_now )
                   );
            printer( buf );
            printer("2个日历天内超时驾驶记录:\r\n");
            mytime_to_bcd(datatime_str, mytime_now);
            current_u32time = Time_sec_u32(datatime_str, 6);
            old2day_u32time = current_u32time - 172800; // 2个日历天内的时间


            if(Vdr_Wr_Rd_Offset.V_11H_full)
                check_limit = VDR_11_MAXindex;
            else
                check_limit = Vdr_Wr_Rd_Offset.V_11H_Write;

            // gb_work(0);  // debug
            //rt_kprintf("\r\n -----------current:%d  old2day_u32time:%d check_limit:%d  \r\n",current_u32time,old2day_u32time,check_limit);

            DF_Sem_Take;
            if(check_limit)
            {
                for(i = 0; i < check_limit; i++)
                {
                    memset(drive_illegalstr, 0, sizeof(drive_illegalstr));
                    if(gb_vdr_get_11h(i, drive_illegalstr) == 0) 						//50  packetsize	  num=100
                    {
                        // rt_kprintf("\r\n -------- continue");
                        continue;
                    }
                    read_u32time = Time_sec_u32(drive_illegalstr + 18, 6);	// 连续驾驶起始时间
                    read_u32_ENDTime = Time_sec_u32(drive_illegalstr + 24, 6); // 连续驾驶结束时间

                    //rt_kprintf("\r\n -----------start_time:%d	endtime:%d  i:%d  \r\n",read_u32time,read_u32_ENDTime,i);
                    if(read_u32time >= old2day_u32time)
                    {
                        // if((read_u32_ENDTime>read_u32time)&&((read_u32_ENDTime-read_u32time)>(4*60*60)))
                        if(read_u32_ENDTime > read_u32time)
                        {
                            //  结束时间大于起始时间，且差值大于4个小时
                            efftive_counter++;
                            memset(buf, 0, sizeof(buf));
                            sprintf(buf, "\r\n记录 %d:", efftive_counter);
                            printer(buf);
                            memset( buf, 64, 0 );
                            sprintf( buf, "机动车驾驶证号码:\r\n    %s\n", jt808_param.id_0xF009 );
                            memset(buf, 0, sizeof(buf));
                            sprintf(buf, "\r\n 连续驾驶开始时间: \r\n  20%2d-%d%d-%d%d %d%d:%d%d:%d%d", BCD2HEX(drive_illegalstr[18]), \
                                    BCD2HEX(drive_illegalstr[19]) / 10, BCD2HEX(drive_illegalstr[19]) % 10, BCD2HEX(drive_illegalstr[20]) / 10, BCD2HEX(drive_illegalstr[20]) % 10, \
                                    BCD2HEX(drive_illegalstr[21]) / 10, BCD2HEX(drive_illegalstr[21]) % 10, BCD2HEX(drive_illegalstr[22]) / 10, BCD2HEX(drive_illegalstr[22]) % 10, \
                                    BCD2HEX(drive_illegalstr[23]) / 10, BCD2HEX(drive_illegalstr[23]) % 10);
                            printer(buf);
                            memset(buf, 0, sizeof(buf));
                            sprintf(buf, "\r\n 连续驾驶结束时间: \r\n  20%2d-%d%d-%d%d %d%d:%d%d:%d%d", BCD2HEX(drive_illegalstr[24]), \
                                    BCD2HEX(drive_illegalstr[25]) / 10, BCD2HEX(drive_illegalstr[25]) % 10, BCD2HEX(drive_illegalstr[26]) / 10, BCD2HEX(drive_illegalstr[26]) % 10, \
                                    BCD2HEX(drive_illegalstr[27]) / 10, BCD2HEX(drive_illegalstr[27]) % 10, BCD2HEX(drive_illegalstr[28]) / 10, BCD2HEX(drive_illegalstr[28]) % 10, \
                                    BCD2HEX(drive_illegalstr[29]) / 10, BCD2HEX(drive_illegalstr[29]) % 10);
                            printer(buf);
                            find_chaoshi_record = enable; // find  record
                        }

                    }
                }

                if(find_chaoshi_record == 0)
                    printer("\r\n无超时驾驶记录1\r\n");
            }
            else
                printer("\r\n无超时驾驶记录2\r\n");
            // 最近15分钟 平均速度
            printer("\r\n最近一次停车前15分钟平均速度:");
            memset(drive_illegalstr, 0, sizeof(drive_illegalstr));
            leftminute = Api_avrg15minSpd_Content_read(drive_illegalstr);
            if(leftminute == 0)
                printer("\r\n 暂无最近15分钟停车记录");
            else if(leftminute == 105)
            {
                for(i = 0; i < 15; i++)
                {
                    memset(buf, 0, sizeof(buf));
                    sprintf(buf, "\r\n  20%2d-%d%d-%d%d %d%d:%d%d  %d km/h", drive_illegalstr[i * 7], \
                            drive_illegalstr[i * 7 + 1] / 10, drive_illegalstr[i * 7 + 1] % 10, drive_illegalstr[i * 7 + 2] / 10, drive_illegalstr[i * 7 + 2] % 10, \
                            drive_illegalstr[i * 7 + 3] / 10, drive_illegalstr[i * 7 + 3] % 10, drive_illegalstr[i * 7 + 4] / 10, drive_illegalstr[i * 7 + 4] % 10, \
                            drive_illegalstr[i * 7 + 6]);
                    printer(buf);
                }

            }
            DF_Sem_Release;
            printer("\r\n\r\n 签名 :________________\r\n");
            printer( "\n\n\n\n\n\n\n" );
#if 0
            print_state = 1;
            sprintf( buf, "车牌号码:%s\n", jt808_param.id_0x0083 );
            printer( buf );
            sprintf( buf, "车牌分类:%s\n", jt808_param.id_0xF00A );
            printer( buf );
            sprintf( buf, "车辆VIN:%s\n", jt808_param.id_0xF005 );
            printer( buf );
            sprintf( buf, "驾驶员姓名:%s\n", jt808_param.id_0xF008 );
            printer( buf );
            sprintf( buf, "驾驶证号码:%s\n", jt808_param.id_0xF009 );
            printer( buf );
            memset( buf, 64, 0 );
            sprintf( buf, "打印时间:20%02d-%02d-%02d %02d:%02d:%02d\n",
                     YEAR( mytime_now ),
                     MONTH( mytime_now ),
                     DAY( mytime_now ),
                     HOUR( mytime_now ),
                     MINUTE( mytime_now ),
                     SEC( mytime_now )
                   );

            printer( buf );
            printer( "停车前15分钟车速:\n" );
            pos = jt808_param_bk.speed_curr_index;
            for( i = 0; i < 15; i++ )
            {
                data_u32	= mytime_to_utc(jt808_param_bk.speed_15min[pos].time);
                if(( jt808_param_bk.speed_15min[pos].time != 0 ) && (utc_now - data_u32 < 900) && ( utc_now > data_u32 )) /*有校数据*/
                    //if(1)
                {
                    hour	= HOUR( jt808_param_bk.speed_15min[pos].time );
                    minute	= MINUTE( jt808_param_bk.speed_15min[pos].time );
                    sprintf( buf, " [%02d] %02d:%02d %03d kmh\n", i + 1, hour, minute, jt808_param_bk.speed_15min[pos].speed );
                    printer( buf );
                }
                else
                {
                    sprintf( buf, " [%02d] --:-- --\n", i + 1 );
                    printer( buf );
                }
                if( pos == 0 )
                {
                    pos = 15;
                }
                pos--;
            }

            //printer( "最近一次疲劳驾驶记录:\n无疲劳驾驶记录\n\n\n\n\n\n\n" );
            printer( "最近一次疲劳驾驶记录:\n" );
            if((jt808_param_bk.utc_over_time_start != 0) && (jt808_param_bk.utc_over_time_start < jt808_param_bk.utc_over_time_end) && (jt808_param_bk.utc_over_time_end <= utc_now))
            {
                printer( "疲劳驾驶开始时间:\n" );
                time	= utc_to_mytime(jt808_param_bk.utc_over_time_start);
                sprintf( buf, "    20%02d-%02d-%02d %02d:%02d:%02d\n",
                         YEAR( time),
                         MONTH( time ),
                         DAY( time ),
                         HOUR( time ),
                         MINUTE( time ),
                         SEC( time )
                       );
                printer( buf );
                printer( "疲劳驾驶结束时间:\n" );
                time	= utc_to_mytime(jt808_param_bk.utc_over_time_end);
                sprintf( buf, "    20%02d-%02d-%02d %02d:%02d:%02d\n",
                         YEAR( time),
                         MONTH( time ),
                         DAY( time ),
                         HOUR( time ),
                         MINUTE( time ),
                         SEC( time )
                       );
                printer( buf );
            }
            else
            {
                printer( "无疲劳驾驶记录\n" );
            }

            printer( "最近一次超速驾驶记录:\n" );
            if((jt808_param_bk.utc_over_speed_start != 0) && (jt808_param_bk.utc_over_speed_start < jt808_param_bk.utc_over_speed_end) && (jt808_param_bk.utc_over_speed_end <= utc_now))
            {
                printer( "超速驾驶开始时间:\n" );
                time	= utc_to_mytime(jt808_param_bk.utc_over_speed_start);
                sprintf( buf, "    20%02d-%02d-%02d %02d:%02d:%02d\n",
                         YEAR( time),
                         MONTH( time ),
                         DAY( time ),
                         HOUR( time ),
                         MINUTE( time ),
                         SEC( time )
                       );
                printer( buf );
                printer( "超速驾驶结束时间:\n" );
                time	= utc_to_mytime(jt808_param_bk.utc_over_speed_end);
                sprintf( buf, "    20%02d-%02d-%02d %02d:%02d:%02d\n",
                         YEAR( time),
                         MONTH( time ),
                         DAY( time ),
                         HOUR( time ),
                         MINUTE( time ),
                         SEC( time )
                       );
                printer( buf );
                sprintf(buf, "超速驾驶最高速度:%d\n", jt808_param_bk.over_speed_max);
                printer( buf );
            }
            else
            {
                printer( "无超速驾驶记录\n" );
            }
            printer( "\n\n\n\n\n\n\n" );
            //GPIO_ResetBits( GPIOB, GPIO_Pin_6 );
#endif
        }
        break;
        /*
        case KEY_MENU_REPEAT: //长按进入设置
        	pMenuItem = &Menu_0_0_password;
        	pMenuItem->show( );
        	break;
        */
    }
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
static void timetick( unsigned int systick )
{
    //#if NEED_TODO

    if(reset_firstset == 6)
    {
        reset_firstset++;
        //----------------------------------------------------------------------------------
        jt808_param.id_0xF00F = 0;   //  恢复设置状态
        param_save(1);
        //----------------------------------------------------------------------------------
    }
    else if(reset_firstset >= 7) //50ms一次,,60s
    {
        reset_firstset++;
        lcd_fill(0);
        lcd_text12(0, 3, "需重新设置车牌号和ID", 20, LCD_MODE_SET);
        lcd_text12(24, 18, "重新加电查看", 12, LCD_MODE_SET);
        lcd_update_all();
    }

    //#endif

    if( systick - lasttick >= 50 )
    {
        Disp_Idle( );
        lasttick = systick;
    }
}

MENUITEM Menu_1_Idle =
{
    "待机界面",
    8,		   0,
    &show,
    &keypress,
    &timetick,
    &msg,
    (void *)0
};

/************************************** The End Of File **************************************/
