#ifndef _GB_SEND_SERVICE_
#define _GB_SEND_SERVICE_

#include "GB_vdr.h"


#define AVRG15MIN     // 



//     DRV  CMD
//  采集命令
#define  GB_SAMPLE_00H    0x00
#define  GB_SAMPLE_01H    0x01
#define  GB_SAMPLE_02H    0x02
#define  GB_SAMPLE_03H    0x03
#define  GB_SAMPLE_04H    0x04
#define  GB_SAMPLE_05H    0x05
#define  GB_SAMPLE_06H    0x06
#define  GB_SAMPLE_07H    0x07
#define  GB_SAMPLE_08H    0x08
#define  GB_SAMPLE_09H    0x09
#define  GB_SAMPLE_10H    0x10
#define  GB_SAMPLE_11H    0x11
#define  GB_SAMPLE_12H    0x12
#define  GB_SAMPLE_13H    0x13
#define  GB_SAMPLE_14H    0x14
#define  GB_SAMPLE_15H    0x15
//   设置命令
#define  GB_SET_82H      0x82
#define  GB_SET_83H      0x83
#define  GB_SET_84H      0x84
#define  GB_SET_82H      0x82
#define  GB_SET_C2H      0x82
#define  GB_SET_C3H      0x83
#define  GB_SET_C4H      0x84




// 一.  记录停车前15分钟，每分钟平均速度相关
#ifdef AVRG15MIN

//  停车前15 分钟 ，每分钟的平均速度
typedef struct  _Avrg15_SPD
{
    u8 write;
    u8 read; // 停车前15 分钟平均速度记录
    u8 content[105]; // 停车前15分钟平均速度
    u8 Ram_counter;
    u8 savefull_state; // 在ram 中存储的计数器
} AVRG15_SPD;
//   停车前15  分钟，每分钟的平均速度
extern AVRG15_SPD  Avrg_15minSpd;
#endif


// 二.  行车记录仪jt808 上报相关相关  -----
typedef struct  _VDR_JT808_SEND
{
    u16  Total_pkt_num;   // 分包总包数
    u16  Current_pkt_num; // 当前发送包数 从 1  开始
    u16  Read_indexNum;  // 读取记录数目 ，不是包数，(有时读取记录数= 报数*子包数)
    u16  Float_ID;     //  中心下发命令的流水号
    u8   CMD;     //  数据采集

    //-----记录仪列表重传
    u8   RSD_State;     //  重传状态   0 : 重传没有启用   1 :  重传开始    2  : 表示顺序传完但是还没收到中心的重传命令
    u8   RSD_Reader;    //  重传计数器当前数值
    u8   RSD_total;     //  重传选项数目
    u16	ReSdList[256]; //  多媒体重传消息列表
} VDR_JT808_SEND;

//  三.   国标安全警示功能
/*
    5  分钟一组， 每组提示3 次
    a.超时驾驶b.未登录 c . 超时报警 d. 记录仪状态异常
*/
typedef struct _GB_WARN
{
    u8   Warn_state_Enable;
    u8   group_playTimes;      //  每一组播放次数
    u32  FiveMin_sec_counter; //  5  分钟定时器
} GB_WARN;


typedef  struct  _GB_STRKT
{
    u8 Vehicle_sensor; // 车辆传感器状态   0.2s  查询一次
    /*
    D7  刹车
    D6  左转灯
    D5  右转灯
    D4  喇叭
    D3  远光灯
    D2  雨刷
    D1  预留
    D0  预留
    */
    u8  workstate;  // 记录仪串口工作模式  1:enable  2:disable
    u8  RX_CMD;   //  接收命令字
    u8  TX_CMD;   //  发送命令字
    u8  RX_FCS;   //   接收校验
    u8  TX_FCS;   //   发送校验
    u8  rx_buf[128]; //接收字符
    u16  rx_infoLen; //信息块长度
    u8  rx_wr; //
    u8  rx_flag;
    u32 u1_rx_timer;

    //---------usb -----------
    u8  usb_exacute_output;  // 使能输出     0 : disable     1:  enable    2:  usb output   working
    u8  usb_out_selectID; //  idle:  FF    0x00~0x15     0xFE :out put all        0xFB : output  recommond info
    /*
                                 Note:  Recommond info  include  below :
                                    0x00-0x05 + 10 (latest)
                         */
    u8 Usbfilename[50];  //256 byte len  ,long file name
    u8 usb_write_step;  //  写入操作步骤
    u8 usb_xor_fcs;

    u8   DB9_7pin;  //    0: instate    1:   outsate
    u16  DeltaPlus_out_Duration;    //  在指定脉冲系数前提下，输入脉冲的间隔
    u8   Deltaplus_outEnble; //  RTC output    1         signal    output     2      3  output ack   idle=0；

    u32  Plus_tick_counter;

    u32  DoubtData3_counter; //  事故疑点3  计数器
    u8   DoubtData3_triggerFlag; //  事故疑点3    触发 标志位
    u32  Delta_lati;
    u32  Delta_longi;

    // 速度日志相关
    u32  speed_larger40km_counter;  //  速度大于40 km/h   couter      5 分钟内 速度差值，速度差值偏差大于11%(差值除以GPS速度) 认为异常
    // 在范围内认为是正常，每个日历天内至少要判断速度状态1次，同存储速度状态
    u8   speedlog_Day_save_times; //  当前日历天内存储的次数(最大255次) ， 跨天状态清 0
    u32  gps_largeThan40_counter;
    u32  delataSpd_largerThan11_counter; //
    u8   start_record_speedlog_flag;  // 开始记录速度日志状态  每天只能记录1 次

    // 起始时间结束时间
    u32  Query_Start_Time;
    u32  Query_End_Time;
    u16  Max_dataBlocks;
    u16  Total_dataBlocks;
    u16  Send_add;



    //  安全警示
    //   a.超时驾驶b.未登录 c . 超速度报警 d. 记录仪状态异常
    GB_WARN   SPK_DriveExceed_Warn;  	// 超时报警
    GB_WARN	 SPK_UnloginWarn;   // 未登录报警
    GB_WARN	 SPK_Speed_Warn;    //   超速报警
    GB_WARN   SPK_SpeedStatus_Abnormal;      // 速度异常报警
    GB_WARN   SPK_PreTired;    //   疲劳驾驶预报警

    // 检定状态
    u8   Checking_status;   // 进入检定状态标志位

} GB_STRKT;
//----------  事故疑点数据
typedef struct DoubtTYPE
{
    u8  DOUBTspeed;   // 0x00-0xFA  KM/h
    u8  DOUBTstatus;  // status
} DOUBT_TYPE;


//      四 .  GB IC  相关
#define  MAX_DriverIC_num   3

typedef struct _DriveInfo
{
    u8 Effective_Date[3];  // 驾驶证有效期
    u8 DriverCard_ID[19];  // 驾驶员驾驶证号码 18位
    u8 DriveName[22];	   // 驾驶员 姓名21
    u8 Drv_CareerID[21];  // 驾驶员从业资格证20
} DRV_INFO;


typedef struct _DRIVE_STRUCT
{
    DRV_INFO     Driver_BASEinfo;  //  驾驶员基本信息
    u8           Working_state;  //  0:  尚未被启用   1:   被启用但是不是当前驾驶员  2 :  被启用且为当前驾驶员
    u8           Start_Datetime[6];  // 开始时间 BCD
    u8           End_Datetime[6];  // 开始时间 BCD
    u32          Running_counter; // 行驶时间
    u32          Stopping_counter; // 停止行驶结束时间
    u8           H_11_start;  // 1 开始完成 2 。结束完成  3 。存储完成 clear
    u8           H_11_lastSave_state; // 是否存储过
    u8           Lati[4];  //    纬度
    u8           Longi[4]; //	 经度
    u16          Hight;  //  高度

} DRIVE_STRUCT;



//    GB  IC 卡相关
extern DRV_INFO	Read_ICinfo_Reg;   // 临时读取的IC 卡信息
extern DRIVE_STRUCT     Drivers_struct[MAX_DriverIC_num]; // 预留5 个驾驶员的插卡对比



//  记录仪记录相关
extern u8      save_sensorCounter, sensor_writeOverFlag;



extern u8 gb_BCD_datetime[6];
extern struct rt_semaphore GB_RX_sem;  //  gb  接收AA 75
extern GB_STRKT GB19056;
extern u8  Warn_Play_controlBit;
/*
									使能播报状态控制BIT
								  BIT	0:	  驾驶员未登录状态提示(建议默认取消)
								  BIT	1:	  超时预警
								  BIT	2:	  超时报警
								  BIT	3:	  速度异常报警(建议默认取消)
								  BIT	4:	  超速报警
								  BIT	5:
						   */


//  五. 扩展功能	变量
extern u8  Limit_max_SateFlag;	//	 限制 最大速度
extern u16  jia_speed;   // debug




extern VDR_JT808_SEND			VDR_send;                        // 行车记录仪
extern void Send_gb_vdr_Rx_8700_ack( uint8_t linkno, uint8_t *pmsg);
extern void Send_gb_vdr_Rx_8701_ack( uint8_t linkno, uint8_t *pmsg );

/*
     GB19056  相关
*/
extern 	void Virtual_thread_of_GB19056( void);
extern  void GB_DOUBT2_Generate(void);
extern void  GB_doubt_Data3_Trigger(u32  lati_old, u32 longi_old, u32  lati_new, u32 longi_new);
extern void  moni_drv(u8 CMD, u16 delta);
extern void  GB_Drv_init(void);
extern void  Virtual_thread_of_GB19056( void);
extern u8	Api_avrg15minSpd_Content_read(u8 *dest);
extern void  gb_work(u8 value);
extern u32 Time_sec_u32(u8 *instr, u8 len);
extern void gb_100ms_timer(void);




//      GB  IC   相关
extern void  GB_IC_Check_IN(void);
extern void  GB_IC_Check_OUT(void);


#endif
