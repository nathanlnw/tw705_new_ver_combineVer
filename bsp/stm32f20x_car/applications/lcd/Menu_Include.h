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
#ifndef _H_MENU_H_
#define _H_MENU_H_

#include <stdio.h>
#include "include.h"
#include <rtthread.h>

#include "jt808.h"
#include "jt808_param.h"
#include "jt808_gps.h"
#include "jt808_misc.h"
#include "sle4442.h"

#define KEY_MENU	0x1
#define KEY_OK		0x2
#define KEY_UP		0x4
#define KEY_DOWN	0x8

#define KEY_MENU_REPEAT 0x10
#define KEY_OK_REPEAT	0x20
#define KEY_UP_REPEAT	0x40
#define KEY_DOWN_REPEAT 0x80

#define 	DEBUG_MSG_NUM			20
#define 	DEBUG_MSG_MAX_LEN		20

typedef struct _IMG_DEF
{
    unsigned char	width_in_pixels;    /* Image width */
    unsigned char	height_in_pixels;   /* Image height*/
    unsigned char	*char_table;        /* Image table start address in memory  */
} IMG_DEF;

#define DECL_BMP( width, height, imgdata ) IMG_DEF BMP_ ## imgdata = { width, height, imgdata }

#define MaxBankIdleTime 1200            //60s  LCD任务执行周期60ms,1min没有按键操作推出到idle状态

typedef void ( *SHOW )( void );
typedef void ( *KEYPRESS )( unsigned int );
typedef void ( *TIMETICK )( unsigned int );
typedef void ( *MSG )( void *p );

typedef   struct _menuitem
{
    char				*caption;   /*菜单项的文字信息*/
    unsigned char		len;
    uint32_t			tick;       /*最近一次有按键的时刻*/
    SHOW				show;       /*显示时调用，初始化显示*/
    KEYPRESS			keypress;   /*发生按键时调用*/
    TIMETICK			timetick;   /*向其提供系统tick，比如返回待机画面*/
    MSG					msg;
    struct _menuitem	*parent;
} MENUITEM;

typedef struct
{
    char	car_type[10];				//车辆类型
    char	chepai[10];					//车牌
    char	VIN[18];					//车辆VIN
    uint8_t	color;						//车辆颜色
    char	SIM_NUM[13];				//卡号
} STYLE_CAR_INFO;

typedef struct _menuitem *PMENUITEM;



extern IMG_DEF	BMP_noselect_log;			///空心圆图标
extern IMG_DEF	BMP_select_log;				///实心圆图标
extern STYLE_CAR_INFO		car_info;



//2014.2.12
extern uint8_t	menu_first_in;		//表示从待机界面进入该界面,每一个BIT位表示不同的主界面
extern unsigned int CounterBack;
extern unsigned char CarSet_0_counter;//记录设置车辆信息的设置内容1:车牌号2:类型3:颜色
extern unsigned char KeyValue;
extern unsigned char Password_correctFlag;
//------------ 使用前锁定相关 ------------------
extern unsigned char Menu_Car_license[10];//存放车牌号码
extern unsigned char  Menu_VechileType[10];  //  车辆类型
extern unsigned char  Menu_VecLogoColor[10]; // 车牌颜色
extern unsigned char Menu_color_num;
extern unsigned char Menu_Vin_Code[17];
extern unsigned char Menu_sim_Code[12];
extern unsigned char menu_type_flag, menu_color_flag;

extern unsigned char MENU_set_carinfor_flag;//菜单进入单项设置车辆信息
extern char PassWord[10];
extern uint8_t 	debug_msg[DEBUG_MSG_NUM][DEBUG_MSG_MAX_LEN];

//=========================
//存储输入的相应信息
extern u8 Menu_MainDns[20];
extern u8 Menu_AuxDns[20];
extern u8 Menu_MainIp[20];
extern u8 Menu_AuxIp[20];
extern u8 Menu_Apn[20];

extern u8 NET_SET_FLAG;
extern u8 DNS_Main_Aux;
extern uint8_t usb_export_status;		///0,表示没有检测到U盘，无法导出；1表示正常导出数据，66表示导出成功
extern uint8_t IC_card_showinfo;		///0,表示正常显示；1表示直接显示驾驶员信息


extern const unsigned char Car_HZ_code[31][2];
extern const unsigned char NUM_AND_ABC[37];
//=========================
#if 0
#define HMI_TEXTMSG_NEW 0x80000000  /*新的文本信息*/

/*定义hmi要显示的信息*/
extern uint32_t			hmi_status;

extern unsigned char	UpAndDown;
extern unsigned char	KeyValue;
extern unsigned char	KeyCheck_Flag[4];

typedef __packed struct
{
    unsigned int	id;
    unsigned int	len;
    unsigned char	*p;
} MB_SendDataType;

typedef __packed struct
{
    unsigned char	Num;
    unsigned char	PCard[18];
    unsigned char	StartTime[6];
    unsigned char	EndTime[6];
} PilaoRecord;

typedef __packed struct
{
    unsigned char	Num;
    unsigned char	PCard[18];
    unsigned char	StartTime[6];
    unsigned char	EndTime[6];
    unsigned char	Speed;
} ChaosuRecord;

typedef __packed struct
{
    unsigned char	Head[3];
    unsigned int	Flag;
    unsigned int	AllLen;
    unsigned char	ZCFlag[2];
    //unsigned char *PInfor;
    unsigned char	PInfor[200];
    unsigned char	CheckOut;
    unsigned char	End[3];
} DispMailBoxInfor;

extern unsigned int		tzxs_value;
extern unsigned char	send_data[10];
extern MB_SendDataType	mb_senddata;

extern unsigned char	Dis_date[22];
extern unsigned char	Dis_speDer[20];

extern unsigned char	GPS_Flag, Gprs_Online_Flag;         //记录gps gprs状态的标志
extern unsigned char	DriverName[22], DriverCardNUM[20];  //从IC卡里读出的驾驶员姓名和驾驶证号码
extern unsigned char	speed_time_rec[15][6];

extern unsigned char	ErrorRecord;
extern PilaoRecord		PilaoJilu[12];
extern ChaosuRecord		ChaosuJilu[20];

extern DispMailBoxInfor LCD_Post, GPStoLCD, OtherToLCD, PiLaoLCD, ChaoSuLCD;

extern unsigned char	StartDisTiredExpspeed;          //开始显示疲劳或者超速驾驶的记录,再判断提示时间信息错误时用
extern unsigned char	tire_Flag, expsp_Flag;
extern unsigned char	pilaoCounter, chaosuCounter;    //记录返回疲劳驾驶和超速驾驶的条数
extern unsigned char	pilaoCouAscii[2], chaosuCouAscii[2];

extern unsigned char	ServiceNum[13];                 //设备的唯一性编码,IMSI号码的后12位
extern unsigned char	DisComFlag;
extern unsigned char	OneKeyCallFlag;
extern unsigned char	data_tirexps[120];
extern u8				CarSet_0_counter;               //记录设置车辆信息的设置内容1:车牌号2:类型3:颜色

#endif

extern MENUITEM			*pMenuItem;
/*
extern MENUITEM			Menu_0_0_password;
extern MENUITEM			Menu_0_1_license;
extern MENUITEM			Menu_0_2_CarType;
extern MENUITEM			Menu_0_3_vin;
extern MENUITEM			Menu_0_4_Colour;
extern MENUITEM			Menu_0_5_DeviceID;
extern MENUITEM			Menu_0_6_Engineer;
extern MENUITEM			Menu_0_7_VehicleSet;
*/
extern MENUITEM			Menu_setting_param;
extern MENUITEM	        Menu_0_0_password;
extern MENUITEM	        Menu_0_0_type;
extern MENUITEM	        Menu_0_1_license;
extern MENUITEM	        Menu_0_2_SpeedType;
extern MENUITEM	        Menu_0_3_Sim;
extern MENUITEM	        Menu_0_4_vin;
extern MENUITEM	        Menu_0_Colour;
extern MENUITEM	        Menu_0_6_PulseCoefficient;

extern MENUITEM			Menu_0_loggingin;

extern MENUITEM			Menu_1_Idle;
extern MENUITEM			Menu_1_menu;

extern MENUITEM			Menu_2_1_Status8;
extern MENUITEM			Menu_2_2_Speed15;
extern MENUITEM			Menu_2_3_CentreTextStor;
extern MENUITEM			Menu_2_4_CarInfor;
extern MENUITEM			Menu_2_5_DriverInfor;
extern MENUITEM			Menu_2_6_Mileage;
extern MENUITEM			Menu_2_7_RequestProgram;
extern MENUITEM			Menu_2_8_DnsIpDisplay;
extern MENUITEM			Menu_2_InforCheck;

extern MENUITEM			Menu_3_1_CenterQuesSend;
extern MENUITEM			Menu_3_2_FullorEmpty;
extern MENUITEM			Menu_3_3_ElectronicInfor;
extern MENUITEM			Menu_3_4_Multimedia;
extern MENUITEM			Menu_3_5_MultimediaTrans;
extern MENUITEM			Menu_3_6_Record;
extern MENUITEM			Menu_3_7_Affair;
extern MENUITEM			Menu_3_8_LogOut;
extern MENUITEM			Menu_3_InforInteract;

extern MENUITEM			Menu_4_1_pilao;
extern MENUITEM			Menu_4_2_chaosu;
extern MENUITEM			Menu_4_3_PulseCoefficient;
extern MENUITEM 		Menu_4_4_ExportData;
extern MENUITEM 		Menu_4_5_ICcard;
extern MENUITEM 		Menu_4_7_speedlog;
extern MENUITEM			Menu_4_recorder;
extern MENUITEM			Menu_4_InforTirExspd;


extern MENUITEM			Menu_5_1_TelDis;
extern MENUITEM			Menu_5_2_TelAtd;
extern MENUITEM			Menu_5_3_bdupgrade;
extern MENUITEM			Menu_5_4_bdColdBoot;
extern MENUITEM			Menu_5_5_can;
extern MENUITEM			Menu_5_6_Concuss;
extern MENUITEM			Menu_5_7_Version;
extern MENUITEM			Menu_5_8_Usb;
extern MENUITEM			Menu_5_9_debug;
extern MENUITEM			Menu_5_other;

//extern MENUITEM			Menu_6_RemoteUpdata;
extern MENUITEM         Menu_6_SetInfor;
extern MENUITEM			Menu_7_CentreTextDisplay;

extern MENUITEM         Menu_8_SetDNS;
extern MENUITEM         Menu_8_1_MainDnsPort;
extern MENUITEM         Menu_8_2_AuxDnsPort;
//extern MENUITEM         Menu_8_3_MainIpPort;
//extern MENUITEM         Menu_8_4_AuxIpPort;


extern MENUITEM			Menu_Popup;
extern MENUITEM			Menu_param_set_01;
extern MENUITEM			Menu_flash_err;

#if 0
extern unsigned char	SetVIN_NUM;                     //   1:设置车牌号码  2:设置VIN
extern unsigned char	OK_Counter;                     //记录在快捷菜单下ok键按下的次数
extern unsigned char	Screen_In, Screen_in0Z;         //记录备选屏内选中的汉字

extern unsigned char	OKorCancel, OKorCancel2, OKorCancelFlag;
extern unsigned char	SetTZXSFlag, SetTZXSCounter;    //SetTZXSFlag  1:校准车辆特征系数开始  2:校准车辆特征系数结束

extern unsigned char	OUT_DataCounter, DataOutStartFlag, DataOutOK;
extern unsigned char	Rx_TZXS_Flag;
extern unsigned char	battery_flag, tz_flag;
extern unsigned char	USB_insertFlag;

extern unsigned char	BuzzerFlag;
extern unsigned char	DaYin;
extern unsigned char	DaYinDelay;

extern unsigned char	FileName_zk[11];
//==============12*12========读字库中汉字的点阵==========
extern unsigned char	test_00[24], Read_ZK[24];

//↑ ↓
extern unsigned char	UpAndDown_nm[4];
extern unsigned char	ICcard_flag;

extern unsigned char	DisInfor_Menu[8][20];
extern unsigned char	DisInfor_Affair[8][20];

//========================================================================
extern unsigned char	UpdataDisp[8];          //北斗升级进度
extern unsigned char	BD_updata_flag;         //北斗升级度u盘文件的标志
extern unsigned int		FilePageBD_Sum;         //记录文件大小，读文件大小/514
extern unsigned int		bd_file_exist;          //读出存在要升级的文件
extern unsigned char	device_version[30];
extern unsigned char	bd_version[20];

extern unsigned char	ISP_Updata_Flag;        //远程升级主机程序进度显示标志   1:开始升级  2:升级完成

extern unsigned char	BD_upgrad_contr;

//------------ 使用前锁定相关 ------------------
extern unsigned char	Menu_Car_license[10];   //存放车牌号码
extern u8				Menu_VechileType[10];   //  车辆类型
extern u8				Menu_VecLogoColor[10];  // 车牌颜色
extern u8				Menu_Vin_Code[17];
extern u8				Menu_color_num;
extern u8				menu_type_flag, menu_color_flag;



extern void version_disp( void );


extern void ReadPiLao( unsigned char NumPilao );


extern void ReadEXspeed( unsigned char NumExspeed );


extern void Dis_pilao( unsigned char *p );


extern void Dis_chaosu( unsigned char *p );


extern unsigned char Fetch_15minSpeed( unsigned char Num15 );
#endif

/*add by bitter*/

#include <rtthread.h>

/*超速驾驶*/
typedef __packed struct
{
    uint8_t start[6];
    uint8_t end[6];
    uint8_t max;
    uint8_t avg;
} REC_OVERSPEED;

/*超时，疲劳驾驶*/
typedef __packed struct
{
    uint8_t start[6];
    uint8_t end[6];
} REC_OVERTIME;


typedef struct
{
    uint16_t	attrib;
    uint8_t		start;
    uint8_t		count;
} DISP_ROW; /*显示行*/

extern IMG_DEF			BMP_arrow_left;
extern IMG_DEF			BMP_arrow_right;

void timetick_default( unsigned int tick );


uint8_t split_content( uint8_t *pinfo, uint16_t len, DISP_ROW *display_rows, uint8_t bytes_per_row );


uint8_t my_strncpy( char *dst, char *src, uint8_t len );
uint8_t lcd_set_param(char *name, uint8_t *data, uint8_t type, uint8_t len , uint8_t is_num , uint8_t (*cb_res)());


#endif

/************************************** The End Of File **************************************/
