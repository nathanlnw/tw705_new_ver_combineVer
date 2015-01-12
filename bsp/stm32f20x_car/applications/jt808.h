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
#ifndef _H_JT808_H_
#define _H_JT808_H_

#include <include.h>
#include <rtthread.h>

#include "msglist.h"
#include "m66.h"
#include "sst25.h"
#include "jt808_area.h"
#include "jt808_util.h"
#include "jt808_vehicle.h"
#include "common.h"

#define NEED_TODO 0

#define   MsgQ_Timeout		3
//#define JT808_PACKAGE_MAX	512
#define JT808_PACKAGE_MAX	800


/*for new use*/

//#define GPS_GGHYPT			///gpsƽ̨Ϊ��������ƽ̨
#define GPS_TJGTGS			///gpsƽ̨Ϊ����ͨ��˾
//#define GPS_HBTJT			///gpsƽ̨Ϊ�ӱ����ͨ


#define FLASH_SEM_DELAY		8
#define FLASH_SEM_DELAY_2 	8

//#define BIT( n ) ( (unsigned long)( 1 << n ) )


typedef struct
{
    int		id;
    short	attr;
    int		latitute;   /*�Զ�λ��λ��γ��ֵ����10��6�η�����ȷ�������֮һ��*/
    int		longitute;
    int		radius;     /*��λΪ��m��·��Ϊ�ùյ㵽��һ�յ�*/
    char	start[6];
    char	end[6];
    short	speed;
    char	interval;   /*����ʱ��,��*/
} GPS_AREA_CIRCLE;

typedef enum
{
    IDLE = 1,           /*���еȴ�����*/
    WAIT_ACK,           /*�ȴ�ACK��*/
    ACK_OK,             /*���յ�ACKӦ��*/
    ACK_TIMEOUT,        /*ACK��ʱ*/
} JT808_MSG_STATE;

typedef enum
{
    SINGLE_REGISTER	= 0,
    SINGLE_FIRST	= 1,
    SINGLE_CMD		= 2,
    SINGLE_ACK		= 3,
    MULTI_CMD		= 4,
    MULTI_CMD_NEXT	= 5,
    MULTI_ACK		= 6
} JT808_MSG_TYPE;


typedef enum
{
    JT808_STOP_REPORT = 0,                    /*ͣ��*/
    JT808_REGISTER,                           /*ע��*/
    JT808_AUTH,                               /*��Ȩ*/
    JT808_REPORT,                             /*�ϱ�*/
    JT808_WAIT                                /*�ȴ�*/
} JT808_STATE;


typedef __packed struct
{
    uint16_t	id;
    uint16_t	attr;
    uint8_t		mobile[6];
    uint16_t	seq;
} JT808_MSG_HEAD;

typedef __packed struct
{
    uint16_t	id;
    uint16_t	attr;
    uint8_t		mobile[6];
    uint16_t	seq;
    uint16_t	packet_num;
    uint16_t	packet_no;
} JT808_MSG_HEAD_EX;

#define JT808HEAD_ID( head )	( ( *( head + 0 ) << 8 ) | ( *( head + 1 ) ) )
#define JT808HEAD_ATTR( head )	( ( *( head + 2 ) << 8 ) | ( *( head + 3 ) ) )
#define JT808HEAD_LEN( head )	( ( *( head + 2 ) << 8 ) | ( *( head + 3 ) ) ) & 0x3FF
#define JT808HEAD_SEQ( head )	( ( *( head + 10 ) << 8 ) | ( *( head + 11 ) ) )

typedef enum
{
    CONNECT_NONE	= 0,        /*������*/
    CONNECT_IDLE	= 1,        /*���У�׼�����ӣ����ӹر�״̬*/
    CONNECT_PEER	= 2,        /*�������ӵ��Զ�*/
    CONNECTED		= 3,        /*���ӳɹ�*/
    CONNECT_ERROR	= 4,        /*���Ӵ���*/
    CONNECT_CLOSING = 5,        /*���ڹر�����*/
    CONNECT_CLOSED	= 6,        /*���ӹرգ��������������Ǳ���*/
} CONN_STATE;


typedef enum
{
    SOCK_NONE		= 0,        /*������*/
    SOCK_CLOSE		= 1,        /*�ر�socket*/
    SOCK_RECONNECT	= 2,        /*��������socket*/
    SOCK_CONNECT	= 3,        /*����socket*/
} ENUM_SOCK_CONTROL;



typedef enum
{
    DEVICE_OFF_DEVICE = 0,			//�ն˹ػ�
    DEVICE_OFF_LINK,				//�ն˹ر�����ͨ��
    DEVICE_OFF_RF,					//�ն˹ر���������ͨ��
    DEVICE_RESET_DEVICE,			//�ն˸�λ
    DEVICE_NORMAL = 0xFF,			//�ն����������������Ὣ������·�Ͽ��ȹ�������
} ENUM_DEVICE_CONTROL;


typedef struct
{
    uint32_t	operator;			//�ն˲���
    uint32_t	delay_counter;		//�ն˲����ӳ�ʱ��,��λΪ��
    uint32_t	off_counter;		//�ն˹ػ�ʱ��,��λΪ��
    uint32_t	off_link_counter;	//�Ͽ�����ͨ��ʱ��,��λΪ��
    uint32_t	off_rf_counter;		//�Ͽ�����ͨ��ʱ��,��λΪ��
} STYLE_DEVICE_CONTROL;

#if 0

typedef struct
{
    char		type;           /*�������� 'u':udp client 't':TCP client  'U' udp server*/
    char		ipstr[64];      /*�������ַ*/
    uint16_t	port;           /*�˿�*/
} IP_PORT;

typedef struct
{
    uint8_t		linkno;         /*��ʹ�õ�link��*/
    uint8_t		index;          /*�����ж��ѡ�񣬶���ѡ�����ӵ����*/
    CONN_STATE	state;          /*����״̬*/
    uint32_t	timecount;      /*����ʱ��*/
    char		ipv4[16];       /*dns���IP xxx.xxx.xxx.xxx*/
    IP_PORT		ip_port[4];     /*�м�����ѡ������*/
} GSM_SOCKET;

#endif

typedef struct
{
    uint8_t linkno;             /*��ʹ�õ�link��*/
    uint8_t index;              /*�����ж��ѡ�񣬶���ѡ�����ӵ����*/
    //int8_t	active;             /*�ⲿ֪ͨ��Ҫ���ӣ�0:Ĭ�� -1:�Ҷ� >1:����*/
    uint8_t err_no;             /*��¼������*/

    CONN_STATE	state;          /*����״̬*/
    char		ip_addr[16];    /*dns���IP xxx.xxx.xxx.xxx*/
    char		type;           /*�������� 'u':udp client 't':TCP client  'U' udp server*/
    char		ipstr[64];      /*�������ַ*/
    uint16_t	port;           /*�˿�*/
    ENUM_SOCK_CONTROL	proc;	/*�˿ڲ���*/
} GSM_SOCKET;


/*
   �洢jt808���͵������Ϣ
 */

typedef __packed struct _jt808_tx_nodedata
{
    /*���ͻ������*/
    uint8_t linkno;                                                                                     /*����ʹ�õ�link,������Э���Զ��socket*/
    //	uint8_t			multipacket;                                                                        /*�ǲ��Ƕ������*/
    JT808_MSG_TYPE	type;                                                                               /*������Ϣ������*/
    JT808_MSG_STATE state;                                                                              /*����״̬*/
    uint32_t		retry;                                                                              /*�ش�����,�������ݼ��Ҳ���*/
    uint32_t		max_retry;                                                                          /*����ش�����*/
    uint32_t		timeout;                                                                            /*��ʱʱ��*/
    uint32_t		timeout_tick;                                                                       /*����ʱ��*/
    /*���յĴ����ж����*/
    JT808_MSG_STATE ( *cb_tx_timeout )( __packed struct _jt808_tx_nodedata *thiz );                     /*���ͳ�ʱ�Ĵ�����*/
    JT808_MSG_STATE ( *cb_tx_response )( __packed struct _jt808_tx_nodedata *thiz, uint8_t *pmsg );     /*�յ�����Ӧ��Ĵ�����*/
    uint16_t	head_id;                                                                                /*��ϢID*/
    uint16_t	head_sn;                                                                                /*��Ϣ��ˮ��*/

    uint16_t	packet_num;                                                                             /*����ܰ���*/
    uint16_t	packet_no;                                                                              /*�����ǰ����*/
    uint32_t	size;                                                                                   /*��Ϣ���ܵ����ݴ�С*/
    uint16_t	msg_len;                                                                                /*������Ϣ����*/
    /*������͵Ĵ���*/
    void	*user_para;                                                                                 /*cb_tx_response������Ҫ�Ĺؼ�ԭʼ���ݲ�����ͨ���ò����ͻص�������������*/
    uint8_t tag_data[];                                                                                 /*ָ�����ݵ�ָ��*/
} JT808_TX_NODEDATA;

extern __IO uint32_t	time_1ms_counter;					///1ms��ʱ��
extern STYLE_DEVICE_CONTROL	device_control;					///TW703/705������ؿ��Ʋ���
extern rt_tick_t	tick_receive_last_pack;				///���һ���յ���λ����Ϣ��ʱ��


void jt808_init( void );


rt_err_t gprs_rx( uint8_t linkno, uint8_t *pinfo, uint16_t length );


void cb_socket_close( uint8_t cid );


JT808_TX_NODEDATA *node_begin( uint8_t linkno,
                               JT808_MSG_TYPE fMultiPacket,    /*�Ƿ�Ϊ���*/
                               uint16_t id,
                               int32_t seq,
                               uint16_t datasize );


JT808_TX_NODEDATA *node_data( JT808_TX_NODEDATA *pnodedata,
                              uint8_t *pinfo, uint16_t len );


void node_end( JT808_MSG_TYPE msgtype,
               JT808_TX_NODEDATA *pnodedata,
               JT808_MSG_STATE ( *cb_tx_timeout )( ),
               JT808_MSG_STATE ( *cb_tx_response )( ),
               void  *userpara );


int jt808_add_tx( uint8_t linkno,
                  JT808_MSG_TYPE msgtype,      /*�Ƿ�Ϊ���*/
                  uint16_t id,
                  int32_t seq,
                  JT808_MSG_STATE ( *cb_tx_timeout )( ),
                  JT808_MSG_STATE ( *cb_tx_response )( ),
                  uint16_t info_len,           /*��Ϣ����*/
                  uint8_t *pinfo,
                  void  *userpara );



///
#define jt808_tx(linkno, id, info, len ) jt808_add_tx( linkno, SINGLE_CMD, id, -1, RT_NULL, RT_NULL, len, info, RT_NULL )

/*ר��Ӧ��*/
#define jt808_tx_ack(id, info, len ) jt808_add_tx( 1, SINGLE_FIRST, id, -1, RT_NULL, RT_NULL, len, info, RT_NULL )
#define jt808_tx_ack_ex(linkno, id, info, len ) jt808_add_tx( linkno, SINGLE_FIRST, id, -1, RT_NULL, RT_NULL, len, info, RT_NULL )


/*
   extern GSM_SOCKET socket_master;
   extern GSM_SOCKET socket_slave;
   extern GSM_SOCKET socket_iccard;
 */
//ͨ��Ӧ��
rt_err_t jt808_tx_0x0001( uint16_t seq, uint16_t id, uint8_t res );
extern rt_err_t jt808_tx_0x0001_ex( uint8_t linkno, u16 fram_num, u16 cmd_id, u8 isFalse );
extern void socket_para_init(void);

extern GSM_SOCKET	*mast_socket;
extern GSM_SOCKET	*iccard_socket;
extern GSM_SOCKET	gsm_socket[3];
extern GSM_SOCKET	*pcurr_socket;
extern JT808_STATE	jt808_state;
extern uint32_t	wdg_thread_counter[16];

extern uint8_t jt808_tx_lock(uint8_t linkno);
extern void control_device(ENUM_DEVICE_CONTROL control, uint32_t timer);
extern void jt808_control_proc(void);
extern uint8_t get_sock_state(uint8_t sock);
extern uint8_t debug_write(char *str);
extern uint8_t sock_proc(GSM_SOCKET *sock, ENUM_SOCK_CONTROL proc1);
#endif

/************************************** The End Of File **************************************/
