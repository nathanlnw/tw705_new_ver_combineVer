#ifndef _GB_SEND_SERVICE_
#define _GB_SEND_SERVICE_

#include "GB_vdr.h"


#define AVRG15MIN     // 



//     DRV  CMD
//  �ɼ�����
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
//   ��������
#define  GB_SET_82H      0x82
#define  GB_SET_83H      0x83
#define  GB_SET_84H      0x84
#define  GB_SET_82H      0x82
#define  GB_SET_C2H      0x82
#define  GB_SET_C3H      0x83
#define  GB_SET_C4H      0x84




// һ.  ��¼ͣ��ǰ15���ӣ�ÿ����ƽ���ٶ����
#ifdef AVRG15MIN

//  ͣ��ǰ15 ���� ��ÿ���ӵ�ƽ���ٶ�
typedef struct  _Avrg15_SPD
{
    u8 write;
    u8 read; // ͣ��ǰ15 ����ƽ���ٶȼ�¼
    u8 content[105]; // ͣ��ǰ15����ƽ���ٶ�
    u8 Ram_counter;
    u8 savefull_state; // ��ram �д洢�ļ�����
} AVRG15_SPD;
//   ͣ��ǰ15  ���ӣ�ÿ���ӵ�ƽ���ٶ�
extern AVRG15_SPD  Avrg_15minSpd;
#endif


// ��.  �г���¼��jt808 �ϱ�������  -----
typedef struct  _VDR_JT808_SEND
{
    u16  Total_pkt_num;   // �ְ��ܰ���
    u16  Current_pkt_num; // ��ǰ���Ͱ��� �� 1  ��ʼ
    u16  Read_indexNum;  // ��ȡ��¼��Ŀ �����ǰ�����(��ʱ��ȡ��¼��= ����*�Ӱ���)
    u16  Float_ID;     //  �����·��������ˮ��
    u8   CMD;     //  ���ݲɼ�

    //-----��¼���б��ش�
    u8   RSD_State;     //  �ش�״̬   0 : �ش�û������   1 :  �ش���ʼ    2  : ��ʾ˳���굫�ǻ�û�յ����ĵ��ش�����
    u8   RSD_Reader;    //  �ش���������ǰ��ֵ
    u8   RSD_total;     //  �ش�ѡ����Ŀ
    u16	ReSdList[256]; //  ��ý���ش���Ϣ�б�
} VDR_JT808_SEND;

//  ��.   ���갲ȫ��ʾ����
/*
    5  ����һ�飬 ÿ����ʾ3 ��
    a.��ʱ��ʻb.δ��¼ c . ��ʱ���� d. ��¼��״̬�쳣
*/
typedef struct _GB_WARN
{
    u8   Warn_state_Enable;
    u8   group_playTimes;      //  ÿһ�鲥�Ŵ���
    u32  FiveMin_sec_counter; //  5  ���Ӷ�ʱ��
} GB_WARN;


typedef  struct  _GB_STRKT
{
    u8 Vehicle_sensor; // ����������״̬   0.2s  ��ѯһ��
    /*
    D7  ɲ��
    D6  ��ת��
    D5  ��ת��
    D4  ����
    D3  Զ���
    D2  ��ˢ
    D1  Ԥ��
    D0  Ԥ��
    */
    u8  workstate;  // ��¼�Ǵ��ڹ���ģʽ  1:enable  2:disable
    u8  RX_CMD;   //  ����������
    u8  TX_CMD;   //  ����������
    u8  RX_FCS;   //   ����У��
    u8  TX_FCS;   //   ����У��
    u8  rx_buf[128]; //�����ַ�
    u16  rx_infoLen; //��Ϣ�鳤��
    u8  rx_wr; //
    u8  rx_flag;
    u32 u1_rx_timer;

    //---------usb -----------
    u8  usb_exacute_output;  // ʹ�����     0 : disable     1:  enable    2:  usb output   working
    u8  usb_out_selectID; //  idle:  FF    0x00~0x15     0xFE :out put all        0xFB : output  recommond info
    /*
                                 Note:  Recommond info  include  below :
                                    0x00-0x05 + 10 (latest)
                         */
    u8 Usbfilename[50];  //256 byte len  ,long file name
    u8 usb_write_step;  //  д���������
    u8 usb_xor_fcs;

    u8   DB9_7pin;  //    0: instate    1:   outsate
    u16  DeltaPlus_out_Duration;    //  ��ָ������ϵ��ǰ���£���������ļ��
    u8   Deltaplus_outEnble; //  RTC output    1         signal    output     2      3  output ack   idle=0��

    u32  Plus_tick_counter;

    u32  DoubtData3_counter; //  �¹��ɵ�3  ������
    u8   DoubtData3_triggerFlag; //  �¹��ɵ�3    ���� ��־λ
    u32  Delta_lati;
    u32  Delta_longi;

    // �ٶ���־���
    u32  speed_larger40km_counter;  //  �ٶȴ���40 km/h   couter      5 ������ �ٶȲ�ֵ���ٶȲ�ֵƫ�����11%(��ֵ����GPS�ٶ�) ��Ϊ�쳣
    // �ڷ�Χ����Ϊ��������ÿ��������������Ҫ�ж��ٶ�״̬1�Σ�ͬ�洢�ٶ�״̬
    u8   speedlog_Day_save_times; //  ��ǰ�������ڴ洢�Ĵ���(���255��) �� ����״̬�� 0
    u32  gps_largeThan40_counter;
    u32  delataSpd_largerThan11_counter; //
    u8   start_record_speedlog_flag;  // ��ʼ��¼�ٶ���־״̬  ÿ��ֻ�ܼ�¼1 ��

    // ��ʼʱ�����ʱ��
    u32  Query_Start_Time;
    u32  Query_End_Time;
    u16  Max_dataBlocks;
    u16  Total_dataBlocks;
    u16  Send_add;



    //  ��ȫ��ʾ
    //   a.��ʱ��ʻb.δ��¼ c . ���ٶȱ��� d. ��¼��״̬�쳣
    GB_WARN   SPK_DriveExceed_Warn;  	// ��ʱ����
    GB_WARN	 SPK_UnloginWarn;   // δ��¼����
    GB_WARN	 SPK_Speed_Warn;    //   ���ٱ���
    GB_WARN   SPK_SpeedStatus_Abnormal;      // �ٶ��쳣����
    GB_WARN   SPK_PreTired;    //   ƣ�ͼ�ʻԤ����

    // �춨״̬
    u8   Checking_status;   // ����춨״̬��־λ

} GB_STRKT;
//----------  �¹��ɵ�����
typedef struct DoubtTYPE
{
    u8  DOUBTspeed;   // 0x00-0xFA  KM/h
    u8  DOUBTstatus;  // status
} DOUBT_TYPE;


//      �� .  GB IC  ���
#define  MAX_DriverIC_num   3

typedef struct _DriveInfo
{
    u8 Effective_Date[3];  // ��ʻ֤��Ч��
    u8 DriverCard_ID[19];  // ��ʻԱ��ʻ֤���� 18λ
    u8 DriveName[22];	   // ��ʻԱ ����21
    u8 Drv_CareerID[21];  // ��ʻԱ��ҵ�ʸ�֤20
} DRV_INFO;


typedef struct _DRIVE_STRUCT
{
    DRV_INFO     Driver_BASEinfo;  //  ��ʻԱ������Ϣ
    u8           Working_state;  //  0:  ��δ������   1:   �����õ��ǲ��ǵ�ǰ��ʻԱ  2 :  ��������Ϊ��ǰ��ʻԱ
    u8           Start_Datetime[6];  // ��ʼʱ�� BCD
    u8           End_Datetime[6];  // ��ʼʱ�� BCD
    u32          Running_counter; // ��ʻʱ��
    u32          Stopping_counter; // ֹͣ��ʻ����ʱ��
    u8           H_11_start;  // 1 ��ʼ��� 2 ���������  3 ���洢��� clear
    u8           H_11_lastSave_state; // �Ƿ�洢��
    u8           Lati[4];  //    γ��
    u8           Longi[4]; //	 ����
    u16          Hight;  //  �߶�

} DRIVE_STRUCT;



//    GB  IC �����
extern DRV_INFO	Read_ICinfo_Reg;   // ��ʱ��ȡ��IC ����Ϣ
extern DRIVE_STRUCT     Drivers_struct[MAX_DriverIC_num]; // Ԥ��5 ����ʻԱ�Ĳ忨�Ա�



//  ��¼�Ǽ�¼���
extern u8      save_sensorCounter, sensor_writeOverFlag;



extern u8 gb_BCD_datetime[6];
extern struct rt_semaphore GB_RX_sem;  //  gb  ����AA 75
extern GB_STRKT GB19056;
extern u8  Warn_Play_controlBit;
/*
									ʹ�ܲ���״̬����BIT
								  BIT	0:	  ��ʻԱδ��¼״̬��ʾ(����Ĭ��ȡ��)
								  BIT	1:	  ��ʱԤ��
								  BIT	2:	  ��ʱ����
								  BIT	3:	  �ٶ��쳣����(����Ĭ��ȡ��)
								  BIT	4:	  ���ٱ���
								  BIT	5:
						   */


//  ��. ��չ����	����
extern u8  Limit_max_SateFlag;	//	 ���� ����ٶ�
extern u16  jia_speed;   // debug




extern VDR_JT808_SEND			VDR_send;                        // �г���¼��
extern void Send_gb_vdr_Rx_8700_ack( uint8_t linkno, uint8_t *pmsg);
extern void Send_gb_vdr_Rx_8701_ack( uint8_t linkno, uint8_t *pmsg );

/*
     GB19056  ���
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




//      GB  IC   ���
extern void  GB_IC_Check_IN(void);
extern void  GB_IC_Check_OUT(void);


#endif
