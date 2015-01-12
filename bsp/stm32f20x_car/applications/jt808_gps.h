/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#ifndef _JT808_GPS_H_
#define _JT808_GPS_H_

#include <rtthread.h>
#include "include.h"
#include "jt808.h"

#define BIT_STATUS_ACC		0x00000001	///0��ACC �أ�1�� ACC ��
#define BIT_STATUS_FIXED	0x00000002	///0��δ��λ��1����λ
#define BIT_STATUS_NS		0x00000004	///0����γ��1����γ
#define BIT_STATUS_EW		0x00000008  ///0��������1������
#define BIT_STATUS_SERVICE	0x00000010  ///0����Ӫ״̬��1��ͣ��״̬
#define BIT_STATUS_ENCRYPT	0x00000020	///0����γ��δ�����ܲ�����ܣ�1����γ���Ѿ����ܲ������

#define BIT_STATUS_EMPTY	0x00000100	///
#define BIT_STATUS_FULL		0x00000200

#define BIT_STATUS_OIL		0x00000400  ///0��������·������1��������·�Ͽ�
#define BIT_STATUS_ELEC		0x00000800  ///0��������·������1��������·�Ͽ�
#define BIT_STATUS_DOORLOCK 0x00001000  ///0�����Ž�����1�����ż���
#define BIT_STATUS_DOOR1	0x00002000	///0���� 1 �أ�1���� 1 ����ǰ�ţ�
#define BIT_STATUS_DOOR2	0x00004000	///0���� 2 �أ�1���� 2 �������ţ�
#define BIT_STATUS_DOOR3	0x00008000	///0���� 3 �أ�1���� 3 �������ţ�
#define BIT_STATUS_DOOR4	0x00010000	///0���� 4 �أ�1���� 4 ������ʻϯ�ţ�
#define BIT_STATUS_DOOR5	0x00020000	///0���� 5 �أ�1���� 5 �����Զ��壩
#define BIT_STATUS_GPS		0x00040000	///0��δʹ�� GPS ���ǽ��ж�λ��1��ʹ�� GPS ���ǽ��ж�λ
#define BIT_STATUS_BD		0x00080000	///0��δʹ�ñ������ǽ��ж�λ��1��ʹ�ñ������ǽ��ж�λ
#define BIT_STATUS_GLONASS	0x00100000	///0��δʹ�� GLONASS ���ǽ��ж�λ��1��ʹ�� GLONASS ���ǽ��ж�λ
#define BIT_STATUS_GALILEO	0x00200000	///0��δʹ�� Galileo ���ǽ��ж�λ��1��ʹ�� Galileo ���ǽ��ж�λ

#define BIT_ALARM_EMG				0x00000001  /*����*/
#define BIT_ALARM_OVERSPEED			0x00000002  /*����*/
#define BIT_ALARM_OVERTIME			0x00000004  /*��ʱ��ƣ�ͼ�ʻ*/
#define BIT_ALARM_DANGER			0x00000008  /*Σ��Ԥ��*/
#define BIT_ALARM_GPS_ERR			0x00000010  /*GNSSģ�����*/
#define BIT_ALARM_GPS_OPEN			0x00000020  /*���߿�·*/
#define BIT_ALARM_GPS_SHORT			0x00000040  /*���߶�·*/
#define BIT_ALARM_LOW_PWR			0x00000080  /*�ն�����ԴǷѹ*/
#define BIT_ALARM_LOST_PWR			0x00000100  /*�ն�����Դ����*/
#define BIT_ALARM_FAULT_LCD			0x00000200  /*LCD����*/
#define BIT_ALARM_FAULT_TTS			0x00000400  /*TTS����*/
#define BIT_ALARM_FAULT_CAM			0x00000800  /*CAM����*/
#define BIT_ALARM_FAULT_ICCARD		0x00001000  /*IC������*/
#define BIT_ALARM_PRE_OVERSPEED		0x00002000  /*����Ԥ�� bit13*/
#define BIT_ALARM_PRE_OVERTIME		0x00004000  /*��ʱԤ�� bit14*/
#define BIT_ALARM_TODAY_OVERTIME	0x00040000  /*�����ۼƼ�ʻ��ʱ*/
#define BIT_ALARM_STOP_OVERTIME		0x00080000  /*��ʱͣʻ*/
#define BIT_ALARM_DEVIATE			0x00800000  /*��·ƫ�Ʊ���*/
#define BIT_ALARM_VSS				0x01000000  /*VSS����*/
#define BIT_ALARM_OIL				0x02000000  /*�����쳣*/
#define BIT_ALARM_STOLEN			0x04000000  /*����*/
#define BIT_ALARM_IGNITION			0x08000000  /*�Ƿ����*/
#define BIT_ALARM_MOVE				0x10000000  /*�Ƿ���λ*/
#define BIT_ALARM_COLLIDE			0x20000000  /*��ײ*/
#define BIT_ALARM_TILT				0x40000000  /*�෭*/
#define BIT_ALARM_DOOR_OPEN			0x80000000  /*�Ƿ�����*/

/*����λ����Ϣ,��Ϊ�ֽڶ���ķ�ʽ������ʹ�����鷽��*/
typedef __packed struct _gps_baseinfo
{
    uint32_t	alarm;
    uint32_t	status;
    uint32_t	latitude;                       /*γ��*/
    uint32_t	longitude;                      /*����*/
    uint16_t	altitude;						/*�߶�*/
    uint16_t	speed_10x;                      /*�Ե��ٶ� 0.1KMH*/
    uint16_t	cog;                            /*�ԵؽǶ�*/
    uint8_t		datetime[6];                    /*BCD��ʽ*/
    uint8_t		mileage_id;                     /*������Ϣ_���ID*/
    uint8_t		mileage_len;                    /*������Ϣ_��̳���*/
    uint32_t	mileage;                   		/*������Ϣ_���*/
    uint8_t		csq_id;                     	/*������Ϣ_����ͨ����ID*/
    uint8_t		csq_len;                    	/*������Ϣ_����ͨ��������*/
    uint8_t		csq;                   			/*������Ϣ_����ͨ�����ź�ǿ��*/
    uint8_t		NoSV_id;                     	/*������Ϣ_��λ��������ID*/
    uint8_t		NoSV_len;                    	/*������Ϣ_��λ������������*/
    uint8_t		NoSV;                   		/*������Ϣ_��λ��������*/
} GPS_BASEINFO;

enum BDGPS_MODE
{
    MODE_GET = 0,                               /*��ѯ*/
    MODE_BD = 1,
    MODE_GPS,
    MODE_BDGPS,
};

typedef  struct  _gps_status
{
    uint32_t		type;           /*�ͺ� 0:δ�� 0x3020x  0x3017x*/
    enum BDGPS_MODE mode;           /* 1: BD   2:  GPS   3: BD+GPS    ��λģ���״̬*/
    uint8_t			Antenna_Flag;   //��ʾ��ʾ��·
    uint8_t			Raw_Output;     //  ԭʼ�������
    uint8_t			NoSV;			//��λ��������
} GPS_STATUS;

extern GPS_STATUS	gps_status;

extern GPS_BASEINFO gps_baseinfo;

extern uint32_t		gps_lati;               /*�ڲ���С�� γ�� 10-E6��*/
extern uint32_t		gps_longi;              /*�ڲ���С�� ���� 10-E6��*/
extern uint16_t		gps_speed;              /*�ٶ� 0.1kmh*/
extern uint16_t		gps_cog;                /*�Եط����*/
extern uint16_t		gps_alti;               /*�߶�*/
extern uint8_t		gps_datetime[6];        /*����ʱ�� hex��ʽ*/

/*�澯��״̬��Ϣ*/
extern uint32_t jt808_alarm;
extern uint32_t jt808_status;

extern uint32_t gps_sec_count;              /*gps���������*/
extern uint32_t utc_now;
extern MYTIME	mytime_now;
extern uint8_t	save_0x0200_data;		//Ϊ0������0200���ݣ�Ϊ1��Ҫ����0200����

extern uint16_t jt808_8202_track_interval;  /*jt808_8202 ��ʱλ�ø��ٿ���*/
extern uint32_t jt808_8202_track_duration;
extern uint16_t jt808_8202_track_counter;

extern uint32_t jt808_8203_manual_ack_seq;  /*�˹�ȷ�ϱ����ı�ʶλ 0,3,20,21,22,27,28*/
extern uint16_t jt808_8203_manual_ack_value;

extern uint32_t gps_notfixed_count;

void gps_rx( uint8_t *pinfo, uint16_t length );

void gps_fact_set(void);
void jt808_gps_data_init( void );
uint8_t jt808_report_get( void );
void jt808_report_init( void );



#endif
/************************************** The End Of File **************************************/
