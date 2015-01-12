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
#include <stdio.h>

#include <board.h>
#include <rtthread.h>
#include <finsh.h>
#include <rtdevice.h>

#include "include.h"
#include "jt808.h"
#include "msglist.h"
#include "jt808_sprintf.h"
#include "sst25.h"

#include "common.h"
#include "m66.h"

#include "jt808_param.h"
#include "jt808_sms.h"
#include "jt808_gps.h"
#include "jt808_area.h"
#include "jt808_misc.h"
#include "jt808_camera.h"
#include "jt808_update.h"
#include "jt808_gps_pack.h"
#include "jt808_can.h"
#include "gps.h"
#include "jt808_SpeedFatigue.h"
#include "hmi.h"
#include "jt808_GB19056.h"
#include "RS232.h"
#include "SLE4442.h"


#pragma diag_error 223

typedef struct
{
    uint16_t id;
    int ( *func )( uint8_t linkno, uint8_t *pmsg );
} HANDLE_JT808_RX_MSG;

/*gprs�յ���Ϣ������*/
static struct rt_mailbox	mb_gprsrx;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsrx_pool[MB_GPRSDATA_POOL_SIZE];

JT808_STATE		jt808_state = JT808_REGISTER;

static uint16_t tx_seq = 0;             /*�������*/

static uint16_t total_send_error = 0;   /*�ܵķ��ͳ����������ﵽһ���Ĵ���Ҫ����M66*/

/*������Ϣ�б�*/
MsgList *list_jt808_tx;

/*������Ϣ�б�*/
MsgList				 *list_jt808_rx;

static rt_tick_t	tick_auth_heartbeat		= 0;
static rt_tick_t	jt808_send_pack_num		= 0;		///û���յ��κ���λ������֮ǰ���͵���Ϣ����
rt_tick_t			tick_receive_last_pack	= 0;		///���һ���յ���λ����Ϣ��ʱ��
static rt_tick_t	socket_idle_tick[3];				///


uint8_t	wdg_reset_flag = 0;

__IO uint32_t	time_1ms_counter = 0;					///1ms��ʱ��

uint32_t	wdg_thread_counter[16];
char		mast_phone_num[13] = "13820554863";

STYLE_DEVICE_CONTROL	device_control;					///TW703/705������ؿ��Ʋ���


extern struct rt_device dev_mma8451;
extern void sd_write_console(char *str);
extern void record(uint32_t time);


void WatchDog_Feed(void)
{
    uint8_t i;
    if(wdg_reset_flag == 0)
    {
        for(i = 0; i < sizeof(wdg_thread_counter) / sizeof(uint32_t); i++)
        {
            if(wdg_thread_counter[i])
                ++wdg_thread_counter[i];
            if(wdg_thread_counter[i] > 300)
                return;
        }
        IWDG_ReloadCounter();
    }
}

static void WatchDogInit(void)
{
    uint8_t i;
    for(i = 0; i < sizeof(wdg_thread_counter) / sizeof(uint32_t); i++)
    {
        wdg_thread_counter[i] = 0;
    }
    /* Enable write access to IWDG_PR and IWDG_RLR registers */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    /* IWDG counter clock: LSI/32 */
    /*   prescaler     min/ms      max/ms
    		4        	0.1			409.6
    		8      		0.2			819.2
    		16			0.4			1638.4
    		32			0.8			3276.8
    		64			1.6			6553.5
    		128			3.2			13107.2
    		256			6.4			26214.4
    */
    IWDG_SetPrescaler(IWDG_Prescaler_64);

    /* Set counter reload value to obtain 250ms IWDG TimeOut.
    Counter Reload Value = 250ms/IWDG counter clock period
                  = 250ms / (LSI/32)
                  = 0.25s / (LsiFreq/32)
                  = LsiFreq/(32 * 4)
                  = LsiFreq/128
    */
    IWDG_SetReload(0x0FFF);//(LsiFreq/128);

    /* Reload IWDG counter */
    IWDG_ReloadCounter();

    /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
    IWDG_Enable();
}


/*
   ���ͺ��յ�Ӧ����
 */
static JT808_MSG_STATE jt808_tx_response( JT808_TX_NODEDATA *nodedata, uint8_t *pmsg )
{
    uint8_t		 *msg = pmsg + 12;
    uint16_t	ack_id;
    //	uint16_t	ack_seq;
    uint8_t		ack_res;
    uint8_t		link_no;

    //	ack_seq = ( msg[0] << 8 ) | msg[1];
    ack_id	= ( msg[2] << 8 ) | msg[3];
    ack_res = *( msg + 4 );


    switch( ack_id )            // �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
    {
    case 0x0002:            //	��������Ӧ��
        //rt_kprintf( "\nCentre  Heart ACK!\n" );
        break;
    case 0x0003:            //	�ն�ע��Ӧ��
        debug_write("�յ�ע��Ӧ��!");
        memset( jt808_param.id_0xF003, 0, 32 );
        param_save(1);
        //gsmstate( GSM_POWEROFF );
        sock_proc(&gsm_socket[nodedata->linkno - 1], SOCK_RECONNECT);
        break;
    case 0x0101:            //	�ն�ע��Ӧ��
        break;
    case 0x0102:            //	�ն˼�Ȩ
        if( ack_res == 0 )  /*�ɹ�*/
        {
            debug_write("�ն˼�ȨOK!");
            jt808_state = JT808_REPORT;
            if( mast_socket->index % 6 < 3 )
            {
                mast_socket->index = 0;
            }
            else
            {
                mast_socket->index = 3;
            }
        }
        else
        {
            jt808_state = JT808_REGISTER;
        }
        break;
    case 0x0110:            //	�ն˽���Ӧ��
        if(( ack_res == 0 ) || ( ack_res == 5 )) /*�ɹ�*/
        {
            jt808_param.id_0xF00F = 1;
            param_save(1);
            //pMenuItem = &Menu_1_Idle;
            //pMenuItem->show( );
            debug_write("�ն˽���OK!");
            jt808_state = JT808_REGISTER;
            sock_proc(mast_socket, SOCK_CONNECT);
            gsmstate(GSM_POWEROFF);
        }
        break;
    case 0x0800: // ��ý���¼���Ϣ�ϴ�
        break;
    case 0x0702:
        rt_kprintf( "\n��ʻԱ��Ϣ�ϱ�---����Ӧ��!" );
        break;
    case 0x0701:
        //rt_kprintf( "�����˵��ϱ�---����Ӧ��!" );
        break;
    default:
        //rt_kprintf( "\nunknown id=%04x", ack_id );
        break;
    }
    return ACK_OK;
}

/*
   ��Ϣ���ͳ�ʱ
 */

static JT808_MSG_STATE jt808_tx_timeout( JT808_TX_NODEDATA *nodedata )
{
    rt_kprintf( "\nsend %04x timeout\n", nodedata->head_id );
    switch( nodedata->head_id )            // �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
    {
    case 0x0002:            //	��������Ӧ��
        //rt_kprintf( "\nCentre  Heart ACK!\n" );
        break;
    case 0x0101:            //	�ն�ע��
        break;
    case 0x0100:            //	�ն�ע��
        jt808_state = JT808_REGISTER;
        break;
    case 0x0102:            //	�ն˼�Ȩ
#ifdef JT808_TEST_JTB
        if( mast_socket->index % 6 < 3 )
        {
            mast_socket->index = 3;
        }
        else
        {
            mast_socket->index = 0;
        }
        gsmstate(GSM_POWEROFF);
#else
        jt808_state = JT808_AUTH;
#endif
        break;
    default:
        //rt_kprintf( "\nunknown id=%04x", ack_id );
        break;
    }
    return ACK_TIMEOUT;
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
static void convert_deviceid( uint8_t *pout, char *psrc)
{
    uint8_t *pdst = pout;
    *pdst++ = ( ( psrc[0] - 0x30 ) << 4 ) | ( psrc[1] - 0x30 );
    *pdst++ = ( ( psrc[2] - 0x30 ) << 4 ) | ( psrc[3] - 0x30 );
    *pdst++ = ( ( psrc[4] - 0x30 ) << 4 ) | ( psrc[5] - 0x30 );
    *pdst++ = ( ( psrc[6] - 0x30 ) << 4 ) | ( psrc[7] - 0x30 );
    *pdst++ = ( ( psrc[8] - 0x30 ) << 4 ) | ( psrc[9] - 0x30 );
    *pdst	= ( ( psrc[10] - 0x30 ) << 4 ) | ( psrc[11] - 0x30 );
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
static void convert_deviceid_to_mobile( uint8_t *pout )
{
#ifdef JT808_TEST_JTB
    convert_deviceid(pout, jt808_param.id_0xF006);
#else
    if(jt808_param.id_0xF00F == 2)
        convert_deviceid(pout, gsm_param.imsi + 3);
    else if(jt808_param.id_0xF00F == 3)
        convert_deviceid(pout, jt808_param.id_0xF002);
    else
        convert_deviceid(pout, jt808_param.id_0xF006);
#endif
    /*
    uint8_t *pdst = pout;
    *pdst++ = ( ( jt808_param.id_0xF006[0] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[1] - 0x30 );
    *pdst++ = ( ( jt808_param.id_0xF006[2] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[3] - 0x30 );
    *pdst++ = ( ( jt808_param.id_0xF006[4] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[5] - 0x30 );
    *pdst++ = ( ( jt808_param.id_0xF006[6] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[7] - 0x30 );
    *pdst++ = ( ( jt808_param.id_0xF006[8] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[9] - 0x30 );
    *pdst	= ( ( jt808_param.id_0xF006[10] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[11] - 0x30 );
    */
}


/*�����������ݵ���Ϣ����*/
void node_datalen( JT808_TX_NODEDATA *pnodedata, uint16_t datalen )
{
    uint8_t *pdata_head = pnodedata->tag_data;

    pdata_head[2]		= datalen >> 8;
    pdata_head[3]		= datalen & 0xFF;
    pnodedata->msg_len	= datalen + 12; /*ȱʡ�ǵ���*/
    if( pnodedata->type >= MULTI_CMD )  /*�������*/
    {
        pdata_head[12]	= pnodedata->packet_num >> 8;
        pdata_head[13]	= pnodedata->packet_num & 0xFF;
        pnodedata->packet_no++;
        pdata_head[14]		= pnodedata->packet_no >> 8;
        pdata_head[15]		= pnodedata->packet_no & 0xFF;
        pdata_head[2]		|= 0x20;    /*�������*/
        pnodedata->msg_len	+= 4;
    }
}


/*
   ����һ��ָ����С��node
 */
JT808_TX_NODEDATA *node_begin( uint8_t linkno,
                               JT808_MSG_TYPE msgtype,    /*�Ƿ�Ϊ���*/
                               uint16_t id,
                               int32_t seq,
                               uint16_t datasize )
{
    JT808_TX_NODEDATA *pnodedata;

    if( msgtype >= MULTI_CMD )
    {
        pnodedata = rt_malloc( sizeof( JT808_TX_NODEDATA ) + sizeof( JT808_MSG_HEAD_EX ) + datasize );
    }
    else
    {
        pnodedata = rt_malloc( sizeof( JT808_TX_NODEDATA ) + sizeof( JT808_MSG_HEAD ) + datasize );
    }
    if( pnodedata == RT_NULL )
    {
        return RT_NULL;
    }
    //rt_kprintf( "\n%d>����(id:%04x size:%d) %p", rt_tick_get( ), id, datasize,pnodedata );
    memset( pnodedata, 0, sizeof( JT808_TX_NODEDATA ) );    ///���Բ����٣�����ϵͳ����
    pnodedata->linkno	= linkno;
    pnodedata->state	= IDLE;
    pnodedata->head_id	= id;
    pnodedata->retry	= 0;
    pnodedata->type		= msgtype;

    if(( msgtype ==  SINGLE_FIRST) || ( msgtype > SINGLE_CMD ))                            /*����͵���Ӧ��ֻ��һ��*/
    {
        pnodedata->max_retry	= 1;
        pnodedata->timeout		= RT_TICK_PER_SECOND * jt808_param.id_0x0002;
    }
    else
    {
        pnodedata->max_retry	= 3;
        //pnodedata->timeout		= RT_TICK_PER_SECOND * 10;
        pnodedata->timeout		= RT_TICK_PER_SECOND * jt808_param.id_0x0002;		///��ͨ�����ԣ���Ϊ���Ḳ��
    }

    pnodedata->packet_num	= 1;
    pnodedata->packet_no	= 0;
    pnodedata->size			= datasize;

    if( seq == -1 )
    {
        pnodedata->head_sn = tx_seq;
        tx_seq++;
    }
    else
    {
        pnodedata->head_sn = seq;
    }
    return pnodedata;
}

/*
   �������������,�γ���Ч������
 */
JT808_TX_NODEDATA *node_data( JT808_TX_NODEDATA *pnodedata,
                              uint8_t *pinfo, uint16_t len )
{
    uint8_t *pdata;

    pdata = pnodedata->tag_data;

    pdata[0]	= pnodedata->head_id >> 8;
    pdata[1]	= pnodedata->head_id & 0xff;
    pdata[2]	= ( len >> 8 );
    pdata[3]	= len & 0xff;
    pdata[10]	= pnodedata->head_sn >> 8;
    pdata[11]	= pnodedata->head_sn & 0xff;

    convert_deviceid_to_mobile( pdata + 4 );
    if( pnodedata->type >= MULTI_CMD )      /*�������*/
    {
        pdata[2] += 0x20;
        memcpy( pdata + 16, pinfo, len );   /*����û�����*/
        pnodedata->msg_len = len + 16;
    }
    else
    {
        memcpy( pdata + 12, pinfo, len );   /*����û�����*/
        pnodedata->msg_len = len + 12;
    }
    return pnodedata;
}


/*��ӵ������б�**/
void node_end( JT808_MSG_TYPE msgtype,
               JT808_TX_NODEDATA *pnodedata,
               JT808_MSG_STATE ( *cb_tx_timeout )( ),
               JT808_MSG_STATE ( *cb_tx_response )( ),
               void  *userpara )
{
    pnodedata->user_para = userpara;

    convert_deviceid_to_mobile( pnodedata->tag_data + 4 );

    if( cb_tx_timeout == RT_NULL )
    {
        pnodedata->cb_tx_timeout = jt808_tx_timeout;
    }
    else
    {
        pnodedata->cb_tx_timeout = cb_tx_timeout;
    }

    if( cb_tx_response == RT_NULL )
    {
        pnodedata->cb_tx_response = jt808_tx_response;
    }
    else
    {
        pnodedata->cb_tx_response = cb_tx_response;
    }

    if( msgtype <= SINGLE_FIRST )
    {
        msglist_prepend( list_jt808_tx, pnodedata );
    }
    else if( msgtype == MULTI_CMD_NEXT)
    {
        msglist_prepend_next( list_jt808_tx, pnodedata );
    }
    else
    {
        msglist_append( list_jt808_tx, pnodedata );
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
int jt808_add_tx( uint8_t linkno,
                  JT808_MSG_TYPE msgtype,              /*�Ƿ�Ϊ���*/
                  uint16_t id,
                  int32_t seq,
                  JT808_MSG_STATE ( *cb_tx_timeout )( ),
                  JT808_MSG_STATE ( *cb_tx_response )( ),
                  uint16_t len,                        /*��Ϣ����*/
                  uint8_t *pinfo,
                  void  *userpara )

{
    JT808_TX_NODEDATA *pnodedata;

    pnodedata = node_begin( linkno, msgtype, id, seq, len );
    if( pnodedata == RT_NULL )
    {
        return 0;
    }
    node_data( pnodedata, pinfo, len );
    node_end( msgtype, pnodedata, cb_tx_timeout, cb_tx_response, userpara );
    return 1;
}


/*********************************************************************************
  *��������:void area_jt808_commit_ack(u16 fram_num,u16 cmd_id,u8 isFalse)
  *��������:ͨ��Ӧ�𣬵���OKӦ��
  *��	��:	fram_num:Ӧ����ˮ��
   cmd_id	:����ʱ��808����
   statue   :��ʾ״̬��0:��ʾOK	1:��ʾʧ��	2:��ʾ��Ϣ����	3:��ʾ��֧��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t jt808_tx_0x0001( u16 fram_num, u16 cmd_id, u8 isFalse )
{
    u8 pbuf[8];
    if(( mast_socket->state != CONNECTED ) || (jt808_state != JT808_REPORT)) /*������*/
    {
        return RT_ERROR;
    }
    data_to_buf( pbuf, fram_num, 2 );
    data_to_buf( pbuf + 2, cmd_id, 2 );
    pbuf[4] = isFalse;
    jt808_tx_ack_ex(mast_socket->linkno, 0x0001, pbuf, 5 );
    return RT_EOK;
}

/*********************************************************************************
  *��������:void jt808_tx_0x0001_ex( uint8_t linkno, u16 fram_num,u16 cmd_id,u8 isFalse)
  *��������:ͨ��Ӧ�𣬵���OKӦ��
  *��	��:	linkno	:��·���
  			fram_num:Ӧ����ˮ��
   			cmd_id	:����ʱ��808����
   			statue   :��ʾ״̬��0:��ʾOK	1:��ʾʧ��	2:��ʾ��Ϣ����	3:��ʾ��֧��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t jt808_tx_0x0001_ex( uint8_t linkno, u16 fram_num, u16 cmd_id, u8 isFalse )
{
    u8 pbuf[8];
    if(( mast_socket->state != CONNECTED ) || (jt808_state != JT808_REPORT)) /*������*/
    {
        return RT_ERROR;
    }
    data_to_buf( pbuf, fram_num, 2 );
    data_to_buf( pbuf + 2, cmd_id, 2 );
    pbuf[4] = isFalse;
    jt808_tx_ack_ex(linkno, 0x0001, pbuf, 5 );
    return RT_EOK;
}


/*
   ƽ̨ͨ��Ӧ��,�յ���Ϣ��ֹͣ����
 */
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{
    MsgListNode			 *iter;
    JT808_TX_NODEDATA	 *iterdata;

    uint16_t	ack_id;
    uint16_t	ack_seq;
    //	uint8_t		ack_res;
    /*������Ϣͷ12byte*/
    ack_seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
    ack_id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
    rt_kprintf( "\n%d>ACK %04x:%04x:%d", rt_tick_get( ), ack_id, ack_seq, pmsg[16] );

    /*��������*/
    iter = list_jt808_tx->first;
    while( iter != RT_NULL )                                                /*���ӱ������п��ܲ���*/
    {
        iterdata = (JT808_TX_NODEDATA *)iter->data;
        if(iterdata->state != WAIT_ACK)
        {
            iter = iter->next;
            break;
        }
        if( ( iterdata->head_id == ack_id ) && ( iterdata->head_sn == ack_seq ) )
        {
            iterdata->state = iterdata->cb_tx_response( iterdata, pmsg );   /*Ӧ������*/
            return 1;
        }
        iter = iter->next;
    }
    return 1;
}

/*�����ְ�����*/
static int handle_rx_0x8003( uint8_t linkno, uint8_t *pmsg )
{
    MsgListNode			 *iter;
    JT808_TX_NODEDATA	 *iterdata;
    //	uint32_t			media_id;

    /*������Ϣͷ12byte*/

    iter = list_jt808_tx->first;
    while( iter != RT_NULL )
    {
        iterdata = (JT808_TX_NODEDATA *)iter->data;
        if(iterdata->state != WAIT_ACK)
        {
            iter = iter->next;
            break;
        }
        if( iterdata->head_id == 0x0801 )
        {
            iterdata->cb_tx_response( iterdata, pmsg ); /*Ӧ������*/
            return 1;
        }
        else if( iterdata->head_id == 0x0700 )
        {
            iterdata->cb_tx_response( iterdata, pmsg ); /*Ӧ������*/
            return 1;
        }
        iter = iter->next;
    }
#if 0
    media_id = ( pmsg[12] << 24 ) | ( pmsg[13] << 16 ) | ( pmsg[14] << 8 ) | ( pmsg[15] );

    while( iter != RT_NULL )
    {
        iterdata = (JT808_TX_NODEDATA *)iter->data;
        if( iterdata->head_id == media_id )             /*���ﲻ��*/
        {
            iterdata->cb_tx_response( iterdata, pmsg ); /*Ӧ������*/
            iterdata->state = ACK_OK;
            break;
        }
        else
        {
            iter = iter->next;
        }
    }
#endif
    return 1;
}

/* ������Ķ��ն�ע����Ϣ��Ӧ��*/
static int handle_rx_0x8100( uint8_t linkno, uint8_t *pmsg )
{
    MsgListNode			 *iter;
    JT808_TX_NODEDATA	 *iterdata;
    uint16_t			body_len; /*��Ϣ�峤��*/
    uint16_t			ack_seq;
    uint8_t				res;
    uint8_t				 *msg;

    body_len	= ( ( *( pmsg + 2 ) << 8 ) | ( *( pmsg + 3 ) ) ) & 0x3FF;
    msg			= pmsg + 12;

    ack_seq = ( *msg << 8 ) | *( msg + 1 );
    res		= *( msg + 2 );

    iter = list_jt808_tx->first;
    while( iter != RT_NULL )
    {
        iterdata = iter->data;
        if( ( iterdata->head_id == 0x0100 ) && ( iterdata->head_sn == ack_seq ) )
        {
            if(( res == 0 ) && ((body_len - 3 ) <= sizeof(jt808_param.id_0xF003)))
            {
                memset(jt808_param.id_0xF003, 0, sizeof(jt808_param.id_0xF003));
                strncpy( jt808_param.id_0xF003, (char *)msg + 3, body_len - 3 );
                param_save( 1 );
                iterdata->state = ACK_OK;
                debug_write("�ն�ע��OK!");
                jt808_state		= JT808_AUTH;
                return 1;
            }
        }
        iter = iter->next;
    }
    return 1;
}

/*�����ն˲���*/
static int handle_rx_0x8103( uint8_t linkno, uint8_t *pmsg )
{
    jt808_param_0x8103( linkno, pmsg );
    return 1;
}

/*��ѯȫ���ն˲������п��ܻᳬ����������ֽ�*/
static int handle_rx_0x8104( uint8_t linkno, uint8_t *pmsg )
{
    jt808_param_0x8104( linkno, pmsg );
    return 1;
}

/*�ն˿���*/
static int handle_rx_0x8105( uint8_t linkno, uint8_t *pmsg )
{
    uint8_t		cmd;
    uint16_t	seq = ( pmsg[10] << 8 ) | pmsg[11];

    cmd = *( pmsg + 12 );
    switch( cmd )
    {
    case 1:                         /*��������*/
        break;
    case 2:                         /*�ն˿�������ָ��������*/
        break;
    case 3:                         /*�ն˹ػ�*/
        control_device(DEVICE_OFF_DEVICE, 0xFFFFFFFF);
        break;
    case 4:                         /*�ն˸�λ*/
        control_device(DEVICE_RESET_DEVICE, 0xFFFFFFFF);
        break;
    case 5:                         /*�ָ���������*/
        factory_ex(3, 0);
        control_device(DEVICE_OFF_DEVICE, 0xFFFFFFFF);
        control_device(DEVICE_RESET_DEVICE, 0xFFFFFFFF);
        break;
    case 6:                         /*�ر�����ͨѶ*/
        control_device(DEVICE_OFF_LINK, 0xFFFFFFFF);
        break;
    case 7:                         /*�ر���������ͨѶ*/
        control_device(DEVICE_OFF_RF, 0xFFFFFFFF);
        break;
    }
    jt808_tx_0x0001_ex(linkno, seq, 0x8105, 0 );  /*ֱ�ӷ��ز�֧��*/
    return 1;
}

/*��ѯָ���ն˲���,����Ӧ��0x0104*/
static int handle_rx_0x8106( uint8_t linkno, uint8_t *pmsg )
{
    jt808_param_0x8106( linkno, pmsg );
    return 1;
}

/*��ѯ�ն�����,Ӧ�� 0x0107*/
static int handle_rx_0x8107( uint8_t linkno, uint8_t *pmsg )
{
    uint8_t				buf[100];
    uint8_t				tempbuf[80];		///��ȡTCB�ļ��е���Ϣ
    uint8_t				len1, len2;
    uint32_t			u32_data;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    sst25_read( 0x001000 + 32, tempbuf, 80 );
    rt_sem_release( &sem_dataflash );
    buf[0]	= jt808_param.id_0xF004 >> 8;
    buf[1]	= jt808_param.id_0xF004 & 0xFF;
#ifdef JT808_TEST_JTB
    memcpy( buf + 2, jt808_param.id_0xF000, 5 );    /*������ID*/
    memcpy( buf + 7, jt808_param.id_0xF001, 20 );   /*�ն��ͺ�*/
    memcpy( buf + 27, jt808_param.id_0xF002, 7 );   /*�ն�ID*/
    memcpy( buf + 34, ICCID, 10 );           		/*�ն�SIM��ICCID*/
    len1	= strlen( jt808_param.id_0xF011 );
    buf[44] = len1;									/*�ն�Ӳ���汾�ų���*/
    memcpy( buf + 45, jt808_param.id_0xF011, len1 );
    if(SOFT_VER == 101)
    {
        len2			= strlen("TW703_SW");
        buf[45 + len1]	= len2;							/*�ն˹̼��汾�ų���*/
        memcpy( buf + 46 + len1, "TW703_SW", len2 );
    }
    else
    {
        /*
        	len2			= strlen("TW705_SW02");
        	buf[45 + len1]	= len2;							//�ն˹̼��汾�ų���
        	memcpy( buf + 46 + len1, "TW705_SW02", len2 );
        	*/

        len2			= strlen( jt808_param.id_0xF010 );
        buf[45 + len1]	= len2;							//�ն˹̼��汾�ų���
        memcpy( buf + 46 + len1, jt808_param.id_0xF010, len2 );

    }
#else
    if(memcmp(tempbuf + 1, "bin", 3))			///���ƥ���ʾ�Ѿ�������TCB�ļ�
    {
        memcpy( buf + 2, jt808_param.id_0xF000, 5 );    /*������ID*/
        memcpy( buf + 7, jt808_param.id_0xF001, 20 );   /*�ն��ͺ�*/
        memcpy( buf + 27, jt808_param.id_0xF002, 7 );   /*�ն�ID*/
        memcpy( buf + 34, ICCID, 10 );           		/*�ն�SIM��ICCID*/
        len1	= strlen( jt808_param.id_0xF011 );
        buf[44] = len1;									/*�ն�Ӳ���汾�ų���*/
        memcpy( buf + 45, jt808_param.id_0xF011, len1 );
        len2			= strlen( jt808_param.id_0xF010 );
        buf[45 + len1]	= len2;							/*�ն˹̼��汾�ų���*/
        memcpy( buf + 46 + len1, jt808_param.id_0xF010, len2 );
    }
    else
    {
        memcpy( buf + 2, tempbuf + 13, 5 );    			/*������ID*/
        memcpy( buf + 7, tempbuf + 18, 20 );   			/*�ն��ͺ�*/
        memcpy( buf + 27, jt808_param.id_0xF002, 7 );   /*�ն�ID*/
        memcpy( buf + 34, ICCID, 10 );           		/*�ն�SIM��ICCID*/
        u32_data	= buf_to_data(tempbuf + 38, 4);
        sprintf(tempbuf, "HV%02d", u32_data);
        len1	= strlen( tempbuf );
        buf[44] = len1;									/*�ն�Ӳ���汾�ų���*/
        memcpy( buf + 45, tempbuf, len1 );
        len2			= strlen( tempbuf + 42 );
        buf[45 + len1]	= len2;							/*�ն˹̼��汾�ų���*/
        memcpy( buf + 46 + len1, tempbuf + 42 , len2 );
    }
#endif
    buf[46 + len1 + len2]		= 0x03;
    buf[46 + len1 + len2 + 1]	= 0x01;
    jt808_tx_ack_ex(linkno, 0x0107, buf, ( 46 + len1 + len2 + 2 ) );
    return 1;
}


/*��ѯָ���ն˲���,����Ӧ��0x0108*/
static int handle_rx_0x8108( uint8_t linkno, uint8_t *pmsg )
{
    return updata_jt808_0x8108( linkno, pmsg );
}


/*λ����Ϣ��ѯ*/
static int handle_rx_0x8201( uint8_t linkno, uint8_t *pmsg )
{
    uint8_t buf[40];
    buf[0]	= pmsg[10];
    buf[1]	= pmsg[11];

    memcpy( buf + 2, (uint8_t *)&gps_baseinfo, 28 );
    jt808_tx_ack_ex(linkno, 0x0201, buf, 30 );

    return 1;
}

/*��ʱλ�ø��ٿ���*/
static int handle_rx_0x8202( uint8_t linkno, uint8_t *pmsg )
{
    uint16_t interval;
    interval = ( pmsg[12] << 8 ) | pmsg[13];
    if( interval == 0 )
    {
        jt808_8202_track_duration = 0; /*ֹͣ����*/
    }
    else
    {
        jt808_8202_track_interval	= interval;
        jt808_8202_track_duration	= ( pmsg[14] << 24 ) | ( pmsg[15] << 16 ) | ( pmsg[16] << 8 ) | ( pmsg[17] );
        jt808_8202_track_counter	= 0;
    }
    jt808_tx_0x0001_ex(linkno, ( pmsg[10] << 8 ) | pmsg[11], 0x8202, 0 );
    return 1;
}

/*�˹�ȷ�ϱ�����Ϣ*/
static int handle_rx_0x8203( uint8_t linkno, uint8_t *pmsg )
{
    uint16_t ack_seq;
    uint32_t alarm_clear_value;
    ack_seq	= ( pmsg[12] << 8 ) | pmsg[13];
    alarm_clear_value = ( pmsg[14] << 24 ) | ( pmsg[15] << 16 ) | ( pmsg[16] << 8 ) | ( pmsg[17] );
    if(ack_seq == 0)
    {
        alarm_clear_value |= (BIT(0) | BIT(3) | BIT(20) | BIT(21) | BIT(22) | BIT(27) | BIT(28));
    }
    alarm_clear_value &= (BIT(0) | BIT(3) | BIT(20) | BIT(21) | BIT(22) | BIT(27) | BIT(28));
    area_clear_alarm(alarm_clear_value);
    jt808_param_bk.car_alarm &= ~alarm_clear_value;
    param_save_bksram();
    return 1;
}

/*�ı���Ϣ�·�*/
static int handle_rx_0x8300( uint8_t linkno, uint8_t *pmsg )
{
    jt808_misc_0x8300(linkno, pmsg );
    return 1;
}

/*�¼�����*/
static int handle_rx_0x8301( uint8_t linkno, uint8_t *pmsg )
{
    jt808_misc_0x8301(linkno, pmsg );
    return 1;
}

/*�����·�*/
static int handle_rx_0x8302( uint8_t linkno, uint8_t *pmsg )
{
    jt808_misc_0x8302(linkno, pmsg );
    return 1;
}

/*��Ϣ�㲥�˵�����*/
static int handle_rx_0x8303( uint8_t linkno, uint8_t *pmsg )
{
    jt808_misc_0x8303(linkno, pmsg );
    return 1;
}

/*��Ϣ����*/
static int handle_rx_0x8304( uint8_t linkno, uint8_t *pmsg )
{
    jt808_misc_0x8304(linkno, pmsg );

    return 1;
}

/*�绰�ز�*/
static int handle_rx_0x8400( uint8_t linkno, uint8_t *pmsg )
{
    jt808_misc_0x8400(linkno, pmsg );
    return 1;
}

/*���õ绰��*/
static int handle_rx_0x8401( uint8_t linkno, uint8_t *pmsg )
{
    jt808_misc_0x8401(linkno, pmsg );
    return 1;
}

/*��������*/
static int handle_rx_0x8500( uint8_t linkno, uint8_t *pmsg )
{
    jt808_misc_0x8500(linkno, pmsg );

    return 1;
}

/*����Բ������*/
static int handle_rx_0x8600( uint8_t linkno, uint8_t *pmsg )
{
    area_jt808_0x8600( linkno, pmsg );
    return 1;
}

/*ɾ��Բ������*/
static int handle_rx_0x8601( uint8_t linkno, uint8_t *pmsg )
{
    area_jt808_0x8601( linkno, pmsg );

    return 1;
}

/*���þ�������*/
static int handle_rx_0x8602( uint8_t linkno, uint8_t *pmsg )
{
    area_jt808_0x8602( linkno, pmsg );

    return 1;
}

/*ɾ����������*/
static int handle_rx_0x8603( uint8_t linkno, uint8_t *pmsg )
{
    area_jt808_0x8603( linkno, pmsg );

    return 1;
}

/*���ö��������*/
static int handle_rx_0x8604( uint8_t linkno, uint8_t *pmsg )
{
    area_jt808_0x8604( linkno, pmsg );

    return 1;
}

/*ɾ�����������*/
static int handle_rx_0x8605( uint8_t linkno, uint8_t *pmsg )
{
    area_jt808_0x8605( linkno, pmsg );

    return 1;
}

/*����·��*/
static int handle_rx_0x8606( uint8_t linkno, uint8_t *pmsg )
{
    area_jt808_0x8606( linkno, pmsg );

    return 1;
}

/*ɾ��·��*/
static int handle_rx_0x8607( uint8_t linkno, uint8_t *pmsg )
{
    area_jt808_0x8607( linkno, pmsg );

    return 1;
}

/*��ʻ��¼�����ݲɼ�*/

static int handle_rx_0x8700( uint8_t linkno, uint8_t *pmsg )
{
    Send_gb_vdr_Rx_8700_ack( linkno, pmsg );
    //vdr_rx_8700( linkno, pmsg );
    return 1;
}

/*��ʻ��¼�ǲ����´�*/
static int handle_rx_0x8701( uint8_t linkno, uint8_t *pmsg )
{
    //vdr_rx_8701( linkno, pmsg );
    Send_gb_vdr_Rx_8701_ack( linkno, pmsg );
    return 1;
}


/*��ʻ��¼�ǲ����´�*/
static int handle_rx_0x8702( uint8_t linkno, uint8_t *pmsg )
{
    IC_CARD_jt808_0x0702(linkno, 1);
    return 1;
}


/*
   ��ý�������ϴ�Ӧ��
   ���в�ͬ����Ϣͨ���˽ӿ�
 */
static int handle_rx_0x8800( uint8_t linkno, uint8_t *pmsg )
{
    MsgListNode			 *iter;
    JT808_TX_NODEDATA	 *iterdata;
    //	uint32_t			media_id;

    /*������Ϣͷ12byte*/

    iter = list_jt808_tx->first;
    while( iter != RT_NULL )
    {
        iterdata = (JT808_TX_NODEDATA *)iter->data;
        if(iterdata->state != WAIT_ACK)
        {
            iter = iter->next;
            break;
        }

        if( iterdata->head_id == 0x0801 )
        {
            iterdata->cb_tx_response( iterdata, pmsg ); /*Ӧ������*/
            return 1;
        }
        iter = iter->next;
    }
#if 0
    media_id = ( pmsg[12] << 24 ) | ( pmsg[13] << 16 ) | ( pmsg[14] << 8 ) | ( pmsg[15] );

    while( iter != RT_NULL )
    {
        iterdata = (JT808_TX_NODEDATA *)iter->data;
        if( iterdata->head_id == media_id )             /*���ﲻ��*/
        {
            iterdata->cb_tx_response( iterdata, pmsg ); /*Ӧ������*/
            iterdata->state = ACK_OK;
            break;
        }
        else
        {
            iter = iter->next;
        }
    }
#endif
    return 1;
}

/*����ͷ������������*/
static int handle_rx_0x8801( uint8_t linkno, uint8_t *pmsg )
{
    Cam_jt808_0x8801( linkno, pmsg );
    return 1;
}

/*
   ��ý����Ϣ����
 */
static int handle_rx_0x8802( uint8_t linkno, uint8_t *pmsg )
{
    Cam_jt808_0x8802( linkno, pmsg );

    return 1;
}

/**/
static int handle_rx_0x8803( uint8_t linkno, uint8_t *pmsg )
{
    Cam_jt808_0x8803( linkno, pmsg );

    return 1;
}

/*¼����ʼ*/
static int handle_rx_0x8804( uint8_t linkno, uint8_t *pmsg )
{
    u8							*msg;
    u16							msg_len;
    u16							fram_num;
    u16							time_record = 0;

    msg_len		= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
    fram_num	= buf_to_data( pmsg + 10, 2 );
    msg			= pmsg + 12;
    if(msg_len >= 5)
    {
        if(msg[0])
        {
            time_record = buf_to_data(msg + 1, 2);
            if(time_record > 20)
                time_record = 20;
        }
    }
    if(time_record == 0)
        time_record = 18;
    record(time_record + 2);
    rt_kprintf("\n ��ʼ¼����time=%d", time_record);
    jt808_tx_0x0001_ex(linkno, fram_num, 0x8804, 0 );
    return 1;
}

/*�����洢��ý�����ݼ����ϴ�*/
static int handle_rx_0x8805( uint8_t linkno, uint8_t *pmsg )
{
    Cam_jt808_0x8805( linkno, pmsg );
    return 1;
}

/*��������͸��*/
static int handle_rx_0x8900( uint8_t linkno, uint8_t *pmsg )
{
    u8							*msg;
    u16							msg_len;
    u16							fram_num;

    msg_len		= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
    if(msg_len)
    {
        switch(pmsg[12])
        {
        case 0x0B:				///͸����IC��������
        {
            pmsg[12] = 0x00;
            iccard_send_data(0x40, pmsg + 12, msg_len );
            break;
        }
        case 0x41:
        {
            rt_kprintf("\n����1͸��:");
            printf_hex_data(pmsg + 13, msg_len - 1);
            break;
        }
        case 0x42:
        {
            rt_kprintf("\n����1͸��:");
            printf_hex_data(pmsg + 13, msg_len - 1);
            break;
        }
        }
    }
    jt808_tx_0x0001_ex(linkno, buf_to_data( pmsg + 10, 2 ), 0x8900, 0 );
    return 1;
}

/*ƽ̨RSA��Կ*/
static int handle_rx_0x8A00( uint8_t linkno, uint8_t *pmsg )
{
    jt808_tx_0x0001_ex(linkno, buf_to_data( pmsg + 10, 2 ), 0x8900, 0 );
    return 1;
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
static int handle_rx_default( uint8_t linkno, uint8_t *pmsg )
{
    rt_kprintf( "\nunknown!\n" );
    return 1;
}



#define DECL_JT808_RX_HANDLE( a ) { a, handle_rx_ ## a }
//#define DECL_JT808_TX_HANDLE( a )	{ a, handle_jt808_tx_ ## a }

HANDLE_JT808_RX_MSG handle_rx_msg[] =
{
    DECL_JT808_RX_HANDLE( 0x8001 ), //	ͨ��Ӧ��
    DECL_JT808_RX_HANDLE( 0x8003 ), //	�����ְ�����
    DECL_JT808_RX_HANDLE( 0x8100 ), //  ������Ķ��ն�ע����Ϣ��Ӧ��
    DECL_JT808_RX_HANDLE( 0x8103 ), //	�����ն˲���
    DECL_JT808_RX_HANDLE( 0x8104 ), //	��ѯ�ն˲���
    DECL_JT808_RX_HANDLE( 0x8105 ), //  �ն˿���
    DECL_JT808_RX_HANDLE( 0x8106 ), //  ��ѯָ���ն˲���
    DECL_JT808_RX_HANDLE( 0x8107 ), //  ��ѯ�ն�����,Ӧ�� 0x0107
    DECL_JT808_RX_HANDLE( 0x8108 ), //  �·��ն�������
    DECL_JT808_RX_HANDLE( 0x8201 ), //  λ����Ϣ��ѯ    λ����Ϣ��ѯ��Ϣ��Ϊ��
    DECL_JT808_RX_HANDLE( 0x8202 ), //  ��ʱλ�ø��ٿ���
    DECL_JT808_RX_HANDLE( 0x8203 ), //  �˹�ȷ�ϱ�����Ϣ
    DECL_JT808_RX_HANDLE( 0x8300 ), //	�ı���Ϣ�·�
    DECL_JT808_RX_HANDLE( 0x8301 ), //	�¼�����
    DECL_JT808_RX_HANDLE( 0x8302 ), //  �����·�
    DECL_JT808_RX_HANDLE( 0x8303 ), //	��Ϣ�㲥�˵�����
    DECL_JT808_RX_HANDLE( 0x8304 ), //	��Ϣ����
    DECL_JT808_RX_HANDLE( 0x8400 ), //	�绰�ز�
    DECL_JT808_RX_HANDLE( 0x8401 ), //	���õ绰��
    DECL_JT808_RX_HANDLE( 0x8500 ), //	��������
    DECL_JT808_RX_HANDLE( 0x8600 ), //	����Բ������
    DECL_JT808_RX_HANDLE( 0x8601 ), //	ɾ��Բ������
    DECL_JT808_RX_HANDLE( 0x8602 ), //	���þ�������
    DECL_JT808_RX_HANDLE( 0x8603 ), //	ɾ����������
    DECL_JT808_RX_HANDLE( 0x8604 ), //	���������
    DECL_JT808_RX_HANDLE( 0x8605 ), //	ɾ���������
    DECL_JT808_RX_HANDLE( 0x8606 ), //	����·��
    DECL_JT808_RX_HANDLE( 0x8607 ), //	ɾ��·��
    DECL_JT808_RX_HANDLE( 0x8700 ), //	�г���¼�����ݲɼ�����
    DECL_JT808_RX_HANDLE( 0x8701 ), //	��ʻ��¼�ǲ����´�����
    DECL_JT808_RX_HANDLE( 0x8800 ), //	��ý�������ϴ�Ӧ��
    DECL_JT808_RX_HANDLE( 0x8801 ), //	����ͷ��������
    DECL_JT808_RX_HANDLE( 0x8802 ), //	�洢��ý�����ݼ���
    DECL_JT808_RX_HANDLE( 0x8803 ), //	�洢��ý�������ϴ�����
    DECL_JT808_RX_HANDLE( 0x8804 ), //	¼����ʼ����
    DECL_JT808_RX_HANDLE( 0x8805 ), //	�����洢��ý�����ݼ����ϴ����� ---- ����Э��Ҫ��
    DECL_JT808_RX_HANDLE( 0x8900 ), //	��������͸��
    DECL_JT808_RX_HANDLE( 0x8A00 ), //	ƽ̨RSA��Կ
};


/*jt808��socket����

   ά����·�����в�ͬ��ԭ��
   �ϱ�״̬��ά��
   1.��δ����
   2.�������ӣ�DNS,��ʱ��Ӧ��
   3.��ֹ�ϱ����ر�ģ�������
   4.��ǰ���ڽ��п��и��£���ý���ϱ��Ȳ���Ҫ��ϵĹ���

 */


/*
   ����ص���Ҫ��M66����·�ض�ʱ��֪ͨ��socket����һ���������߳�
   1.2.3.5.6
 */
void cb_socket_close( uint8_t cid )
{
    //gsm_socket[linkno-1].state = cid;
    //rt_kprintf( "\n%d>linkno %id close:%d", rt_tick_get( ),linkno, cid );
    if( cid == 1 )
    {
        gsm_socket[0].index++;
        gsm_socket[0].state = CONNECT_IDLE;     /*������cb_socket_close���ж�*/
        //jt808_state			= JT808_AUTH;             /*��������Ҫ���¼�Ȩ*/
    }
    if( cid == 2 )
    {
        gsm_socket[1].index++;
        gsm_socket[1].state = CONNECT_IDLE;     /*������cb_socket_close���ж�*/
    }
    if( cid == 3 )
    {
        gsm_socket[2].index++;
        gsm_socket[2].state = CONNECT_IDLE;     /*������cb_socket_close���ж�*/
    }
    if( cid == 5 )                                  /*ȫ�����ӹҶ�*/
    {
    }
    //pcurr_socket = RT_NULL;
}

/*
   ���մ���
   ����jt808��ʽ������
   <linkno><����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   20130625 ����ճ�������

 */
void jt808_rx_proc_old( uint8_t *pinfo )
{
    uint8_t		 *psrc, *pdst, *pdata;
    uint16_t	total_len;
    uint8_t		linkno;
    uint16_t	i, id;
    uint8_t		flag_find	= 0;
    uint8_t		fcs			= 0;
    uint16_t	count;
    uint8_t		fstuff = 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*�Ƿ��ֽ����*/

    linkno		= pinfo [0];
    total_len	= ( pinfo [1] << 8 ) | pinfo [2];

    psrc	= pinfo + 3;
    pdst	= pinfo + 3;

    count = 0;

    /*����ճ��*/
    while( total_len )
    {
        if( *psrc == 0x7e )         /*��ͷ��β��־*/
        {
            if( count )             /*������*/
            {
                //if( fcs == 0 )      /*������ȷ,*/
                if(( fcs == 0 ) || (fcs == 0x7E) || (fcs == 0x7D))  /*������ȷ,�����ӵ�0x7E,0x7D����Ϊ��������Ĵ���û�н�У�����ת��*/
                {
                    *psrc	= 0;    /*20120711 ��Ϊ�ַ���������־*/
                    id		= ( ( *pdata ) << 8 ) | *( pdata + 1 );
                    for( i = 0; i < sizeof( handle_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
                    {
                        if( id == handle_rx_msg [i].id )
                        {
                            handle_rx_msg [i].func( linkno, pdata );
                            flag_find = 1;
                        }
                    }
                    if( !flag_find )
                    {
                        handle_rx_default( linkno, pdata );
                        flag_find = 0;
                    }
                }
                else
                {
                    rt_kprintf( "\ncount=%d,fcs err=%d", count, fcs );
                }
            }
            fcs		= 0;
            pdata	= psrc;         /*ָ������ͷ,0x7E��λ��*/
            pdst	= psrc;
        }
        else if( *psrc == 0x7d )    /*��ת���ַ��ȴ�������һ��*/
        {
            fstuff = 0x7c;
        }
        else
        {
            *pdst	= *psrc + fstuff;
            fstuff	= 0;
            count++;
            fcs ^= *pdst;
            pdst++;
        }
        psrc++;
        total_len--;
    }
}

/*
   ���մ���
   ����jt808��ʽ������
   <linkno><����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   20130625 ����ճ�������

 */
void jt808_rx_proc( uint8_t *pinfo )
{
    uint8_t		 *psrc, *pdst, *pdata;
    uint16_t	total_len;
    uint8_t		linkno;
    uint16_t	i, id;
    uint8_t		flag_find	= 0;
    uint8_t		fcs			= 0;
    uint16_t	count;
    uint8_t		fstuff = 0;				/*�Ƿ��ֽ����*/

    uint16_t			msg_len;
    uint16_t			fram_num;
    uint16_t			cur_package_total;			///��ǰ�յ��ְ����ݵ��ܰ���
    uint16_t			cur_package_num;			///��ǰ�յ��ְ����ݷְ����
    static uint16_t		first_fram_num		= 0;	///��һ���ְ��İ�֡���
    static uint16_t		last_id				= 0;	///�����յ�808����ID
    static uint16_t		multi_package_total	= 0;	///�ְ����ܰ���
    static uint16_t		last_package_num	= 0;	///�����յİ����
    static uint8_t 		jt808_last_linkno 	= 0;	///���һ�ε���·���
    static uint8_t 		jt808_rx_buf[2048];			///���ն�����ݵ���buf�����֧��2048�ֽ�
    static uint16_t		jt808_rx_buf_wr 	= 0;	///bufд���λ��
    static uint32_t		multi_pack_mark		= 0;	///�౦���ձ�ǣ���BIT0��ʼ��Ϊ1��ʾ�ɹ����յ��ð�����
    char				tempbuf[20];
    linkno		= pinfo [0];
    total_len	= ( pinfo [1] << 8 ) | pinfo [2];

    psrc	= pinfo + 3;
    pdst	= pinfo + 3;

    count = 0;

    /*����ճ��*/
    while( total_len )
    {
        if( *psrc == 0x7e )         /*��ͷ��β��־*/
        {
            if( count )             /*������*/
            {
                //if( fcs == 0 )      /*������ȷ,*/
                if(( fcs == 0 ) || (fcs == 0x7E) || (fcs == 0x7D))  /*������ȷ,�����ӵ�0x7E,0x7D����Ϊ��������Ĵ���û�н�У�����ת��*/
                {
                    jt808_send_pack_num = 0;
                    modem_poweron_count = 0;
                    tick_receive_last_pack = rt_tick_get();
                    *psrc	= 0;    /*20120711 ��Ϊ�ַ���������־*/
                    id		= ( ( *pdata ) << 8 ) | *( pdata + 1 );
                    flag_find = 0;
                    if(( ( pdata[2] & 0x20 ) == 0 ) || ( 0x8108 == id))
                    {
                        for( i = 0; i < sizeof( handle_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
                        {
                            if( id == handle_rx_msg [i].id )
                            {
                                if(id != 0x8001)
                                {
                                    sprintf(tempbuf, "�յ�����%04X", id);
                                    debug_write(tempbuf);
                                }
                                handle_rx_msg [i].func( linkno, pdata );
                                flag_find = 1;
                            }
                        }
                        if( !flag_find )
                        {
                            handle_rx_default( linkno, pdata );
                        }
                    }
                    else///������˳�����������Ķ����������,�����������������кϰ���������Ϣ���ȸ�ʽ�޸�Ϊbit11~bit0��ʾ��Ϣ���ȣ�Ҳ����֧��4096�ֽڵĳ��ȸ�ʽ
                    {
                        msg_len		= buf_to_data( pdata + 2, 2 ) & 0x3FF;
                        fram_num	= buf_to_data( pdata + 10, 2 );
                        cur_package_total	= buf_to_data( pdata + 12, 2 );
                        cur_package_num		= buf_to_data( pdata + 14, 2 );

                        rt_kprintf("\n multi_pack_rx");

                        if((1 == cur_package_num) || (last_id != id) || (cur_package_num > 31) || (multi_package_total != cur_package_total))
                        {
                            memset(jt808_rx_buf, 0, sizeof(jt808_rx_buf));
                            jt808_rx_buf_wr		= 0;
                            jt808_last_linkno	= 0;
                            multi_pack_mark		= 0;
                            last_package_num	= 0;
                            last_id				= id;
                            first_fram_num		= fram_num;
                            multi_package_total	= cur_package_total;
                            jt808_rx_buf_wr 	= 12;
                            memcpy(jt808_rx_buf, pdata, 12);
                            rt_kprintf("_0");
                        }
                        /*
                        rt_kprintf("\n msg_len=%d,cur_package_total=%d,cur_package_num=%d,last_package_num=%d,jt808_rx_buf_wr=%d,multi_package_total=%d,last_id=%d",
                        	msg_len,
                        	cur_package_total,
                        	cur_package_num,
                        	last_package_num,
                        	jt808_rx_buf_wr,
                        	multi_package_total,
                        	last_id
                        	);
                        	*/
                        if((jt808_rx_buf_wr) && (cur_package_num = last_package_num + 1))
                        {
                            if(jt808_rx_buf_wr + msg_len < sizeof(jt808_rx_buf))
                            {
                                memcpy(jt808_rx_buf + jt808_rx_buf_wr, pdata + 16, msg_len);
                                jt808_rx_buf_wr		+= msg_len;
                                multi_pack_mark 	|= BIT(cur_package_num);
                                last_package_num	= cur_package_num;
                                last_id				= id;
                                if( cur_package_total == cur_package_num )
                                {
                                    for(i = 1; i <= cur_package_total; i++)
                                    {
                                        if((multi_pack_mark & (BIT(i))) == 0)
                                            break;
                                    }
                                    if(i > cur_package_total)
                                    {
                                        rt_kprintf("\n �ְ����ݺͰ�OK,LEN=%d", jt808_rx_buf_wr);
                                        jt808_rx_buf[2]	= (uint8_t)(jt808_rx_buf_wr >> 8);
                                        jt808_rx_buf[3]	= (uint8_t)jt808_rx_buf_wr;
                                        flag_find = 0;
                                        for( i = 0; i < sizeof( handle_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
                                        {
                                            if( id == handle_rx_msg [i].id )
                                            {
                                                handle_rx_msg [i].func( linkno, jt808_rx_buf );
                                                flag_find = 1;
                                            }
                                        }
                                        if( !flag_find )
                                        {
                                            handle_rx_default( linkno, pdata );
                                        }
                                    }
                                }
                            }
                            else
                            {
                                rt_kprintf("\n �ְ��������;");
                            }
                        }

                        if( !flag_find )
                        {
                            /*
                            if(cur_package_num <= last_package_num+1)
                            {
                            	jt808_tx_0x0001_ex(linkno, fram_num, id, 0 );
                            }
                            else
                            {
                            	jt808_tx_0x0001_ex(linkno, fram_num, id, 1 );
                            }
                            */
                            jt808_tx_0x0001_ex(linkno, fram_num, id, 0 );
                            flag_find = 1;
                        }
                    }
                }
                else
                {
                    rt_kprintf( "\ncount=%d,fcs err=%d", count, fcs );
                }
            }
            fcs		= 0;
            count	= 0;
            pdata	= psrc;         /*ָ������ͷ,0x7E��λ��*/
            pdst	= psrc;
        }
        else if( *psrc == 0x7d )    /*��ת���ַ��ȴ�������һ��*/
        {
            fstuff = 0x7c;
        }
        else
        {
            *pdst	= *psrc + fstuff;
            fstuff	= 0;
            count++;
            fcs ^= *pdst;
            pdst++;
        }
        psrc++;
        total_len--;
    }
}


/*���Ϳ���*/
static void jt808_tx_proc( MsgListNode *node )
{
    MsgListNode			 *pnode		= ( MsgListNode * )node;
    JT808_TX_NODEDATA	 *pnodedata = ( JT808_TX_NODEDATA * )( pnode->data );
    rt_err_t			ret;
    char				tempbuf[24];
    uint8_t				sock = 1;

    sock	= pnodedata->linkno - 1;
    if(sock > 2)
        sock = 0;
    if( pnodedata->state == IDLE ) /*���У�������Ϣ��ʱ��û������*/
    {
        /*Ҫ�ж��ǲ��ǳ���GSM_TCPIP״̬,��ǰsocket�Ƿ����*/
        if( gsmstate( GSM_STATE_GET ) != GSM_TCPIP )
        {
            return;
        }
        //if( mast_socket->state != CONNECTED )
        if(gsm_socket[sock].state != CONNECTED)
        {
            //sock_proc(&gsm_socket[sock],SOCK_CONNECT);
            return;
        }
        if((jt808_state != JT808_REPORT) && (pnodedata->type > SINGLE_REGISTER))
        {
            return;
        }
        gsmstate( GSM_AT_SEND );
        rt_kprintf( "\n%d socket(id=%4X,sn=%d,type=%d)>", rt_tick_get( ), pnodedata->head_id, pnodedata->head_sn, pnodedata->type);
        ret = socket_write( pnodedata->linkno, pnodedata->tag_data, pnodedata->msg_len );
        gsmstate( GSM_TCPIP );
        if( ret != RT_EOK ) /*gsm<ERROR:41 ERROR:35	��������û�еȵ�ģ�鷵�ص�OK�������ط������ǵ�һ��ʱ���ٷ�*/
        {
            //gsmstate( GSM_POWEROFF );
            sock_proc(&gsm_socket[sock], SOCK_RECONNECT);
            return;
        }
        socket_idle_tick[sock] = rt_tick_get( );

        if(( pnodedata->type == SINGLE_ACK ) || (pnodedata->type == SINGLE_FIRST))                          /*Ӧ����Ϣ��ֻ��һ�飬����ɾ������*/
        {
            pnodedata->state = ACK_OK;                                                                      /*����Ϳ���ɾ����*/
        }
        else
        {

            jt808_send_pack_num++;
            pnodedata->timeout_tick = rt_tick_get( ) + ( pnodedata->retry + 1 ) * pnodedata->timeout;  /*��10��Ϊ������*/
            pnodedata->state		= WAIT_ACK;
            rt_kprintf( "\n%d>SEND %04x:%04x (%d/%d:%dms)",
                        rt_tick_get( ),
                        pnodedata->head_id,
                        pnodedata->head_sn,
                        pnodedata->retry + 1,
                        pnodedata->max_retry,
                        ( pnodedata->retry + 1 ) * pnodedata->timeout * 10 );
        }
    }

    if( pnodedata->state == WAIT_ACK )                                      /*�������Ӧ���Ƿ�ʱ*/
    {
        if( rt_tick_get( ) >= pnodedata->timeout_tick )
        {
            pnodedata->retry++;
            if( pnodedata->retry >= pnodedata->max_retry )
            {
                sprintf(tempbuf, "��Ϣ%04X���ͳ�ʱ", pnodedata->head_id);
                debug_write(tempbuf);
                pnodedata->state = pnodedata->cb_tx_timeout( pnodedata );   /*20130912 ��������,�Ѿ��жϳ�ʱ��,���ڶ�ý����Ϣ����*/
            }
            else
            {
                pnodedata->state = IDLE;                                    /*�ȴ��´η���*/
            }
        }
    }

    if( ( pnodedata->state == ACK_TIMEOUT ) || ( pnodedata->state == ACK_OK ) )
    {
        //rt_kprintf( "\n%d>free node(%04x) %p", rt_tick_get( ), pnodedata->head_id, pnodedata );
        //rt_kprintf("\n jt808_free node_(id=%4X,sn=%d,type=%d)_1",pnodedata->head_id,pnodedata->head_sn,pnodedata->type);
        rt_free( pnodedata->user_para );
        //rt_kprintf("_2");
        rt_free( pnodedata );               /*ɾ���ڵ�����*/
        list_jt808_tx->first = node->next;  /*ָ����һ��*/
        memset(node, 0, sizeof(MsgListNode));
        if(list_jt808_tx->first)
            list_jt808_tx->first->prev = RT_NULL;
        //rt_kprintf("_3");
        rt_free( node );
        //rt_kprintf("_4");
    }
}

/*
   M66�涨����socket 0..2 ��Ӧ��linknoΪ1..3
   ����linkno
   1 �ϱ���������ƽ̨
   2 �ϱ��ӱ�������ƽ̨--�п��ܻ�ͬʱ�ϱ�
   3 �ϱ�IC�����Ļ���·���������

 */
GSM_SOCKET	*mast_socket;
GSM_SOCKET	*iccard_socket;
GSM_SOCKET	gsm_socket[3];

GSM_SOCKET	*pcurr_socket = RT_NULL;

/*********************************************************************************
  *��������:uint8_t jt808_tx_register(void)
  *��������:����ע����Ϣ����λ��
  *��	��:none
  *��	��:none
  *�� �� ֵ:uint8_t:0��ʾʧ�ܣ�1��ʾ����������䷢�Ͱ�����������֤�ܷ��ͳɹ�
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t jt808_tx_register(void)
{
    uint8_t buf[64];
    uint8_t len = 0;
    uint8_t ret;
    memset(buf, 0, sizeof(buf));
    buf[len++]	= jt808_param.id_0x0081 >> 8;				/*ʡ��*/
    buf[len++]	= jt808_param.id_0x0081 & 0xff;
    buf[len++]	= jt808_param.id_0x0082 >> 8;				/*����*/
    buf[len++]	= jt808_param.id_0x0082 & 0xff;
    memcpy( buf + len, jt808_param.id_0xF000, 5 );		  /*������ID*/
    len += 5;
    memcpy( buf + len, jt808_param.id_0xF001, 20 ); 	  /*�ն��ͺ�*/
    len += 20;
    //memcpy( buf + 29, jt808_param.id_0xF002, 7 ); 	  /*�ն�ID*/
    //convert_deviceid(buf + 29,jt808_param.id_0xF002);
    if(jt808_param.id_0xF00F == 3)
        memcpy(buf + len, jt808_param.id_0xF002 + 5, 7);		///�ն�ID��7λ
    else
        memcpy(buf + len, jt808_param.id_0xF006 + 5, 7);		///�ֻ������7λ
    len += 7;

    buf[len++] = jt808_param.id_0x0084;
    if( jt808_param.id_0x0084)
    {
        strcpy( (char *)buf + len, jt808_param.id_0x0083 );	 /*��������*/
        len += strlen(jt808_param.id_0x0083);
    }
    else
    {
        strcpy( (char *)buf + len, jt808_param.id_0xF005 );	 /*����VIN*/
        len += strlen(jt808_param.id_0xF005);
    }
    ret = jt808_add_tx( mast_socket->linkno,
                        SINGLE_REGISTER,
                        0x0100,
                        -1, RT_NULL, RT_NULL,
                        len, buf, RT_NULL );
    jt808_state = JT808_WAIT;
    return ret;
}

//���У� key Ϊ�ն˲���˽ԿҲ�����ն������� ID�� M1�� IA1�� IC1 Ϊƽ̨�����ն˳��̷������֤��Կ
u8 encrypt( unsigned long key, unsigned long M1, unsigned long IA1, unsigned long IC1, unsigned char *buf, unsigned int len )
{
    int i = 0 ;
    unsigned int mkey = M1;
    if ( key == 0 )
        key = 1;
    // �������
    if (0 == mkey)
        mkey = 1;
    // ��ʼ���ܴ���
    while ( i < len )
    {
        key = IA1 * ( key % mkey) + IC1 ;
        buf[i++] ^= (unsigned char)( (key >> 20) & 0xFF ) ;
    }
    return 1 ;
}


/*********************************************************************************
  *��������:uint8_t jt808_tx_lock(uint8_t linkno)
  *��������:����ʹ��ǰ������Ϣ����λ��
  *��	��:none
  *��	��:none
  *�� �� ֵ:uint8_t:0��ʾʧ�ܣ�1��ʾ����������䷢�Ͱ�����������֤�ܷ��ͳɹ�
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t jt808_tx_lock(uint8_t linkno)
{
    uint8_t buf[96];
    uint8_t len = 0;
    uint8_t ret;
    memset(buf, 0, sizeof(buf));

    sock_proc(mast_socket, SOCK_CONNECT);
    buf[len++]	= 0;              							/*0:δ���ܣ�1:����*/
    len += data_to_buf( buf + len, 70420, 4 );        		/*������ID*/

    memcpy( buf + len, jt808_param.id_0xF014, 12 );        	/*������ϵ�ֻ��ţ�λ������ʱ���󲹡�0X00��*/
    len += 12;

    buf[len++]	= jt808_param.id_0x0081 >> 8;               /*ʡ��*/
    buf[len++]	= jt808_param.id_0x0081 & 0xff;
    buf[len++]	= jt808_param.id_0x0082 >> 8;               /*����*/
    buf[len++]	= jt808_param.id_0x0082 & 0xff;
    memcpy( buf + len, jt808_param.id_0xF000, 5 );        	/*������ID*/
    len += 5;
    memcpy( buf + len, jt808_param.id_0xF001, 20 );       	/*�ն��ͺ�*/
    len += 20;
    //memcpy( buf + 29, jt808_param.id_0xF002, 7 );       	/*�ն�ID*/
    //convert_deviceid(buf + 29,jt808_param.id_0xF002);
    if(jt808_param.id_0xF00F == 3)
        memcpy(buf + len, jt808_param.id_0xF002 + 5, 7);		///�ն�ID��7λ
    else
        memcpy(buf + len, jt808_param.id_0xF006 + 5, 7);		///�ֻ������7λ
    len += 7;

    buf[len++] = jt808_param.id_0x0084;
    strcpy( (char *)buf + len, jt808_param.id_0x0083 );  /*��������*/
    len += strlen(jt808_param.id_0x0083);
    strcpy( (char *)buf + len, jt808_param.id_0xF005 );  /*����VIN*/
    len += strlen(jt808_param.id_0xF005);
    rt_kprintf("\n��������1Ϊ:");
    printf_hex_data(buf, len);
    //encrypt(70420,22185791,23898412,87946901,buf+5,len-5);
    ret = jt808_add_tx( mast_socket->linkno,
                        SINGLE_REGISTER,
                        0x0110,
                        -1, RT_NULL, RT_NULL,
                        len, buf, RT_NULL );
    jt808_state = JT808_WAIT;
    return ret;
}


/*********************************************************************************
  *��������:uint8_t jt808_tx_register(void)
  *��������:���ͼ�Ȩ��Ϣ����λ��
  *��	��:none
  *��	��:none
  *�� �� ֵ:uint8_t:0��ʾʧ�ܣ�1��ʾ����������䷢�Ͱ�����������֤�ܷ��ͳɹ�
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t jt808_tx_auth(void)
{
    uint8_t ret;
    ret = jt808_add_tx( mast_socket->linkno,
                        SINGLE_REGISTER,
                        0x0102,
                        -1, RT_NULL, RT_NULL,
                        strlen( jt808_param.id_0xF003 ),
                        (uint8_t *)( jt808_param.id_0xF003 ), RT_NULL );
    jt808_state = JT808_WAIT;
    return ret;
}


/*********************************************************************************
  *��������:static void socket_master_proc( void )
  *��������:�����ӵĿ��ƣ�������Ϊ���豸����ͨ����������(��:��������ƽ̨��808����ƽ̨����ͨƽ̨��)
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:������
  *�޸�����:2014-11-25
  *�޸�����:
*********************************************************************************/
static void socket_master_proc( void )
{
    uint8_t buf[2];
    uint16_t len;

    if( mast_socket->proc == SOCK_NONE)   /*�����ӣ�������*/
        return;

    if( mast_socket->state == CONNECT_IDLE )
    {
        if( mast_socket->proc == SOCK_CLOSE)
            return;
        if( jt808_param_bk.updata_utctime)
        {
            strcpy( mast_socket->ipstr, jt808_param_bk.update_ip);
            mast_socket->port = jt808_param_bk.update_port;
        }
#ifdef JT808_TEST_JTB
        if(device_unlock == 0)
        {
            strcpy( mast_socket->ipstr, jt808_param.id_0x001D );
            mast_socket->port = jt808_param.id_0x001C;
            //strcpy( mast_socket->ipstr, "jt1.gghypt.net" );
            //mast_socket->port = 1008;
        }
        else
        {
            if(device_unlock == 1)
            {
                if( mast_socket->index % 6 < 3 )          /*����������*/
                {
                    strcpy( mast_socket->ipstr, jt808_param.id_0x0013 );
                    mast_socket->port = jt808_param.id_0x0018;
                }
                else  /*�����÷�����*/
                {
                    strcpy( mast_socket->ipstr, jt808_param.id_0x0017 );
                    mast_socket->port = jt808_param.id_0xF031;
                }
            }
            else
            {
                strcpy( mast_socket->ipstr, jt808_param.id_0xF049 );
                mast_socket->port = jt808_param.id_0xF04A;
            }
        }
#else
        else if( mast_socket->index % 6 < 3 )          /*����������*/
        {
            strcpy( mast_socket->ipstr, jt808_param.id_0x0013 );
            mast_socket->port = jt808_param.id_0x0018;
        }
        else  /*�����÷�����*/
        {
            strcpy( mast_socket->ipstr, jt808_param.id_0x0017 );
            mast_socket->port = jt808_param.id_0xF031;
        }
#endif
        //mast_socket->state = CONNECT_PEER;                         /*��ʱgsm_state���� GSM_SOCKET_PROC������󷵻� GSM_TCPIP*/
        sock_proc(mast_socket, mast_socket->proc);		//�����������������sock
        gsmstate( GSM_SOCKET_PROC );
        //mast_socket->proc = SOCK_CONNECT;				//��������sock
        //pcurr_socket		= mast_socket;
        //gsmstate( GSM_SOCKET_PROC );
        //jt808_state = JT808_AUTH;                                         /*��������Ҫ��Ȩ��ʼ*/
        return;
    }

    if( mast_socket->state == CONNECT_ERROR )                      /*û�����ӳɹ�,�л�������*/
    {
        mast_socket->index++;
        if(mast_socket->index >= 6)
        {
            modem_reset_proc2();
            //gsmstate( GSM_POWEROFF );
            mast_socket->index = 0;
        }
        else
        {
            mast_socket->state = CONNECT_IDLE;
        }
    }

    if( mast_socket->state == CONNECTED )                          /*��·ά��������*/
    {
        if( mast_socket->proc != SOCK_CONNECT )
        {
            sock_proc(mast_socket, mast_socket->proc);
            gsmstate( GSM_SOCKET_PROC );
            return;
        }
        //pcurr_socket = RT_NULL;
        switch( jt808_state )
        {
        case JT808_REGISTER:
            jt808_tx_register();
            break;
        case JT808_AUTH:
            jt808_tx_auth();
            break;
        case JT808_REPORT:
            if( socket_idle_tick[0] )
            {
                /*Ҫ����������*/
                if( ( rt_tick_get( ) - socket_idle_tick[0] ) >= ( jt808_param.id_0x0001 * RT_TICK_PER_SECOND ) )
                {
                    jt808_tx_ack_ex(mast_socket->linkno, 0x0002, buf, 0 );
                    socket_idle_tick[0] = rt_tick_get( ); /*�״��ñ�����ǰʱ��*/
                }
            }
            else
            {
                socket_idle_tick[0] = rt_tick_get( );     /*�״��ñ�����ǰʱ��*/
            }

            if(tick_receive_last_pack == 0)     /*�״��ñ�����ǰʱ��*/
            {
                tick_receive_last_pack 	= rt_tick_get( );
            }
            ///����������3�������ݣ����ҳ���120��û���յ���λ��Ӧ������ģ��
            else if((jt808_send_pack_num > 3) && ( ( rt_tick_get( ) - tick_receive_last_pack) >= ( 120 * RT_TICK_PER_SECOND ) ))
            {
                jt808_send_pack_num 	= 0;
                //gsmstate( GSM_POWEROFF );
                sock_proc(mast_socket, SOCK_RECONNECT);
                gsmstate( GSM_SOCKET_PROC );
            }
            break;
        }
    }
}
/*********************************************************************************
  *��������:static void socket_slave_proc( void )
  *��������:�������ӵĿ���
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:������
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static void socket_slave_proc( void )
{
}


/*********************************************************************************
  *��������:static void socket_slave_proc( void )
  *��������:����IC��������ͨ��ƽ̨�˿ڵĹ���
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:������
  *�޸�����:2014-11-25
  *�޸�����:
*********************************************************************************/
static void socket_iccard_iap_proc( void )
{
    uint8_t buf[2];
    uint16_t len;

    if( iccard_socket->proc == SOCK_NONE)   /*�����ӣ�������*/
        return;

    if( iccard_socket->state == CONNECT_IDLE )
    {
        if( iccard_socket->proc == SOCK_CLOSE)
            return;

#ifdef JT808_TEST_JTB
        strcpy( iccard_socket->ipstr, jt808_param.id_0x001A );
        iccard_socket->port = jt808_param.id_0x001B;
#else
        if( iccard_socket->index % 6 < 3 )          /*����������*/
        {
            strcpy( iccard_socket->ipstr, jt808_param.id_0x001A );
            iccard_socket->port = jt808_param.id_0x001B;
        }
        else  /*����������*/
        {
            strcpy( iccard_socket->ipstr, jt808_param.id_0x001D );
            iccard_socket->port = jt808_param.id_0x001B;
        }
#endif
        //mast_socket->state = CONNECT_PEER;                         /*��ʱgsm_state���� GSM_SOCKET_PROC������󷵻� GSM_TCPIP*/
        sock_proc(iccard_socket, mast_socket->proc);		//�����������������sock
        gsmstate( GSM_SOCKET_PROC );
        //mast_socket->proc = SOCK_CONNECT;				//��������sock
        //pcurr_socket		= mast_socket;
        //gsmstate( GSM_SOCKET_PROC );
        //jt808_state = JT808_AUTH;                                         /*��������Ҫ��Ȩ��ʼ*/
        return;
    }

    if( iccard_socket->state == CONNECT_ERROR )                      /*û�����ӳɹ�,�л�������*/
    {
        iccard_socket->index++;
        if(iccard_socket->index >= 6)
        {
            sock_proc(iccard_socket, SOCK_CLOSE);
            gsmstate( GSM_SOCKET_PROC );
            iccard_socket->state = CONNECT_IDLE;
            iccard_socket->index = 0;
        }
        else
        {
            iccard_socket->state = CONNECT_IDLE;
        }
    }

    if( iccard_socket->state == CONNECTED )                          /*��·ά��������*/
    {
        if( iccard_socket->proc != SOCK_CONNECT )
        {
            sock_proc(iccard_socket, iccard_socket->proc);
            gsmstate( GSM_SOCKET_PROC );
            return;
        }
        //pcurr_socket = RT_NULL;
        iccard_socket->index = 0;
        if( socket_idle_tick[2] )
        {
            /*Ҫ����������*/
            if( ( rt_tick_get( ) - socket_idle_tick[2] ) >= ( 180 * RT_TICK_PER_SECOND ) )
            {
                sock_proc(iccard_socket, SOCK_CLOSE);
                gsmstate( GSM_SOCKET_PROC );
                socket_idle_tick[2] = rt_tick_get( ); /*�״��ñ�����ǰʱ��*/
            }
        }
        else
        {
            socket_idle_tick[2] = rt_tick_get( );     /*�״��ñ�����ǰʱ��*/
        }
    }
}

/*
   ����IC����Զ������ƽ̨
   �Ƿ���Ҫһ�ϵ�����ӣ����ǰ�������

 */
static void socket_iccard_iap_proc_old( void )
{
    if( iccard_socket->state == CONNECT_IDLE )
    {
        if( iccard_socket->index != 2 )      /*������oiap*/
        {
            if( iccard_socket->index % 2 )   /*�����÷�����*/
            {
                strcpy( iccard_socket->ipstr, jt808_param.id_0x001A );
                iccard_socket->port = jt808_param.id_0x001B;
            }
            else  /*����������*/
            {
                strcpy( iccard_socket->ipstr, jt808_param.id_0x001D );
                iccard_socket->port = jt808_param.id_0x001B;
            }
        }
        iccard_socket->state = CONNECT_PEER;
        sock_proc(iccard_socket, SOCK_CONNECT);			//��������sock
        gsmstate( GSM_SOCKET_PROC );
    }

    if( iccard_socket->state == CONNECT_ERROR ) /*û�����ӳɹ�,�л�������*/
    {
        //gsm_socket[2].index++;
        //gsm_socket[2].state = CONNECT_IDLE;
        rt_kprintf( "\nsocket2 ���Ӵ���" );
    }
}

/*
   808���Ӵ���
   ����gsm״̬Ǩ�Ƶ�,����Ѳ��״̬����ģ�ͣ�
   �ǲ��ǴӶ����׵�״̬��������Щ��

   jt808_state   ע�ᣬ��Ȩ �������ϱ���ͣ��
   socket_state
   gsm_state

 */
static void jt808_socket_proc( void )
{
    T_GSM_STATE state;
    uint8_t		i;

    /*���GSM״̬*/
    state = gsmstate( GSM_STATE_GET );
    if( state == GSM_IDLE )
    {
        //if((device_control.off_rf_counter == 0) && jt808_param.id_0xF00F )
        if(device_control.off_rf_counter == 0)
        {
            gsmstate( GSM_POWERON );        /*��������*/
            return;
        }
    }

    /*���Ƶ���*/
    if( state == GSM_AT )               /*����Ҫ�ж����Ǹ�apn user psw ����*/
    {
        for(i = 0; i < 3; i++)
        {
            if(gsm_socket[i].proc >= SOCK_RECONNECT)
                break;
        }
        if(i < 3)
        {
            ctl_gprs( jt808_param.id_0x0010, \
                      jt808_param.id_0x0011, \
                      jt808_param.id_0x0012, \
                      1 );
        }
        /*
        if( gsm_socket[0].index % 6 < 3  )   //����������
        {
        	ctl_gprs( jt808_param.id_0x0010, \
        	          jt808_param.id_0x0011, \
        	          jt808_param.id_0x0012, \
        	          1 );
        }else 				//�ñ��÷�����
        {
        	ctl_gprs( jt808_param.id_0x0014, \
        	          jt808_param.id_0x0015, \
        	          jt808_param.id_0x0016, \
        	          1 );
        }
        */
        return;
    }
    /*���ƽ�������,�п��ܻ��޸�gsmstate״̬*/
    if( gsmstate( GSM_STATE_GET ) == GSM_TCPIP )        /*�Ѿ������ˣ�û�д�������socket*/
    {
        socket_master_proc( );
    }
    if( gsmstate( GSM_STATE_GET ) == GSM_TCPIP )        /*�Ѿ������ˣ�û�д�������socket*/
    {
        socket_slave_proc( );
    }
    if( gsmstate( GSM_STATE_GET ) == GSM_TCPIP )        /*�Ѿ������ˣ�û�д�������socket*/
    {
        socket_iccard_iap_proc( );
    }
}
extern uint8_t updata_process(void);

void jt808_tx_data_proc(void)
{
    MsgListNode			 *iter;
    /*������Ϣ��������*/
    iter = list_jt808_tx->first;
    if( iter == RT_NULL )		/*û��Ҫ���͵�����*/
    {
        if( mast_socket->state == CONNECTED )		///����
        {
            if(jt808_state == JT808_REPORT)  		///�Ѿ���ȨOK
            {
                if(updata_process())
                {
                    ;
                }
                else if(jt808_report_get( ) )	//�����û��Ҫ���͵�GPS��������
                {
                    ;//Cam_jt808_get();			///�����û��Ҫ���͵�ͼƬ��������
                }
                else if(Cam_jt808_get())		///�����û��Ҫ���͵�ͼƬ��������)
                {
                    ;
                }
                else if(jt808_packt_get())
                {
                    ;
                }
                else if(jt808_can_get())
                {
                    ;
                }
                else		///��·������
                {
                    if(( jt808_status & BIT_STATUS_ACC ) == 0 )         /*��ǰ״̬ΪACC��,��̨��Ҫ����*/
                    {
                        if((jt808_param.id_0x0027 > 120 ) && ( rt_tick_get() - tick_receive_last_pack > RT_TICK_PER_SECOND * 10))
                        {
                            if((jt808_param_bk.car_alarm & BIT_ALARM_LOST_PWR) == 0)
                            {
                                //control_device(DEVICE_OFF_LINK,0xFFFFFFFF);
                                //gsmstate( GSM_POWEROFF );
                                sock_proc(mast_socket, SOCK_CLOSE);
                                return;
                            }
                        }
                    }
                }
            }
            else
            {
                ///����ʹ��ǰ����2����·�������������û�н����ɹ�����Ҫ�ر���·
                if((device_unlock == 0) && (rt_tick_get() - tick_receive_last_pack > RT_TICK_PER_SECOND * 60))
                {
                    sock_proc(mast_socket, SOCK_CLOSE);
                    return;
                }
            }
        }
    }
    else /*�����ͽڵ�״̬*/
    {
        jt808_tx_proc( iter );
    }

}

/*
   ����״̬ά��
   jt808Э�鴦��

 */
ALIGN( RT_ALIGN_SIZE )
static char thread_jt808_stack [2048];
//static char thread_jt808_stack [2048] __attribute__((section("CCM_RT_STACK")));
struct rt_thread thread_jt808;


/***/
static void rt_thread_entry_jt808( void *parameter )
{
    rt_err_t			ret;
    uint8_t				 *pstr;

    MsgListNode			 *iter;
    JT808_TX_NODEDATA	 *pnodedata;
    static uint32_t		counter_1s_tick	= 0;

    while( 1 )
    {
        /*����gprs��Ϣ,Ҫ���ַ� �������Ƕ�����ô��?�����в��ϱ�����*/
        wdg_thread_counter[3] = 1;
        ret = rt_mb_recv( &mb_gprsrx, ( rt_uint32_t * )&pstr, RT_TICK_PER_SECOND / 20 );
        if( ret == RT_EOK )
        {
            if(device_control.off_counter == 0)
            {
                jt808_rx_proc( pstr );
            }
            rt_free( pstr );
        }
        /*
        if(counter_1s%4 == 0)
        {
        	rt_device_read(&dev_mma8451, 0, RT_NULL, 0);
        }
        */
        if(rt_tick_get( ) - counter_1s_tick >= RT_TICK_PER_SECOND)
        {
            counter_1s_tick	= rt_tick_get( );
            jt808_control_proc( );
        }

        //rt_thread_delay( RT_TICK_PER_SECOND / 20 );
        if(device_control.off_counter)
        {
            continue;
        }
#if 0
        /*������Ϣ��������*/
        iter = list_jt808_tx->first;
        if( iter == RT_NULL )       /*û��Ҫ���͵�����*/
        {
            if(jt808_report_get( ) == 0)    /*�����û��Ҫ���͵�GPS��������*/
            {
                Cam_jt808_get();			///�����û��Ҫ���͵�ͼƬ��������
            }

        }
        else  /*�����ͽڵ�״̬*/
        {
            jt808_tx_proc( iter );
        }
#endif

        jt808_socket_proc( );       /*jt808 socket����*/
    }

    msglist_destroy( list_jt808_tx );
}




/*********************************************************************************
  *��������:void socket_para_init(void)
  *��������:��ʼ�����Ӷ˿ں�808��������ģ���ʼ��ʱ��Ҫ���øú���
  *��	��:	none
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-11-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void socket_para_init(void)
{
    uint8_t i;

    for(i = 0; i < 3; i++)
    {
        gsm_socket[i].state		= CONNECT_IDLE; /*����������socket����*/
        gsm_socket[i].err_no 	= 0;
        gsm_socket[i].linkno	= i + 1;
    }
    //mast_socket->proc	= SOCK_CONNECT; /*Ĭ������gsm_socket[0]����*/
    mast_socket 	= &gsm_socket[0];
    iccard_socket 	= &gsm_socket[2];
}


/*jt808�����̳߳�ʼ��*/
void jt808_init( void )
{
    //vdr_init( );
    WatchDogInit( );
    bkpsram_init( );
    jt808_vehicle_init( );  /*������Ϣ��ʼ��*/
    memset(gsm_socket, 0, sizeof(gsm_socket));
    socket_para_init( );

    if(device_unlock)
    {
        sock_proc(mast_socket, SOCK_CONNECT);
    }

    /*��ʼ��������Ϣ*/
#if 0
    if( strlen( jt808_param.id_0xF003 ) )   /*�Ƿ����м�Ȩ��*/
    {
        jt808_state = JT808_AUTH;
    }
    mast_socket->state		= CONNECT_IDLE; /*����gsm_socket[0]����*/
    mast_socket->index		= 0;
    mast_socket->linkno	= 1;
    gsm_socket[1].linkno	= 2;
    gsm_socket[2].linkno	= 3;
#endif
    list_jt808_tx	= msglist_create( );
    memset(&device_control, 0, sizeof(device_control));



    //	list_jt808_rx	= msglist_create( );
    rt_mb_init( &mb_gprsrx, "mb_gprs", &mb_gprsrx_pool, MB_GPRSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
    rt_thread_init( &thread_jt808,
                    "jt808",
                    rt_thread_entry_jt808,
                    RT_NULL,
                    &thread_jt808_stack [0],
                    sizeof( thread_jt808_stack ), 10, 5 );
    rt_thread_startup( &thread_jt808 );
}

uint8_t gprs_send_mb( uint8_t linkno, uint8_t *gprs_rx_buf, uint16_t gprs_rx_buf_wr )
{
    uint8_t *pmsg;
    if(gprs_rx_buf_wr == 0)
        return 0;
    pmsg = rt_malloc(gprs_rx_buf_wr + 3 );		/*����������Ϣ*/

    if( pmsg != RT_NULL )
    {
        pmsg [0]	= linkno;
        pmsg [1]	= gprs_rx_buf_wr >> 8;
        pmsg [2]	= gprs_rx_buf_wr & 0xff;
        memcpy( pmsg + 3, gprs_rx_buf, gprs_rx_buf_wr );
        rt_mb_send( &mb_gprsrx, ( rt_uint32_t )pmsg );
        return 0;
    }
    return 1;

}

/*gprs���մ���,�յ�����Ҫ���촦��*/
rt_err_t gprs_rx( uint8_t linkno, uint8_t *pinfo, uint16_t length )
{
    uint8_t *pmsg;
    static uint8_t 		last_linkno = 0;
    static uint8_t 		gprs_rx_buf[1600];
    //static uint8_t 		gprs_rx_buf[4096];
    static uint16_t		gprs_rx_buf_wr = 0;
    uint32_t 			i;
    uint16_t			msg_len;

    if(last_linkno != linkno)
    {
        gprs_rx_buf_wr = 0;
        last_linkno = linkno;
    }

    if((pinfo[0] == 0x7E) && (length > 2))
    {
        gprs_rx_buf_wr = 0;
    }
    /////////////////////���M66ģ���ͷ��βû��7E������start1
    if((0 == gprs_rx_buf_wr) && (pinfo[0] != 0x7E))
    {
        gprs_rx_buf[gprs_rx_buf_wr++] = 0x7E;
    }
    /////////////////////���M66ģ���ͷ��βû��7E������end1
    if( gprs_rx_buf_wr + length <= sizeof(gprs_rx_buf) )
    {
        for(i = 0; i < length; i++)
        {
            gprs_rx_buf[gprs_rx_buf_wr++] = pinfo[i];
            if((pinfo[i] == 0x7E) && (gprs_rx_buf_wr > 1))
            {
                gprs_send_mb(linkno, gprs_rx_buf, gprs_rx_buf_wr);
                gprs_rx_buf_wr = 0;
            }
        }
        //memcpy( gprs_rx_buf + gprs_rx_buf_wr, pinfo, length );
        //gprs_rx_buf_wr	+= length;
    }
    else
    {
        gprs_rx_buf_wr = 0;
    }

    if(pinfo[length - 1] != 0x7E)
    {
        /////////////////////���M66ģ���βû��7E������start2
        msg_len	= buf_to_data( pinfo + 3, 2 ) & 0x3FF;
        msg_len += 14;
        if(msg_len == gprs_rx_buf_wr)
        {
            gprs_rx_buf[gprs_rx_buf_wr++] = 0x7E;
        }
        else
            /////////////////////���M66ģ���βû��7E������end2
        {
            rt_kprintf("\n �������ݰ����ְ�;");
            return 1;
        }
    }
    i = gprs_rx_buf_wr;
    gprs_rx_buf_wr = 0;
    return gprs_send_mb(linkno, gprs_rx_buf, i);
    /*
    pmsg = rt_malloc(gprs_rx_buf_wr + 3 );		//����������Ϣ

    if( pmsg != RT_NULL )
    {
    	pmsg [0]	= linkno;
    	pmsg [1]	= gprs_rx_buf_wr >> 8;
    	pmsg [2]	= gprs_rx_buf_wr & 0xff;
    	memcpy( pmsg + 3, gprs_rx_buf, gprs_rx_buf_wr );
    	rt_mb_send( &mb_gprsrx, ( rt_uint32_t )pmsg );
    	return 0;
    }
    return 1;
    */
    ////////
#if 0
    pmsg = rt_malloc( length + 3 );
    /*����������Ϣ*/
    if( pmsg != RT_NULL )
    {
        pmsg [0]	= linkno;
        pmsg [1]	= length >> 8;
        pmsg [2]	= length & 0xff;
        memcpy( pmsg + 3, pinfo, length );
        rt_mb_send( &mb_gprsrx, ( rt_uint32_t )pmsg );
        return 0;
    }
    return 1;
#endif
}




void RX1( void )
{
    uint8_t linkno = 1;
    static u8 tempbuf[380];
    static uint16_t len = 0;
    char *str = "7E860601340130007501018B2D000000640033130101100303150101102447001000000010000000100260887F06EA05051E000000000F0000000F0260C92806EA05051E000000000E0000000E026109D106EA05051E000000000D0000000D02614A7906EA05051E000000000C0000000C02618B2206EA050A1E000000000B0000000B0261CBCB06EA05051E000000000A0000000A0261D83706EA05041E0000000009000000090261E4A206EA05041E0000000008000000080261F10E06EA05021E0000000007000000070261FD7906EA050A1E000000000600000006026209E506EA05001E000000000500000005026219EA06EA05001E000000000400000004026229F006EA05051E000000000300000003026239F006EA05001E000000000200000002026249FB06EA05051E00000000010000000102625A0006EA05001E00137E";

    rt_kprintf("\nRX1_TEST!");

    memset(tempbuf, 0, sizeof(tempbuf));
    len = Ascii_To_Hex(tempbuf, str, sizeof(tempbuf));
    if(tempbuf[0] == 0x7E)
    {
        if(tempbuf[len - 1] == 0x7E)
        {
            rt_kprintf("\n RX=\'");
            printf_hex_data(tempbuf, len);
            rt_kprintf("\'");
            gprs_rx(linkno, tempbuf, len);
            len = 0;
            memset(tempbuf, 0, sizeof(tempbuf));
        }
        return;
    }
}
FINSH_FUNCTION_EXPORT( RX1,  RX1 );




void RX(uint8_t linkno, char *str)
{
    static u8 tempbuf[256];
    static uint16_t len = 0;
    if(strncmp(str, "7E", 2) == 0)
    {
        memset(tempbuf, 0, sizeof(tempbuf));
        len = 0;
    }
    if(len < sizeof(tempbuf))
    {
        len += Ascii_To_Hex(tempbuf + len, str, sizeof(tempbuf) - len);
        if(tempbuf[0] == 0x7E)
        {
            if(tempbuf[len - 1] == 0x7E)
            {
                rt_kprintf("\n RX=\'");
                printf_hex_data(tempbuf, len);
                rt_kprintf("\'");
                gprs_rx(linkno, tempbuf, len);
                len = 0;
                memset(tempbuf, 0, sizeof(tempbuf));
            }
            return;
        }
    }
    memset(tempbuf, 0, sizeof(tempbuf));
    len = 0;
}
FINSH_FUNCTION_EXPORT( RX,  RX );

/*gprs���մ���,�յ�����Ҫ���촦��*/
rt_err_t gprs_rx_old( uint8_t linkno, uint8_t *pinfo, uint16_t length )
{
    uint8_t *pmsg;
    static uint8_t 		last_linkno = 0;
    //static uint8_t 		gprs_rx_buf[1280];
    static uint8_t 		gprs_rx_buf[4096];
    static uint16_t		gprs_rx_buf_wr = 0;

    if(last_linkno != linkno)
    {
        gprs_rx_buf_wr = 0;
        last_linkno = linkno;
    }

    if((pinfo[0] == 0x7E) && (length > 2))
    {
        gprs_rx_buf_wr = 0;
    }
    if( gprs_rx_buf_wr + length <= sizeof(gprs_rx_buf) )
    {
        memcpy( gprs_rx_buf + gprs_rx_buf_wr, pinfo, length );
        gprs_rx_buf_wr	+= length;
    }
    else
    {
        gprs_rx_buf_wr = 0;
    }

    if(pinfo[length - 1] != 0x7E)
    {
        rt_kprintf("\n �������ݰ����ְ�;");
        return 1;
    }
    pmsg = rt_malloc(gprs_rx_buf_wr + 3 );		//����������Ϣ

    if( pmsg != RT_NULL )
    {
        pmsg [0]	= linkno;
        pmsg [1]	= gprs_rx_buf_wr >> 8;
        pmsg [2]	= gprs_rx_buf_wr & 0xff;
        memcpy( pmsg + 3, gprs_rx_buf, gprs_rx_buf_wr );
        rt_mb_send( &mb_gprsrx, ( rt_uint32_t )pmsg );
        return 0;
    }
    return 1;
    ////////
#if 0
    pmsg = rt_malloc( length + 3 );
    /*����������Ϣ*/
    if( pmsg != RT_NULL )
    {
        pmsg [0]	= linkno;
        pmsg [1]	= length >> 8;
        pmsg [2]	= length & 0xff;
        memcpy( pmsg + 3, pinfo, length );
        rt_mb_send( &mb_gprsrx, ( rt_uint32_t )pmsg );
        return 0;
    }
    return 1;
#endif
}

/*
   �����豸
   �������ԭ��
 */
void reset( unsigned int reason )
{
    uint32_t i = 0x7FFFFFF;
    char temp_buf[50];
    beep(4, 4, 3);
    /*û�з��͵�����Ҫ����*/

    /*�ر�����*/


    /*��־��¼ʱ������ԭ��*/
    sprintf(temp_buf, "reset>tick=%d, reason=%u", rt_tick_get( ), reason );
    sd_write_console(temp_buf);
    wdg_reset_flag = 1;
    rt_kprintf( "\n%d reset>reason=%08x", rt_tick_get( ), reason );
    /*ִ������*/
    //rt_thread_delay( RT_TICK_PER_SECOND * 3 );

    while( i-- )
    {
        ;
    }
    NVIC_SystemReset( );
    while(1);
}
FINSH_FUNCTION_EXPORT( reset, restart device );


void off_device(void)
{

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
uint8_t list_node( void )
{
    MsgListNode			 *iter;
    JT808_TX_NODEDATA	 *pnodedata;
    uint8_t				count = 0;

    iter = list_jt808_tx->first;
    while( iter != NULL )
    {
        pnodedata = ( JT808_TX_NODEDATA * )( iter->data );
        rt_kprintf( "\nid=%04x\tseq=%04x len=%d", pnodedata->head_id, pnodedata->head_sn, pnodedata->msg_len );
        iter = iter->next;
        count++;
    }
    return count;
}

FINSH_FUNCTION_EXPORT( list_node, list node );


/*********************************************************************************
  *��������:void control_device(ENUM_DEVICE_CONTROL control,uint32_t timer)
  *��������:�Գ�̨������ؿ��ƵĿ��ƺ�������Ҫ������̨����λģ�飬�ر�������·��
  			�ر�����ͨ�ŵȲ����ǵ��øú������ú���Ĭ����10��������ز���
  *��	��:	control	��ʾҪ���еĲ���
  			timer	��ʾ���иò����ĳ���ʱ�䣬����Ҫ�ر���·ͨ��ʱ����ʱ���ʾ�ر���·ͨ�ŵ�ʱ����
  					��λ��̨�̶��ȴ�ʱ��Ϊ20��
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-010-10
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void control_device(ENUM_DEVICE_CONTROL control, uint32_t timer)
{
    uint8_t value;
    rt_kprintf("\n%d  control_device = %d", rt_tick_get(), control);
    if(DEVICE_NORMAL == control)
    {
        memset(&device_control, 0, sizeof(device_control));
        return;
    }
    if(DEVICE_OFF_DEVICE == control)
    {
        device_control.off_counter		= timer;
        if(timer)
        {
            device_control.operator |= BIT(DEVICE_OFF_DEVICE);
        }
    }
    else if(DEVICE_OFF_LINK == control)
    {
        device_control.off_link_counter	= timer;
        if(timer > 30)
        {
            device_control.operator |= BIT(DEVICE_OFF_LINK);
        }
    }
    else if(DEVICE_OFF_RF == control)
    {
        device_control.off_rf_counter	= timer;
        if(timer > 30)
        {
            device_control.operator |= BIT(DEVICE_OFF_RF);
        }
    }
    else if(DEVICE_RESET_DEVICE == control)
    {
        device_control.operator |= BIT(DEVICE_RESET_DEVICE);
        device_control.delay_counter = 20;
    }
    else
    {
        return;
    }

    value	= (uint8_t)control;
    if(device_control.delay_counter < 10)
        device_control.delay_counter	= 10;	//10������
    device_control.delay_counter	= 0;	//10������
    if(timer == 0)
    {
        device_control.operator &= ~ (BIT(control));
    }

}
FINSH_FUNCTION_EXPORT( control_device, control_device );




/*********************************************************************************
  *��������:void jt808_control_proc(void)
  *��������:�Գ�̨������ؿ��ƵĴ��������ú��������ڱ�ĵط����ã�ֻ����һ���̶����߳��ж�ʱ1���ӵ���һ��
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-010-10
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void jt808_control_proc(void)
{
    if(device_control.delay_counter)
    {
        device_control.delay_counter--;
        return;
    }
    if(device_control.off_link_counter)
    {
        device_control.off_link_counter--;
    }
    if(device_control.off_rf_counter)
    {
        device_control.off_rf_counter--;
    }

    if(device_control.off_counter)
    {
        device_control.off_counter--;
    }
    if(device_control.operator & BIT(DEVICE_OFF_DEVICE))		///ִ�йػ�����
    {
        device_control.operator &= ~(BIT(DEVICE_OFF_DEVICE));
        ///����û��ػ�����
        gsmstate( GSM_POWEROFF );
        rt_device_close( &dev_gps );
    }
    if(device_control.operator & BIT(DEVICE_OFF_LINK))			///ִ�йر�����ͨ�Ų���
    {
        device_control.operator &= ~(BIT(DEVICE_OFF_LINK));
        gsmstate( GSM_POWEROFF );
    }
    if(device_control.operator & BIT(DEVICE_OFF_RF))			///ִ�йر�����ͨ��
    {
        device_control.operator &= ~(BIT(DEVICE_OFF_RF));
        gsmstate( GSM_POWEROFF );
    }
    if(device_control.operator & BIT(DEVICE_RESET_DEVICE))		///ִ�и�λ����
    {
        device_control.operator &= ~(BIT(DEVICE_RESET_DEVICE));
        reset( 3 );
    }
}


/*********************************************************************************
  *��������:uint8_t get_sock_state(uint8_t sock)
  *��������:��ȡָ�������ӵ�״̬��0��ʾû�����ӳɹ���1��ʾTCP���ӳɹ���2��ʾ���Ӻͼ�Ȩ��OK
  *��	��:	sock	:��·״̬
  *��	��:	none
  *�� �� ֵ:uint8_t.	0��ʾû�����ӳɹ���1��ʾTCP���ӳɹ���2��ʾ���Ӻͼ�Ȩ��OK
  *��	��:������
  *��������:2014-01-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t get_sock_state(uint8_t sock)
{
    uint8_t ret = 0;
    if( gsm_socket[sock].state == CONNECTED )
    {
        ret++;
        if(jt808_state == JT808_REPORT)
            ret++;
    }
    return ret;
}



/*********************************************************************************
  *��������:uint8_t sock_proc(ENUM_SOCK_CONTROL	proc1)
  *��������:�Ե�ǰ��·���в���
  *��	��:	sock	:�����Ķ˿�
  			proc1	:��Ҫ���еĲ���
  *��	��:	uint8_t	:1��ʾ������Ч��0��ʾ������Ч��������ģ��
  *�� �� ֵ:void
  *��	��:������
  *��������:2014-01-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t sock_proc(GSM_SOCKET *sock, ENUM_SOCK_CONTROL proc1)
{
    char tempbuf[21];
    uint8_t ret = 0;
    T_GSM_STATE tempState;

    //if(proc1 > SOCK_CLOSE)
    //	control_device(DEVICE_NORMAL,0);
    tempState = gsmstate( GSM_STATE_GET );
    if((tempState >= GSM_POWERON) && (tempState <= GSM_SOCKET_PROC))
    {
        if(tempState == GSM_TCPIP)
        {
            pcurr_socket	= sock;
            ret = 1;
        }
        //pcurr_socket->state = CONNECT_IDLE;
    }
    else
    {
        //���������ǰ���ܶ�SOCK���в�����Ҫ����ģ��
        gsmstate( GSM_POWEROFF );
    }

    if(sock->proc == proc1)		///�������������ӡ������Ϣ
        return 0;

    sock->proc 		= proc1;

    sprintf(tempbuf, "�˿�%d����%d,S=%d", sock->linkno, (uint8_t)proc1, tempState);
    debug_write(tempbuf);
    return ret;
}

void sock(uint8_t sock_num, ENUM_SOCK_CONTROL proc1)
{
    sock_proc(&gsm_socket[sock_num - 1], proc1);
}
FINSH_FUNCTION_EXPORT( sock, sock );


void jt808_send(uint8_t linkno, uint32_t id, char *info, uint16_t len)
{
    jt808_tx_ack_ex(linkno, id, info, len );
}
FINSH_FUNCTION_EXPORT( jt808_send, jt808_tx );
/************************************** The End Of File **************************************/
