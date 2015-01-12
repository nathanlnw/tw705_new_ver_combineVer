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

uint32_t 			updata_tick = 0;		///升级tick
static uint16_t		updata_delay = 5;		///发送0x8003数据的间隔，单位为秒
static uint16_t		updata_packnum = 2;		///每次补包的数量，默认为2
static uint8_t 		updata_run_state = 0;	///0表示没有升级，1表示升级中，2表示升级ok
static uint16_t 	updata_page = 0;		///当前请求0x8003数据的最后一包数据包

//#define UPDATA_USE_CONTINUE
//#define JT808_UPDATA_JTB


#ifndef BIT
#define BIT( i ) ( (unsigned long)( 1 << i ) )
#endif

typedef  __packed struct
{
    u32 file_size;      ///程序文件大小
    u16 package_total;  ///总包数量
    u16 package_size;   ///每包的大小
    u16 fram_num_first; ///第一包的帧序号
    u16 pack_len_first; ///第一包的长度
    u8	style;          ///升级设备类型
    u8	Pack_Mark[128]; ///包标记,其中任意一位为0表示该包存在
} STYLE_UPDATA_STATE;




/*********************************************************************************
  *函数名称:void updata_flash_read_para(STYLE_UPDATA_STATE *para)
  *功能描述:读取升级参数
  *输	入:	para	:升级参数结构体
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void updata_flash_read_para( STYLE_UPDATA_STATE *para )
{
    sst25_read( ADDR_DF_UPDATA_PARA, (u8 *)para, sizeof( STYLE_UPDATA_STATE ) );
}

/*********************************************************************************
  *函数名称:u8 updata_flash_write_para(STYLE_UPDATA_STATE *para)
  *功能描述:写入升级参数
  *输	入:	para	:升级参数结构体
  *输	出:	none
  *返 回 值:u8	0:false	,	1:OK
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
            rt_kprintf( "\n 写入升级参数错误!" );
            return 0;
        }
    }
    return 1;
}

/*********************************************************************************
  *函数名称:void updata_flash_write_recv_page(STYLE_UPDATA_STATE *para)
  *功能描述:写入升级参数，该方法并不擦除扇区，所以只能将flash中的每个字节的1改为0，并不能讲0改为1
  *输	入:	para	:升级参数结构体
   page	:当前成功接收到得包序号
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void updata_flash_write_recv_page( STYLE_UPDATA_STATE *para, u16 page )
{
    page -= 1;
    para->Pack_Mark[page / 8] &= ~( BIT( page % 8 ) );
    sst25_write_through( ADDR_DF_UPDATA_PARA, (u8 *)para, sizeof( STYLE_UPDATA_STATE ) );
}

/*********************************************************************************
  *函数名称:u8 updata_flash_wr_filehead(u8 *pmsg,u16 len)
  *功能描述:程序升级结果通知(808命令0x0108)
  *输	入:	pmsg		:文件头信息
   len			:写入的第一包的长度
  *输	出:	none
  *返 回 值:u8	0:false	,	1:OK
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
        rt_kprintf( "\n 分配空间错误错误!" );
        return 0;
    }
    for( i = 0; i < 5; i++ )
    {
        msg[32] = 0xFF; 		///清空升级标记,该值只有为0x01时boot程序才会引导升级程序
        sst25_erase_4k( ADDR_DF_UPDATA_START );
        sst25_write_through( ADDR_DF_UPDATA_START, msg, len );
        sst25_read( ADDR_DF_UPDATA_START, tempbuf, len );
        if( 0 == memcmp( tempbuf, msg, len ) )
        {
            break;
        }
        else if( i == 5 )
        {
            rt_kprintf( "\n 写入升级文件信息头错误!" );
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
    rt_kprintf( "\n 写入升级文件信息头OK!" );
#endif
    return 1;
}

/*********************************************************************************
  *函数名称:u8 updata_flash_wr_file(u32 addr,u8 *pmsg,u16 len)
  *功能描述:程序升级结果通知(808命令0x0108)
  *输	入:	addr		:写入的地址
   pmsg		:文件头信息
   len			:写入的第一包的长度
  *输	出:	none
  *返 回 值:u8	0:false	,	1:OK
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
u8 updata_flash_wr_file( u32 addr, u8 *pmsg, u16 len )
{
    u8	*msg = pmsg;
    u8	*tempbuf;
    u16 i;

    tempbuf = rt_malloc( len );
    if( tempbuf == RT_NULL )
    {
        rt_kprintf( "\n 分配空间错误!" );
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
            rt_kprintf( "\n 写入升级文件信息错误!" );
            rt_free( tempbuf );
            return 0;
        }
    }
    rt_free( tempbuf );
    return 1;
}

/*********************************************************************************
  *函数名称:u8 updata_comp_file(u8 *file_info,u8 *msg_info)
  *功能描述:准备要升级的程序文件盒终端内部存储的文件程序比较
  *输	入:	file_info	:终端存储的文件信息
   msg_info	:808消息发送的程序文件信息
  *输	出:	none
  *返 回 值:	u8	0:表示比较失败，不允许升级		1:可以升级，重新加载所有参数
    2:之前升级了一半，可以续传升级	3:之前已经升级成，并且和当前版本相同，不需要重复升级
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
u8 updata_comp_file( u8 *file_info, u8 *msg_info )
{
    u16					i;
    STYLE_UPDATA_STATE	updata_state;

    ///比较TCB文件头信息是否OK
    if( memcmp( (const char *)msg_info, "TCB.GPS.01", 10 ) != 0 )
    {
        debug_write("升级文件错误!");
        return 0;
    }
    ///数据指向程序文件信息区
    msg_info	+= 32;
    file_info	+= 32;

    ///跳过升级标记部分

    ///比较从"程序文件格式"到"产品运营商"之间的所有部分，必须完全匹配才行
    if( memcmp( (const char *)msg_info + 1, (const char *)file_info + 1, 72 - 1 ) != 0 )
    {
        return 0;
    }

    ///比较从"软件版本号"到"程序信息及内容CRC16"之间的所有部分，必须完全匹配才行
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
        ///程序之前已经升级成功，不需要重新升级
        ///增加用户操作代码
#ifdef JT808_UPDATA_JTB
        return 1;
#endif
        return 3;
    }
    return 2; 		///表明本次升级的版本和上次的相同，可以继续升级。
}

#if 0
/*********************************************************************************
  *函数名称:void updata_commit_ack_err(u16 fram_num)
  *功能描述:通用应答，错误应答
  *输	入:	fram_num:应答流水号
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
  *函数名称:void updata_commit_ack_ok(u16 fram_num)
  *功能描述:通用应答，单包OK应答
  *输	入:	fram_num:应答流水号
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
  *函数名称:void updata_ack_ok(u16 fram_num,u8 style,u8 updata_ok)
  *功能描述:程序升级结果通知(808命令0x0108)
  *输	入:	fram_num	:应答流水号
   style		:升级设备类型
   updata_ok	:升级成功为0，失败为1，取消为2
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void updata_ack_ok( u16 fram_num, u8 style, u8 updata_ok )
{
    u8 pbuf[8];
    pbuf[0] = style;
    pbuf[1] = updata_ok;
    jt808_tx_ack_ex(mast_socket->linkno, 0x0108, pbuf, 2 );
}

/*********************************************************************************
  *函数名称:u8 updata_ack(STYLE_UPDATA_STATE *para,u8 check)
  *功能描述:多包接收
  *输	入:	check	:进行应答检查，0表示不检查直接应答，1表示如果升级OK就进行应答
   para	:升级参数
  *输	出:	none
  *返 回 值:u8	0:没有升级成功。	1:升级成功
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
    ///原始消息流水号
    len += data_to_buf( pbuf, para->fram_num_first, 4 );

    ///重传总包数
    len++;

    ///重传包 ID 列表
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

    size		= para->file_size - 256;	/// bin文件大小
    if(( lostpage == 0 ) && (size < 0x40000)) /// 升级成功
    {
        debug_write( "升级完成!" );

        ///对文件进行CRC校验检测
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
            debug_write( "升级CRC_错误!" );
        }
    }
#ifdef UPDATA_USE_CONTINUE
    else if( check == 0 ) ///升级没有成功
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
    ///原始消息流水号
    len += data_to_buf( pbuf, para->fram_num_first, 4 );

    ///重传总包数
    len++;

    ///重传包 ID 列表
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

    size		= para->file_size - 256;	/// bin文件大小
    if(( lostpage == 0 ) && (size < 0x40000)) /// 升级成功
    {
        rt_kprintf( "\n 升级完成!,\n文件长度 bin=%d,tcb=%d\n数据如下:\n\n", size, para->file_size );

        sst25_read( ADDR_DF_UPDATA_START, pbuf, 256 );
        printf_hex_data(pbuf, 256);

        ///对文件进行CRC校验检测
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
        rt_kprintf( "\n\n\n\n文件结束\n" );
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
            rt_kprintf( "\n CRC_错误,cal_crc=0x%4X,file_crc=0x%4X", crc , tempu16data);
        }
    }
#ifdef UPDATA_USE_CONTINUE
    else if( check == 0 ) ///升级没有成功
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
  *函数名称:rt_err_t updata_jt808_0x8108(uint8_t *pmsg,u16 msg_len)
  *功能描述:平台下发拍照命令处理函数
  *输	入:	pmsg	:808消息体数据
   msg_len	:808消息体长度
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
    rt_kprintf( "\n 收到程序包,总包数=%4d，包序号=%4d,LEN=%4d", cur_package_total, cur_package_num, msg_len );
#endif
    ///非法包
    if( ( cur_package_num > cur_package_total ) || ( cur_package_num == 0 ) || ( cur_package_total <= 1 ) )
    {
        //updata_commit_ack_err( fram_num );
        jt808_tx_0x0001_ex(linkno, fram_num, 0x8108, 1);
        goto UPDATA_END;
    }
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    //updata_commit_ack_ok( fram_num );                       ///通用应答
    if(!first_package_mk)
    {
        updata_flash_read_para( &updata_state );
        first_package_mk	= 1;
    }
    if(updata_page == cur_package_num)
    {
        updata_tick = 0;
    }
    if( cur_package_num == 1 )                              ///第一包
    {
        ///第一包处理
        style = msg[0];

        if( strncmp( (const char *)msg + 1, "70420", 5 ) )  ///判断是否712设备程序
        {
            updata_ack_ok( fram_num, style, 2 );
            goto UPDATA_END;
        }
        if( style != 0 )                                   ///判断是否对终端进行升级
        {
            updata_ack_ok( fram_num, style, 2 );
            goto UPDATA_END;
        }
        ///获取升级版本号
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

        ///升级数据包长度
        datalen = msg_len - 11 - msg[6];

        //文件的总长度
        msg		+= 7 + msg[6];
        file_lens = buf_to_data( msg, 4 );

        //指向程序数据
        msg		+= 4;

#ifdef UPDATA_DEBUG
        rt_kprintf( "\n 程序升级版本=\"%s\"", soft_ver );
#endif
        debug_write( "收到升级命令!" );
        ///升级程序包版本比较
        memset( tempbuf, 0, sizeof( tempbuf ) );
        sst25_read( ADDR_DF_UPDATA_START, tempbuf, 256 );

        i = updata_comp_file( tempbuf, msg );
        if(new_files)			///如果808下发的软件版本是新版本，就算之前升级成功也需要重新升级(因为该版本是在808消息中填充的版本，和文件没有关系)
        {
            if(i == 3)
            {
                i = 1;
            }
        }
        if( i == 0 ) ///不需要升级，文件不匹配
        {
#ifdef UPDATA_DEBUG
            rt_kprintf( "\n 程序不匹配，升级错误!" );
#endif
            updata_ack_ok( fram_num, style, 2 );
            goto UPDATA_END;
        }
        else if( i == 1 )                                            ///重新升级程序
        {
#ifdef UPDATA_DEBUG
            rt_kprintf( "\n 程序匹配，开始升级!" );
#endif
            if( updata_flash_wr_filehead( msg, datalen ) == 0 ) ///写入关键头信息失败
            {
                updata_ack_ok( fram_num, style, 1 );
                goto UPDATA_END;
            }
            memset( &updata_state, 0xFF, sizeof( updata_state ) );
            updata_state.Pack_Mark[0]	&= ~( BIT( 0 ) );       ///第一包正确存入
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
        else if( i == 2 ) ///续传升级程序
        {
            ///判断上位机下发的数据格式是否和之前升级了一半的格式相同，相同就可以继续续传升级，否则只能重新升级
            if( ( updata_state.file_size == file_lens ) && ( updata_state.package_total == cur_package_total ) && ( updata_state.pack_len_first == datalen ) )
            {
#ifdef UPDATA_DEBUG
                rt_kprintf( "\n 程序匹配，续传升级!" );
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
                rt_kprintf( "\n 程序匹配，续传升级参数不匹配，重新升级!" );
#endif
                if( updata_flash_wr_filehead( msg, datalen ) == 0 )
                {
                    updata_ack_ok( fram_num, style, 1 );
                    goto UPDATA_END;
                }
                memset( &updata_state, 0xFF, sizeof( updata_state ) );
                updata_state.Pack_Mark[0]	&= ~( BIT( 0 ) ); ///第一包正确存入
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
        else if( i == 3 ) ///程序升级成功
        {
            ///程序之前已经升级成功，不需要重新升级
            ///增加用户操作代码
#ifdef UPDATA_DEBUG
            rt_kprintf( "\n 程序之前已经升级成功!" );
#endif
#ifndef JT808_UPDATA_JTB
            updata_ack_ok( fram_num, style, 0 );
#endif
            goto UPDATA_SUCCESS;
        }
#ifdef UPDATA_DEBUG
        rt_kprintf("\n 程序开始升级!" );
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
    else   ///其它包
    {
        if(( cur_package_total != updata_state.package_total ) || (first_package_mk == 0))
        {
            updata_ack_ok( updata_state.fram_num_first, updata_state.style, 2 ); ///通知取消升级
            goto UPDATA_END;
        }
        if( cur_package_num < updata_state.package_total )
        {
            /*
            ///当前包是第二包，检查第二包大小是否等于第一包大小，如果不相等，则认为第二包大小为分包大小
            if( ( cur_package_num == 2 ) && ( updata_state.package_size != msg_len ) )
            {
            	updata_state.package_size = msg_len;
            #ifdef UPDATA_DEBUG
            rt_kprintf("\n第二包修改包大小:  package_size  =%d",updata_state.package_size);
            #endif
            	updata_flash_write_para( &updata_state );
            }else ///比较后面的数据包是否和分包大小相同
            {
            */
            if( updata_state.package_size != msg_len )
            {
#ifdef UPDATA_DEBUG
                rt_kprintf( "\n 分包大小不匹配，包序号=%4d，包长度=%4d", fram_num, msg_len );
#endif
                updata_ack_ok( updata_state.fram_num_first, updata_state.style, 1 );
                goto UPDATA_END;
            }
            //}
        }
        ///其它包处理
        Tempu32data = ADDR_DF_UPDATA_START + ( ( cur_package_num - 2 ) * updata_state.package_size ) + updata_state.pack_len_first;
        if( updata_flash_wr_file( Tempu32data, msg, msg_len ) )
        {
            updata_flash_write_recv_page( &updata_state, cur_package_num ); ///修改包标记，表示该包数据成功接收

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
    debug_write( "升级成功!" );
    rt_kprintf( "\n 程序升级完成，10秒后复位设备。" );
    rt_thread_delay( RT_TICK_PER_SECOND * 5 );
    reset( 2 );
    return RT_EOK;
}

void send_info(void)
{
    uint16_t	i, j;

    u8			tempbuf[256];

    sprintf(tempbuf, "卡号=%s", jt808_param.id_0xF006);
    debug_write(tempbuf);
    sprintf(tempbuf, "%s/%d", jt808_param.id_0x0013, jt808_param.id_0x0018);
    debug_write(tempbuf);
    sprintf(tempbuf, "%s:%d", jt808_param.id_0x0017, jt808_param.id_0xF031);
    debug_write(tempbuf);
    sprintf(tempbuf, "车辆类型=%s", jt808_param.id_0xF00A);
    debug_write(tempbuf);
    if(jt808_param.id_0xF00D == 2)
        debug_write("公共货运");
    else
        debug_write("两客一危");
    sprintf(tempbuf, "车牌=%s", jt808_param.id_0x0083);
    debug_write(tempbuf);

    rt_kprintf("\n 硬       件:HV%d", HARD_VER);
    rt_kprintf("\n BIN文件版本:V%d.%02d", SOFT_VER / 100, SOFT_VER % 100);

    memset( tempbuf, 0, sizeof( tempbuf ) );
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    sst25_read( ADDR_DF_UPDATA_START, tempbuf, 256 );
    rt_sem_release( &sem_dataflash );
    rt_kprintf("\n TCB文件信息:");
    if( memcmp( (const char *)tempbuf, "TCB.GPS.01", 10 ) != 0 )
    {
        rt_kprintf("无信息!");
        return;
    }
    rt_kprintf("\n 设备类型:GPS终端设备");
    rt_kprintf("\n 设备型号:%s", tempbuf + 50);
    rt_kprintf("\n 硬件版本:HV%d", tempbuf[73]);		///忽略了高位
    rt_kprintf("\n 固件版本:%s", tempbuf + 74);
    rt_kprintf("\n 运 营 商:%s", tempbuf + 94);
    rt_kprintf("\n 开始地址:");
    printf_hex_data(tempbuf + 110, 4);
    rt_kprintf("\n 软件版本:");
    printf_hex_data(tempbuf + 104, 6);
    rt_kprintf("\n ");
}
FINSH_FUNCTION_EXPORT( send_info, send_info );


/*********************************************************************************
  *函数名称:u8 updata_ack(STYLE_UPDATA_STATE *para,u8 check)
  *功能描述:多包接收
  *输	入:	check	:进行应答检查，0表示不检查直接应答，1表示如果升级OK就进行应答
   para	:升级参数
  *输	出:	none
  *返 回 值:u8	0:没有升级。	1:升级中
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
    ///原始消息流水号
    len += data_to_buf( pbuf, para->fram_num_first, 2 );

    ///重传总包数
    //len += data_to_buf( pbuf+len, 1, 1 );
    len++;

    ///重传包 ID 列表
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
        rt_kprintf("\n 升级完成!");
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
