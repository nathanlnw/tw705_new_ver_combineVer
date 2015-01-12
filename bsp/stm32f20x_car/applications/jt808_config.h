#ifndef _H_JT808_CONFIG
#define _H_JT808_CONFIG

#include "include.h"
//#define JT808_TEST_JTB			///表示程序为交通部测试程序
#define DEVICE_IS_TW703			///表示程序为交通部测试程序

/***************************************************************************************************************/
//设置存储相关地址，对应的设备的所有需要存储的区域
/***************************************************************************************************************/
/*设备主要参数存储位置*/
#define ADDR_DF_PARAM_MAIN 			0x000000				///设备主要参数存储位置
#define ADDR_DF_PARAM_MAIN_SECT		1						///扇区总数量

/*程序升级存储位置*/
#define ADDR_DF_UPDATA_START		0x001000  				///程序数据存储开始位置
#define ADDR_DF_UPDATA_END			0x03A000 				///程序数据存储结束位置
#define ADDR_DF_UPDATA_PARA			ADDR_DF_UPDATA_END 		///程序相关参数存储位置,该参数大小不能超过0x1000
#define ADDR_DF_UPDATA_SECT			58						///扇区总数量

/*设备主要参数存储备份位置*/
#define ADDR_DF_PARAM_MAIN_BK 		0x040000				///设备主要参数存储备份位置
#define ADDR_DF_PARAM_MAIN_BK_SECT	1						///扇区总数量

/*设备其它参数和数据存储位置*/
#define ADDR_DF_PARAM_BASE 			0x050000				///用来存储设备非主要参数和用户数据等信息
#define ADDR_DF_PARAM_BASE_SECT		32						///扇区总数量

/*文本信息,事件报告,中心提问,信息点播,电话簿,存储区域*/
#define ADDR_DF_MISC_BASE 			0x08D000				///用来存储808中相关如下信息:文本信息,事件报告,中心提问,信息点播,电话簿,存储区域
#define ADDR_DF_MISC_BASE_SECT		7						///扇区总数量

/*线路存储区域*/
#define ADDR_DF_LINE_START			0x094000     			///路线数据存储开始位置
#define ADDR_DF_LINE_END			0x0A2000    			///路线数据存储结束位置
#define ADDR_DF_LINE_SECT			14						///扇区总数量

/*电子围栏存储区域*/
#define ADDR_DF_AREA_START			0x0A2000    			///电子围栏数据存储开始位置
#define ADDR_DF_AREA_END			0x0A4000    			///电子围栏数据存储结束位置
#define ADDR_DF_AREA_SECT			2						///扇区总数量

/*多媒体存储位置*/
#define ADDR_DF_CAM_START			0x0A4000    			///图片及录音数据存储开始位置
#define ADDR_DF_CAM_END				0X1D0000    			///图片及录音数据存储结束位置
#define ADDR_DF_CAM_SECT			300						///扇区总数量
//#define ADDR_DF_VOC_START			0x1D0000    			///录音数据存储开始位置
//#define ADDR_DF_VOC_END			0X298000    			///录音数据存储结束位置
//#define ADDR_DF_VOC_SECT		 	0x400       			///录音数据存储大小

/*行驶记录仪VDR数据存储区域*/
#define ADDR_DF_VDR_BASE 			0x1D0000				///行驶记录仪VDR数据存储区域开始位置
#define ADDR_DF_VDR_END				0x2AC000				///行驶记录仪VDR数据存储区域结束位置
#define ADDR_DF_VDR_SECT			220						///多给分配了一些空间，实际需要213 


/*gps补报数据存储区域*/
#define ADDR_DF_POSTREPORT_START	0x2AC000				///gps补报数据存储区域开始位置
#define ADDR_DF_POSTREPORT_END		0x390000				///gps补报数据存储区域结束位置
#define ADDR_DF_POSTREPORT_SECT		228						///扇区总数量

/*gps压缩数据存储区域*/
//正常1秒 517字节  压缩后 260
//近似 100个扇区 保存25分钟原始数据
#define ADDR_DF_GPS_PACK_START		0x390000				///gps原始压缩数据存储区域开始位置
#define ADDR_DF_GPS_PACK_END		0x3E0000				///gps原始压缩数据存储区域结束位置
#define ADDR_DF_GPS_PACK_SECT		80						///扇区总数量

/*CAN数据存储区域*/
//1条CAN数据17字节
//近似 100个扇区 保存25分钟原始数据
#define ADDR_DF_CAN_PACK_START		0x3E0000				///gps原始压缩数据存储区域开始位置
#define ADDR_DF_CAN_PACK_END		0x3F4000				///gps原始压缩数据存储区域结束位置
#define ADDR_DF_CAN_PACK_SECT		20						///扇区总数量  


/*字库数据存储区域*/
#ifdef FONT_IS_OUT_FLASH
#define ADDR_DF_FONT_BASE			0x00740000				///字库数据存储开始位置，该地址为WQ25地址
#define ADDR_DF_FONT_BASE_SECT		768						///扇区总数量
#define ADDR_DF_FONT_BASE_END		0x00800000				///字库数据存储结束位置，
#else
#define ADDR_DF_FONT_BASE			0x08040000				///该地址为STM32F407片内地址
#endif
#endif
