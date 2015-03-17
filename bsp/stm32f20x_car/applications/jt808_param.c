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

#include "jt808_param.h"
#include "sst25.h"
#include "jt808.h"
#include <finsh.h>
#include "string.h"
#include "jt808_gps.h"
#include "GB_vdr.h"
#include "SPI_SD_driver.h"


#define MAIN_PARA_CRC_ERROR				0x01			///设备主参数CRC错误
#define MAIN_PARA_VER_UP				0x02			///设备主参数版本升级
#define MAIN_PARA_FORMAT_ERR			0x04			///设备主参数格式错误


#define JT808_FLASH_FORMA_VER		0x464D5431			///"FMT1",采用可见字符是为了显示和后期远程修改方便
#define JT808_FLASH_DATA_VER		0x30303030			///"0000",采用可见字符是为了显示和后期远程修改方便

#define BKSRAM_VERSION				0x00000002
#define BKSRAM_FORMAT				0x57584448


uint8_t		HardWareVerion =0;				///读取到的PCB板的版本号，对应的口为PA14,PA13,PB3
uint16_t 	current_speed = 0;				///单位为km/h,当前速度，可能为GPS速度，也可能为脉冲速度，取决于参数id_0xF043，
uint16_t 	current_speed_10x = 0;			///单位为0.1km/h,当前速度，可能为GPS速度，也可能为脉冲速度，取决于参数id_0xF043，
uint8_t		param_set_speed_state = 0;		///该值为非零时才有效，1表示速度为0时立刻上报一包数据，2表示速度超过为10km/h时上报一包数据
JT808_PARAM jt808_param;
static JT808_PARAM jt808_param_fun; 		//本函数内部使用，用于比较惨是匹配
//***********************************************************************************************************************
#ifdef STM32F4XX
uint8_t	HARD_VER = 1;			///硬件版本,该版本的设备使用单片机为STM32F407
#else
uint8_t	HARD_VER = 2;			///硬件版本,该版本的设备使用单片机为STM32F207
#endif
uint8_t SOFT_VER = 105;			///软件版本
/*  软件版本更新说明:
	101:日期:2014-04-08.
		说明:第一版本的程序发布,CPU型号为STM32F207,FLASH类型为W25Q64FV，该版本程序没有经过长期加电测试。
			flash进行清空是会保留设备ID(jt808_param.id_0xF002)，同时清空是使用了64K 擦除函数。
	102:日期:2014-05-27.
		说明:对程序进行了优化，增加了一定时间没有上位机下发数据重新连接的功能，同时将207和407的程序进行了整合，
			增加了文件"include.h"，区分不同ARM，以后可以进行更方便的互相移植；
			设备开机后取消了调试信息，如果用户按住"菜单"键或者按下"报警"按钮则开机后会显示调试信息。
	103:日期:2014-08-01.
		说明:语音播报2次，增加了定时复位，复位周期为2天，复位时间为凌晨00:08，短信部分进行了优化修改
	104:日期:2014-09-12.
		说明:M66模块开关机频率进行了控制，如果模块连续6次不能连接服务器，则后面需要等待6分钟才能重新连接
			另外对搅拌桶正反转判断部分进行了优化处理，将打印机的一些参数写成了固定值(id_0xF040->行间隔为6,id_0xF041->左边界为0,id_0xF042->右边界为0)
			将参数id_0xF042修改特性为设备休眠开关,9月22日增加了默认设置参数，将车辆默认参数设置为天津"客运车"辆,修改了一点车辆信息
			输入的部分代码，增加了通过命令设置车辆相关参数的功能。
	105:日期:2014-11-24.
		说明:修改了多媒体照片部分查看照片的代码(之前会复位异常)，修改了密码为112233时清空参数部分的界面，修改了读取IC卡部分使用电池供电时
			重复读取的问题。修改了录入车台信息相关界面的问题。增加了读取超声波测量油耗的内容；增加了调试信息显示在液晶上的功能。增加了将
			油耗数据存储在FLASH中的功能，当插入U盘时可以将油耗数据保存在文件名为"油耗.txt"的文件中(前提条件是车台上电30秒，并且参数
			jt808_param.id_0xF04B为非0值)。增加了对设备参数区域的保护功能，该区域不能自动删除，只能通过手动删除(界面或命令)，这样用来防止数据的丢失。
			如果在开机的时候同时按下"up"和"down"键，则当检测到flash错误后会马上清空flash，目的是为了生产时快速格式化flash。

*/
//***********************************************************************************************************************


#if 0
JT808_PARAM jt808_param =
{
    0x13091405,                         /*0x0000 版本*/
    50,                                 /*0x0001 心跳发送间隔*/
    10,                                 /*0x0002 TCP应答超时时间*/
    3,                                  /*0x0003 TCP超时重传次数*/
    10,                                 /*0x0004 UDP应答超时时间*/
    5,                                  /*0x0005 UDP超时重传次数*/
    60,                                 /*0x0006 SMS消息应答超时时间*/
    3,                                  /*0x0007 SMS消息重传次数*/
    "CMNET",                            /*0x0010 主服务器APN*/
    "",                                 /*0x0011 用户名*/
    "",                                 /*0x0012 密码*/
    //"jt1.gghypt.net",                   /*0x0013 主服务器地址*/
    "60.28.50.210",
    "CMNET",                            /*0x0014 备份APN*/
    "",                                 /*0x0015 备份用户名*/
    "",                                 /*0x0016 备份密码*/
    "jt2.gghypt.net",                   /*0x0017 备份服务器地址，ip或域名*/
    //7008,                               /*0x0018 TCP端口*/
    9131,
    5678,                               /*0x0019 UDP端口*/
    "www.google.com",                   /*0x001A ic卡主服务器地址，ip或域名*/
    9901,                               /*0x001B ic卡服务器TCP端口*/
    8875,                               /*0x001C ic卡服务器UDP端口*/
    "www.google.com",                   /*0x001D ic卡备份服务器地址，ip或域名*/
    0,                                  /*0x0020 位置汇报策略*/
    1,                                  /*0x0021 位置汇报方案*/
    30,                                 /*0x0022 驾驶员未登录汇报时间间隔*/
    120,                                /*0x0027 休眠时汇报时间间隔*/
    5,                                  /*0x0028 紧急报警时汇报时间间隔*/
    30,                                 /*0x0029 缺省时间汇报间隔*/
    500,                                /*0x002c 缺省距离汇报间隔*/
    1000,                               /*0x002d 驾驶员未登录汇报距离间隔*/
    1000,                               /*0x002e 休眠时距离汇报间隔*/
    100,                                /*0x002f 紧急时距离汇报间隔*/
    270,                                /*0x0030 拐点补传角度*/
    500,                                /*0x0031 电子围栏半径（非法位移阈值），单位为米*/
    "1008611",                          /*0x0040 监控平台电话号码*/
    "",                                 /*0x0041 复位电话号码*/
    "",                                 /*0x0042 恢复出厂设置电话号码*/
    "",                                 /*0x0043 监控平台SMS号码*/
    "",                                 /*0x0044 接收终端SMS文本报警号码*/
    5,                                  /*0x0045 终端接听电话策略*/
    3,                                  /*0x0046 每次通话时长*/
    3,                                  /*0x0047 当月通话时长*/
    "",                                 /*0x0048 监听电话号码*/
    "",                                 /*0x0049 监管平台特权短信号码*/
    5,                                  /*0x0050 报警屏蔽字*/
    3,                                  /*0x0051 报警发送文本SMS开关*/
    5,                                  /*0x0052 报警拍照开关*/
    3,                                  /*0x0053 报警拍摄存储标志*/
    5,                                  /*0x0054 关键标志*/
    90,                                 /*0x0055 最高速度kmh*/
    5,                                  /*0x0056 超速持续时间*/
    4 * 60 * 60,                        /*0x0057 连续驾驶时间门限*/
    8 * 60 * 60,                        /*0x0058 当天累计驾驶时间门限*/
    20 * 60,                            /*0x0059 最小休息时间*/
    12 * 60 * 60,                       /*0x005A 最长停车时间*/
    100,                                /*0x005B 超速报警预警差值，单位为 1/10Km/h */
    90,                                 /*0x005C 疲劳驾驶预警差值，单位为秒（s），>0*/
    0x200a,                             /*0x005D 碰撞报警参数设置:*/
    30,                                 /*0x005E 侧翻报警参数设置： 侧翻角度，单位 1 度，默认为 30 度*/
    0,                                  /*0x0064 定时拍照控制*/
    0,                                  /*0x0065 定距拍照控制*/
    3,                                  /*0x0070 图像视频质量(1-10)*/
    5,                                  /*0x0071 亮度*/
    3,                                  /*0x0072 对比度*/
    5,                                  /*0x0073 饱和度*/
    3,                                  /*0x0074 色度*/
    5,                                  /*0x0080 车辆里程表读数0.1km*/
    12,                                 /*0x0081 省域ID*/
    101,                                /*0x0082 市域ID*/
    "津AP6834",                         /*0x0083 机动车号牌*/
    2,                                  /*0x0084 车牌颜色	1蓝色 2黄色 3黑色 4白色 9其他*/
    0x0f,                               /*0x0090 GNSS 定位模式*/
    0x01,                               /*0x0091 GNSS 波特率*/
    0x01,                               /*0x0092 GNSS 模块详细定位数据输出频率*/
    0x01,                               /*0x0093 GNSS 模块详细定位数据采集频率*/
    0x01,                               /*0x0094 GNSS 模块详细定位数据上传方式*/
    0x01,                               /*0x0095 GNSS 模块详细定位数据上传设置*/
    0,                                  /*0x0100 CAN 总线通道 1 采集时间间隔(ms)，0 表示不采集*/
    0,                                  /*0x0101 CAN 总线通道 1 上传时间间隔(s)，0 表示不上传*/
    0,                                  /*0x0102 CAN 总线通道 2 采集时间间隔(ms)，0 表示不采集*/
    0,                                  /*0x0103 CAN 总线通道 2 上传时间间隔(s)，0 表示不上传*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0110 CAN 总线 ID 单独采集设置*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0111 其他CAN 总线 ID 单独采集设置*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0112 其他CAN 总线 ID 单独采集设置*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0113 其他CAN 总线 ID 单独采集设置*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0114 其他CAN 总线 ID 单独采集设置*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0115 其他CAN 总线 ID 单独采集设置*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0116 其他CAN 总线 ID 单独采集设置*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0117 其他CAN 总线 ID 单独采集设置*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0118 其他CAN 总线 ID 单独采集设置*/
    { 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0119 其他CAN 总线 ID 单独采集设置*/

    "70420",                            /*0xF000 制造商ID  70420*/
    "TW705",                            /*0xF001 终端型号TW703-BD*/
    "0614100",                          /*0xF002 终端ID*/
    "12345",                            /*0xF003 鉴权码*/
    0x07,                               /*0xF004 终端类型*/
    "0000000000000000",                 /*0xF005 车辆VIN*/
    "012002444390",                     /*0xF006 DeviceID*/
    "驾驶证代码",                       /*0xF007 驾驶证代码*/
    "张三",                             /*0xF008 驾驶员姓名*/
    "120104197712015381",               /*0xF009 驾驶证号码*/
    "大型货运",                         /*0xF00A 车辆类型*/
    "未知",                             /*0xF00B 从业资格证*/
    "未知      ",                       /*0xF00C 发证机构*/

    "1.00",                             /*0xF010 软件版本号*/
    "1.00",                             /*0xF011 硬件版本号*/
    "TJ.GT",                            /*0xF012 销售客户代码*/
    0x3020,                             /*0xF013 北斗模块型号0,未确定 ,0x3020 默认 0x3017*/

    0,                                  /*0xF020 总里程*/
    0,                                  /*0xF021 车辆状态*/

    0x35DECC80,                         /*0xF030 记录仪初次安装时间,mytime格式*/
    0,                                  /*id_0xF031;      初始里程*/
    6250,                               /*id_0xF032;      车辆脉冲系数*/

    6,                                  //line_space;               //行间隔
    0,                                  //margin_left;				//左边界
    0,                                  //margin_right;				//右边界
    1,                                  //step_delay;               //步进延时,影响行间隔
    1,                                  //gray_level;               //灰度等级,加热时间
    5,                                  //heat_delay[0];			//加热延时
    10,                                 //heat_delay[1];			//加热延时
    15,                                 //heat_delay[2];			//加热延时
    20,                                 //heat_delay[3];			//加热延时
};
#endif
JT808_PARAM_BK 	jt808_param_bk;		///存入bksram中的所有参数，该参数的保存时间为8小时左右
STYLE_JT808_DATA	jt808_data;		///保存车辆需要定期保存的数据
TYPE_CAR_DATA	car_data;			///车辆相关当前数据
uint32_t		param_factset_tick = 0;	///清空设备参数的时间，单位为tick
uint16_t		donotsleeptime = 360;	///该值为非0时设备不能休眠，单位为秒，默认值为360表示6分钟不能休眠设备
uint8_t			device_unlock	= 0;	///说明:0表示车台未解锁，1表示车台已经解锁
uint8_t			device_flash_err = 0;	///说明:0表示设备flash正常，1表示设备flash错误
uint8_t			device_factory_oper = 0;	///说明:0表示正常操作，1表示当前为工厂操作


#define FLAG_DISABLE_REPORT_INVALID 1   /*设备非法*/

#define FLAG_DISABLE_REPORT_AREA 2      /*区域内禁止上报*/

//static uint32_t flag_disable_report = 0;    /*禁止上报的标志位*/

#define ID_LOOKUP( id, type ) { id, type, (uint8_t*)&( jt808_param.id_ ## id ) }

struct _tbl_id_lookup
{
    uint16_t	id;
    uint8_t		type;
    uint8_t		 *val;
} tbl_id_lookup[] =
{
    ID_LOOKUP( 0x0000, TYPE_STR ),    	//uint32_t  id_0x0000;   /*0x0000 版本*/
    ID_LOOKUP( 0x0001, TYPE_DWORD ),    //uint32_t  id_0x0001;   /*0x0001 心跳发送间隔*/
    ID_LOOKUP( 0x0002, TYPE_DWORD ),    //uint32_t  id_0x0002;   /*0x0002 TCP应答超时时间*/
    ID_LOOKUP( 0x0003, TYPE_DWORD ),    //uint32_t  id_0x0003;   /*0x0003 TCP超时重传次数*/
    ID_LOOKUP( 0x0004, TYPE_DWORD ),    //uint32_t  id_0x0004;   /*0x0004 UDP应答超时时间*/
    ID_LOOKUP( 0x0005, TYPE_DWORD ),    //uint32_t  id_0x0005;   /*0x0005 UDP超时重传次数*/
    ID_LOOKUP( 0x0006, TYPE_DWORD ),    //uint32_t  id_0x0006;   /*0x0006 SMS消息应答超时时间*/
    ID_LOOKUP( 0x0007, TYPE_DWORD ),    //uint32_t  id_0x0007;   /*0x0007 SMS消息重传次数*/
    ID_LOOKUP( 0x0010, TYPE_STR ),      //char   id_0x0010[32];  /*0x0010 主服务器APN*/
    ID_LOOKUP( 0x0011, TYPE_STR ),      //char   id_0x0011[32];  /*0x0011 用户名*/
    ID_LOOKUP( 0x0012, TYPE_STR ),      //char   id_0x0012[32];  /*0x0012 密码*/
    ID_LOOKUP( 0x0013, TYPE_STR ),      //char   id_0x0013[32];  /*0x0013 主服务器地址*/
    ID_LOOKUP( 0x0014, TYPE_STR ),      //char   id_0x0014[32];  /*0x0014 备份APN*/
    ID_LOOKUP( 0x0015, TYPE_STR ),      //char   id_0x0015[32];  /*0x0015 备份用户名*/
    ID_LOOKUP( 0x0016, TYPE_STR ),      //char   id_0x0016[32];  /*0x0016 备份密码*/
    ID_LOOKUP( 0x0017, TYPE_STR ),      //char   id_0x0017[32];  /*0x0017 备份服务器地址，ip或域名*/
    ID_LOOKUP( 0x0018, TYPE_DWORD ),    //uint32_t  id_0x0018;   /*0x0018 TCP端口*/
    ID_LOOKUP( 0x0019, TYPE_DWORD ),    //uint32_t  id_0x0019;   /*0x0019 UDP端口*/
    ID_LOOKUP( 0x001A, TYPE_STR ),      //char   id_0x001A[32];  /*0x001A ic卡主服务器地址，ip或域名*/
    ID_LOOKUP( 0x001B, TYPE_DWORD ),    //uint32_t  id_0x001B;   /*0x001B ic卡服务器TCP端口*/
    ID_LOOKUP( 0x001C, TYPE_DWORD ),    //uint32_t  id_0x001C;   /*0x001C ic卡服务器UDP端口*/
    ID_LOOKUP( 0x001D, TYPE_STR ),      //char   id_0x001D[32];  /*0x001D ic卡备份服务器地址，ip或域名*/
    ID_LOOKUP( 0x0020, TYPE_DWORD ),    //uint32_t  id_0x0020;   /*0x0020 位置汇报策略*/
    ID_LOOKUP( 0x0021, TYPE_DWORD ),    //uint32_t  id_0x0021;   /*0x0021 位置汇报方案*/
    ID_LOOKUP( 0x0022, TYPE_DWORD ),    //uint32_t  id_0x0022;   /*0x0022 驾驶员未登录汇报时间间隔*/
    ID_LOOKUP( 0x0027, TYPE_DWORD ),    //uint32_t  id_0x0027;   /*0x0027 休眠时汇报时间间隔*/
    ID_LOOKUP( 0x0028, TYPE_DWORD ),    //uint32_t  id_0x0028;   /*0x0028 紧急报警时汇报时间间隔*/
    ID_LOOKUP( 0x0029, TYPE_DWORD ),    //uint32_t  id_0x0029;   /*0x0029 缺省时间汇报间隔*/
    ID_LOOKUP( 0x002C, TYPE_DWORD ),    //uint32_t  id_0x002C;   /*0x002c 缺省距离汇报间隔*/
    ID_LOOKUP( 0x002D, TYPE_DWORD ),    //uint32_t  id_0x002D;   /*0x002d 驾驶员未登录汇报距离间隔*/
    ID_LOOKUP( 0x002E, TYPE_DWORD ),    //uint32_t  id_0x002E;   /*0x002e 休眠时距离汇报间隔*/
    ID_LOOKUP( 0x002F, TYPE_DWORD ),    //uint32_t  id_0x002F;   /*0x002f 紧急时距离汇报间隔*/
    ID_LOOKUP( 0x0030, TYPE_DWORD ),    //uint32_t  id_0x0030;   /*0x0030 拐点补传角度*/
    ID_LOOKUP( 0x0031, TYPE_DWORD ),    //uint16_t  id_0x0031;   /*0x0031 电子围栏半径（非法位移阈值），单位为米*/
    ID_LOOKUP( 0x0040, TYPE_STR ),      //char   id_0x0040[32];  /*0x0040 监控平台电话号码*/
    ID_LOOKUP( 0x0041, TYPE_STR ),      //char   id_0x0041[32];  /*0x0041 复位电话号码*/
    ID_LOOKUP( 0x0042, TYPE_STR ),      //char   id_0x0042[32];  /*0x0042 恢复出厂设置电话号码*/
    ID_LOOKUP( 0x0043, TYPE_STR ),      //char   id_0x0043[32];  /*0x0043 监控平台SMS号码*/
    ID_LOOKUP( 0x0044, TYPE_STR ),      //char   id_0x0044[32];  /*0x0044 接收终端SMS文本报警号码*/
    ID_LOOKUP( 0x0045, TYPE_DWORD ),    //uint32_t  id_0x0045;   /*0x0045 终端接听电话策略*/
    ID_LOOKUP( 0x0046, TYPE_DWORD ),    //uint32_t  id_0x0046;   /*0x0046 每次通话时长*/
    ID_LOOKUP( 0x0047, TYPE_DWORD ),    //uint32_t  id_0x0047;   /*0x0047 当月通话时长*/
    ID_LOOKUP( 0x0048, TYPE_STR ),      //char   id_0x0048[32];  /*0x0048 监听电话号码*/
    ID_LOOKUP( 0x0049, TYPE_STR ),      //char   id_0x0049[32];  /*0x0049 监管平台特权短信号码*/
    ID_LOOKUP( 0x0050, TYPE_DWORD ),    //uint32_t  id_0x0050;   /*0x0050 报警屏蔽字*/
    ID_LOOKUP( 0x0051, TYPE_DWORD ),    //uint32_t  id_0x0051;   /*0x0051 报警发送文本SMS开关*/
    ID_LOOKUP( 0x0052, TYPE_DWORD ),    //uint32_t  id_0x0052;   /*0x0052 报警拍照开关*/
    ID_LOOKUP( 0x0053, TYPE_DWORD ),    //uint32_t  id_0x0053;   /*0x0053 报警拍摄存储标志*/
    ID_LOOKUP( 0x0054, TYPE_DWORD ),    //uint32_t  id_0x0054;   /*0x0054 关键标志*/
    ID_LOOKUP( 0x0055, TYPE_DWORD ),    //uint32_t  id_0x0055;   /*0x0055 最高速度kmh*/
    ID_LOOKUP( 0x0056, TYPE_DWORD ),    //uint32_t  id_0x0056;   /*0x0056 超速持续时间*/
    ID_LOOKUP( 0x0057, TYPE_DWORD ),    //uint32_t  id_0x0057;   /*0x0057 连续驾驶时间门限*/
    ID_LOOKUP( 0x0058, TYPE_DWORD ),    //uint32_t  id_0x0058;   /*0x0058 当天累计驾驶时间门限*/
    ID_LOOKUP( 0x0059, TYPE_DWORD ),    //uint32_t  id_0x0059;   /*0x0059 最小休息时间*/
    ID_LOOKUP( 0x005A, TYPE_DWORD ),    //uint32_t  id_0x005A;   /*0x005A 最长停车时间*/
    ID_LOOKUP( 0x005B, TYPE_WORD ),     //uint16_t  id_0x005B;   /*0x005B 超速报警预警差值，单位为 1/10Km/h */
    ID_LOOKUP( 0x005C, TYPE_WORD ),     //uint16_t  id_0x005C;   /*0x005C 疲劳驾驶预警差值，单位为秒（s），>0*/
    ID_LOOKUP( 0x005D, TYPE_WORD ),     //uint16_t  id_0x005D;   /*0x005D 碰撞报警参数设置:b7..0：碰撞时间(4ms) b15..8：碰撞加速度(0.1g) 0-79 之间，默认为10 */
    ID_LOOKUP( 0x005E, TYPE_WORD ),     //uint16_t  id_0x005E;   /*0x005E 侧翻报警参数设置： 侧翻角度，单位 1 度，默认为 30 度*/
    ID_LOOKUP( 0x0064, TYPE_DWORD ),    //uint32_t  id_0x0064;   /*0x0064 定时拍照控制*/
    ID_LOOKUP( 0x0065, TYPE_DWORD ),    //uint32_t  id_0x0065;   /*0x0065 定距拍照控制*/
    ID_LOOKUP( 0x0070, TYPE_DWORD ),    //uint32_t  id_0x0070;   /*0x0070 图像视频质量(1-10)*/
    ID_LOOKUP( 0x0071, TYPE_DWORD ),    //uint32_t  id_0x0071;   /*0x0071 亮度*/
    ID_LOOKUP( 0x0072, TYPE_DWORD ),    //uint32_t  id_0x0072;   /*0x0072 对比度*/
    ID_LOOKUP( 0x0073, TYPE_DWORD ),    //uint32_t  id_0x0073;   /*0x0073 饱和度*/
    ID_LOOKUP( 0x0074, TYPE_DWORD ),    //uint32_t  id_0x0074;   /*0x0074 色度*/
    ID_LOOKUP( 0x0080, TYPE_DWORD ),    //uint32_t  id_0x0080;   /*0x0080 车辆里程表读数0.1km*/
    ID_LOOKUP( 0x0081, TYPE_WORD ),     //uint16_t  id_0x0081;   /*0x0081 省域ID*/
    ID_LOOKUP( 0x0082, TYPE_WORD ),     //uint16_t  id_0x0082;   /*0x0082 市域ID*/
    ID_LOOKUP( 0x0083, TYPE_STR ),      //char   id_0x0083[32];  /*0x0083 机动车号牌*/
    ID_LOOKUP( 0x0084, TYPE_BYTE ),     //uint8_t		 id_0x0084;      /*0x0084 车牌颜色	1蓝色 2黄色 3黑色 4白色 9其他*/
    ID_LOOKUP( 0x0090, TYPE_BYTE ),     //uint8_t		 id_0x0090;      /*0x0090 GNSS 定位模式*/
    ID_LOOKUP( 0x0091, TYPE_BYTE ),     //uint8_t		 id_0x0091;      /*0x0091 GNSS 波特率*/
    ID_LOOKUP( 0x0092, TYPE_BYTE ),     //uint8_t		 id_0x0092;      /*0x0092 GNSS 模块详细定位数据输出频率*/
    ID_LOOKUP( 0x0093, TYPE_DWORD ),    //uint32_t  id_0x0093;   /*0x0093 GNSS 模块详细定位数据采集频率*/
    ID_LOOKUP( 0x0094, TYPE_BYTE ),     //uint8_t		 id_0x0094;      /*0x0094 GNSS 模块详细定位数据上传方式*/
    ID_LOOKUP( 0x0095, TYPE_DWORD ),    //uint32_t  id_0x0095;   /*0x0095 GNSS 模块详细定位数据上传设置*/
    ID_LOOKUP( 0x0100, TYPE_DWORD ),    //uint32_t  id_0x0100;   /*0x0100 CAN 总线通道 1 采集时间间隔(ms)，0 表示不采集*/
    ID_LOOKUP( 0x0101, TYPE_WORD ),     //uint16_t  id_0x0101;   /*0x0101 CAN 总线通道 1 上传时间间隔(s)，0 表示不上传*/
    ID_LOOKUP( 0x0102, TYPE_DWORD ),    //uint32_t  id_0x0102;   /*0x0102 CAN 总线通道 2 采集时间间隔(ms)，0 表示不采集*/
    ID_LOOKUP( 0x0103, TYPE_WORD ),     //uint16_t  id_0x0103;   /*0x0103 CAN 总线通道 2 上传时间间隔(s)，0 表示不上传*/
    ID_LOOKUP( 0x0110, TYPE_CAN ), 		//uint8_t		 id_0x0110[8];	 /*0x0110 CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0x0111, TYPE_CAN ), 		//uint8_t		 id_0x0111[8];	 /*0x0111 其他CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0x0112, TYPE_CAN ), 		//uint8_t		 id_0x0112[8];	 /*0x0112 其他CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0x0113, TYPE_CAN ), 		//uint8_t		 id_0x0113[8];	 /*0x0113 其他CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0x0114, TYPE_CAN ), 		//uint8_t		 id_0x0114[8];	 /*0x0114 其他CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0x0115, TYPE_CAN ), 		//uint8_t		 id_0x0115[8];	 /*0x0115 其他CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0x0116, TYPE_CAN ), 		//uint8_t		 id_0x0116[8];	 /*0x0116 其他CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0x0117, TYPE_CAN ), 		//uint8_t		 id_0x0117[8];	 /*0x0117 其他CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0x0118, TYPE_CAN ), 		//uint8_t		 id_0x0118[8];	 /*0x0118 其他CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0x0119, TYPE_CAN ), 		//uint8_t		 id_0x0119[8];	 /*0x0119 其他CAN 总线 ID 单独采集设置*/

    ID_LOOKUP( 0xF000, TYPE_STR ),      //uint8_t		 id_0x0119[8];	 /*0x0119 其他CAN 总线 ID 单独采集设置*/
    ID_LOOKUP( 0xF001, TYPE_STR ),      /*0xF001 终端型号 20byte*/
    ID_LOOKUP( 0xF002, TYPE_STR ),      /*0xF002 终端ID 7byte*/
    ID_LOOKUP( 0xF003, TYPE_STR ),      /*0xF003 鉴权码*/
    ID_LOOKUP( 0xF004, TYPE_BYTE ),     /*0xF004 终端类型*/
    ID_LOOKUP( 0xF005, TYPE_STR ),      /*0xF005 车辆标识,VIN*/
    ID_LOOKUP( 0xF006, TYPE_STR ),      /*0xF006 车辆标识,MOBILE*/
    ID_LOOKUP( 0xF008, TYPE_STR ),      /*0xF008 驾驶员姓名*/
    ID_LOOKUP( 0xF009, TYPE_STR ),      /*0xF009 驾驶证号码*/
    ID_LOOKUP( 0xF00A, TYPE_STR ),      /*0xF00A 车辆类型*/
    ID_LOOKUP( 0xF00B, TYPE_STR ),      /*0xF00B 从业资格证*/
    ID_LOOKUP( 0xF00C, TYPE_STR ),      /*0xF00C 发证机构*/
    ID_LOOKUP( 0xF00D, TYPE_BYTE ), 	//0xF00D 连接模式设置			0:货运模式	 1:两客一危
    ID_LOOKUP( 0xF00E, TYPE_BYTE ), 	//0xF00E 有无车牌号				0:无车牌号	 1:有车牌号，需要设置
    ID_LOOKUP( 0xF00F, TYPE_BYTE ), 	//0xF00F 车辆信息设置是否完成	0:未设置	 1:设置完成

    ID_LOOKUP( 0xF010, TYPE_STR ),      /*0xF010 软件版本号 32 byte*/
    ID_LOOKUP( 0xF011, TYPE_L(9) ),     /*0xF011 硬件版本号 9 byte*/
    ID_LOOKUP( 0xF012, TYPE_L(9) ),     /*0xF012 销售客户代码 9 byte*/
    ID_LOOKUP( 0xF013, TYPE_DWORD ),    /*0xF013 北斗模块型号0,未确定 ,0x3020 0x3017 4 byte*/
    ID_LOOKUP( 0xF014, TYPE_L(16)),     /*0xF014 车主电话号码 16 byte*/
    ID_LOOKUP( 0xF015, TYPE_L(30) ),    /*0xF015 预留空间，30 byte*/

    ID_LOOKUP( 0xF030, TYPE_DWORD ), 	/*0xF030 记录仪初次安装时间,mytime格式*/
    ID_LOOKUP( 0xF031, TYPE_DWORD ), 	/*0xF031 备份TCP端口1*/
    ID_LOOKUP( 0xF032, TYPE_DWORD ), 	/*0xF032 车辆脉冲系数,有效位数为低16位,表示为每公里的脉冲数，最高位为1表示需要校准，为0表示不需要校准*/

    ID_LOOKUP( 0xF040, TYPE_BYTE ),     //line_space;               //行间隔
    ID_LOOKUP( 0xF041, TYPE_BYTE ),     //margin_left;				//左边界
    ID_LOOKUP( 0xF042, TYPE_BYTE ),     //margin_right;				//右边界
    ID_LOOKUP( 0xF043, TYPE_BYTE ),     //step_delay;               //步进延时,影响行间隔
    ID_LOOKUP( 0xF044, TYPE_BYTE ),     //gray_level;               //灰度等级,加热时间
    ID_LOOKUP( 0xF045, TYPE_BYTE ),     //heat_delay[0];			//加热延时
    ID_LOOKUP( 0xF046, TYPE_BYTE ),     //heat_delay[1];			//加热延时
    ID_LOOKUP( 0xF047, TYPE_BYTE ),     //heat_delay[2];			//加热延时
    ID_LOOKUP( 0xF048, TYPE_BYTE ),     //heat_delay[3];			//加热延时
    ID_LOOKUP( 0xF049, TYPE_STR ),     	/*0xF049 备份服务器地址2*/
    ID_LOOKUP( 0xF04A, TYPE_WORD ),     /*0xF04A 备份TCP端口2*/
    ID_LOOKUP( 0xF04B, TYPE_DWORD ),    /*0xF04B 邮箱的面积*/
    ID_LOOKUP( 0xF04C, TYPE_DWORD ),    /*0xF04C 邮箱的最大深度*/
    ID_LOOKUP( 0xF04D, TYPE_WORD ),      /*0xF04D 超速过滤值*/
    ID_LOOKUP( 0xF04E, TYPE_STR )       /*0xF04E 预留数据区域*/
};

extern void printer_load_param( void );
static uint8_t param_save_ex( uint8_t sem_get , uint8_t save_sequ);

/*********************************************************************************
  *函数名称:void data_save( void )
  *功能描述:保存设备状态数据到flash中
  *输	入:	none
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void data_save( void )
{
    rt_kprintf("\n data_save");
    jt808_data.id_0xFA03 = jt808_param_bk.mileage_pulse;
    if( jt808_status & BIT_STATUS_FIXED )
    {
        jt808_data.id_0xFA00 = utc_now;
    }
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    sst25_write_back( ADDR_DF_PARAM_BASE, (uint8_t *)&jt808_data, sizeof(jt808_data) );
    rt_sem_release( &sem_dataflash );
}
FINSH_FUNCTION_EXPORT( data_save, save data);

/*********************************************************************************
  *函数名称:void data_load( void )
  *功能描述:读取需要定时保存的设备状态等数据
  *输	入:	none
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void data_load( void )
{
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    sst25_read( ADDR_DF_PARAM_BASE, (uint8_t *)&jt808_data, sizeof(jt808_data) );
    if(jt808_data.id_0xFA00 == 0xFFFFFFFF)
    {
        memset(&jt808_data, 0, sizeof(jt808_data));
        sst25_write_back( ADDR_DF_PARAM_BASE, (uint8_t *)&jt808_data, sizeof(jt808_data) );
    }
    rt_sem_release( &sem_dataflash );
}


/*********************************************************************************
  *函数名称:void param_fact_set(void)
  *功能描述:将设备主要参数设置为出厂参数
  *输	入: none
  *输	出: none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void param_fact_set(void)
{
    uint8_t 	buf[] = "MAIN_PARA_FACT_SET!";
    uint32_t	flash_ver;
    uint32_t	data_ver;
    uint32_t 	i;
    uint8_t		device_id[sizeof(jt808_param.id_0xF002)];
    /*
    交通部测试IP地址和端口号	202.96.42.115:9999
    驾驶员身份识别认证中心IP为	202.96.42.113:11001
    使用前锁定二终端通过域名	jt1.gghypt.net:1008
    */

    memcpy(device_id, jt808_param.id_0xF002, sizeof(jt808_param.id_0xF002));
    memset((uint8_t *)&jt808_param, 0, sizeof(jt808_param));
    flash_ver 	= BYTESWAP4(JT808_FLASH_FORMA_VER);
    data_ver 	= BYTESWAP4(JT808_FLASH_DATA_VER);
    memcpy(jt808_param.id_0x0000, (uint8_t *)&flash_ver, 4);
    memcpy(jt808_param.id_0x0000 + 4, (uint8_t *)&data_ver, 4);
    jt808_param.id_0x0001 = 50;
    jt808_param.id_0x0003 = 3;
    jt808_param.id_0x0004 = 10;
    jt808_param.id_0x0005 = 5;
    jt808_param.id_0x0006 = 60;
    jt808_param.id_0x0007 = 3;
    strcpy(jt808_param.id_0x0010, "CMNET");
#ifdef JT808_TEST_JTB
    jt808_param.id_0x0002 = 30;
    strcpy(jt808_param.id_0x0013, "jt1.gghypt.net");
    strcpy(jt808_param.id_0x0014, "CMNET");
    strcpy(jt808_param.id_0x0017, "jt2.gghypt.net");
    jt808_param.id_0x0018 = 7008;
    jt808_param.id_0x0019 = 7008;
    strcpy(jt808_param.id_0x001A, "202.96.42.113");
    jt808_param.id_0x001B = 11001;
    jt808_param.id_0x001C = 1008;							///使用前锁定2 端口号
    strcpy(jt808_param.id_0x001D, "jt1.gghypt.net");			///使用前锁定2 IP地址或域名
#else
    jt808_param.id_0x0002 = 10;
    strcpy(jt808_param.id_0x0013, "60.28.50.210");
    strcpy(jt808_param.id_0x0014, "CMNET");
    strcpy(jt808_param.id_0x0017, "60.28.50.210");
    jt808_param.id_0x0018 = 9131;
    jt808_param.id_0x0019 = 9131;
    strcpy(jt808_param.id_0x001A, "202.96.42.113");
    jt808_param.id_0x001B = 11001;
    jt808_param.id_0x001C = 1008;							///使用前锁定2 端口号
    strcpy(jt808_param.id_0x001D, "jt1.gghypt.net");			///使用前锁定2 IP地址或域名
#endif
    jt808_param.id_0x0021 = 1;
    jt808_param.id_0x0022 = 30;
    jt808_param.id_0x0027 = 30;
    jt808_param.id_0x0028 = 30;
    jt808_param.id_0x0029 = 30;
    jt808_param.id_0x002C = 500;
    jt808_param.id_0x002D = 1000;
    jt808_param.id_0x002E = 1000;
    jt808_param.id_0x002F = 100;
    jt808_param.id_0x0030 = 270;
    jt808_param.id_0x0031 = 500;
    strcpy(jt808_param.id_0x0040, "1008611");
    jt808_param.id_0x0045 = 5;
    jt808_param.id_0x0046 = 3;
    jt808_param.id_0x0047 = 3;
    jt808_param.id_0x0051 = 3;
    jt808_param.id_0x0052 = 5;
    jt808_param.id_0x0053 = 3;
    jt808_param.id_0x0054 = 5;
    jt808_param.id_0x0055 = 120;
    jt808_param.id_0x0056 = 5;
    jt808_param.id_0x0057 = 360000;
    jt808_param.id_0x0058 = 360000;
    jt808_param.id_0x0059 = 60;
    jt808_param.id_0x005A = 360000;
    jt808_param.id_0x005B = 100;
    jt808_param.id_0x005C = 90;
    jt808_param.id_0x005D = 0x2030;

#ifdef JT808_TEST_JTB
    jt808_param.id_0x005E = 30;
    jt808_param.id_0x0050 = 0;
#else
    jt808_param.id_0x005E = 80;
    jt808_param.id_0x0050 = BIT_ALARM_OVERTIME + \
                            BIT_ALARM_PRE_OVERSPEED + \
                            BIT_ALARM_PRE_OVERTIME + \
                            BIT_ALARM_TODAY_OVERTIME + \
                            BIT_ALARM_STOP_OVERTIME + \
                            BIT_ALARM_OVERSPEED;
#endif
    jt808_param.id_0x0070 = 3;
    jt808_param.id_0x0071 = 5;
    jt808_param.id_0x0072 = 3;
    jt808_param.id_0x0073 = 5;
    jt808_param.id_0x0074 = 3;
    jt808_param.id_0x0080 = 5;
#ifdef JT808_TEST_JTB
    jt808_param.id_0x0081 = 11;				///天津市辖区120100，河北130000，北京市辖区110100
    jt808_param.id_0x0082 = 108;			///
    strcpy(jt808_param.id_0x0083, "津TW7000");
    jt808_param.id_0x0084 = 1;
#else
    jt808_param.id_0x0081 = 12;				///天津市辖区120100，河北130000，北京市辖区110100
    jt808_param.id_0x0082 = 100;			///
    strcpy(jt808_param.id_0x0083, "津TW7000");
    jt808_param.id_0x0084 = 2;
#endif
    jt808_param.id_0x0090 = 0x03;
    jt808_param.id_0x0091 = 0x01;
    jt808_param.id_0x0092 = 0x01;
    jt808_param.id_0x0093 = 0x01;
    jt808_param.id_0x0094 = 0x01;
    jt808_param.id_0x0095 = 0x01;
    strcpy(jt808_param.id_0xF000, "70420");
    strcpy(jt808_param.id_0xF001, "TW705");
    //strcpy(jt808_param.id_0xF002,"0614100");
    for(i = 0; i < sizeof(device_id); i++)
    {
        if(device_id[i] == 0xFF)
        {
            memset(device_id, 0, sizeof(jt808_param.id_0xF002));
            memcpy(device_id, "0000000", 7);
            //device_id[0]='0';
            break;
        }
    }
    memcpy(jt808_param.id_0xF002, device_id, sizeof(jt808_param.id_0xF002));
    strcpy(jt808_param.id_0xF003, "");
    jt808_param.id_0xF004 = 0x07;
    strcpy(jt808_param.id_0xF005, "0000000000000000");
    strcpy(jt808_param.id_0xF006, "013000000001");
    strcpy(jt808_param.id_0xF007, "驾驶证代码");
    strcpy(jt808_param.id_0xF008, "张三");
    strcpy(jt808_param.id_0xF009, "120104197712015381");
    strcpy(jt808_param.id_0xF00A, "大型货运");
    strcpy(jt808_param.id_0xF00B, "01234567890");
    strcpy(jt808_param.id_0xF00C, "天津七一二");
    strcpy(jt808_param.id_0xF010, "TW705-SW");
    strcpy(jt808_param.id_0xF011, "1.00");
    strcpy(jt808_param.id_0xF012, "TJ.GT");
    jt808_param.id_0xF013 = 0X3020;
#ifdef JT808_TEST_JTB
    jt808_param.id_0xF031 = 7008;			///备用IP端口号
#else
    jt808_param.id_0xF031 = 9131;
#endif
    jt808_param.id_0xF030 = 0x35DECC80;
    jt808_param.id_0xF032 = 6250;
    jt808_param.id_0xF032 |= BIT(31);
    jt808_param.id_0xF040 = 6;
    jt808_param.id_0xF041 = 2;  //      抽检时该数值为0 ，过检满记录仪数据1  ，  正式该数值为2
    jt808_param.id_0xF042 = 0;
    jt808_param.id_0xF043 = 1;
    jt808_param.id_0xF044 = 1;
    jt808_param.id_0xF045 = 5;
    jt808_param.id_0xF046 = 10;
    jt808_param.id_0xF047 = 15;
    jt808_param.id_0xF048 = 20;
    strcpy(jt808_param.id_0xF049, "60.28.50.210");
    jt808_param.id_0xF04A = 9131;
    rt_kprintf("\n%d>%s", rt_tick_get(), buf);
    sd_write_console(buf);
}

/*********************************************************************************
  *函数名称:static void factory_ex(uint8_t n,uint8_t save_sequ)
  *功能描述:恢复出厂设置
  *输	入:	n			:0,清空所有数据；1，清空数据，并且恢复出厂所有参数
  			save_sequ	:0表示先存储主区域，再存储主备份区域，1表示相反
  *输	出:none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void factory_ex(uint8_t n, uint8_t save_sequ)
{
    uint32_t	i, addr = 58 * 4096;

    char tempbuf[32] = "";
    param_factset_tick	= rt_tick_get();

    rt_kprintf("\n%d>清空=%02X", rt_tick_get(), n);
    if(n & BIT(0))
    {
        param_fact_set();
        param_save_ex( 1 , save_sequ);
        jt808_param_bk.updata_utctime = 0;
        param_save_bksram();
        printer_load_param();
        strcat(tempbuf, "恢复出厂参数.");
    }

    if(n & BIT(1))
    {
        strcat(tempbuf, "清空数据.");
        rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY_2);
        if(rt_tick_get() > RT_TICK_PER_SECOND * 6)
        {
            rt_enter_critical( );
        }
        /*
        for( i = ADDR_DF_PARAM_BASE/4096; i < 1024; i++ )
        {
        	addr	= i*4096;
        	sst25_erase_4k( addr );
        }
        */
        for( addr = ADDR_DF_PARAM_BASE; addr < 0x400000;  )
        {
            if(addr <= ADDR_DF_VDR_BASE - 0x10000)
            {
                sst25_erase_64k( addr );
                addr += 0x10000;
            }
            else if(addr >= ADDR_DF_VDR_END)
            {
                if(addr & 0x0000FFFF)
                {
                    sst25_erase_4k( addr );
                    addr += 0x1000;
                }
                else
                {
                    sst25_erase_64k( addr );
                    addr += 0x10000;
                }
            }
            else
            {
                addr += 0x1000;
            }
        }
        //  GB19056   相关 区域  擦除和初始化
        gb_vdr_erase();

        /*
        for( i = ADDR_DF_PARAM_BASE/0x10000; i < 128; i++ )
        {
        	addr	= i*0x10000;
        	if(addr >= ADDR_DF_FONT_BASE)
        	{
        		break;
        	}
        	else
        	{
        		sst25_erase_64k( addr );
        	}
        }
        */
        if(rt_tick_get() > RT_TICK_PER_SECOND * 6)
        {
            rt_exit_critical();
        }
        rt_sem_release( &sem_dataflash );
        /*
        Cam_format();
        gps_format();
        */
        factory_bksram();
        jt808_gps_data_init( );
    }
    //rt_kprintf("\n%d>%s",rt_tick_get(),tempbuf);
    sd_write_console(tempbuf);
}

/*********************************************************************************
  *函数名称:void factory( uint8_t n )
  *功能描述:恢复出厂设置
  *输	入:	n	:0,清空所有数据；1，清空数据，并且恢复出厂所有参数
  *输	出:none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void factory( uint8_t n )
{
    factory_ex(n, 0);
    reset( 1 );
}
FINSH_FUNCTION_EXPORT( factory, reset to factory );


/*********************************************************************************
  *函数名称:uint8_t param_save_addr( uint8_t sem_get ,uint32_t addr)
  *功能描述:保存设备主要参数到flash中,并且指定了保存在flash中的物理地址
  *输	入:	sem_get	:0表示不需要获取flash操作信号量，1表示需要获取信号量
  			addr	:主要参数的存储位置
  *输	出:	none
  *返 回 值:uint8_t :0表示保存失败，1表示保存成功
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t param_save_addr( uint8_t sem_get , uint32_t addr)
{
    uint8_t i;
    uint16_t crc_wr, crc_rd;
    char	buf[64];
    uint32_t	flash_ver;
    uint32_t	data_ver;
    uint32_t	counter;
    uint32_t	counter_max;
    uint8_t		retry_max;

    if(device_factory_oper)
        retry_max	= 5;
    else
        retry_max	= 20;

    flash_ver 	= BYTESWAP4(JT808_FLASH_FORMA_VER);
    data_ver 	= BYTESWAP4(JT808_FLASH_DATA_VER);
    memcpy(jt808_param.id_0x0000, (uint8_t *)&flash_ver, 4);
    memcpy(jt808_param.id_0x0000 + 4, (uint8_t *)&data_ver, 4);
    crc_wr	= CalcCRC16((uint8_t *)&jt808_param, 0, sizeof( jt808_param ), 0xFFFF );
    for(i = 0; i < retry_max; i++)
    {
        counter = 0;
        counter_max = 300000 * i;		///每次等待时长增加20ms
        while(counter++ < counter_max)	///
        {
            ;
        }
        rt_kprintf("\n%d>MAIN_PARA_SAVE,  ADDR=0x%06X, RETRY=%d", rt_tick_get(), addr, i);
        if(sem_get)
        {
            rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
        }
        SPI_SetSpeed(SPI_SPEED_LOW);
        sst25_erase_4k( addr );
        sst25_write_through( addr, (uint8_t *)&jt808_param, sizeof( jt808_param ) );
        sst25_write_through( addr + 4094, (uint8_t *)&crc_wr, 2 );
        sst25_read( addr, (uint8_t *)&jt808_param_fun, sizeof( jt808_param_fun ) );
        sst25_read( addr + 4094, (uint8_t *)&crc_rd, 2 );
        if(sem_get)
        {
            rt_sem_release( &sem_dataflash );
        }
        if(memcmp((uint8_t *)&jt808_param, (uint8_t *)&jt808_param_fun, sizeof( jt808_param_fun )) == 0)
        {
            if(crc_wr == crc_rd)
                break;
        }
        sst25_init();
    }
    SPI_SetSpeed(SPI_SPEED_HIGH);
    if(i >= retry_max)
    {
        sprintf(buf, "MAIN_PARA_SAVE ERROR! ADDR=0x%06X", addr);
        sd_write_console(buf);
        reset( 1 );
        return  0;
    }
    return 1;
}





/*********************************************************************************
  *函数名称:static uint8_t param_save_ex( uint8_t sem_get ,uint8_t save_sequ)
  *功能描述:保存设备主要参数到flash中,并且指定了保存的先后顺序(主存储区域和备份主存储区域)
  *输	入:	sem_get		:0表示不需要获取flash操作信号量，1表示需要获取信号量
  			save_sequ	:0表示先存储主区域，再存储主备份区域，1表示相反
  *输	出:	none
  *返 回 值:uint8_t :0表示保存失败，其它数字表示保存成功的区域数量，共两个存储区域。
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static uint8_t param_save_ex( uint8_t sem_get , uint8_t save_sequ)
{
    u8 i = 0;
    if(save_sequ)
    {
        i += param_save_addr(sem_get, ADDR_DF_PARAM_MAIN_BK);
        i += param_save_addr(sem_get, ADDR_DF_PARAM_MAIN);
    }
    else
    {
        i += param_save_addr(sem_get, ADDR_DF_PARAM_MAIN);
        i += param_save_addr(sem_get, ADDR_DF_PARAM_MAIN_BK);
    }
    if(i != 2)
    {
        rt_kprintf("\n 设备关键参数保存失败!");
        reset(1);
    }
    else
    {
        rt_kprintf("\n%d>MAIN_PARA_SAVE OK!", rt_tick_get());
        device_unlock = jt808_param.id_0xF00F;
    }
    return i;
}


/*********************************************************************************
  *函数名称:uint8_t param_save( uint8_t sem_get )
  *功能描述:保存设备主要参数到flash中
  *输	入:	sem_get	:0表示不需要获取flash操作信号量，1表示需要获取信号量
  *输	出:	none
  *返 回 值:uint8_t :0表示保存失败，其它数字表示保存成功的区域数量，共两个存储区域。
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t param_save( uint8_t sem_get )
{
    return param_save_ex(sem_get, 0);
}
FINSH_FUNCTION_EXPORT( param_save, save param );


/*********************************************************************************
  *函数名称:void param_load( void )
  *功能描述:从flash中加载设备主要参数，这个时候可以不用sem_dataflash，因为没有其他使用
  *输	入: Read_addr	:加载参数的flash地址
  *输	出: none
  *返 回 值:uint8_t		:0表示数据完全OK,其它含义如下
  						BIT0		表示CRC错误
  						BIT1		表示数据版本不一致，需要升级数据
  						BIT2		表示数据格式完全不匹配，需要重新初始化该区域参数
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t param_load_addr(JT808_PARAM *param, uint32_t Read_addr )
{
    uint8_t		i;
    uint8_t		ver8[16];
    uint32_t	flash_ver;
    uint32_t	data_ver;
    uint8_t		ret;
    uint16_t 	crc, crc_rd;
    uint32_t	counter = 0;
    uint32_t	counter_max = 0;
    uint8_t		retry_max;

    if(device_factory_oper)
        retry_max	= 5;
    else
        retry_max	= 20;
    for(i = 0; i < retry_max; i++)
    {
        ret =  MAIN_PARA_CRC_ERROR | MAIN_PARA_VER_UP | MAIN_PARA_FORMAT_ERR;
        sst25_init( );
        counter = 0;
        counter_max = 300000 * i;		///每次等待时长增加20ms
        while(counter++ < counter_max)	///
        {
            ;
        }
        rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
        SPI_SetSpeed(SPI_SPEED_LOW);
        sst25_read( Read_addr, (uint8_t *)param, sizeof( JT808_PARAM) );
        sst25_read( Read_addr + 4094, (uint8_t *)&crc_rd, 2 );
        rt_sem_release( &sem_dataflash );
        crc = CalcCRC16((uint8_t *)param, 0, sizeof( JT808_PARAM ), 0xFFFF );
        memcpy((uint8_t *)&flash_ver,	(uint8_t *)&param->id_0x0000,	4);
        memcpy((uint8_t *)&data_ver,	(uint8_t *)&param->id_0x0000 + 4,	4);
        flash_ver 	= BYTESWAP4(flash_ver);
        data_ver 	= BYTESWAP4(data_ver);
        if(crc_rd == crc)
        {
            ret &= ~MAIN_PARA_CRC_ERROR;
        }
        if(data_ver == JT808_FLASH_DATA_VER)
        {
            ret &= ~MAIN_PARA_VER_UP;
        }
        if(flash_ver == JT808_FLASH_FORMA_VER)
        {
            ret &= ~MAIN_PARA_FORMAT_ERR;
        }
        SPI_SetSpeed(SPI_SPEED_HIGH);
        if(ret == 0)
        {
            rt_kprintf("\n%d>MAIN_PARA_OK!    ADDR=0x%06X", rt_tick_get(), Read_addr);
            return ret;
        }

    }
    rt_kprintf("\n%d>MAIN_PARA_ERROR! ADDR=0x%06X, ERR=%d", rt_tick_get(), Read_addr, ret );
    return ret;
}


/*********************************************************************************
  *函数名称:void param_load( void )
  *功能描述:从flash中加载设备主要参数，这个时候可以不用sem_dataflash，因为没有其他使用
  *输	入: none
  *输	出: none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-12-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void param_load( void )
{
    /*预读一部分数据*/
    uint8_t		i;
    uint8_t		ver8[16];
    uint32_t	flash_ver;
    uint32_t	data_ver;
    uint32_t	addr;
    uint8_t		ret_a, ret_b;
    uint8_t		ret;
    uint8_t		save_sequ = 0;

    ret_a	= param_load_addr(&jt808_param, ADDR_DF_PARAM_MAIN);
    ret_b	= param_load_addr(&jt808_param_fun, ADDR_DF_PARAM_MAIN_BK);
    if((ret_a == ret_b) && (ret_a == 0))
    {
        device_unlock = jt808_param.id_0xF00F;
        debug_write("设备参数读取OK");
        //rt_kprintf("\n%d>MAIN_PARA_INIT_OK!",rt_tick_get());
        device_flash_err = 0;
        device_factory_oper = 0;
        return;
    }
    debug_write("设备参数读取错误");
    if(ret_a < ret_b)
    {
        save_sequ	= 1;
        //param_load_addr(ADDR_DF_PARAM_MAIN);
        ret = ret_a;
    }
    else
    {
        memcpy((uint8_t *)&jt808_param, (uint8_t *)&jt808_param_fun, sizeof(JT808_PARAM));
        save_sequ	= 0;
        ret = ret_b;
    }
    memcpy((uint8_t *)&data_ver, jt808_param.id_0x0000 + 4, 4);
    if((ret & MAIN_PARA_FORMAT_ERR) || (data_ver < 0x30303030) || (data_ver > JT808_FLASH_DATA_VER))
    {
        if(device_factory_oper == 1)
        {
            lcd_fill(0);
            lcd_text12( 9, 11, "恢复出厂设置中...", 17, 1 );
            lcd_update_all();
            factory_ex(3, save_sequ);
        }
        else
        {
            device_flash_err = 1;//
        }
    }
    else
    {
        switch(data_ver)
        {
        case JT808_FLASH_DATA_VER:
        {
            break;
        }
        }
        param_save_ex( 1, save_sequ);
    }
    device_factory_oper = 0;
    rt_kprintf("\n%d>PARAM VER=%s SIZE=%d\n", rt_tick_get(), jt808_param.id_0x0000, sizeof( jt808_param ) );
}


/*********************************************************************************
  *函数名称:void factory_bkp_sram( void )
  *功能描述:格式化bkp存储区域
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-11-18
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t factory_bksram( void )
{
    memset((uint8_t *)&jt808_param_bk, 0, sizeof(jt808_param_bk));
    jt808_param_bk.format_para	= BKSRAM_FORMAT;
    jt808_param_bk.data_version	= BKSRAM_VERSION;
    param_save_bksram();
}


/*********************************************************************************
  *函数名称:uint8_t param_save_bksram( void )
  *功能描述:backup sram 数据写入定义的结构体
  *输	入:	none
  *输	出:	none
  *返 回 值:uint8_t	:	1:表示操作失败，	0:表示操作成功
  *作	者:白养民
  *创建日期:2013-11-18
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t param_save_bksram( void )
{
    return bkpsram_write( 0, (uint8_t *)&jt808_param_bk, sizeof(jt808_param_bk));
}


/*********************************************************************************
  *函数名称:uint8_t param_load_bksram( void )
  *功能描述:backup sram 数据读取定义的结构体
  *输	入:	none
  *输	出:	none
  *返 回 值:uint8_t	:	1:表示读取数据时发现数据已经丢失，进行了重新格式化，	0:表示读取成功
  *作	者:白养民
  *创建日期:2013-11-18
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t param_load_bksram( void )
{
    u8 write_data = 0;
    bkpsram_read( 0, (uint8_t *)&jt808_param_bk, sizeof(jt808_param_bk));
    if(jt808_param_bk.format_para != BKSRAM_FORMAT)
    {
        memset((uint8_t *)&jt808_param_bk, 0, sizeof(jt808_param_bk));
        jt808_param_bk.format_para	= BKSRAM_FORMAT;
        jt808_param_bk.data_version	= BKSRAM_VERSION;
        jt808_param_bk.car_mileage  = jt808_data.id_0xFA01;
        jt808_param_bk.mileage_pulse = jt808_data.id_0xFA03;
        jt808_param_bk.car_mileage *= 36;
        param_save_bksram();

        return 1;
    }
    else
    {
        switch(jt808_param_bk.data_version)
        {
        case 0x00000001:
        {
            jt808_param_bk.acc_check_state = 0;
            write_data = 1;
        }
        case BKSRAM_VERSION:
        {
            break;
        }
        default:
        {
            break;
        }
        }
    }
    if(write_data)
    {
        jt808_param_bk.data_version	= BKSRAM_VERSION;
        param_save_bksram();
        rt_kprintf("\n bksram 更新参数!");
    }
    //rt_kprintf("\n%d>speed_curr_index=%d",rt_tick_get(),jt808_param_bk.speed_curr_index);
    return 0;
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
uint8_t param_put( uint16_t id, uint8_t len, uint8_t *value )
{
    int			i;
    uint8_t 	*psrc, *pdst;
    uint32_t	u32data;

    for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
    {
        if( id == tbl_id_lookup[i].id )
        {
            if((tbl_id_lookup[i].type <= TYPE_DWORD) && (tbl_id_lookup[i].type) && (len <= 4 ))
            {
                u32data = buf_to_data(value, len);
                if(tbl_id_lookup[i].type == TYPE_BYTE)
                {
                    *tbl_id_lookup[i].val = u32data;
                }
                else if(tbl_id_lookup[i].type == TYPE_WORD)
                {
                    *(uint16_t *)tbl_id_lookup[i].val = u32data;
                }
                else
                {
                    *(uint32_t *)tbl_id_lookup[i].val = u32data;
                }
                return 0;
            }
            else if( tbl_id_lookup[i].type == TYPE_CAN )
            {
                psrc	= value;
                pdst	= tbl_id_lookup[i].val;
                memcpy(pdst, psrc, 8);
                /*
                *pdst++ = *( psrc + 3 );
                *pdst++ = *( psrc + 2 );
                *pdst++ = *( psrc + 1 );
                *pdst++ = *psrc;
                *pdst++ = *( psrc + 7 );
                *pdst++ = *( psrc + 6 );
                *pdst++ = *( psrc + 5 );
                *pdst	= *( psrc + 4 );
                */
                return 0;
            }
            else if(tbl_id_lookup[i].type > TYPE_CAN)
            {
                if(strlen(value) < tbl_id_lookup[i].type)
                {
                    psrc = tbl_id_lookup[i].val;
                    memset(tbl_id_lookup[i].val, 0, tbl_id_lookup[i].type);
                    strncpy( (char *)psrc, (char *)value, strlen(value) );
                    return 0;
                }
            }
        }
    }
    return 1;
}

/*设置参数*/
void param_put_int( uint16_t id, uint32_t value )
{
    uint32_t	i, j;
    uint8_t		*p;

    for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
    {
        if( id == tbl_id_lookup[i].id )
        {
            p		= tbl_id_lookup[i].val;
            j		= value;
            *p++	= ( j & 0xff );
            *p++	= j >> 8;
            *p++	= j >> 16;
            *p		= j >> 24;
            break;
        }
    }
}


/*写入字符串*/
static void param_put_str( uint16_t id, uint8_t *value )
{
    int		i;
    uint8_t *p;

    for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
    {
        if( id == tbl_id_lookup[i].id )
        {
            p = tbl_id_lookup[i].val;
            strncpy( (char *)p, (char *)value, 32 );
            break;
        }
    }
}




uint8_t jt808_param_set(uint16_t id, char *value)
{
    int		i;
    uint32_t u32data;
    uint8_t *p;

    for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
    {
        if( id == tbl_id_lookup[i].id )
        {
            switch(tbl_id_lookup[i].type)
            {
            case TYPE_BYTE:
            case TYPE_WORD:
            case TYPE_DWORD:
                if( sscanf(value, "%d", &u32data) == 1)
                {
                    if(tbl_id_lookup[i].type == TYPE_BYTE)
                    {
                        *tbl_id_lookup[i].val = u32data;
                    }
                    else if(tbl_id_lookup[i].type == TYPE_WORD)
                    {
                        *(uint16_t *)tbl_id_lookup[i].val = u32data;
                    }
                    else
                    {
                        *(uint32_t *)tbl_id_lookup[i].val = u32data;
                    }
                    return 1;
                }
                break;
            case TYPE_CAN:
                if(strlen(value) < 16)
                {
                    p = tbl_id_lookup[i].val;
                    memset(tbl_id_lookup[i].val, 0, 8);
                    Ascii_To_Hex(p, value, 8);
                    //strncpy( (char*)p, (char*)value, strlen(value) );
                    return 1;
                }
                break;
                /*
                case TYPE_STR:
                	if(strlen(value) < 32)
                		{
                		p = tbl_id_lookup[i].val;
                		memset(tbl_id_lookup[i].val,0,32);
                		strncpy( (char*)p, (char*)value, strlen(value) );
                		return 1;
                		}
                	break;
                */
            default :
                if(strlen(value) < tbl_id_lookup[i].type)
                {
                    p = tbl_id_lookup[i].val;
                    memset(tbl_id_lookup[i].val, 0, tbl_id_lookup[i].type);
                    strncpy( (char *)p, (char *)value, strlen(value) );
                    return 1;
                }
                break;
            }
        }
    }
    return  0;
}


static void param_set(uint16_t id, char *value)
{
    if(jt808_param_set(id, value))
    {
        param_save( 1 );
    }
}
FINSH_FUNCTION_EXPORT( param_set, modify param );


static void param_out(uint16_t id)
{
    int		i;
    uint32_t u32data;
    uint8_t *p;
    char buffer[33];

    //param_load_addr(&jt808_param,ADDR_DF_PARAM_MAIN);
    for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
    {
        if(( id == tbl_id_lookup[i].id ) || ( 0 == id ))
        {
            rt_kprintf("\r\n jt808_param.id_0x%04X = ", tbl_id_lookup[i].id );
            switch(tbl_id_lookup[i].type)
            {
            case TYPE_BYTE:
            case TYPE_WORD:
            case TYPE_DWORD:
                if(tbl_id_lookup[i].type == TYPE_BYTE)
                {
                    u32data = *tbl_id_lookup[i].val;
                }
                else if(tbl_id_lookup[i].type == TYPE_WORD)
                {
                    u32data = *(uint16_t *)tbl_id_lookup[i].val;
                }
                else
                {
                    u32data = *(uint32_t *)tbl_id_lookup[i].val;
                }
                rt_kprintf("%u;", u32data);
                break;
            case TYPE_CAN:
                p = tbl_id_lookup[i].val;
                printf_hex_data(p, 8);
                break;
                /*
                case TYPE_STR:
                	memset(buffer,0,sizeof(buffer));
                	memcpy(buffer,tbl_id_lookup[i].val,32);
                	rt_kprintf("\"%s\";",buffer);
                	break;
                */
            default :
                memset(buffer, 0, sizeof(buffer));
                memcpy(buffer, tbl_id_lookup[i].val, tbl_id_lookup[i].type);
                rt_kprintf("\"%s\";", buffer);
                break;
            }
        }
    }
    rt_kprintf("\r\n \r\n" );
}
FINSH_FUNCTION_EXPORT( param_out, modify param );


uint8_t	*jt808_param_get(uint16_t id )
{
    int		i;
    uint32_t u32data;
    uint8_t *p;

    for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
    {
        if( id == tbl_id_lookup[i].id )
        {
            return tbl_id_lookup[i].val;
        }
    }
    return  0;
}


#if 0
/*读取参数,返回参数类型参数*/
uint8_t param_get( uint16_t id, uint8_t *value )
{
    int			i;
    uint8_t		*p;
    uint32_t	val;
    for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
    {
        if( id == tbl_id_lookup[i].id )
        {
            if( tbl_id_lookup[i].type == TYPE_DWORD )
            {
                p = tbl_id_lookup[i].val;
                memcpy( value, p, 4 );
                return 4;
            }

            if( tbl_id_lookup[i].type == TYPE_WORD )
            {
                p = tbl_id_lookup[i].val;
                memcpy( value, p, 2 );
                return 2;
            }

            if( tbl_id_lookup[i].type == TYPE_BYTE )
            {
                p		= tbl_id_lookup[i].val;
                *value	= *p;
                return 1;
            }
            if( tbl_id_lookup[i].type == TYPE_STR )
            {
                p = tbl_id_lookup[i].val;
                memcpy( value, p, strlen( p ) );
                return strlen( p );
            }
            if( tbl_id_lookup[i].type == TYPE_CAN )
            {
                p = tbl_id_lookup[i].val;
                memcpy( value, p, 8 );
                return 8;
            }
        }
    }
    return 0;
}

#endif

/*读取参数*/
uint32_t param_get_int( uint16_t id )
{
    int			i;
    uint8_t		*p;
    uint32_t	val = 0;
    for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
    {
        if( id == tbl_id_lookup[i].id )
        {
            p	= tbl_id_lookup[i].val;
            val |= ( *p++ );
            val |= ( ( *p++ ) << 8 );
            val |= ( ( *p++ ) << 16 );
            val |= ( ( *p ) << 24 );
            break;
        }
    }
    return val;
}


/*打印参数信息*/
void param_dump( void )
{
    int		i, count = 0;
    uint8_t c;
    uint8_t *p = (uint8_t *)&jt808_param;
    uint8_t printbuf[70];
    int32_t len = sizeof( jt808_param );

    while( len > 0 )
    {
        count = ( len < 16 ) ? len : 16;
        memset( printbuf, 0x20, 70 );
        for( i = 0; i < count; i++ )
        {
            c					= *p;
            printbuf[i * 3]		= tbl_hex_to_assic[c >> 4];
            printbuf[i * 3 + 1] = tbl_hex_to_assic[c & 0x0f];
            if( c < 0x20 )
            {
                c = '.';
            }
            if( c > 0x7f )
            {
                c = '.';
            }
            printbuf[50 + i] = c;
            p++;
        }
        printbuf[69] = 0;
        rt_kprintf( "%s\n", printbuf );
        len -= count;
    }
}


/*手动设置apn*/
void apn( uint8_t *s )
{
    param_set( 0x0010, s );
}

FINSH_FUNCTION_EXPORT( apn, set apn );

/*设置主ip port*/
void ipport1( uint8_t *ip, uint8_t *port )
{
    param_set( 0x0013, ip );
    param_set( 0x0018, port );
    param_save( 1 );
}

FINSH_FUNCTION_EXPORT( ipport1, set ipport );

/*获取车辆的mobile终端手机号 6字节,不足12位补数字0*/
void deviceid( uint8_t *s )
{
    uint8_t len, i;
    char	buf[13];
    len = strlen( s );
    memset( buf, '0', 13 );
    if( len >= 12 )
    {
        memcpy( buf, s, 12 );
    }
    else
    {
        memcpy( buf + 12 - len, s, len );
    }
    buf[12] = 0;
    param_set( 0xF002, buf );
    param_save( 1 );
}

FINSH_FUNCTION_EXPORT( deviceid, set deviceid );

static uint16_t id_get = 1; /*保存当前发送的id*/


/*
   读参数数据
 */
uint16_t get_param_and_fill_buf( uint8_t *pdest, uint16_t dest_pos )
{
    uint16_t	i, j;
    uint8_t		*p;
    uint8_t 	*pbuf;
    static uint8_t	last_para_data[64];			//表示当前包的最后一次读取到的参数
    static uint8_t	last_para_len	= 0;				//表示当前包的最后一次读取到的参数长度
    static uint8_t	last_para_rd	= 0;				//表示当前包的最后一次读取到的参数的读取位置

    if(id_get < 3)
    {
        memset(last_para_data, 0, sizeof(last_para_data));
        last_para_len	= 0;
        last_para_rd	= 0;
    }
    for(; last_para_rd < last_para_len;)
    {
        pdest[dest_pos++] = last_para_data[last_para_rd++];
    }

    for( i = id_get; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
    {
        if( tbl_id_lookup[i].id >= 0xF000 )
        {
            continue;
        }
        memset(last_para_data, 0, sizeof(last_para_data));
        pbuf	= last_para_data;
        *pbuf++ = ( tbl_id_lookup[i].id ) >> 24;
        *pbuf++ = ( tbl_id_lookup[i].id ) >> 16;
        *pbuf++ = ( tbl_id_lookup[i].id ) >> 8;
        *pbuf++ = ( tbl_id_lookup[i].id ) & 0xFF;
        last_para_len	= 4;
        last_para_rd	= 0;

        if( tbl_id_lookup[i].type == TYPE_DWORD )
        {
            p		= tbl_id_lookup[i].val;
            *pbuf++ = 4;
            *pbuf++ = p[3];
            *pbuf++ = p[2];
            *pbuf++ = p[1];
            *pbuf++ = p[0];
            last_para_len	+= 5;
        }

        else if( tbl_id_lookup[i].type == TYPE_WORD )
        {
            p		= tbl_id_lookup[i].val;
            *pbuf++ = 2;
            *pbuf++ = p[1];
            *pbuf++ = p[0];
            last_para_len	+= 3;
        }

        else if( tbl_id_lookup[i].type == TYPE_BYTE )
        {
            p		= tbl_id_lookup[i].val;
            *pbuf++ = 1;
            *pbuf++ = *p++;
            last_para_len	+= 2;
        }
        else if( tbl_id_lookup[i].type == TYPE_CAN )
        {
            *pbuf++ = 8;
            p		= tbl_id_lookup[i].val;
            memcpy( pbuf, p, 8 );
            last_para_len	+= 9;
            pbuf	+= 8;
        }
        /*
        else if( tbl_id_lookup[i].type == TYPE_STR )
        {
        	p		= tbl_id_lookup[i].val;
        	*pbuf++ = strlen( (char*)p );
        	memcpy( pbuf, p, strlen( (char*)p ) );
        	last_para_len	+= ( strlen( (char*)p ) + 1 );
        	pbuf	+= strlen( (char*)p );
        }
        */
        else if( tbl_id_lookup[i].type > TYPE_CAN )
        {
            p		= tbl_id_lookup[i].val;
            *pbuf++ = strlen( (char *)p );
            memcpy( pbuf, p, strlen( (char *)p ) );
            last_para_len	+= ( strlen( (char *)p ) + 1 );
            pbuf	+= strlen( (char *)p );
        }
        for(j = 0; j < last_para_len; j++)
        {
            pdest[dest_pos++] = last_para_data[last_para_rd++];
            if(dest_pos >= 512)
            {
                id_get = i + 1;
                return dest_pos;
            }
        }
    }
    id_get = i;
    return dest_pos;
}

/*填充数据*/
void jt808_0x8104_fill_data( JT808_TX_NODEDATA *pnodedata )
{
    uint8_t		buf[600];
    uint16_t	count;
    uint16_t	seg_num;

    count = get_param_and_fill_buf( buf, 0 );             /*字节填数据*/
    rt_kprintf( "\ncount=%d id_get=%d\n", count, id_get );

    pnodedata->packet_no++;
    if( pnodedata->packet_no == pnodedata->packet_num ) /*达到最后一包*/
    {
        pnodedata->timeout = RT_TICK_PER_SECOND * 10;
    }
    seg_num = 0xA000 + pnodedata->packet_no;
    memcpy( pnodedata->tag_data + 16, buf, count );
    pnodedata->retry		= 0;
    pnodedata->msg_len		= count + 16;
    pnodedata->tag_data[2]	= 0x20 + ( count >> 8 );
    pnodedata->tag_data[3]	= count & 0xFF;
    pnodedata->tag_data[10] = seg_num >> 8;
    pnodedata->tag_data[11] = seg_num & 0xFF;
    pnodedata->tag_data[12] = pnodedata->packet_num >> 8;
    pnodedata->tag_data[13] = pnodedata->packet_num & 0xFF;
    pnodedata->tag_data[14] = pnodedata->packet_no >> 8;
    pnodedata->tag_data[15] = pnodedata->packet_no & 0xFF;
    pnodedata->state		= IDLE;
}

/*应答
   天津中心会收到应答
 */
static JT808_MSG_STATE jt808_0x8104_response( JT808_TX_NODEDATA *pnodedata, uint8_t *pmsg )
{
    if( pnodedata->packet_num == pnodedata->packet_no ) /*已经发送了所有包*/
    {
        rt_kprintf( "0x8104_response_delete\n" );
        pnodedata->state = ACK_OK;
        return ACK_OK;
    }
    rt_kprintf( "0x8104_response_idle\n" );
    jt808_0x8104_fill_data( pnodedata );
    return IDLE;
}

/*超时后的处理函数*/
static JT808_MSG_STATE jt808_0x8104_timeout( JT808_TX_NODEDATA *pnodedata )
{
    if( pnodedata->packet_num == pnodedata->packet_no ) /*已经发送了所有包*/
    {
        rt_kprintf( "0x8104_timeout_delete\n" );
        pnodedata->state = ACK_OK;
        return ACK_OK;
    }
    rt_kprintf( "0x8104_timeout_idle\n" );
    jt808_0x8104_fill_data( pnodedata );
    return IDLE;
}

/*上报所有终端参数*/
void jt808_param_0x8104( uint8_t linkno, uint8_t *pmsg )
{
    JT808_TX_NODEDATA *pnodedata;
    //	uint8_t				* pdata;
    //	uint16_t			id;
    uint8_t		buf[600];
    //	uint8_t				*p;
    uint16_t	param_size	= 0;
    uint16_t	param_count = 0;
    uint16_t	i, count;

    pnodedata = node_begin( linkno, MULTI_CMD, 0x0104, 0xA001, 600 );
    if( pnodedata == RT_NULL )
    {
        return;
    }

    memset( buf, 0, sizeof( buf ) );
    /*计算总数和总大小，不统计0x0000和0xFxxx的*/

    for( i = 1; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ) - 1; i++ )
    {
        if( tbl_id_lookup[i].id >= 0xF000 )
        {
            continue;
        }
        param_count++;
        switch( tbl_id_lookup[i].type )
        {
        case TYPE_DWORD:
            param_size += 9;
            break;
        case TYPE_WORD:
            param_size += 7;
            break;
        case  TYPE_BYTE:
            param_size += 6;
            break;
        case TYPE_CAN:
            param_size += 13;
            break;
            /*
            case TYPE_STR:
            	param_size += ( strlen( (char*)( tbl_id_lookup[i].val ) ) + 5 );
            	break;
            */
        default :
            param_size += ( strlen( (char *)( tbl_id_lookup[i].val ) ) + 5 );
            break;
        }
    }
    rt_kprintf( "\ntotal param_count=%d size=%d\n", param_count, param_size );
    pnodedata->packet_num	= ( param_size + 511 ) / 512;   /*默认512分包*/
    pnodedata->packet_no	= 1;
    rt_kprintf( "\npacket_num=%d \n", pnodedata->packet_num );

    id_get	= 1;
    count	= get_param_and_fill_buf( buf , 3 );            /*空出三个字节，填写应答流水号参数总数*/
    rt_kprintf( "\ncount=%d id_get=%d\n", count, id_get );

    buf[0]	= pmsg[10];
    buf[1]	= pmsg[11];
    buf[2]	= param_count;
    node_data( pnodedata, buf, count );
    pnodedata->tag_data[12] = pnodedata->packet_num >> 8;
    pnodedata->tag_data[13] = pnodedata->packet_num & 0xFF;
    pnodedata->tag_data[14] = pnodedata->packet_no >> 8;
    pnodedata->tag_data[15] = pnodedata->packet_no & 0xFF;
    node_end(SINGLE_ACK, pnodedata, jt808_0x8104_timeout, jt808_0x8104_response, RT_NULL );
}


/*分项查询，只应答单包*/
void jt808_param_0x8106( uint8_t linkno, uint8_t *pmsg )
{
    JT808_TX_NODEDATA	 *pnodedata;
    uint8_t				 *pdata;
    uint32_t			id;
    uint8_t				buf[600];
    uint8_t				 *pbuf;
    uint8_t				*p;
    uint16_t			param_size	= 0;
    uint16_t			param_count = 0;
    uint16_t			i, count;

    memset( buf, 0, sizeof( buf ) );
    /*计算总数和总大小，不统计0x0000和0xFxxx的*/
    pdata	= pmsg + 13;    /*指向数据区*/
    count	= 0;
    pbuf	= buf + 3;      /*空出三个字节，填写应答流水号参数总数*/
    for( param_count = 0; param_count < pmsg[12]; param_count++ )
    {
        id	= *pdata++ << 24;
        id	|= *pdata++ << 16;
        id	|= *pdata++ << 8;
        id	|= *pdata++ & 0xFF;

        for( i = 1; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ) - 1; i++ )
        {
            if( tbl_id_lookup[i].id == id )
            {
                *pbuf++ = id >> 24;
                *pbuf++ = id >> 16;
                *pbuf++ = id >> 8;
                *pbuf++ = id & 0xFF;
                count	+= 4;

                if( tbl_id_lookup[i].type == TYPE_DWORD )
                {
                    p		= tbl_id_lookup[i].val;
                    *pbuf++ = 4;
                    *pbuf++ = p[3];
                    *pbuf++ = p[2];
                    *pbuf++ = p[1];
                    *pbuf++ = p[0];
                    count	+= 5;
                }

                else if( tbl_id_lookup[i].type == TYPE_WORD )
                {
                    p		= tbl_id_lookup[i].val;
                    *pbuf++ = 2;
                    *pbuf++ = p[1];
                    *pbuf++ = p[0];
                    count	+= 3;
                }

                else if( tbl_id_lookup[i].type == TYPE_BYTE )
                {
                    p		= tbl_id_lookup[i].val;
                    *pbuf++ = 1;
                    *pbuf++ = *p++;
                    count	+= 2;
                }
                else if( tbl_id_lookup[i].type == TYPE_CAN )
                {
                    *pbuf++ = 8;
                    p		= tbl_id_lookup[i].val;
                    memcpy( pbuf, p, 8 );
                    count	+= 9;
                    pbuf	+= 8;
                }
                /*
                else if( tbl_id_lookup[i].type == TYPE_STR )
                {
                	p		= tbl_id_lookup[i].val;
                	*pbuf++ = strlen( (char*)p );
                	memcpy( pbuf, p, strlen( (char*)p ) );
                	count	+= ( strlen( (char*)p ) + 1 );
                	pbuf	+= strlen( (char*)p );
                }
                */
                else if( tbl_id_lookup[i].type > TYPE_CAN )
                {
                    p		= tbl_id_lookup[i].val;
                    *pbuf++ = strlen( (char *)p );
                    memcpy( pbuf, p, strlen( (char *)p ) );
                    count	+= ( strlen( (char *)p ) + 1 );
                    pbuf	+= strlen( (char *)p );
                }
                if( count > 512 )
                {
                    break;
                }
            }
        }
    }
    rt_kprintf( "\ntotal count=%d size=%d\n", param_count, param_size );

    pnodedata = node_begin( linkno, SINGLE_ACK, 0x0104, -1, 600 );
    if( pnodedata == RT_NULL )
    {
        return;
    }

    buf[0]				= pmsg[10];
    buf[1]				= pmsg[11];
    buf[2]				= param_count;
    pnodedata->timeout	= RT_TICK_PER_SECOND * 5;
    node_data( pnodedata, buf, count + 3 );
    node_end( SINGLE_ACK, pnodedata, jt808_0x8104_timeout, jt808_0x8104_response, RT_NULL );
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
void jt808_param_0x8103( uint8_t linkno, uint8_t *pmsg )
{
    uint8_t		 *p;
    uint8_t		res = 0;
    uint32_t	param_id;
    uint8_t		param_len;
    uint8_t		param_count;
    uint16_t	offset;
    uint16_t	seq, id;

    id	= ( pmsg[0] << 8 ) | pmsg[1];
    seq = ( pmsg[10] << 8 ) | pmsg[11];

    ///接受到参数修改后需要将一些状态值清空，并且在速度发生变化后需要上报0200数据 start
    if(current_speed == 0)
    {
        param_set_speed_state = 2;
    }
    over_time_reset();
    ///接受到参数修改后需要将一些状态值清空，并且在速度发生变化后需要上报0200数据 end
    if( *( pmsg + 2 ) >= 0x20 ) /*如果是多包的设置参数*/
    {
        rt_kprintf( "\n>%s multi packet no support!", __func__ );
        jt808_tx_0x0001_ex(linkno, seq, id, 1 );
    }
    else
    {
        param_count = pmsg[12];
        offset		= 13;
        for( param_count = 0; param_count < pmsg[12]; param_count++ )
        {
            p			= pmsg + offset;
            param_id	= ( p[0] << 24 ) | ( p[1] << 16 ) | ( p[2] << 8 ) | ( p[3] );
            param_len	= p[4];
            res			|= param_put( param_id, param_len, &p[5] );
            offset		+= ( param_len + 5 );
            //rt_kprintf( "\n0x8103>id=%x res=%d \n", param_id, res );
        }

        if( res ) /*有错误*/
        {
            jt808_tx_0x0001_ex(linkno, seq, id, 1 );
        }
        else
        {
            jt808_tx_0x0001_ex(linkno, seq, id, 0 );
            param_save( 1 );
        }
    }
}

void put_car_param(void)
{
    rt_kprintf("\n车辆信息为:&&&&&&&&&&&&&&");
    rt_kprintf("\n 激活状态=%d", jt808_param.id_0xF00F);
    rt_kprintf("\n SIM_ID  =%s", jt808_param.id_0xF006);
    rt_kprintf("\n IP:PORT1=%s:%d", jt808_param.id_0x0013, jt808_param.id_0x0018);
    rt_kprintf("\n IP:PORT2=%s:%d", jt808_param.id_0x0017, jt808_param.id_0xF031);
    rt_kprintf("\n 车辆类型=%s", jt808_param.id_0xF00A);
    if(jt808_param.id_0xF00D == 2)
        rt_kprintf("\n 连接模式=%s", "公共货运");
    else
        rt_kprintf("\n 车牌状态=%s", "两客一危");
    if(jt808_param.id_0xF00E)
        rt_kprintf("\n 车牌状态=%s", "无车牌");
    else
        rt_kprintf("\n 车牌状态=%s", "有车牌");
    rt_kprintf("\n&&&&&&&&&&&&&&");
}

/*********************************************************************************
  *函数名称:void password(uint8_t i)
  *功能描述:激活车台
  *输	入:	i:0表示关闭车台，1表示激活车台
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-09-22
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void password(uint8_t i)
{
    jt808_param.id_0xF00F = i;
    if(i == 0)
    {
        param_fact_set();
        rt_kprintf("\n恢复出厂参数");
    }
    param_save(1);
    put_car_param();
}
FINSH_FUNCTION_EXPORT( password, password );

/*********************************************************************************
  *函数名称:void simid(char *str)
  *功能描述:设置车台SIM卡号码
  *输	入:	str:SIM卡号码
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-09-22
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void simid(char *str)
{
    strcpy(jt808_param.id_0xF006, str);
    jt808_param.id_0xF00F = 1;
    jt808_param.id_0xF00E = 0x01;
    param_save(1);
    put_car_param();
}
FINSH_FUNCTION_EXPORT( simid, simid );

/*********************************************************************************
  *函数名称:void ipset(char *str,uint16_t port)
  *功能描述:设置车台登录IP地址和端口号
  *输	入:	str:IP地址，port端口号
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-09-22
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void ipset(char *str, uint16_t port)
{
    memset(jt808_param.id_0x0013, 0, sizeof(jt808_param.id_0x0013));
    memset(jt808_param.id_0x0017, 0, sizeof(jt808_param.id_0x0017));
    strcpy(jt808_param.id_0x0013, str);
    strcpy(jt808_param.id_0x0017, str);
    jt808_param.id_0x0018 = port;
    jt808_param.id_0xF031 = port;
    param_save(1);
    put_car_param();
}
FINSH_FUNCTION_EXPORT( ipset, ipset);

/*********************************************************************************
  *函数名称:void ipset(char *str,uint16_t port)
  *功能描述:设置车台登录IP地址和端口号
  *输	入:	str:IP地址，port端口号
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-09-22
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void ipset2(char *str, uint16_t port)
{
    memset(jt808_param.id_0x0017, 0, sizeof(jt808_param.id_0x0017));
    strcpy(jt808_param.id_0x0017, str);
    jt808_param.id_0xF031 = port;
    param_save(1);
    put_car_param();
}
FINSH_FUNCTION_EXPORT( ipset2, ipset2);


/*********************************************************************************
  *函数名称:void cartype(char type)
  *功能描述:设置车辆类型，客运或货运车辆
  *输	入:	type:	1两客一危，2货运车辆
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2014-09-22
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void cartype(char type)
{
    const char *ip1 = "60.28.50.210";
    const char *ip2 = "jt1.gghypt.net";
    const char *ip3 = "jt2.gghypt.net";

    memset(jt808_param.id_0xF00A, 0, sizeof(jt808_param.id_0xF00A));
    memset(jt808_param.id_0x0013, 0, sizeof(jt808_param.id_0x0013));
    memset(jt808_param.id_0x0017, 0, sizeof(jt808_param.id_0x0017));
    if(type == 1)
    {
        jt808_param.id_0xF00D = 1;
        strcpy(jt808_param.id_0x0013, ip1);
        strcpy(jt808_param.id_0x0017, ip1);
        jt808_param.id_0x0018 = 9131;
        jt808_param.id_0xF031 = 9131;
        strcpy(jt808_param.id_0xF00A, "客运车");
    }
    else if(type == 2)
    {
        jt808_param.id_0xF00D = 2;
        strcpy(jt808_param.id_0x0013, ip2);
        strcpy(jt808_param.id_0x0017, ip3);
        jt808_param.id_0x0018 = 7008;
        jt808_param.id_0xF031 = 7008;
        strcpy(jt808_param.id_0xF00A, "货运车");

    }
    param_save(1);
    put_car_param();
}
FINSH_FUNCTION_EXPORT( cartype, cartype );


/************************************** The End Of File **************************************/
