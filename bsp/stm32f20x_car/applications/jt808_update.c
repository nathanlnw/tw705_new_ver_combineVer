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
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "include.h"
#include "jt808.h"
#include <finsh.h>
#include "sst25.h"
#include "jt808_util.h"
#include "jt808_param.h"
#include "math.h"
#include "jt808_config.h"
#include "jt808_update.h"

#define UPDATA_DEBUG

uint32_t 			updata_tick = 0;		///����tick
static uint16_t		updata_delay = 5;		///����0x8003���ݵļ������λΪ��
static uint16_t		updata_packnum = 2;		///ÿ�β�����������Ĭ��Ϊ2
static uint8_t 		updata_run_state = 0;	///0��ʾû��������1��ʾ�����У�2��ʾ����ok
static uint16_t 	updata_page = 0;		///��ǰ����0x8003���ݵ����һ�����ݰ�

//#define UPDATA_USE_CONTINUE
//#define JT808_UPDATA_JTB


#ifndef BIT
#define BIT( i ) ( (unsigned long)( 1 << i ) )
#endif

typedef  __packed struct
{
    u32 file_size;      ///�����ļ���С
    u16 package_total;  ///�ܰ�����
    u16 package_size;   ///ÿ���Ĵ�С
    u16 fram_num_first; ///��һ����֡���
    u16 pack_len_first; ///��һ���ĳ���
    u8	style;          ///�����豸����
    u8	Pack_Mark[128]; ///�����,��������һλΪ0��ʾ�ð�����
} STYLE_UPDATA_STATE;




/*********************************************************************************
  *��������:void updata_flash_read_para(STYLE_UPDATA_STATE *para)
  *��������:��ȡ��������
  *��	��:	para	:���������ṹ��
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_flash_read_para( STYLE_UPDATA_STATE *para )
{
    sst25_read( ADDR_DF_UPDATA_PARA, (u8 *)para, sizeof( STYLE_UPDATA_STATE ) );
}

/*********************************************************************************
  *��������:u8 updata_flash_write_para(STYLE_UPDATA_STATE *para)
  *��������:д����������
  *��	��:	para	:���������ṹ��
  *��	��:	none
  *�� �� ֵ:u8	0:false	,	1:OK
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_flash_write_para( STYLE_UPDATA_STATE *para )
{
    u16 i;
    u8	tempbuf[256];

    for( i = 0; i < 5; i++ )
    {
        sst25_write_back( ADDR_DF_UPDATA_PARA, (u8 *)para, sizeof( STYLE_UPDATA_STATE ) );
        sst25_read( ADDR_DF_UPDATA_PARA, tempbuf, sizeof( STYLE_UPDATA_STATE ) );
        if( 0 == memcmp( tempbuf, (u8 *)para, sizeof( STYLE_UPDATA_STATE ) ) )
        {
            break;
        }
        else if( i == 5 )
        {
            rt_kprintf( "\n д��������������!" );
            return 0;
        }
    }
    return 1;
}

/*********************************************************************************
  *��������:void updata_flash_write_recv_page(STYLE_UPDATA_STATE *para)
  *��������:д�������������÷���������������������ֻ�ܽ�flash�е�ÿ���ֽڵ�1��Ϊ0�������ܽ�0��Ϊ1
  *��	��:	para	:���������ṹ��
   page	:��ǰ�ɹ����յ��ð����
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_flash_write_recv_page( STYLE_UPDATA_STATE *para, u16 page )
{
    page -= 1;
    para->Pack_Mark[page / 8] &= ~( BIT( page % 8 ) );
    sst25_write_through( ADDR_DF_UPDATA_PARA, (u8 *)para, sizeof( STYLE_UPDATA_STATE ) );
}

/*********************************************************************************
  *��������:u8 updata_flash_wr_filehead(u8 *pmsg,u16 len)
  *��������:�����������֪ͨ(808����0x0108)
  *��	��:	pmsg		:�ļ�ͷ��Ϣ
   len			:д��ĵ�һ���ĳ���
  *��	��:	none
  *�� �� ֵ:u8	0:false	,	1:OK
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_flash_wr_filehead( u8 *pmsg, u16 len )
{
    u8	*msg = pmsg;
    u8	*tempbuf;
    u16 i;
    u32 tempAddress;
    tempbuf = rt_malloc( len );
    if( tempbuf == RT_NULL )
    {
        rt_kprintf( "\n ����ռ�������!" );
        return 0;
    }
    for( i = 0; i < 5; i++ )
    {
        msg[32] = 0xFF; 		///����������,��ֵֻ��Ϊ0x01ʱboot����Ż�������������
        sst25_erase_4k( ADDR_DF_UPDATA_START );
        sst25_write_through( ADDR_DF_UPDATA_START, msg, len );
        sst25_read( ADDR_DF_UPDATA_START, tempbuf, len );
        if( 0 == memcmp( tempbuf, msg, len ) )
        {
            break;
        }
        else if( i == 5 )
        {
            rt_kprintf( "\n д�������ļ���Ϣͷ����!" );
            rt_free( tempbuf );
            return 0;
        }
    }
    rt_free( tempbuf );
    for(tempAddress = ADDR_DF_UPDATA_START + 0x1000; tempAddress < ADDR_DF_UPDATA_END; tempAddress += 0x1000)
    {
        sst25_erase_4k( tempAddress );
    }
#ifdef UPDATA_DEBUG
    rt_kprintf( "\n д�������ļ���ϢͷOK!" );
#endif
    return 1;
}

/*********************************************************************************
  *��������:u8 updata_flash_wr_file(u32 addr,u8 *pmsg,u16 len)
  *��������:�����������֪ͨ(808����0x0108)
  *��	��:	addr		:д��ĵ�ַ
   pmsg		:�ļ�ͷ��Ϣ
   len			:д��ĵ�һ���ĳ���
  *��	��:	none
  *�� �� ֵ:u8	0:false	,	1:OK
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_flash_wr_file( u32 addr, u8 *pmsg, u16 len )
{
    u8	*msg = pmsg;
    u8	*tempbuf;
    u16 i;

    tempbuf = rt_malloc( len );
    if( tempbuf == RT_NULL )
    {
        rt_kprintf( "\n ����ռ����!" );
        return 0;
    }
    for( i = 0; i < 5; i++ )
    {
        if(i < 2)
            sst25_write_through( addr, msg, len );
        else
            sst25_write_back( addr, msg, len );

        sst25_read( addr, tempbuf, len );
        if( 0 == memcmp( tempbuf, msg, len ) )
        {
            break;
        }
        else if( i == 5 )
        {
            rt_kprintf( "\n д�������ļ���Ϣ����!" );
            rt_free( tempbuf );
            return 0;
        }
    }
    rt_free( tempbuf );
    return 1;
}

/*********************************************************************************
  *��������:u8 updata_comp_file(u8 *file_info,u8 *msg_info)
  *��������:׼��Ҫ�����ĳ����ļ����ն��ڲ��洢���ļ�����Ƚ�
  *��	��:	file_info	:�ն˴洢���ļ���Ϣ
   msg_info	:808��Ϣ���͵ĳ����ļ���Ϣ
  *��	��:	none
  *�� �� ֵ:	u8	0:��ʾ�Ƚ�ʧ�ܣ�����������		1:�������������¼������в���
    2:֮ǰ������һ�룬������������	3:֮ǰ�Ѿ������ɣ����Һ͵�ǰ�汾��ͬ������Ҫ�ظ�����
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_comp_file( u8 *file_info, u8 *msg_info )
{
    u16					i;
    STYLE_UPDATA_STATE	updata_state;

    ///�Ƚ�TCB�ļ�ͷ��Ϣ�Ƿ�OK
    if( memcmp( (const char *)msg_info, "TCB.GPS.01", 10 ) != 0 )
    {
        debug_write("�����ļ�����!");
        return 0;
    }
    ///����ָ������ļ���Ϣ��
    msg_info	+= 32;
    file_info	+= 32;

    ///����������ǲ���

    ///�Ƚϴ�"�����ļ���ʽ"��"��Ʒ��Ӫ��"֮������в��֣�������ȫƥ�����
    if( memcmp( (const char *)msg_info + 1, (const char *)file_info + 1, 72 - 1 ) != 0 )
    {
        return 0;
    }

    ///�Ƚϴ�"����汾��"��"������Ϣ������CRC16"֮������в��֣�������ȫƥ�����
    if( memcmp( (const char *)msg_info + 72, (const char *)file_info + 72, 104 - 72 ) != 0 )
    {
        return 1;
    }
    updata_flash_read_para( &updata_state );

    for( i = 0; i < updata_state.package_total; i++ )
    {
        if( updata_state.Pack_Mark[i / 8] & ( BIT( i % 8 ) ) )
        {
            break;
        }
    }
    if( i == updata_state.package_total )
    {
        ///����֮ǰ�Ѿ������ɹ�������Ҫ��������
        ///�����û���������
#ifdef JT808_UPDATA_JTB
        return 1;
#endif
        return 3;
    }
    return 2; 		///�������������İ汾���ϴε���ͬ�����Լ���������
}

#if 0
/*********************************************************************************
  *��������:void updata_commit_ack_err(u16 fram_num)
  *��������:ͨ��Ӧ�𣬴���Ӧ��
  *��	��:	fram_num:Ӧ����ˮ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_commit_ack_err( u16 fram_num )
{
    u8 pbuf[8];
    data_to_buf( pbuf, fram_num, 2 );
    data_to_buf( pbuf + 2, 0x8108, 2 );
    pbuf[4] = 1;
    jt808_tx_ack_ex(mast_socket->linkno, 0x0001, pbuf, 5 );
}

/*********************************************************************************
  *��������:void updata_commit_ack_ok(u16 fram_num)
  *��������:ͨ��Ӧ�𣬵���OKӦ��
  *��	��:	fram_num:Ӧ����ˮ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_commit_ack_ok( u16 fram_num )
{
    u8 pbuf[8];
    data_to_buf( pbuf, fram_num, 2 );
    data_to_buf( pbuf + 2, 0x8108, 2 );
    pbuf[4] = 0;
    jt808_tx_ack_ex(mast_socket->linkno, 0x0001, pbuf, 5 );
}
#endif
/*********************************************************************************
  *��������:void updata_ack_ok(u16 fram_num,u8 style,u8 updata_ok)
  *��������:�����������֪ͨ(808����0x0108)
  *��	��:	fram_num	:Ӧ����ˮ��
   style		:�����豸����
   updata_ok	:�����ɹ�Ϊ0��ʧ��Ϊ1��ȡ��Ϊ2
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_ack_ok( u16 fram_num, u8 style, u8 updata_ok )
{
    u8 pbuf[8];
    pbuf[0] = style;
    pbuf[1] = updata_ok;
    jt808_tx_ack_ex(mast_socket->linkno, 0x0108, pbuf, 2 );
}

/*********************************************************************************
  *��������:u8 updata_ack(STYLE_UPDATA_STATE *para,u8 check)
  *��������:�������
  *��	��:	check	:����Ӧ���飬0��ʾ�����ֱ��Ӧ��1��ʾ�������OK�ͽ���Ӧ��
   para	:��������
  *��	��:	none
  *�� �� ֵ:u8	0:û�������ɹ���	1:�����ɹ�
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_ack( STYLE_UPDATA_STATE *para, u8 check )
{
    u16 i;
    u8	ret = 0;
    u8	*pbuf;
    u16 crc;
    u16 tempu16data;
    u32 len = 0;
    u32 tempAddr;
    u32 tempu32data;
    u32 size;
    u16 lostpage = 0;

    ///
    if( JT808_PACKAGE_MAX < 512 )
    {
        len = 512;
    }
    else
    {
        len = JT808_PACKAGE_MAX;
    }
    pbuf = rt_malloc( len );
    if( RT_NULL == pbuf )
    {
        return 0;
    }
    memset( pbuf, 0, len );
    len = 0;
    ///ԭʼ��Ϣ��ˮ��
    len += data_to_buf( pbuf, para->fram_num_first, 4 );

    ///�ش��ܰ���
    len++;

    ///�ش��� ID �б�
    for( i = 0; i < para->package_total; i++ )
    {
        if( para->Pack_Mark[i / 8] & ( BIT( i % 8 ) ) )
        {
            lostpage++;
            len += data_to_buf( pbuf + len, i + 1, 2 );
            if(( len >= JT808_PACKAGE_MAX - 8 ) || ( lostpage >= 255 ))
            {
                break;
            }
        }
    }

    size		= para->file_size - 256;	/// bin�ļ���С
    if(( lostpage == 0 ) && (size < 0x40000)) /// �����ɹ�
    {
        debug_write( "�������!" );

        ///���ļ�����CRCУ����
        tempu32data = 0;
        crc			= 0xFFFF;


        while( 1 )
        {
            tempAddr	= ADDR_DF_UPDATA_START + 256 + tempu32data;
            len			= size - tempu32data;
            if( len > 512 )
            {
                len = 512;
            }

            sst25_read( tempAddr, pbuf, len );
            crc			= CalcCRC16(pbuf, 0, len, crc );
            tempu32data += 512;
            if( tempu32data >= size )
            {
                break;
            }
        }
        sst25_read( ADDR_DF_UPDATA_START, pbuf, 256 );
        tempu16data = buf_to_data( pbuf + 134, 2 );
        if( crc == tempu16data )
        {
            pbuf[32] = 1;
            sst25_write_through( ADDR_DF_UPDATA_START + 32, pbuf + 32, 1 );
            ret = 1;
#ifndef JT808_UPDATA_JTB
            updata_ack_ok( para->fram_num_first, para->style, 0 );
#endif
        }
        else
        {
            updata_ack_ok( para->fram_num_first, para->style, 1 );
            debug_write( "����CRC_����!" );
        }
    }
#ifdef UPDATA_USE_CONTINUE
    else if( check == 0 ) ///����û�гɹ�
    {
        pbuf[4] = lostpage;
        jt808_tx_ack_ex(mast_socket->linkno, 0x8003, pbuf, len );
    }
#endif
    rt_free( pbuf );
    return ret;
}


void update_file(void)
{
    u16 i;
    u8	ret = 0;
    u8	*pbuf;
    u16 crc;
    u16 tempu16data;
    u32 len = 0;
    u32 tempAddr;
    u32 tempu32data;
    u32 size;
    u16 lostpage = 0;
    STYLE_UPDATA_STATE	temp_para;
    STYLE_UPDATA_STATE	*para;
    updata_flash_read_para( &temp_para );
    para = &temp_para;
    ///
    if( JT808_PACKAGE_MAX < 512 )
    {
        len = 512;
    }
    else
    {
        len = JT808_PACKAGE_MAX;
    }
    pbuf = rt_malloc( len );
    if( RT_NULL == pbuf )
    {
        return;
    }
    memset( pbuf, 0, len );
    len = 0;
    ///ԭʼ��Ϣ��ˮ��
    len += data_to_buf( pbuf, para->fram_num_first, 4 );

    ///�ش��ܰ���
    len++;

    ///�ش��� ID �б�
    for( i = 0; i < para->package_total; i++ )
    {
        if( para->Pack_Mark[i / 8] & ( BIT( i % 8 ) ) )
        {
            lostpage++;
            len += data_to_buf( pbuf + len, i + 1, 2 );
            if(( len >= JT808_PACKAGE_MAX - 8 ) || ( lostpage >= 255 ))
            {
                break;
            }
        }
    }

    size		= para->file_size - 256;	/// bin�ļ���С
    if(( lostpage == 0 ) && (size < 0x40000)) /// �����ɹ�
    {
        rt_kprintf( "\n �������!,\n�ļ����� bin=%d,tcb=%d\n��������:\n\n", size, para->file_size );

        sst25_read( ADDR_DF_UPDATA_START, pbuf, 256 );
        printf_hex_data(pbuf, 256);

        ///���ļ�����CRCУ����
        tempu32data = 0;
        crc			= 0xFFFF;
        rt_kprintf( "\n\n" );


        while( 1 )
        {
            tempAddr	= ADDR_DF_UPDATA_START + 256 + tempu32data;
            len			= size - tempu32data;
            if( len > 512 )
            {
                len = 512;
            }

            sst25_read( tempAddr, pbuf, len );
            printf_hex_data(pbuf, len);
            crc			= CalcCRC16(pbuf, 0, len, crc );
            tempu32data += 512;
            if( tempu32data >= size )
            {
                break;
            }
        }
        rt_kprintf( "\n\n\n\n�ļ�����\n" );
        sst25_read( ADDR_DF_UPDATA_START, pbuf, 256 );
        tempu16data = buf_to_data( pbuf + 134, 2 );
        if( crc == tempu16data )
        {
            pbuf[32] = 1;
            sst25_write_through( ADDR_DF_UPDATA_START + 32, pbuf + 32, 1 );
            ret = 1;
#ifndef JT808_UPDATA_JTB
            updata_ack_ok( para->fram_num_first, para->style, 0 );
#endif
        }
        else
        {
            updata_ack_ok( para->fram_num_first, para->style, 1 );
            rt_kprintf( "\n CRC_����,cal_crc=0x%4X,file_crc=0x%4X", crc , tempu16data);
        }
    }
#ifdef UPDATA_USE_CONTINUE
    else if( check == 0 ) ///����û�гɹ�
    {
        pbuf[4] = lostpage;
        jt808_tx_ack_ex(mast_socket->linkno, 0x8003, pbuf, len );
    }
#endif
    rt_free( pbuf );
    return;
}

FINSH_FUNCTION_EXPORT( update_file, update_file );

/*********************************************************************************
  *��������:rt_err_t updata_jt808_0x8108(uint8_t *pmsg,u16 msg_len)
  *��������:ƽ̨�·������������
  *��	��:	pmsg	:808��Ϣ������
   msg_len	:808��Ϣ�峤��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t updata_jt808_0x8108( uint8_t linkno, uint8_t *pmsg )
{
    u16							i;
    u8							*msg;
    u16							datalen;
    u32							Tempu32data;
    u32							file_lens;
    u16							msg_len;
    u16							fram_num;
    static STYLE_UPDATA_STATE	updata_state = { 0, 0, 0, 0, 0 };
    static u8					first_package_mk	= 0;
    u16							cur_package_total;
    u16							cur_package_num;
    u8							tempbuf[256];
    u8							style;
    u8 							soft_ver[64];
    u8 							new_files = 0;

    msg_len		= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
    fram_num	= buf_to_data( pmsg + 10, 2 );
    if( ( pmsg[2] & 0x20 ) == 0 )
    {
        //updata_commit_ack_err( fram_num );
        jt808_tx_0x0001_ex(linkno, fram_num, 0x8108, 1);
        goto UPDATA_END;
    }
    cur_package_total	= buf_to_data( pmsg + 12, 2 );
    cur_package_num		= buf_to_data( pmsg + 14, 2 );

    msg					= pmsg + 16;

#ifdef UPDATA_DEBUG
    rt_kprintf( "\n �յ������,�ܰ���=%4d�������=%4d,LEN=%4d", cur_package_total, cur_package_num, msg_len );
#endif
    ///�Ƿ���
    if( ( cur_package_num > cur_package_total ) || ( cur_package_num == 0 ) || ( cur_package_total <= 1 ) )
    {
        //updata_commit_ack_err( fram_num );
        jt808_tx_0x0001_ex(linkno, fram_num, 0x8108, 1);
        goto UPDATA_END;
    }
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    //updata_commit_ack_ok( fram_num );                       ///ͨ��Ӧ��
    if(!first_package_mk)
    {
        updata_flash_read_para( &updata_state );
        first_package_mk	= 1;
    }
    if(updata_page == cur_package_num)
    {
        updata_tick = 0;
    }
    if( cur_package_num == 1 )                              ///��һ��
    {
        ///��һ������
        style = msg[0];

        if( strncmp( (const char *)msg + 1, "70420", 5 ) )  ///�ж��Ƿ�712�豸����
        {
            updata_ack_ok( fram_num, style, 2 );
            goto UPDATA_END;
        }
        if( style != 0 )                                   ///�ж��Ƿ���ն˽�������
        {
            updata_ack_ok( fram_num, style, 2 );
            goto UPDATA_END;
        }
        ///��ȡ�����汾��
        if(msg[6] > 31)
        {
            Tempu32data = 31;
        }
        else
        {
            Tempu32data = msg[6];
        }
        memset( soft_ver, 0, sizeof( soft_ver ) );
        memcpy( soft_ver, msg + 7, Tempu32data );
        if(memcmp(jt808_param.id_0xF010, soft_ver, Tempu32data ))
        {
            memcpy(jt808_param.id_0xF010, soft_ver, Tempu32data + 1 );
            param_save( 0 );
            new_files = 1;
        }

        ///�������ݰ�����
        datalen = msg_len - 11 - msg[6];

        //�ļ����ܳ���
        msg		+= 7 + msg[6];
        file_lens = buf_to_data( msg, 4 );

        //ָ���������
        msg		+= 4;

#ifdef UPDATA_DEBUG
        rt_kprintf( "\n ���������汾=\"%s\"", soft_ver );
#endif
        debug_write( "�յ���������!" );
        ///����������汾�Ƚ�
        memset( tempbuf, 0, sizeof( tempbuf ) );
        sst25_read( ADDR_DF_UPDATA_START, tempbuf, 256 );

        i = updata_comp_file( tempbuf, msg );
        if(new_files)			///���808�·�������汾���°汾������֮ǰ�����ɹ�Ҳ��Ҫ��������(��Ϊ�ð汾����808��Ϣ�����İ汾�����ļ�û�й�ϵ)
        {
            if(i == 3)
            {
                i = 1;
            }
        }
        if( i == 0 ) ///����Ҫ�������ļ���ƥ��
        {
#ifdef UPDATA_DEBUG
            rt_kprintf( "\n ����ƥ�䣬��������!" );
#endif
            updata_ack_ok( fram_num, style, 2 );
            goto UPDATA_END;
        }
        else if( i == 1 )                                            ///������������
        {
#ifdef UPDATA_DEBUG
            rt_kprintf( "\n ����ƥ�䣬��ʼ����!" );
#endif
            if( updata_flash_wr_filehead( msg, datalen ) == 0 ) ///д��ؼ�ͷ��Ϣʧ��
            {
                updata_ack_ok( fram_num, style, 1 );
                goto UPDATA_END;
            }
            memset( &updata_state, 0xFF, sizeof( updata_state ) );
            updata_state.Pack_Mark[0]	&= ~( BIT( 0 ) );       ///��һ����ȷ����
            updata_state.file_size		= file_lens;
            updata_state.package_total	= cur_package_total;
            //updata_state.package_size	= msg_len;
            updata_state.package_size	= datalen;
            updata_state.fram_num_first = fram_num;
            updata_state.pack_len_first = datalen;
            updata_state.style			= style;
            updata_flash_write_para( &updata_state );
        }
        //#ifdef UPDATA_USE_CONTINUE
        else if( i == 2 ) ///������������
        {
            ///�ж���λ���·������ݸ�ʽ�Ƿ��֮ǰ������һ��ĸ�ʽ��ͬ����ͬ�Ϳ��Լ�����������������ֻ����������
            if( ( updata_state.file_size == file_lens ) && ( updata_state.package_total == cur_package_total ) && ( updata_state.pack_len_first == datalen ) )
            {
#ifdef UPDATA_DEBUG
                rt_kprintf( "\n ����ƥ�䣬��������!" );
#endif
                if(updata_state.fram_num_first != fram_num)
                {
                    updata_state.fram_num_first = fram_num;
                    updata_state.package_size	= datalen;
                    updata_flash_write_para( &updata_state );
                }

            }
            else
            {
#ifdef UPDATA_DEBUG
                rt_kprintf( "\n ����ƥ�䣬��������������ƥ�䣬��������!" );
#endif
                if( updata_flash_wr_filehead( msg, datalen ) == 0 )
                {
                    updata_ack_ok( fram_num, style, 1 );
                    goto UPDATA_END;
                }
                memset( &updata_state, 0xFF, sizeof( updata_state ) );
                updata_state.Pack_Mark[0]	&= ~( BIT( 0 ) ); ///��һ����ȷ����
                updata_state.file_size		= file_lens;
                updata_state.package_total	= cur_package_total;
                //updata_state.package_size	= msg_len;
                updata_state.package_size	= datalen;
                updata_state.fram_num_first = fram_num;
                updata_state.pack_len_first = datalen;
                updata_state.style			= style;
                updata_flash_write_para( &updata_state );
            }
        }
        //#endif
        else if( i == 3 ) ///���������ɹ�
        {
            ///����֮ǰ�Ѿ������ɹ�������Ҫ��������
            ///�����û���������
#ifdef UPDATA_DEBUG
            rt_kprintf( "\n ����֮ǰ�Ѿ������ɹ�!" );
#endif
#ifndef JT808_UPDATA_JTB
            updata_ack_ok( fram_num, style, 0 );
#endif
            goto UPDATA_SUCCESS;
        }
#ifdef UPDATA_DEBUG
        rt_kprintf("\n ����ʼ����!" );
        rt_kprintf("\n file_size     =%d", updata_state.file_size);
        rt_kprintf("\n package_total =%d", updata_state.package_total);
        rt_kprintf("\n package_size  =%d", updata_state.package_size);
        rt_kprintf("\n fram_num_first=%d", updata_state.fram_num_first);
        rt_kprintf("\n pack_len_first=%d", updata_state.pack_len_first);
        rt_kprintf("\n style         =%d", updata_state.style);
#endif
        jt808_param_bk.update_port |= 0x40000000;
        updata_run_state = 1;
        updata_page	= updata_state.package_total;
        updata_tick = rt_tick_get() +  RT_TICK_PER_SECOND * 20;
        param_save_bksram();
    }
    else   ///������
    {
        if(( cur_package_total != updata_state.package_total ) || (first_package_mk == 0))
        {
            updata_ack_ok( updata_state.fram_num_first, updata_state.style, 2 ); ///֪ͨȡ������
            goto UPDATA_END;
        }
        if( cur_package_num < updata_state.package_total )
        {
            /*
            ///��ǰ���ǵڶ��������ڶ�����С�Ƿ���ڵ�һ����С���������ȣ�����Ϊ�ڶ�����СΪ�ְ���С
            if( ( cur_package_num == 2 ) && ( updata_state.package_size != msg_len ) )
            {
            	updata_state.package_size = msg_len;
            #ifdef UPDATA_DEBUG
            rt_kprintf("\n�ڶ����޸İ���С:  package_size  =%d",updata_state.package_size);
            #endif
            	updata_flash_write_para( &updata_state );
            }else ///�ȽϺ�������ݰ��Ƿ�ͷְ���С��ͬ
            {
            */
            if( updata_state.package_size != msg_len )
            {
#ifdef UPDATA_DEBUG
                rt_kprintf( "\n �ְ���С��ƥ�䣬�����=%4d��������=%4d", fram_num, msg_len );
#endif
                updata_ack_ok( updata_state.fram_num_first, updata_state.style, 1 );
                goto UPDATA_END;
            }
            //}
        }
        ///����������
        Tempu32data = ADDR_DF_UPDATA_START + ( ( cur_package_num - 2 ) * updata_state.package_size ) + updata_state.pack_len_first;
        if( updata_flash_wr_file( Tempu32data, msg, msg_len ) )
        {
            updata_flash_write_recv_page( &updata_state, cur_package_num ); ///�޸İ���ǣ���ʾ�ð����ݳɹ�����

            if( updata_state.package_total == cur_package_num )
            {
                if( updata_ack( &updata_state, 0 ) )
                {
                    goto UPDATA_SUCCESS;
                }
            }
            else
            {
                if( updata_ack( &updata_state, 1 ) )
                {
                    goto UPDATA_SUCCESS;
                }
            }
        }
    }
UPDATA_OK:
#ifndef JT808_UPDATA_JTB
    jt808_tx_0x0001_ex(linkno, fram_num, 0x8108, 0);
#endif
    rt_sem_release( &sem_dataflash );
    return RT_EOK;
UPDATA_END:
    rt_sem_release( &sem_dataflash );
    return RT_ERROR;
UPDATA_SUCCESS:
    jt808_param_bk.updata_utctime = 0;
    jt808_param_bk.update_port &= ~0xF0000000;
    jt808_param_bk.update_port |= 0x80000000;
    param_save_bksram();
    rt_sem_release( &sem_dataflash );
    updata_run_state = 2;
    debug_write( "�����ɹ�!" );
    rt_kprintf( "\n ����������ɣ�10���λ�豸��" );
    rt_thread_delay( RT_TICK_PER_SECOND * 5 );
    reset( 2 );
    return RT_EOK;
}

void send_info(void)
{
    uint16_t	i, j;

    u8			tempbuf[256];

    sprintf(tempbuf, "����=%s", jt808_param.id_0xF006);
    debug_write(tempbuf);
    sprintf(tempbuf, "%s/%d", jt808_param.id_0x0013, jt808_param.id_0x0018);
    debug_write(tempbuf);
    sprintf(tempbuf, "%s:%d", jt808_param.id_0x0017, jt808_param.id_0xF031);
    debug_write(tempbuf);
    sprintf(tempbuf, "��������=%s", jt808_param.id_0xF00A);
    debug_write(tempbuf);
    if(jt808_param.id_0xF00D == 2)
        debug_write("��������");
    else
        debug_write("����һΣ");
    sprintf(tempbuf, "����=%s", jt808_param.id_0x0083);
    debug_write(tempbuf);

    rt_kprintf("\n Ӳ       ��:HV%d", HARD_VER);
    rt_kprintf("\n BIN�ļ��汾:V%d.%02d", SOFT_VER / 100, SOFT_VER % 100);

    memset( tempbuf, 0, sizeof( tempbuf ) );
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    sst25_read( ADDR_DF_UPDATA_START, tempbuf, 256 );
    rt_sem_release( &sem_dataflash );
    rt_kprintf("\n TCB�ļ���Ϣ:");
    if( memcmp( (const char *)tempbuf, "TCB.GPS.01", 10 ) != 0 )
    {
        rt_kprintf("����Ϣ!");
        return;
    }
    rt_kprintf("\n �豸����:GPS�ն��豸");
    rt_kprintf("\n �豸�ͺ�:%s", tempbuf + 50);
    rt_kprintf("\n Ӳ���汾:HV%d", tempbuf[73]);		///�����˸�λ
    rt_kprintf("\n �̼��汾:%s", tempbuf + 74);
    rt_kprintf("\n �� Ӫ ��:%s", tempbuf + 94);
    rt_kprintf("\n ��ʼ��ַ:");
    printf_hex_data(tempbuf + 110, 4);
    rt_kprintf("\n ����汾:");
    printf_hex_data(tempbuf + 104, 6);
    rt_kprintf("\n ");
}
FINSH_FUNCTION_EXPORT( send_info, send_info );


/*********************************************************************************
  *��������:u8 updata_ack(STYLE_UPDATA_STATE *para,u8 check)
  *��������:�������
  *��	��:	check	:����Ӧ���飬0��ʾ�����ֱ��Ӧ��1��ʾ�������OK�ͽ���Ӧ��
   para	:��������
  *��	��:	none
  *�� �� ֵ:u8	0:û��������	1:������
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t updata_process(void)
{
#ifdef JT808_UPDATA_JTB
    u16 i;
    u8	ret = 0;
    //u8	*pbuf;
    u16 crc;
    u16 tempu16data;
    u32 len = 0;
    u32 tempAddr;
    u32 tempu32data;
    u32 size;
    u16 lostpage = 0;
    u8  pbuf[32];
    STYLE_UPDATA_STATE	temp_para;
    STYLE_UPDATA_STATE	*para;
    if((jt808_param_bk.update_port & 0x80000000) && (updata_run_state == 0))
    {
        rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
        updata_flash_read_para( &temp_para );
        rt_sem_release( &sem_dataflash );
        updata_ack_ok( 0, temp_para.style, 0 );
        updata_ack_ok( 0, temp_para.style, 0 );
        jt808_param_bk.update_port &= ~0xF0000000;
    }
    else if(jt808_param_bk.update_port & 0x40000000)
    {
        updata_run_state = 1;
    }
    if(updata_run_state != 1)
        return 0;
    if((rt_tick_get() - updata_tick < RT_TICK_PER_SECOND * updata_delay) || (rt_tick_get() < updata_tick))
        return 1;
    updata_tick = rt_tick_get();

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    updata_flash_read_para( &temp_para );
    rt_sem_release( &sem_dataflash );
    para = &temp_para;

    memset( pbuf, 0, sizeof(pbuf) );
    len = 0;
    ///ԭʼ��Ϣ��ˮ��
    len += data_to_buf( pbuf, para->fram_num_first, 2 );

    ///�ش��ܰ���
    //len += data_to_buf( pbuf+len, 1, 1 );
    len++;

    ///�ش��� ID �б�
    for( i = 0; i < para->package_total; i++ )
    {
        if( para->Pack_Mark[i / 8] & ( BIT( i % 8 ) ) )
        {
            lostpage++;
            updata_page = i + 1;
            len += data_to_buf( pbuf + len, i + 1, 2 );
            //if(( len >= sizeof(pbuf) - 8 )||( lostpage>= 3 ))
            if(( len >= sizeof(pbuf) - 8 ) || ( lostpage >= updata_packnum ))
            {
                break;
            }
        }
    }
    if(lostpage == 0)
    {
        len += data_to_buf( pbuf + len, 0, 2 );
        pbuf[2] = 0;
        jt808_tx_ack_ex(mast_socket->linkno, 0x8003, pbuf, len );
        rt_kprintf("\n �������!");
        jt808_param_bk.update_port |= 0x80000000;
        updata_run_state = 0;
        ret = 1;
    }
    else
    {
        pbuf[2] = lostpage;
        jt808_tx_ack_ex(mast_socket->linkno, 0x8003, pbuf, len );
        ret = 1;
    }
    return ret;
#else
    return 0;
#endif
}


void updata_test(uint16_t delay_data, uint16_t num)
{
    if(delay_data == 0)
    {
        updata_run_state = 1;
        updata_page	= 250;
        updata_tick = rt_tick_get() +  RT_TICK_PER_SECOND * 10;
    }
    else
    {
        updata_tick = 0;
        updata_delay = delay_data;

        if((num) && (num <= 10))
        {
            updata_packnum = num;
        }
    }
}
FINSH_FUNCTION_EXPORT( updata_test, updata_test );

/************************************** The End Of File **************************************/
