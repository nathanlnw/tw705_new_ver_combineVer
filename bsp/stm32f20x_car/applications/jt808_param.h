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
#ifndef _H_JT808_PARAM_
#define _H_JT808_PARAM_

#include "include.h"
#include "jt808_util.h"
#include "jt808_config.h"


#define TYPE_BYTE	1                /*固定为1字节,小端对齐*/
#define TYPE_WORD	2                /*固定为2字节,小端对齐*/
#define TYPE_DWORD	4                /*固定为4字节,小端对齐*/
#define TYPE_CAN	8                /*固定为8字节,当前存储CAN_ID参数*/
#define TYPE_STR	32               /*固定为32字节,网络顺序*/
#define TYPE_L(n)	n

#define HARD_VER_PCB01 		0x05
#define HARD_VER_PCB02 		0x01



/*
   存储区域分配,采用绝对地址,以4K(0x1000)为一个扇区
 */



typedef __packed struct _jt808_param
{
    char		id_0x0000[16];   /*0x0000 版本*/
    uint32_t	id_0x0001;      /*0x0001 心跳发送间隔*/
    uint32_t	id_0x0002;      /*0x0002 TCP应答超时时间*/
    uint32_t	id_0x0003;      /*0x0003 TCP超时重传次数*/
    uint32_t	id_0x0004;      /*0x0004 UDP应答超时时间*/
    uint32_t	id_0x0005;      /*0x0005 UDP超时重传次数*/
    uint32_t	id_0x0006;      /*0x0006 SMS消息应答超时时间*/
    uint32_t	id_0x0007;      /*0x0007 SMS消息重传次数*/
    char		id_0x0010[32];  /*0x0010 主服务器APN*/
    char		id_0x0011[32];  /*0x0011 用户名*/
    char		id_0x0012[32];  /*0x0012 密码*/
    char		id_0x0013[64];  /*0x0013 主服务器地址*/
    char		id_0x0014[32];  /*0x0014 备份APN*/
    char		id_0x0015[32];  /*0x0015 备份用户名*/
    char		id_0x0016[32];  /*0x0016 备份密码*/
    char		id_0x0017[64];  /*0x0017 备份服务器地址，ip或域名*/
    uint32_t	id_0x0018;      /*0x0018 TCP端口*/
    uint32_t	id_0x0019;      /*0x0019 UDP端口*/
    char		id_0x001A[32];  /*0x001A ic卡主服务器地址，ip或域名*/
    uint32_t	id_0x001B;      /*0x001B ic卡服务器TCP端口*/
    uint32_t	id_0x001C;      /*0x001C ic卡服务器UDP端口。目前使用为使用前锁定端口号*/
    char		id_0x001D[32];  /*0x001D ic卡备份服务器地址，ip或域名。目前使用为使用前锁定IP或域名*/
    uint32_t	id_0x0020;      /*0x0020 位置汇报策略*/
    uint32_t	id_0x0021;      /*0x0021 位置汇报方案*/
    uint32_t	id_0x0022;      /*0x0022 驾驶员未登录汇报时间间隔*/
    uint32_t	id_0x0027;      /*0x0027 休眠时汇报时间间隔*/
    uint32_t	id_0x0028;      /*0x0028 紧急报警时汇报时间间隔*/
    uint32_t	id_0x0029;      /*0x0029 缺省时间汇报间隔*/
    uint32_t	id_0x002C;      /*0x002c 缺省距离汇报间隔*/
    uint32_t	id_0x002D;      /*0x002d 驾驶员未登录汇报距离间隔*/
    uint32_t	id_0x002E;      /*0x002e 休眠时距离汇报间隔*/
    uint32_t	id_0x002F;      /*0x002f 紧急时距离汇报间隔*/
    uint32_t	id_0x0030;      /*0x0030 拐点补传角度*/
    uint32_t	id_0x0031;      /*0x0031 电子围栏半径（非法位移阈值），单位为米*/
    char		id_0x0040[32];  /*0x0040 监控平台电话号码*/
    char		id_0x0041[32];  /*0x0041 复位电话号码*/
    char		id_0x0042[32];  /*0x0042 恢复出厂设置电话号码*/
    char		id_0x0043[32];  /*0x0043 监控平台SMS号码*/
    char		id_0x0044[32];  /*0x0044 接收终端SMS文本报警号码*/
    uint32_t	id_0x0045;      /*0x0045 终端接听电话策略*/
    uint32_t	id_0x0046;      /*0x0046 每次通话时长*/
    uint32_t	id_0x0047;      /*0x0047 当月通话时长*/
    char		id_0x0048[32];  /*0x0048 监听电话号码*/
    char		id_0x0049[32];  /*0x0049 监管平台特权短信号码*/
    uint32_t	id_0x0050;      /*0x0050 报警屏蔽字*/
    uint32_t	id_0x0051;      /*0x0051 报警发送文本SMS开关*/
    uint32_t	id_0x0052;      /*0x0052 报警拍照开关*/
    uint32_t	id_0x0053;      /*0x0053 报警拍摄存储标志*/
    uint32_t	id_0x0054;      /*0x0054 关键标志*/
    uint32_t	id_0x0055;      /*0x0055 最高速度kmh*/
    uint32_t	id_0x0056;      /*0x0056 超速持续时间*/
    uint32_t	id_0x0057;      /*0x0057 连续驾驶时间门限*/
    uint32_t	id_0x0058;      /*0x0058 当天累计驾驶时间门限*/
    uint32_t	id_0x0059;      /*0x0059 最小休息时间*/
    uint32_t	id_0x005A;      /*0x005A 最长停车时间*/
    uint32_t	id_0x005B;      /*0x005B 超速报警预警差值，单位为 1/10Km/h */
    uint32_t	id_0x005C;      /*0x005C 疲劳驾驶预警差值，单位为秒（s），>0*/
    uint32_t	id_0x005D;      /*0x005D 碰撞报警参数设置:b7..0：碰撞时间(4ms) b15..8：碰撞加速度(0.1g) 0-79 之间，默认为10 */
    uint32_t	id_0x005E;      /*0x005E 侧翻报警参数设置： 侧翻角度，单位 1 度，默认为 30 度*/
    uint32_t	id_0x0064;      /*0x0064 定时拍照控制*/
    uint32_t	id_0x0065;      /*0x0065 定距拍照控制*/
    uint32_t	id_0x0070;      /*0x0070 图像视频质量(1-10)*/
    uint32_t	id_0x0071;      /*0x0071 亮度*/
    uint32_t	id_0x0072;      /*0x0072 对比度*/
    uint32_t	id_0x0073;      /*0x0073 饱和度*/
    uint32_t	id_0x0074;      /*0x0074 色度*/
    uint32_t	id_0x0080;      /*0x0080 车辆里程表读数0.1km*/
    uint32_t	id_0x0081;      /*0x0081 省域ID*/
    uint32_t	id_0x0082;      /*0x0082 市域ID*/
    char		id_0x0083[32];  /*0x0083 机动车号牌*/
    uint32_t	id_0x0084;      /*0x0084 车牌颜色  0 未上牌 1蓝色 2黄色 3黑色 4白色 9其他*/
    uint32_t	id_0x0090;      /*0x0090 GNSS 定位模式*/
    uint32_t	id_0x0091;      /*0x0091 GNSS 波特率*/
    uint32_t	id_0x0092;      /*0x0092 GNSS 模块详细定位数据输出频率*/
    uint32_t	id_0x0093;      /*0x0093 GNSS 模块详细定位数据采集频率*/
    uint32_t	id_0x0094;      /*0x0094 GNSS 模块详细定位数据上传方式*/
    uint32_t	id_0x0095;      /*0x0095 GNSS 模块详细定位数据上传设置*/
    uint32_t	id_0x0100;      /*0x0100 CAN 总线通道 1 采集时间间隔(ms)，0 表示不采集*/
    uint32_t	id_0x0101;      /*0x0101 CAN 总线通道 1 上传时间间隔(s)，0 表示不上传*/
    uint32_t	id_0x0102;      /*0x0102 CAN 总线通道 2 采集时间间隔(ms)，0 表示不采集*/
    uint32_t	id_0x0103;      /*0x0103 CAN 总线通道 2 上传时间间隔(s)，0 表示不上传*/
    uint8_t		id_0x0110[8];   /*0x0110 CAN 总线 ID 单独采集设置*/
    uint8_t		id_0x0111[8];   /*0x0111 其他CAN 总线 ID 单独采集设置*/
    uint8_t		id_0x0112[8];   /*0x0112 其他CAN 总线 ID 单独采集设置*/
    uint8_t		id_0x0113[8];   /*0x0113 其他CAN 总线 ID 单独采集设置*/
    uint8_t		id_0x0114[8];   /*0x0114 其他CAN 总线 ID 单独采集设置*/
    uint8_t		id_0x0115[8];   /*0x0115 其他CAN 总线 ID 单独采集设置*/
    uint8_t		id_0x0116[8];   /*0x0116 其他CAN 总线 ID 单独采集设置*/
    uint8_t		id_0x0117[8];   /*0x0117 其他CAN 总线 ID 单独采集设置*/
    uint8_t		id_0x0118[8];   /*0x0118 其他CAN 总线 ID 单独采集设置*/
    uint8_t		id_0x0119[8];   /*0x0119 其他CAN 总线 ID 单独采集设置*/

    char		id_0xF000[32];  /*0xF000 制造商ID 5byte*/
    char		id_0xF001[32];  /*0xF001 终端型号 20byte*/
    char		id_0xF002[32];  /*0xF002 终端ID 7byte*/
    char		id_0xF003[32];  /*0xF003 鉴权码*/
    uint16_t	id_0xF004;      /*0xF004 终端类型*/
    char		id_0xF005[32];  /*0xF005 VIN*/
    char		id_0xF006[32];  /*0xF006 CARID 上报的终端手机号，系统原来的mobile */
    char		id_0xF007[32];  /*0xF007 驾驶证代码*/
    char		id_0xF008[32];  /*0xF008 驾驶员姓名*/
    char		id_0xF009[32];  /*0xF009 驾驶证号码*/
    char		id_0xF00A[32];  /*0xF00A 车辆类型*/
    char		id_0xF00B[32];  /*0xF00B 从业资格证*/
    char		id_0xF00C[32];  /*0xF00C 发证机构*/
    uint8_t     id_0xF00D;		//0xF00D 连接模式设置,			2:货运模式   1:两客一危
    uint8_t     id_0xF00E;		//0xF00E 有无车牌号,			1:无车牌号   0:有车牌号，需要设置
    uint8_t     id_0xF00F;		//0xF00F 车辆信息设置是否完成,	0:未设置     1:设置完成,消息头为终端手机号，2:消息头为IMSI号码后12为，3消息头为设备ID

    char		id_0xF010[32];  /*0xF010 软件版本号*/
    char		id_0xF011[9];   /*0xF011 硬件版本号*/
    char		id_0xF012[9];   /*0xF012 销售客户代码*/
    uint32_t	id_0xF013;      /*0xF013 北斗模块型号0,未确定 ,0x3020 0x3017*/
    char		id_0xF014[16];  /*0xF014 车主电话号码*/
    char		id_0xF015[30];  /*0xF015 预留空间，32字节*/

    /*行车记录仪*/
    uint32_t	id_0xF030;      /*0xF030 记录仪初次安装时间,mytime格式*/
    uint32_t	id_0xF031;      /*0xF031 备份TCP端口1*/
    uint32_t	id_0xF032;      /*车辆脉冲系数,有效位数为低16位,表示为每公里的脉冲数，最高位为1表示需要校准，为0表示不需要校准*/
    /*新修改属性的参数*/
    uint8_t 	id_0xF040;      //
    uint8_t 	id_0xF041;      //表示是否采集行驶记录仪数据，为1表示需要采集行驶记录仪数据，为0表示不采集
    //      抽检时该数值为0 ，过检满记录仪数据1  ，  正式该数值为2
    uint8_t 	id_0xF042;      //为0表示不关心ACC的状态，为非0表示当ACC关闭后需要休眠车辆，来省电，1表示关闭GPRS通信，2表示关闭GSM模块，3表示关闭GPS和GSM
    uint8_t 	id_0xF043;      //表示速度模式，0表示为脉冲速度模式，1表示为GPS速度模式
    ////////////////////////
    /*打印相关*/
    //uint8_t 	id_0xF040;      //line_space;               //行间隔
    //uint8_t 	id_0xF041;      //margin_left;				//左边界
    //uint8_t 	id_0xF042;      //margin_right;				//右边界
    //uint8_t 	id_0xF043;      //step_delay;               //步进延时,影响行间隔
    uint8_t 	id_0xF044;      //gray_level;               //灰度等级,加热时间
    uint8_t 	id_0xF045;      //heat_delay[0];			//加热延时
    uint8_t 	id_0xF046;      //heat_delay[1];			//加热延时
    uint8_t 	id_0xF047;      //heat_delay[2];			//加热延时
    uint8_t 	id_0xF048;      //heat_delay[3];			//加热延时
    char		id_0xF049[32];  /*0xF049 备份服务器地址2*/
    uint16_t	id_0xF04A;      /*0xF04A 备份TCP端口2*/
    uint32_t 	id_0xF04B;      //邮箱的面积单位为平方厘米
    uint32_t 	id_0xF04C;      //邮箱的最大深度单位为mm
    uint16_t	id_0xF04D;      //超速过滤值，当车辆速度超过该速度后认为车辆速度非法，用上次的速度代替当前速度。该值为0表示无效，不过滤，单位为0.1km/h
    char		id_0xF04E[88];  /*0xF04E 预留数据区域*/
} JT808_PARAM;





/*
   存储区域分配,采用绝对地址,以4K(0x1000)为一个扇区
 */



typedef __packed struct _jt808_data
{
    uint32_t	id_0xFA00;		/*0xFA00 最后一次写入的utc时间*/
    uint32_t	id_0xFA01;		/*0xFA01 总里程,单位为米,GPS*/
    uint32_t	id_0xFA02;		/*0xFA02 车辆状态*/
    uint32_t	id_0xFA03;		/*0xFA01 总里程,单位为米,脉冲线*/
} STYLE_JT808_DATA;


enum _stop_run
{
    STOP = 0,
    RUN = 1,
};

typedef __packed struct
{
    uint16_t			get_speed;				///根据脉速表计算出的速度,单位为0.1KM/H
    uint32_t			get_speed_tick;			///脉速表上次获取到速度的时刻，单位为tick
    uint32_t			get_pulse_tick;			///脉速表上次获取到脉冲的时刻，单位为tick
    volatile uint32_t	get_speed_cap_sum;		///根据脉速表计算速度多次扑获脉冲的总计数和
    volatile uint16_t	get_speed_cap_num;		///根据脉速表计算速度多次扑获的脉冲次数
    uint32_t			pulse_cap_num;			///脉冲系数校准时扑获的脉冲数量
    long long			pulse_mileage;			///脉冲系数校准开始时的总里程，单位为 100/3600 米,设置这个单位是为了尽量减少累计误差
} TYPE_CAR_DATA;


typedef __packed struct
{
    MYTIME	time;
    uint8_t speed;
} TYPE_15MIN_SPEED;


typedef __packed struct _jt808_param_bkram
{
    uint32_t 	format_para;        		///该值为0x57584448时表示该区域值有效
    uint32_t 	utctime;     				///最后一次写入的UTC时间
    uint32_t 	data_version;     			///当前备份存储区的版本，从0x00000000开始
    uint32_t 	updata_utctime;     		///远程升级结束的UTC时间
    char		update_ip[32];  			///远程升级IP
    uint32_t	update_port;				///远程升级IP Port;

    enum _stop_run	car_stop_run;			//车辆状态，停止还是运行
    uint32_t	utc_car_stop;    			//车辆开始停驶时刻
    uint32_t	utc_car_run;    			//车辆开始行驶时刻

    uint32_t	utc_car_today;				//表示车辆当天统计信息的开始时间
    uint32_t	car_run_time;				//表示车辆当天运行的总时长
    uint32_t	car_alarm;					//车辆报警状态信息
    long long	car_mileage;				//表示车辆的总的行驶里程,单位为 100/3600 米,设置这个单位是为了尽量减少累计误差

    uint32_t	utc_over_speed_start;		//超速开始时间
    uint32_t	utc_over_speed_end;			//超速结束时间
    uint32_t	over_speed_max;				//超速最大速度

    /*疲劳驾驶参数*/
    uint32_t	utc_over_time_start;		//疲劳驾驶开始时间
    uint32_t	utc_over_time_end;			//疲劳驾驶结束时间
    uint32_t 	vdr11_lati_start;			//疲劳驾驶开始纬度
    uint32_t 	vdr11_longi_start;			//疲劳驾驶开始经度
    uint16_t 	vdr11_alti_start;			//疲劳驾驶开始高度
    uint32_t 	vdr11_lati_end;				//疲劳驾驶开始纬度
    uint32_t 	vdr11_longi_end;			//疲劳驾驶开始经度
    uint16_t 	vdr11_alti_end;				//疲劳驾驶开始高度

    TYPE_15MIN_SPEED speed_15min[15];
    uint8_t			speed_curr_index;

    uint32_t	mileage_pulse;				//表示车辆的总的行驶里程,单位为 1米,该里程是通过脉冲线采集到的
    uint32_t	mileage_pulse_start; 		//表示开始打表计费时的里程,单位为 1米,该里程是通过脉冲线采集到的
    uint32_t	mileage_pulse_end; 			//表示结束打表计费时的里程,单位为 1米,该里程是通过脉冲线采集到的
    uint32_t	mileage_pulse_utc_start; 	//表示开始打表计费时的UTC时间
    uint32_t	mileage_pulse_utc_end; 		//表示结束打表计费时的UTC时间
    uint32_t	low_speed_wait_time;		//表示最后一次清空车辆的总的行驶里程后车辆等待的时间,
    uint8_t		tel_month;					//进行通话时长统计的月标记
    uint32_t	tel_timer_this_month;		//这个月呼叫的总时长，当该时长大于当月总时长后将不允许呼叫
    uint8_t		acc_check_state;			//表示检测车辆是否将ACC连接在了车台ACC线的状态，bit0表示检测到了ACC关，bit1表示检测到了ACC开，初始状态为0
} JT808_PARAM_BK;


#if 0
typedef struct
{
    uint16_t	type;           /*终端类型,参见0x0107 终端属性应答*/
    uint8_t		producer_id[5];
    uint8_t		model[20];
    uint8_t		terminal_id[7];
    uint8_t		sim_iccid[10];
    uint8_t		hw_ver_len;
    uint8_t		hw_ver[32];
    uint8_t		sw_ver_len;
    uint8_t		sw_ver[32];
    uint8_t		gnss_attr;  /*gnss属性,参见0x0107 终端属性应答*/
    uint8_t		gsm_attr;   /*gnss属性,参见0x0107 终端属性应答*/
} TERM_PARAM;
#endif
extern unsigned char			printf_on;		///为1表示打印调试消息，

extern uint8_t  SOFT_VER;			///软件版本
extern uint8_t	HARD_VER;			///硬件版本
extern uint8_t			HardWareVerion;				///读取到的PCB板的版本号，对应的口为PA14,PA13,PB3
extern uint16_t 		current_speed;		///单位为km/h,当前速度，可能为GPS速度，也可能为脉冲速度，取决于参数id_0xF043，
extern uint16_t 		current_speed_10x;	///单位为0.1km/h,当前速度，可能为GPS速度，也可能为脉冲速度，取决于参数id_0xF043，
extern uint8_t			param_set_speed_state;	///该值为非零时才有效，1表示速度为0时立刻上报一包数据，2表示速度超过为10km/h时上报一包数据

extern JT808_PARAM 		jt808_param;		///和车辆参数相关以及808协议中定义的所有参数
extern JT808_PARAM_BK 	jt808_param_bk;		///存入bksram中的所有参数，该参数的保存时间为8小时左右
extern STYLE_JT808_DATA	jt808_data;		///保存车辆需要定期保存的数据
extern TYPE_CAR_DATA	car_data;			///车辆相关当前数据
extern uint32_t			param_factset_tick;	///清空设备参数的时间，单位为tick
extern uint16_t			donotsleeptime;		///该值为非0时设备不能休眠，单位为秒，默认值为360表示6分钟不能休眠设备
extern uint8_t			device_unlock;		///说明:0表示车台未解锁，1表示车台已经解锁
extern uint8_t			device_flash_err;	///说明:0表示设备flash正常，1表示设备flash错误
extern uint8_t			device_factory_oper;///说明:0表示正常操作，1表示当前为工厂操作

void 	data_save( void );
void 	data_load( void );
void 	param_fact_set(void);
void 	factory_ex(uint8_t n, uint8_t save_sequ);
uint8_t param_save( uint8_t sem_get );
void 	param_load( void );
uint8_t factory_bksram( void );
uint8_t param_save_bksram( void );
uint8_t param_load_bksram( void );
uint8_t param_put( uint16_t id, uint8_t len, uint8_t *value );
void 	param_put_int( uint16_t id, uint32_t value );
uint8_t param_get( uint16_t id, uint8_t *value );
uint8_t jt808_param_set(uint16_t id, char *value);
uint32_t param_get_int( uint16_t id );
void 	jt808_param_0x8103( uint8_t linkno, uint8_t *pmsg );
void 	jt808_param_0x8104( uint8_t linkno, uint8_t *pmsg );
void 	jt808_param_0x8106( uint8_t linkno, uint8_t *pmsg );
extern uint8_t	*jt808_param_get(uint16_t id );
extern void cartype(char type);
#endif
/************************************** The End Of File **************************************/
