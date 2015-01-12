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
#ifndef _H_JT808_PARAM_
#define _H_JT808_PARAM_

#include "include.h"
#include "jt808_util.h"
#include "jt808_config.h"


#define TYPE_BYTE	1                /*�̶�Ϊ1�ֽ�,С�˶���*/
#define TYPE_WORD	2                /*�̶�Ϊ2�ֽ�,С�˶���*/
#define TYPE_DWORD	4                /*�̶�Ϊ4�ֽ�,С�˶���*/
#define TYPE_CAN	8                /*�̶�Ϊ8�ֽ�,��ǰ�洢CAN_ID����*/
#define TYPE_STR	32               /*�̶�Ϊ32�ֽ�,����˳��*/
#define TYPE_L(n)	n

#define HARD_VER_PCB01 		0x05
#define HARD_VER_PCB02 		0x01



/*
   �洢�������,���þ��Ե�ַ,��4K(0x1000)Ϊһ������
 */



typedef __packed struct _jt808_param
{
    char		id_0x0000[16];   /*0x0000 �汾*/
    uint32_t	id_0x0001;      /*0x0001 �������ͼ��*/
    uint32_t	id_0x0002;      /*0x0002 TCPӦ��ʱʱ��*/
    uint32_t	id_0x0003;      /*0x0003 TCP��ʱ�ش�����*/
    uint32_t	id_0x0004;      /*0x0004 UDPӦ��ʱʱ��*/
    uint32_t	id_0x0005;      /*0x0005 UDP��ʱ�ش�����*/
    uint32_t	id_0x0006;      /*0x0006 SMS��ϢӦ��ʱʱ��*/
    uint32_t	id_0x0007;      /*0x0007 SMS��Ϣ�ش�����*/
    char		id_0x0010[32];  /*0x0010 ��������APN*/
    char		id_0x0011[32];  /*0x0011 �û���*/
    char		id_0x0012[32];  /*0x0012 ����*/
    char		id_0x0013[64];  /*0x0013 ����������ַ*/
    char		id_0x0014[32];  /*0x0014 ����APN*/
    char		id_0x0015[32];  /*0x0015 �����û���*/
    char		id_0x0016[32];  /*0x0016 ��������*/
    char		id_0x0017[64];  /*0x0017 ���ݷ�������ַ��ip������*/
    uint32_t	id_0x0018;      /*0x0018 TCP�˿�*/
    uint32_t	id_0x0019;      /*0x0019 UDP�˿�*/
    char		id_0x001A[32];  /*0x001A ic������������ַ��ip������*/
    uint32_t	id_0x001B;      /*0x001B ic��������TCP�˿�*/
    uint32_t	id_0x001C;      /*0x001C ic��������UDP�˿ڡ�Ŀǰʹ��Ϊʹ��ǰ�����˿ں�*/
    char		id_0x001D[32];  /*0x001D ic�����ݷ�������ַ��ip��������Ŀǰʹ��Ϊʹ��ǰ����IP������*/
    uint32_t	id_0x0020;      /*0x0020 λ�û㱨����*/
    uint32_t	id_0x0021;      /*0x0021 λ�û㱨����*/
    uint32_t	id_0x0022;      /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
    uint32_t	id_0x0027;      /*0x0027 ����ʱ�㱨ʱ����*/
    uint32_t	id_0x0028;      /*0x0028 ��������ʱ�㱨ʱ����*/
    uint32_t	id_0x0029;      /*0x0029 ȱʡʱ��㱨���*/
    uint32_t	id_0x002C;      /*0x002c ȱʡ����㱨���*/
    uint32_t	id_0x002D;      /*0x002d ��ʻԱδ��¼�㱨������*/
    uint32_t	id_0x002E;      /*0x002e ����ʱ����㱨���*/
    uint32_t	id_0x002F;      /*0x002f ����ʱ����㱨���*/
    uint32_t	id_0x0030;      /*0x0030 �յ㲹���Ƕ�*/
    uint32_t	id_0x0031;      /*0x0031 ����Χ���뾶���Ƿ�λ����ֵ������λΪ��*/
    char		id_0x0040[32];  /*0x0040 ���ƽ̨�绰����*/
    char		id_0x0041[32];  /*0x0041 ��λ�绰����*/
    char		id_0x0042[32];  /*0x0042 �ָ��������õ绰����*/
    char		id_0x0043[32];  /*0x0043 ���ƽ̨SMS����*/
    char		id_0x0044[32];  /*0x0044 �����ն�SMS�ı���������*/
    uint32_t	id_0x0045;      /*0x0045 �ն˽����绰����*/
    uint32_t	id_0x0046;      /*0x0046 ÿ��ͨ��ʱ��*/
    uint32_t	id_0x0047;      /*0x0047 ����ͨ��ʱ��*/
    char		id_0x0048[32];  /*0x0048 �����绰����*/
    char		id_0x0049[32];  /*0x0049 ���ƽ̨��Ȩ���ź���*/
    uint32_t	id_0x0050;      /*0x0050 ����������*/
    uint32_t	id_0x0051;      /*0x0051 ���������ı�SMS����*/
    uint32_t	id_0x0052;      /*0x0052 �������տ���*/
    uint32_t	id_0x0053;      /*0x0053 ��������洢��־*/
    uint32_t	id_0x0054;      /*0x0054 �ؼ���־*/
    uint32_t	id_0x0055;      /*0x0055 ����ٶ�kmh*/
    uint32_t	id_0x0056;      /*0x0056 ���ٳ���ʱ��*/
    uint32_t	id_0x0057;      /*0x0057 ������ʻʱ������*/
    uint32_t	id_0x0058;      /*0x0058 �����ۼƼ�ʻʱ������*/
    uint32_t	id_0x0059;      /*0x0059 ��С��Ϣʱ��*/
    uint32_t	id_0x005A;      /*0x005A �ͣ��ʱ��*/
    uint32_t	id_0x005B;      /*0x005B ���ٱ���Ԥ����ֵ����λΪ 1/10Km/h */
    uint32_t	id_0x005C;      /*0x005C ƣ�ͼ�ʻԤ����ֵ����λΪ�루s����>0*/
    uint32_t	id_0x005D;      /*0x005D ��ײ������������:b7..0����ײʱ��(4ms) b15..8����ײ���ٶ�(0.1g) 0-79 ֮�䣬Ĭ��Ϊ10 */
    uint32_t	id_0x005E;      /*0x005E �෭�����������ã� �෭�Ƕȣ���λ 1 �ȣ�Ĭ��Ϊ 30 ��*/
    uint32_t	id_0x0064;      /*0x0064 ��ʱ���տ���*/
    uint32_t	id_0x0065;      /*0x0065 �������տ���*/
    uint32_t	id_0x0070;      /*0x0070 ͼ����Ƶ����(1-10)*/
    uint32_t	id_0x0071;      /*0x0071 ����*/
    uint32_t	id_0x0072;      /*0x0072 �Աȶ�*/
    uint32_t	id_0x0073;      /*0x0073 ���Ͷ�*/
    uint32_t	id_0x0074;      /*0x0074 ɫ��*/
    uint32_t	id_0x0080;      /*0x0080 ������̱����0.1km*/
    uint32_t	id_0x0081;      /*0x0081 ʡ��ID*/
    uint32_t	id_0x0082;      /*0x0082 ����ID*/
    char		id_0x0083[32];  /*0x0083 ����������*/
    uint32_t	id_0x0084;      /*0x0084 ������ɫ  0 δ���� 1��ɫ 2��ɫ 3��ɫ 4��ɫ 9����*/
    uint32_t	id_0x0090;      /*0x0090 GNSS ��λģʽ*/
    uint32_t	id_0x0091;      /*0x0091 GNSS ������*/
    uint32_t	id_0x0092;      /*0x0092 GNSS ģ����ϸ��λ�������Ƶ��*/
    uint32_t	id_0x0093;      /*0x0093 GNSS ģ����ϸ��λ���ݲɼ�Ƶ��*/
    uint32_t	id_0x0094;      /*0x0094 GNSS ģ����ϸ��λ�����ϴ���ʽ*/
    uint32_t	id_0x0095;      /*0x0095 GNSS ģ����ϸ��λ�����ϴ�����*/
    uint32_t	id_0x0100;      /*0x0100 CAN ����ͨ�� 1 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
    uint32_t	id_0x0101;      /*0x0101 CAN ����ͨ�� 1 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
    uint32_t	id_0x0102;      /*0x0102 CAN ����ͨ�� 2 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
    uint32_t	id_0x0103;      /*0x0103 CAN ����ͨ�� 2 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
    uint8_t		id_0x0110[8];   /*0x0110 CAN ���� ID �����ɼ�����*/
    uint8_t		id_0x0111[8];   /*0x0111 ����CAN ���� ID �����ɼ�����*/
    uint8_t		id_0x0112[8];   /*0x0112 ����CAN ���� ID �����ɼ�����*/
    uint8_t		id_0x0113[8];   /*0x0113 ����CAN ���� ID �����ɼ�����*/
    uint8_t		id_0x0114[8];   /*0x0114 ����CAN ���� ID �����ɼ�����*/
    uint8_t		id_0x0115[8];   /*0x0115 ����CAN ���� ID �����ɼ�����*/
    uint8_t		id_0x0116[8];   /*0x0116 ����CAN ���� ID �����ɼ�����*/
    uint8_t		id_0x0117[8];   /*0x0117 ����CAN ���� ID �����ɼ�����*/
    uint8_t		id_0x0118[8];   /*0x0118 ����CAN ���� ID �����ɼ�����*/
    uint8_t		id_0x0119[8];   /*0x0119 ����CAN ���� ID �����ɼ�����*/

    char		id_0xF000[32];  /*0xF000 ������ID 5byte*/
    char		id_0xF001[32];  /*0xF001 �ն��ͺ� 20byte*/
    char		id_0xF002[32];  /*0xF002 �ն�ID 7byte*/
    char		id_0xF003[32];  /*0xF003 ��Ȩ��*/
    uint16_t	id_0xF004;      /*0xF004 �ն�����*/
    char		id_0xF005[32];  /*0xF005 VIN*/
    char		id_0xF006[32];  /*0xF006 CARID �ϱ����ն��ֻ��ţ�ϵͳԭ����mobile */
    char		id_0xF007[32];  /*0xF007 ��ʻ֤����*/
    char		id_0xF008[32];  /*0xF008 ��ʻԱ����*/
    char		id_0xF009[32];  /*0xF009 ��ʻ֤����*/
    char		id_0xF00A[32];  /*0xF00A ��������*/
    char		id_0xF00B[32];  /*0xF00B ��ҵ�ʸ�֤*/
    char		id_0xF00C[32];  /*0xF00C ��֤����*/
    uint8_t     id_0xF00D;		//0xF00D ����ģʽ����,			2:����ģʽ   1:����һΣ
    uint8_t     id_0xF00E;		//0xF00E ���޳��ƺ�,			1:�޳��ƺ�   0:�г��ƺţ���Ҫ����
    uint8_t     id_0xF00F;		//0xF00F ������Ϣ�����Ƿ����,	0:δ����     1:�������,��ϢͷΪ�ն��ֻ��ţ�2:��ϢͷΪIMSI�����12Ϊ��3��ϢͷΪ�豸ID

    char		id_0xF010[32];  /*0xF010 ����汾��*/
    char		id_0xF011[9];   /*0xF011 Ӳ���汾��*/
    char		id_0xF012[9];   /*0xF012 ���ۿͻ�����*/
    uint32_t	id_0xF013;      /*0xF013 ����ģ���ͺ�0,δȷ�� ,0x3020 0x3017*/
    char		id_0xF014[16];  /*0xF014 �����绰����*/
    char		id_0xF015[30];  /*0xF015 Ԥ���ռ䣬32�ֽ�*/

    /*�г���¼��*/
    uint32_t	id_0xF030;      /*0xF030 ��¼�ǳ��ΰ�װʱ��,mytime��ʽ*/
    uint32_t	id_0xF031;      /*0xF031 ����TCP�˿�1*/
    uint32_t	id_0xF032;      /*��������ϵ��,��Чλ��Ϊ��16λ,��ʾΪÿ����������������λΪ1��ʾ��ҪУ׼��Ϊ0��ʾ����ҪУ׼*/
    /*���޸����ԵĲ���*/
    uint8_t 	id_0xF040;      //
    uint8_t 	id_0xF041;      //��ʾ�Ƿ�ɼ���ʻ��¼�����ݣ�Ϊ1��ʾ��Ҫ�ɼ���ʻ��¼�����ݣ�Ϊ0��ʾ���ɼ�
    //      ���ʱ����ֵΪ0 ����������¼������1  ��  ��ʽ����ֵΪ2
    uint8_t 	id_0xF042;      //Ϊ0��ʾ������ACC��״̬��Ϊ��0��ʾ��ACC�رպ���Ҫ���߳�������ʡ�磬1��ʾ�ر�GPRSͨ�ţ�2��ʾ�ر�GSMģ�飬3��ʾ�ر�GPS��GSM
    uint8_t 	id_0xF043;      //��ʾ�ٶ�ģʽ��0��ʾΪ�����ٶ�ģʽ��1��ʾΪGPS�ٶ�ģʽ
    ////////////////////////
    /*��ӡ���*/
    //uint8_t 	id_0xF040;      //line_space;               //�м��
    //uint8_t 	id_0xF041;      //margin_left;				//��߽�
    //uint8_t 	id_0xF042;      //margin_right;				//�ұ߽�
    //uint8_t 	id_0xF043;      //step_delay;               //������ʱ,Ӱ���м��
    uint8_t 	id_0xF044;      //gray_level;               //�Ҷȵȼ�,����ʱ��
    uint8_t 	id_0xF045;      //heat_delay[0];			//������ʱ
    uint8_t 	id_0xF046;      //heat_delay[1];			//������ʱ
    uint8_t 	id_0xF047;      //heat_delay[2];			//������ʱ
    uint8_t 	id_0xF048;      //heat_delay[3];			//������ʱ
    char		id_0xF049[32];  /*0xF049 ���ݷ�������ַ2*/
    uint16_t	id_0xF04A;      /*0xF04A ����TCP�˿�2*/
    uint32_t 	id_0xF04B;      //����������λΪƽ������
    uint32_t 	id_0xF04C;      //����������ȵ�λΪmm
    uint16_t	id_0xF04D;      //���ٹ���ֵ���������ٶȳ������ٶȺ���Ϊ�����ٶȷǷ������ϴε��ٶȴ��浱ǰ�ٶȡ���ֵΪ0��ʾ��Ч�������ˣ���λΪ0.1km/h
    char		id_0xF04E[88];  /*0xF04E Ԥ����������*/
} JT808_PARAM;





/*
   �洢�������,���þ��Ե�ַ,��4K(0x1000)Ϊһ������
 */



typedef __packed struct _jt808_data
{
    uint32_t	id_0xFA00;		/*0xFA00 ���һ��д���utcʱ��*/
    uint32_t	id_0xFA01;		/*0xFA01 �����,��λΪ��,GPS*/
    uint32_t	id_0xFA02;		/*0xFA02 ����״̬*/
    uint32_t	id_0xFA03;		/*0xFA01 �����,��λΪ��,������*/
} STYLE_JT808_DATA;


enum _stop_run
{
    STOP = 0,
    RUN = 1,
};

typedef __packed struct
{
    uint16_t			get_speed;				///�������ٱ��������ٶ�,��λΪ0.1KM/H
    uint32_t			get_speed_tick;			///���ٱ��ϴλ�ȡ���ٶȵ�ʱ�̣���λΪtick
    uint32_t			get_pulse_tick;			///���ٱ��ϴλ�ȡ�������ʱ�̣���λΪtick
    volatile uint32_t	get_speed_cap_sum;		///�������ٱ�����ٶȶ���˻�������ܼ�����
    volatile uint16_t	get_speed_cap_num;		///�������ٱ�����ٶȶ���˻���������
    uint32_t			pulse_cap_num;			///����ϵ��У׼ʱ�˻����������
    long long			pulse_mileage;			///����ϵ��У׼��ʼʱ������̣���λΪ 100/3600 ��,���������λ��Ϊ�˾��������ۼ����
} TYPE_CAR_DATA;


typedef __packed struct
{
    MYTIME	time;
    uint8_t speed;
} TYPE_15MIN_SPEED;


typedef __packed struct _jt808_param_bkram
{
    uint32_t 	format_para;        		///��ֵΪ0x57584448ʱ��ʾ������ֵ��Ч
    uint32_t 	utctime;     				///���һ��д���UTCʱ��
    uint32_t 	data_version;     			///��ǰ���ݴ洢���İ汾����0x00000000��ʼ
    uint32_t 	updata_utctime;     		///Զ������������UTCʱ��
    char		update_ip[32];  			///Զ������IP
    uint32_t	update_port;				///Զ������IP Port;

    enum _stop_run	car_stop_run;			//����״̬��ֹͣ��������
    uint32_t	utc_car_stop;    			//������ʼͣʻʱ��
    uint32_t	utc_car_run;    			//������ʼ��ʻʱ��

    uint32_t	utc_car_today;				//��ʾ��������ͳ����Ϣ�Ŀ�ʼʱ��
    uint32_t	car_run_time;				//��ʾ�����������е���ʱ��
    uint32_t	car_alarm;					//��������״̬��Ϣ
    long long	car_mileage;				//��ʾ�������ܵ���ʻ���,��λΪ 100/3600 ��,���������λ��Ϊ�˾��������ۼ����

    uint32_t	utc_over_speed_start;		//���ٿ�ʼʱ��
    uint32_t	utc_over_speed_end;			//���ٽ���ʱ��
    uint32_t	over_speed_max;				//��������ٶ�

    /*ƣ�ͼ�ʻ����*/
    uint32_t	utc_over_time_start;		//ƣ�ͼ�ʻ��ʼʱ��
    uint32_t	utc_over_time_end;			//ƣ�ͼ�ʻ����ʱ��
    uint32_t 	vdr11_lati_start;			//ƣ�ͼ�ʻ��ʼγ��
    uint32_t 	vdr11_longi_start;			//ƣ�ͼ�ʻ��ʼ����
    uint16_t 	vdr11_alti_start;			//ƣ�ͼ�ʻ��ʼ�߶�
    uint32_t 	vdr11_lati_end;				//ƣ�ͼ�ʻ��ʼγ��
    uint32_t 	vdr11_longi_end;			//ƣ�ͼ�ʻ��ʼ����
    uint16_t 	vdr11_alti_end;				//ƣ�ͼ�ʻ��ʼ�߶�

    TYPE_15MIN_SPEED speed_15min[15];
    uint8_t			speed_curr_index;

    uint32_t	mileage_pulse;				//��ʾ�������ܵ���ʻ���,��λΪ 1��,�������ͨ�������߲ɼ�����
    uint32_t	mileage_pulse_start; 		//��ʾ��ʼ���Ʒ�ʱ�����,��λΪ 1��,�������ͨ�������߲ɼ�����
    uint32_t	mileage_pulse_end; 			//��ʾ�������Ʒ�ʱ�����,��λΪ 1��,�������ͨ�������߲ɼ�����
    uint32_t	mileage_pulse_utc_start; 	//��ʾ��ʼ���Ʒ�ʱ��UTCʱ��
    uint32_t	mileage_pulse_utc_end; 		//��ʾ�������Ʒ�ʱ��UTCʱ��
    uint32_t	low_speed_wait_time;		//��ʾ���һ����ճ������ܵ���ʻ��̺����ȴ���ʱ��,
    uint8_t		tel_month;					//����ͨ��ʱ��ͳ�Ƶ��±��
    uint32_t	tel_timer_this_month;		//����º��е���ʱ��������ʱ�����ڵ�����ʱ���󽫲��������
    uint8_t		acc_check_state;			//��ʾ��⳵���Ƿ�ACC�������˳�̨ACC�ߵ�״̬��bit0��ʾ��⵽��ACC�أ�bit1��ʾ��⵽��ACC������ʼ״̬Ϊ0
} JT808_PARAM_BK;


#if 0
typedef struct
{
    uint16_t	type;           /*�ն�����,�μ�0x0107 �ն�����Ӧ��*/
    uint8_t		producer_id[5];
    uint8_t		model[20];
    uint8_t		terminal_id[7];
    uint8_t		sim_iccid[10];
    uint8_t		hw_ver_len;
    uint8_t		hw_ver[32];
    uint8_t		sw_ver_len;
    uint8_t		sw_ver[32];
    uint8_t		gnss_attr;  /*gnss����,�μ�0x0107 �ն�����Ӧ��*/
    uint8_t		gsm_attr;   /*gnss����,�μ�0x0107 �ն�����Ӧ��*/
} TERM_PARAM;
#endif
extern unsigned char			printf_on;		///Ϊ1��ʾ��ӡ������Ϣ��

extern uint8_t  SOFT_VER;			///����汾
extern uint8_t	HARD_VER;			///Ӳ���汾
extern uint8_t			HardWareVerion;				///��ȡ����PCB��İ汾�ţ���Ӧ�Ŀ�ΪPA14,PA13,PB3
extern uint16_t 		current_speed;		///��λΪkm/h,��ǰ�ٶȣ�����ΪGPS�ٶȣ�Ҳ����Ϊ�����ٶȣ�ȡ���ڲ���id_0xF043��
extern uint16_t 		current_speed_10x;	///��λΪ0.1km/h,��ǰ�ٶȣ�����ΪGPS�ٶȣ�Ҳ����Ϊ�����ٶȣ�ȡ���ڲ���id_0xF043��
extern uint8_t			param_set_speed_state;	///��ֵΪ����ʱ����Ч��1��ʾ�ٶ�Ϊ0ʱ�����ϱ�һ�����ݣ�2��ʾ�ٶȳ���Ϊ10km/hʱ�ϱ�һ������

extern JT808_PARAM 		jt808_param;		///�ͳ�����������Լ�808Э���ж�������в���
extern JT808_PARAM_BK 	jt808_param_bk;		///����bksram�е����в������ò����ı���ʱ��Ϊ8Сʱ����
extern STYLE_JT808_DATA	jt808_data;		///���泵����Ҫ���ڱ��������
extern TYPE_CAR_DATA	car_data;			///������ص�ǰ����
extern uint32_t			param_factset_tick;	///����豸������ʱ�䣬��λΪtick
extern uint16_t			donotsleeptime;		///��ֵΪ��0ʱ�豸�������ߣ���λΪ�룬Ĭ��ֵΪ360��ʾ6���Ӳ��������豸
extern uint8_t			device_unlock;		///˵��:0��ʾ��̨δ������1��ʾ��̨�Ѿ�����
extern uint8_t			device_flash_err;	///˵��:0��ʾ�豸flash������1��ʾ�豸flash����
extern uint8_t			device_factory_oper;///˵��:0��ʾ����������1��ʾ��ǰΪ��������

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
