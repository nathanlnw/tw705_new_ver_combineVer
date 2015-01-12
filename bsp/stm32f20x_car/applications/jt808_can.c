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
#include <string.h>

#include <board.h>
#include <rtthread.h>
#include <finsh.h>

#include "include.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "jt808_param.h"
#include "rtc.h"
#include "jt808_util.h"
#include "jt808_vehicle.h"
#include "jt808_area.h"
#include "menu_include.h"
#include "jt808_gps_pack.h"
#include "math.h"
#include "jt808_config.h"
#include "can.h"
#include "jt808_can.h"



#define CAN_SECT_SIZE				256
#define CAN_HEAD					0xABCDAAA1

#define CAN_BUF_SIZE				490

#ifndef PACK_NONET_NOREPORT
#define PACK_NONET_NOREPORT 		0xFF    /*δ���� δ�ϱ� */
#define PACK_REPORTED_OK			0x7F    /*���ϱ� OK		*/
#endif

typedef __packed struct
{
    uint32_t	mn;
    uint32_t	id;
    uint8_t		attr;
    uint8_t		unused;
    uint16_t	len;
    //uint32_t	msg_id;		/*�ϱ�����Ϣͷ0x0200*/
} CAN_PACK_HEAD;

typedef struct
{
    uint32_t	addr;
    uint32_t	id;
    uint8_t		attr;
    uint16_t	len;
    uint16_t	pack_count;		//��û���ϱ��ļ�¼��
    uint8_t		get_count;    	//��ȡ��δ�ϱ���¼��
    uint32_t 	get_addr;		//��ȡ��δ�ϱ���¼��Сid�Ĵ洢��ַ
    uint32_t	pack_addr[4];	//�ϱ������¼�ĵ�ַ
} CAN_PACK_INFO;

u8			save_can_pack = 1;		///��ֵΪ��0��ʾ��ҪCAN����
CAN_PACK_INFO		can_pack_curr = { 0, 0, 0, 0, 0, 0 };

extern uint8_t	*jt808_param_get(uint16_t id );


/*
   �ϱ���Ϣ�յ���Ӧ,ͨ��Ӧ�� 0x8001
 */
static JT808_MSG_STATE can_pack_response( JT808_TX_NODEDATA *nodedata, uint8_t *pmsg )
{
    uint8_t			 *msg = pmsg + 12;
    uint16_t		ack_id;
    uint16_t		ack_seq;
    uint8_t			ack_res;
    uint32_t		addr;
    CAN_PACK_HEAD 	head;
    uint32_t		i;

    ack_seq = ( msg[0] << 8 ) | msg[1];
    ack_id	= ( msg[2] << 8 ) | msg[3];
    ack_res = *( msg + 4 );
    //rt_kprintf("\r\n JT808 TX 0x0200 RESPONSE_1");
    //if( ack_res == 0 )
    if((ack_res != 1) && (ack_res != 2))
    {
        //rt_kprintf("_2");
        rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
        if( ack_id == 0x0705 )              /*�����ϱ�*/
        {
            for( i = 0; i < can_pack_curr.get_count; i++ )
            {
                sst25_read( can_pack_curr.pack_addr[i], (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) );
                if(( head.mn == CAN_HEAD) && (head.attr == PACK_NONET_NOREPORT))
                {

                    head.attr = PACK_REPORTED_OK;      /*���λ��0*/
                    rt_kprintf("\n DEL_can_pack:addr=0x%08X,id=%d", can_pack_curr.pack_addr[i], head.id);
                    sst25_write_through( can_pack_curr.pack_addr[i], (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) );
                    can_pack_curr.get_addr = can_pack_curr.pack_addr[i];
                    if( can_pack_curr.pack_count )
                    {
                        can_pack_curr.pack_count--;
                    }
                }
            }
        }
        rt_sem_release( &sem_dataflash );
    }

    return ACK_OK;
}

/*********************************************************************************
  *��������:void can_pack_init( void )
  *��������:�������һ��д���λ��,  ����δ�ϱ���¼��Сid��λ��,
  			��ʼ�����ݴ洢��ز�������ʼ��CANѹ���������ݴ洢��ز���(can_pack_init)
  			ע��:�ú���ֻ��������ʼ���洢���򣬲�������ֻ�ܳ�ʼ��һ�εĴ��룬���粻�ܳ�ʼ���ź���
  			���ܳ�ʼ���̵߳�
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-04-15
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void can_pack_init( void )
{
    uint32_t		addr;
    CAN_PACK_HEAD head;
    uint32_t		read_id = 0xFFFFFFFF;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    can_pack_curr.pack_count		= 0; /*���²���*/
    can_pack_curr.addr	= 0;
    can_pack_curr.id		= 0;
    can_pack_curr.len		= 0;

    memset( can_pack_curr.pack_addr, 0, sizeof( can_pack_curr.pack_addr ) );

    for( addr = ADDR_DF_CAN_PACK_START; addr < ADDR_DF_CAN_PACK_END; addr += CAN_SECT_SIZE )
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) );
        if( head.mn == CAN_HEAD )         /*��Ч��¼*/
        {
            if( head.id >= can_pack_curr.id ) /*���������¼,��ǰд���λ��*/
            {
                can_pack_curr.id		= head.id;
                can_pack_curr.addr		= addr; /*��ǰ��¼�ĵ�ַ*/
                can_pack_curr.len		= head.len;
            }
            if( head.attr == PACK_NONET_NOREPORT )          /*bit7=1 ��û�з���*/
            {
                can_pack_curr.pack_count++;             /*ͳ��δ�ϱ���¼��*/
                if( read_id > head.id )     /*Ҫ�ҵ���Сid�ĵ�ַ*/
                {
                    can_pack_curr.get_addr = addr;
                    read_id			= head.id;
                }
            }
        }
    }
    if(can_pack_curr.pack_count == 0)
    {
        can_pack_curr.get_addr =  can_pack_curr.addr;
    }
    rt_sem_release( &sem_dataflash );
    rt_kprintf( "\n%d>(%d)CAN_PACKд��λ��:%x id=%d ����λ��:%x id=%d",
                rt_tick_get( ),
                can_pack_curr.pack_count,
                can_pack_curr.addr,
                can_pack_curr.id,
                can_pack_curr.get_addr,
                read_id );
}



/*********************************************************************************
  *��������:uint16_t can_pack_put( uint8_t* pinfo, uint16_t len )
  *��������:������CANԭʼ���ݴ���flash
  *�� ��:	pinfo	:pinfo ����CAN���ݵ�ָ��
  			len		:can���ݳ���
  *�� ��: none
  *�� �� ֵ:uint16_t
  *�� ��:������
  *��������:2014-04-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void can_pack_put( uint8_t *pinfo, uint16_t len )
{
    CAN_PACK_HEAD head;
    uint32_t *p_addr;
    uint32_t  last_addr;

    if( can_pack_curr.addr )                  /*���ȵ��ϱ���ַ*/
    {
        last_addr			= can_pack_curr.addr;
        can_pack_curr.addr	+= ( sizeof( CAN_PACK_HEAD ) + can_pack_curr.len + CAN_SECT_SIZE - 1 );	//��63��Ϊ��ȡһ���Ƚϴ��ֵ
        //can_pack_curr.addr	&= 0xFFFFFFC0;  /*ָ����һ����ַ*/
        can_pack_curr.addr	= can_pack_curr.addr / CAN_SECT_SIZE * CAN_SECT_SIZE;  /*ָ����һ����ַ*/
        //Ŀ���ǵ�д���λ�ó�����Ҫ��ȡ��λ���ǽ���ȡ��λ�ú���һ������λ
        if(( can_pack_curr.get_addr > last_addr) && (can_pack_curr.get_addr <= can_pack_curr.addr) && (can_pack_curr.pack_count))
        {
            can_pack_curr.get_addr	= can_pack_curr.addr + CAN_SECT_SIZE;
            can_pack_curr.pack_count--;
            if( can_pack_curr.get_addr >= ADDR_DF_CAN_PACK_END )
            {
                can_pack_curr.get_addr = can_pack_curr.get_addr - ADDR_DF_CAN_PACK_END + ADDR_DF_CAN_PACK_START;
            }
        }
    }
    else                                    /*��û�м�¼*/
    {
        can_pack_curr.addr = ADDR_DF_CAN_PACK_START;
    }

    if( can_pack_curr.addr >= ADDR_DF_CAN_PACK_END )
    {
        can_pack_curr.addr = can_pack_curr.addr - ADDR_DF_CAN_PACK_END + ADDR_DF_CAN_PACK_START;
        //Ŀ���ǵ�д���λ�ó�����Ҫ��ȡ��λ���ǽ���ȡ��λ�ú���һ������λ
        if(( can_pack_curr.get_addr <= can_pack_curr.addr) && (can_pack_curr.pack_count))
        {
            can_pack_curr.get_addr	= can_pack_curr.addr + CAN_SECT_SIZE;
            can_pack_curr.pack_count--;
        }
    }
    /*
    if( ( can_pack_curr.addr & 0xFFF ) == 0 )
    {
    	sst25_erase_4k( can_pack_curr.addr );
    }
    */

    can_pack_curr.id++;
    can_pack_curr.len = len;

    head.mn		= CAN_HEAD;
    head.id		= can_pack_curr.id;
    head.len	= len;
    head.attr 	= 0xFF;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    //sst25_write_through( can_pack_curr.addr, (uint8_t*)&head, sizeof( GPS_PACK_HEAD ) );
    //sst25_write_through( can_pack_curr.addr + sizeof( GPS_PACK_HEAD ), pinfo, len );
    sst25_write_auto( can_pack_curr.addr, (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) , ADDR_DF_CAN_PACK_START, ADDR_DF_CAN_PACK_END);
    sst25_write_auto( can_pack_curr.addr + sizeof( CAN_PACK_HEAD ), pinfo, len , ADDR_DF_CAN_PACK_START, ADDR_DF_CAN_PACK_END);
    rt_sem_release( &sem_dataflash );

    rt_kprintf("\n PUT_can_pack:addr=0x%08X,id=%d", can_pack_curr.addr, head.id);
    can_pack_curr.pack_count++;                     /*����δ�ϱ���¼��*/
    if( can_pack_curr.pack_count == 1 )
    {
        can_pack_curr.get_addr = can_pack_curr.addr;
    }
}


/*********************************************************************************
  *��������:uint16_t can_pack_save( uint8_t* pinfo, uint16_t len )
  *��������:�����ܵ���һ��CAN���ݴ���buf�У����buf���ˣ������ú���can_pack_put������CANԭʼ���ݴ���flash
  *�� ��:	pinfo	:pinfo һ��CAN���ݵ�ָ��
  			len		:can���ݳ���
  *�� ��: none
  *�� �� ֵ:uint16_t
  *�� ��:������
  *��������:2014-04-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint16_t can_pack_save( uint8_t *pinfo, uint16_t len )
{
    static uint8_t data_buf[CAN_BUF_SIZE];
    static uint16_t data_len = 0;
    uint16_t time_ms;
    static uint16_t can_save_num = 0;

    if((!save_can_pack) || ( len > 16 ))		///��ȥ16û��̫���壬��������Ҫ��ȥsizeof(CAN_PACK_HEAD)+7
    {
        data_len = 0;
        return 0;
    }
    if( len > 4 )
    {
        //rt_kprintf("\n CAN_SAVE: ID=0x%08X",buf_to_data(pinfo,4));
        can_save_num++;
        rt_kprintf("\n %d>CAN_SAVE=%04d: ID=0x%08X,DATA=", rt_tick_get(), can_save_num, buf_to_data(pinfo, 4));
        printf_hex_data(pinfo + 4, len - 4);
    }
    ///len=0,��ʾ����һ��ʱ��û��can�����ˣ���Ҫ��֮ǰ�����ݴ洢
    if(0 == len)
    {
        if(data_len)
        {
            can_pack_put(data_buf, data_len);
            data_len = 0;
        }
        rt_kprintf("\n CAN_SAVE_END");
        return 0;
    }
    if(data_len + len > sizeof(data_buf))
    {
        can_pack_put(data_buf, data_len);
        data_len = 0;
    }
    if(0 == data_len)
    {
        data_len = 7;
        time_ms = time_1ms_counter % 1000;
        memset(data_buf, 0, sizeof(data_buf));
        data_buf[2] = HOUR( mytime_now );
        data_buf[3] = MINUTE( mytime_now );
        data_buf[4] = SEC( mytime_now );
        data_buf[5] = HEX2BCD(time_ms / 100);
        data_buf[6] = SEC( time_ms % 100 );
    }
    if(data_buf[1] >= 255)
    {
        data_buf[0]++;
    }
    data_buf[1]++;

    memcpy(data_buf + data_len, pinfo, len);
    data_len += len;
    return data_len;
}

/*********************************************************************************
  *��������uint8_t jt808_can_get( void )
  *��������:��ȡCAN���ݣ�������
  *�� ��: none
  *�� ��: none
  *�� �� ֵ:uint8_t
  *�� ��:������
  *��������:2014-04-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t jt808_can_get( void )
{
    CAN_PACK_HEAD 	head;
    uint8_t			buf[1024];              /*�����ϱ����ֽڲ�����256*/
    uint32_t		i;
    uint32_t		addr;
    uint16_t		get_size;               /*��ȡ��¼�Ĵ�С*/
    uint16_t		pos;
    uint8_t			read_count;             /*Ҫ��ȡ�ļ�¼��*/
    uint32_t 		*p_addr;

    if( can_pack_curr.pack_count == 0 )                 /*û����δ�ϱ���¼*/
    {
        return 0;
    }

    //if(( mast_socket->state != CONNECTED )||(jt808_state != JT808_REPORT))  /*������*/
    //{
    //	return 0;
    //}


    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    /*�������Ҽ�¼*/
    can_pack_curr.get_count	= 0;
    addr				= can_pack_curr.get_addr;
    get_size			= 3;
    pos					= 0;                            /*0x0705���ݰ�����ʼ��ƫ��*/
    read_count			= can_pack_curr.pack_count;                 /*��û�б��ļ�¼����*/
    if( can_pack_curr.pack_count > sizeof(can_pack_curr.pack_addr) / sizeof(uint32_t) )                           /*���1���ϱ�10����¼ 303�ֽ�*/
    {
        read_count = sizeof(can_pack_curr.pack_addr) / sizeof(uint32_t);
    }
    read_count = 1;		///��ȡһ�����ݣ��������θ���
    for( i = 0; i < ADDR_DF_CAN_PACK_SECT * ( 4096 / CAN_SECT_SIZE ); i++ )   /*ÿ��¼64�ֽ�,ÿsector(4096Byte)��64��¼*/
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( CAN_PACK_HEAD ) );
        if( head.mn == CAN_HEAD )                     /*��Ч��¼*/
        {
            if( head.attr == PACK_NONET_NOREPORT)      /*��û�з���*/
            {
                //rt_kprintf("\r\njt808_report_get addr=%X",addr);
                //rt_kprintf("   head=");
                //printf_hex_data((const u8 *)&head,sizeof(head));
                //�ж���Ҫ���͵��ܳ����Ƿ񳬹��������͵���󳤶�
                if( ( pos + head.len  ) >= JT808_PACKAGE_MAX )
                {
                    break;
                }
                //sst25_read( addr + sizeof(GPS_PACK_HEAD), buf + pos, head.len );
                sst25_read_auto( addr + sizeof(CAN_PACK_HEAD), buf + pos, head.len , ADDR_DF_CAN_PACK_START, ADDR_DF_CAN_PACK_END);
                pos								+=  head.len;
                can_pack_curr.pack_addr[can_pack_curr.get_count++]	= addr;

                rt_kprintf("\n GET_can_pack:addr=0x%08X,id=%d", addr, head.id);
                if( can_pack_curr.get_count >= read_count ) /*�յ�ָ����������*/
                {
                    break;
                }
            }
            addr	+= ( sizeof(CAN_PACK_HEAD) + head.len + CAN_SECT_SIZE - 1 );	//��63��Ϊ��ȡһ���Ƚϴ��ֵ
            //addr	&= 0xFFFFFFC0;  			//ָ����һ����ַ
            addr	= addr / CAN_SECT_SIZE * CAN_SECT_SIZE;			//ָ����һ����ַ
        }
        else
        {
            addr += CAN_SECT_SIZE;
        }
        if( addr >= ADDR_DF_CAN_PACK_END )
        {
            addr = ADDR_DF_CAN_PACK_START;
        }
    }
    rt_sem_release( &sem_dataflash );

    if( can_pack_curr.get_count ) /*�õ��ļ�¼��*/
    {
        buf[0] = 0x00;
        jt808_add_tx( mast_socket->linkno, SINGLE_CMD, 0x0705, -1, RT_NULL, can_pack_response, pos, buf, RT_NULL);
        return 1;

    }
    else
    {
        can_pack_curr.pack_count = 0; /*û�еõ�Ҫ�ϱ�������*/
    }
    return 0;
    //rt_kprintf( "\n%d>(%d)����λ��:%x,��¼��:%d,����:%d", rt_tick_get( ), can_pack_curr.pack_count, can_pack_curr.get_addr, can_pack_curr.get_count, pos );
}


/*********************************************************************************
  *��������void can_proc(void)
  *��������:CAN���ݱ���
  *�� ��: none
  *�� ��: none
  *�� �� ֵ:none
  *�� ��:������
  *��������:2014-04-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void can_proc(void)
{
    uint32_t i;
    uint8_t *param;
    uint32_t can_id;
    uint8_t  buf[16];
    static uint32_t	last_len = 0;
    static uint32_t	last_tick = 0;
    uint32_t			temp_u32data = 0;
    uint8_t			save_data = 1;
    static uint8_t		can_rx_data = 0;
    static uint16_t	no_save_num = 0;

    while(can_message_rxbuf_rd != can_message_rxbuf_wr )
    {
        save_data = 1;
        if(can_rx_data == 0)
        {
            can_rx_data = 1;
            debug_write("�յ�CAN����");
        }
        for(i = 0x0110; i < 0x0119; i++)
        {
            param = jt808_param_get(i);
            if(param == 0)
                continue;
            temp_u32data = buf_to_data(param, 4);
            can_id = buf_to_data(param + 4, 4);
            if(((can_id & 0x1FFFFFFF) == (can_message_rxbuf[can_message_rxbuf_rd].ExtId & 0x1FFFFFFF)) && (temp_u32data == 0))
            {
                no_save_num++;
                save_data = 0;
                rt_kprintf("\n %d>CAN_NNNN=%04d: ID=0x%08X,DATA=", rt_tick_get(), no_save_num, can_message_rxbuf[can_message_rxbuf_rd].ExtId & 0x1FFFFFFF);
                printf_hex_data(can_message_rxbuf[can_message_rxbuf_rd].Data, 8);
                break;
            }
        }
        /////
        if(save_data)
        {
            can_id = can_message_rxbuf[can_message_rxbuf_rd].ExtId & 0x1FFFFFFF;
            can_id &= 0x1FFFFFFF;
            can_id |= BIT(30);
            data_to_buf(buf, can_id, 4);
            memcpy(buf + 4, can_message_rxbuf[can_message_rxbuf_rd].Data, 8);
            last_len = can_pack_save(buf, 12);
            last_tick = rt_tick_get();
        }
        /////
        can_message_rxbuf_rd++;
        if(can_message_rxbuf_rd >= sizeof(can_message_rxbuf) / sizeof(CanRxMsg))
            can_message_rxbuf_rd = 0;
    }
    if((last_len) && (rt_tick_get() - last_tick > RT_TICK_PER_SECOND * 5))
    {
        memset(buf, 0, sizeof(buf));
        last_len = can_pack_save(buf, 0);
    }
}

void can_save(uint8_t i)
{
    save_can_pack = i;
    rt_kprintf("\n save_can_pack=%d", save_can_pack);
}
FINSH_FUNCTION_EXPORT( can_save, can_save_pack([1 | 0] ) );

/************************************** The End Of File **************************************/

