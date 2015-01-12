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
#include "rs485.h"
#include "jt808.h"
#include "jt808_param.h"
#include "camera.h"
#include "jt808_camera.h"
#include <finsh.h>
#include "sst25.h"

#define JT808_PACKAGE_MAX_MEDIA		512

typedef __packed struct
{
    u32 Address;        ///地址
    u32 Data_ID;        ///数据ID
    u8	Delete;         ///删除标记
    u8	Pack_Mark[16];  ///包标记
} TypePicMultTransPara;


/*********************************************************************************
  *函数名称:u16 Cam_add_tx_pic_getdata( JT808_TX_NODEDATA * nodedata )
  *功能描述:在jt808_tx_proc当状态为GET_DATA时获取照片数据，该函数是 JT808_TX_NODEDATA 的 get_data 回调函数
  *输 入:	nodedata	:正在处理的发送链表
  *输 出: none
  *返 回 值:rt_err_t
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static u16 Cam_add_tx_pic_getdata( JT808_TX_NODEDATA *nodedata )
{
    JT808_TX_NODEDATA		 *iterdata	= nodedata;
    TypePicMultTransPara	 *p_para	= (TypePicMultTransPara *)nodedata->user_para;
    TypeDF_PackageHead		TempPackageHead;
    uint16_t				i, wrlen;   //, pack_num;
    uint16_t				body_len;   /*消息体长度*/
    //	uint8_t					* msg;
    uint32_t				tempu32data;
    u16						ret = 0;
    uint8_t					 *pdata;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    tempu32data = Cam_Flash_FindPicID( p_para->Data_ID, &TempPackageHead , 0x00);
    if( tempu32data == 0xFFFFFFFF )
    {
        rt_kprintf( "\n 没有找到图片，ID=%d", p_para->Data_ID );
        ret = 0xFFFF;
        goto FUNC_RET;
    }
    pdata = nodedata->tag_data;

    for( i = 0; i < iterdata->packet_num; i++ )
    {
        if( p_para->Pack_Mark[i / 8] & BIT( i % 8 ) )
        {
            if( ( i + 1 ) < iterdata->packet_num )
            {
                body_len = JT808_PACKAGE_MAX_MEDIA;
            }
            else  /*最后一包*/
            {
                body_len = iterdata->size - JT808_PACKAGE_MAX_MEDIA * i;
            }
            pdata[0]	= 0x08;
            pdata[1]	= 0x01;
            pdata[2]	= 0x20 | ( body_len >> 8 ); /*消息体长度*/
            pdata[3]	= body_len & 0xff;

            iterdata->packet_no = i + 1;
            iterdata->head_sn	= 0xF001 + i;
            pdata[10]			= ( iterdata->head_sn >> 8 );
            pdata[11]			= ( iterdata->head_sn & 0xFF );

            wrlen				= 16;                                                                   /*跳过消息头*/
            iterdata->msg_len	= body_len + 16;                                                        ///发送数据的长度加上消息头的长度
            if( i == 0 )
            {
                sst25_read( tempu32data, (u8 *)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
                wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.id, 4 );            ///多媒体ID
                wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Media_Style, 1 );   ///多媒体类型
                wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Media_Format, 1 );  ///多媒体格式编码
                wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.TiggerStyle, 1 );   ///事件项编码
                wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Channel_ID, 1 );    ///通道 ID
                memcpy( iterdata->tag_data + wrlen, TempPackageHead.position, 28 );                     ///位置信息汇报
                wrlen += 28;
                sst25_read( tempu32data + 64, iterdata->tag_data + wrlen, ( 512 - 36 ) );               /*当前把消息头也计算进来了*/
            }
            else
            {
                tempu32data = tempu32data + JT808_PACKAGE_MAX_MEDIA * i + 64 - 36;
                sst25_read( tempu32data, iterdata->tag_data + wrlen, body_len );
            }
            p_para->Pack_Mark[i / 8] &= ~( BIT( i % 8 ) );
            rt_kprintf( "\n cam_get_data ok PAGE=%d\n", iterdata->packet_no );
            ret = iterdata->packet_no;
            /*调整数据 设置消息头*/
            pdata[12]			= ( iterdata->packet_num >> 8 );
            pdata[13]			= ( iterdata->packet_num & 0xFF );
            pdata[14]			= ( iterdata->packet_no >> 8 );
            pdata[15]			= ( iterdata->packet_no & 0xFF );
            nodedata->state		= IDLE;
            nodedata->retry		= 0;
            ///如果以后再没有数据的话下包超时为60秒
            nodedata->max_retry = 3;
#ifdef JT808_TEST_JTB
            nodedata->timeout	= 50 * RT_TICK_PER_SECOND;
            ///如果以后还有数据的话下包超时为1秒
            for( i = 0; i < iterdata->packet_num; i++ )
            {
                if( p_para->Pack_Mark[i / 8] & BIT( i % 8 ) )
                {
                    nodedata->max_retry = 1;
                    nodedata->timeout	=  RT_TICK_PER_SECOND / 2;
                    break;
                }
            }
#else
            nodedata->timeout	= RT_TICK_PER_SECOND * jt808_param.id_0x0002;
            ///如果以后还有数据的话下包超时为1秒
            for( i = 0; i < iterdata->packet_num; i++ )
            {
                if( p_para->Pack_Mark[i / 8] & BIT( i % 8 ) )
                {
                    nodedata->max_retry = 1;
                    //nodedata->timeout	=  RT_TICK_PER_SECOND / 2;
                    break;
                }
            }
#endif
            goto FUNC_RET;
        }
    }
    //rt_kprintf( "\n cam_get_data_false!" );
    //nodedata->timeout	= 60 * RT_TICK_PER_SECOND;
    ret = 0;
FUNC_RET:
    rt_sem_release( &sem_dataflash );
    return ret;
}

/*********************************************************************************
  *函数名称:JT808_MSG_STATE Cam_jt808_timeout( JT808_TX_NODEDATA * nodedata )
  *功能描述:发送图片相关数据的超时处理函数
  *输 入:	nodedata	:正在处理的发送链表
  *输 出: none
  *返 回 值:JT808_MSG_STATE
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static JT808_MSG_STATE Cam_jt808_timeout( JT808_TX_NODEDATA *nodedata )
{
    u16					cmd_id;
    TypePicMultTransPara *p_para;
    uint16_t			ret;

    cmd_id	= nodedata->head_id;
    p_para	= (TypePicMultTransPara *)( nodedata->user_para );
    switch( cmd_id )
    {
    case 0x0800:                                            /*超时以后，直接上报数据*/
        Cam_jt808_0x0801( nodedata->linkno, p_para->Data_ID, p_para->Delete );
        nodedata->state = ACK_OK;
        return ACK_OK;
        break;
    case 0x0801:                                            ///上报图片数据
        /*
        	if( nodedata->packet_no == nodedata->packet_num )   ///都上报完了，还超时
        	{
        		rt_kprintf( "\n全部上报完成\n" );               ///等待应答0x8800
        		return nodedata->state = ACK_OK;
        	}
        	ret = Cam_add_tx_pic_getdata( nodedata );
        	if( ret == 0xFFFF )                                 ///bitter 没有找到图片id
        	{
        		rt_kprintf( "\n全部上报完成\n" );               ///等待应答0x8800
        		return nodedata->state = ACK_OK;
        	}
        	*/
        ret = Cam_add_tx_pic_getdata( nodedata );
        if(( ret == 0xFFFF ) || (ret == 0 ))                               ///bitter 没有找到图片id
        {
            rt_kprintf( "\n全部上报完成\n" );               ///等待应答0x8800
            return nodedata->state = ACK_OK;
        }
        break;
    default:
        break;
    }
    return IDLE;
}

/*********************************************************************************
  *函数名称:JT808_MSG_STATE Cam_jt808_0x801_response( JT808_TX_NODEDATA * nodedata , uint8_t *pmsg )
  *功能描述:在jt808中处理照片数据传输ACK_ok函数，该函数是 JT808_TX_NODEDATA 的 cb_tx_response 回调函数
  *输 入:	nodedata	:正在处理的发送链表
  *输 出: none
  *返 回 值:JT808_MSG_STATE
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static JT808_MSG_STATE Cam_jt808_0x0801_response( JT808_TX_NODEDATA *nodedata, uint8_t *pmsg )
{
    JT808_TX_NODEDATA		 *iterdata = nodedata;
    TypePicMultTransPara	 *p_para;
    uint16_t				temp_msg_id;
    //	uint16_t				temp_msg_len;
    uint16_t				i;                              //, pack_num;
    uint32_t				tempu32data;
    uint16_t				ret;
    uint16_t				ack_seq;                        /*应答序号*/
    u16						fram_num;

    temp_msg_id = buf_to_data( pmsg, 2 );
    fram_num	= buf_to_data( pmsg + 10, 2 );
    p_para		= nodedata->user_para;

    if( 0x8001 == temp_msg_id )                             ///通用应答,有可能应答延时
    {
        ack_seq = ( pmsg[12] << 8 ) | pmsg[13];             /*判断应答流水号*/
        if( ack_seq != nodedata->head_sn )                  /*流水号对应*/
        {
            nodedata->timeout	= 30 * RT_TICK_PER_SECOND;  /*等30秒*/
            nodedata->state		= WAIT_ACK;
            return WAIT_ACK;
        }
        if( nodedata->packet_no == nodedata->packet_num )   /*所有数据包上报完成，等待中心下发0x8800*/
        {
            rt_kprintf( "\n等待8800/8003应答\n" );
            //nodedata->timeout	= 30 * RT_TICK_PER_SECOND;  /*等30秒*/
            nodedata->state		= WAIT_ACK;
            //nodedata->packet_no++;
            return WAIT_ACK;
        }

        if( nodedata->packet_no > nodedata->packet_num )    /*超时等待0x8800应答*/
        {
            nodedata->state = ACK_OK;
            return ACK_OK;
        }

        ret = Cam_add_tx_pic_getdata( nodedata );
        if( ret == 0xFFFF )                                 /*bitter 没有找到图片id*/
        {
            //rt_free( p_para );
            //p_para = RT_NULL;
            nodedata->state = ACK_OK;
            return ACK_OK;
        }
        nodedata->state = IDLE;                     /*正常发送*/
        return IDLE;
    }

    if( 0x8800 == temp_msg_id )                     ///专用应答
    {
        //debug_write("收到专用应答8800");
        tempu32data = buf_to_data( pmsg + 12, 4 );  /*应答media ID*/

        if( tempu32data != p_para->Data_ID )
        {
            rt_kprintf( "\n应答ID不正确 %08x %08x", tempu32data, p_para->Data_ID );
            nodedata->state = ACK_OK;
            return ACK_OK;
        }
        if( pmsg[16] == 0 ) /*重传包总数*/
        {
            if( p_para->Delete )
            {
                //Cam_Flash_DelPic(p_para->Data_ID);
                ;
            }
            Cam_Flash_TransOkSet(p_para->Data_ID);
            debug_write("单幅图片上报完成!");
            nodedata->state = ACK_OK;
            return ACK_OK;
        }

        p_para = (TypePicMultTransPara *)( iterdata->user_para );
        memset( p_para->Pack_Mark, 0, sizeof( p_para->Pack_Mark ) );

        for( i = 0; i < pmsg[16]; i++ )
        {
            tempu32data = buf_to_data( pmsg + i * 2 + 17, 2 );
            if( tempu32data )
            {
                tempu32data--;
            }
            p_para->Pack_Mark[tempu32data / 8] |= BIT( tempu32data % 8 );
        }
        rt_kprintf( "\n Cam_jt808_0x801_response\n lost_pack=%d", pmsg[16] );   /*重新获取发送数据*/
        ret = Cam_add_tx_pic_getdata( nodedata );
        if( ret == 0xFFFF )                                                     /*bitter 没有找到图片id*/
        {
            //rt_free( p_para );
            //p_para = RT_NULL;
            nodedata->state = ACK_OK;
            return ACK_OK;
        }
        nodedata->state = IDLE;
        return IDLE;
    }
    if( 0x8003 == temp_msg_id ) 					///专用应答
    {
        //debug_write("收到补传应答8003");
        tempu32data = buf_to_data( pmsg + 12, 2 );	/*应答media ID*/

        if( tempu32data != 0xF001 )
        {
            rt_kprintf( "\n应答序号不正确 %08x", tempu32data);
        }
        if( pmsg[14] == 0 ) /*重传包总数*/
        {
            if( p_para->Delete )
            {
                //Cam_Flash_DelPic(p_para->Data_ID);
                ;
            }
            Cam_Flash_TransOkSet(p_para->Data_ID);
            debug_write("单幅图片上报完成!");
            nodedata->state = ACK_OK;
            //jt808_tx_0x0001( fram_num, 0x8003, 0 );
            jt808_tx_0x0001_ex(nodedata->linkno, fram_num, 0x8003, 0 );
            return ACK_OK;
        }

        p_para = (TypePicMultTransPara *)( iterdata->user_para );
        memset( p_para->Pack_Mark, 0, sizeof( p_para->Pack_Mark ) );

        for( i = 0; i < pmsg[14]; i++ )
        {
            tempu32data = buf_to_data( pmsg + i * 2 + 15, 2 );
            if( tempu32data )
            {
                tempu32data--;
            }
            p_para->Pack_Mark[tempu32data / 8] |= BIT( tempu32data % 8 );
        }
        rt_kprintf( "\n Cam_jt808_0x801_response\n lost_pack=%d", pmsg[14] );	/*重新获取发送数据*/
        ret = Cam_add_tx_pic_getdata( nodedata );
        if( ret == 0xFFFF ) 													/*bitter 没有找到图片id*/
        {
            //rt_free( p_para );
            //p_para = RT_NULL;
            nodedata->state = ACK_OK;
            return ACK_OK;
        }
        nodedata->state = IDLE;
        return IDLE;
    }

    return nodedata->state;
}

/*********************************************************************************
  *函数名称:rt_err_t Cam_jt808_0x0801( uint8_t linkno, u32 mdeia_id, u8 media_delete )
  *功能描述:添加一个多媒体图片到发送列表中
  *输 入:	linkno		:链路编号
  			mdeia_id	:多媒体ID
  			media_delete:表示发送成功后是否删除多媒体体，非0表示删除，0表示不删除
  *输 出: none
  *返 回 值:rt_err_t
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
rt_err_t Cam_jt808_0x0801( uint8_t linkno, u32 mdeia_id, u8 media_delete )
{
    u32						TempAddress;
    TypePicMultTransPara	 *p_para;
    TypeDF_PackageHead		TempPackageHead;

    uint16_t				ret;

    JT808_TX_NODEDATA		 *pnodedata;

    ///查找多媒体ID是否存在

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    TempAddress = Cam_Flash_FindPicID( mdeia_id, &TempPackageHead , 0x00);
    rt_sem_release( &sem_dataflash );

    if( TempAddress == 0xFFFFFFFF )
    {
        rt_kprintf( "\n没有找到图片" );
        return RT_ERROR;
    }

    ///分配多媒体私有资源
    p_para = rt_malloc( sizeof( TypePicMultTransPara ) );
    if( p_para == NULL )
    {
        return RT_ENOMEM;
    }

    ///填充用户区数据
    memset( p_para, 0xFF, sizeof( TypePicMultTransPara ) );

    p_para->Address = TempAddress;
    p_para->Data_ID = mdeia_id;
    p_para->Delete	= media_delete;

    pnodedata = node_begin( linkno, MULTI_CMD, 0x0801, 0xF001, JT808_PACKAGE_MAX_MEDIA + 64 );
    if( pnodedata == RT_NULL )
    {
        rt_free( p_para );
        rt_kprintf( "\n分配资源失败" );
        return RT_ENOMEM;
    }
    pnodedata->size			= TempPackageHead.Len - 64 + 36; /*空出了64字节的头部，加上36字节图片信息*/
    pnodedata->packet_num	= ( pnodedata->size + JT808_PACKAGE_MAX_MEDIA - 1 ) / JT808_PACKAGE_MAX_MEDIA;
    pnodedata->packet_no	= 0;
    pnodedata->user_para	= p_para;

    ret = Cam_add_tx_pic_getdata( pnodedata );
    if(ret && ret != 0xFFFF)
    {
        //pnodedata->retry		= 0;
        //pnodedata->max_retry	= 1;
        //pnodedata->timeout		= 1 * RT_TICK_PER_SECOND;
        node_end( MULTI_CMD_NEXT, pnodedata, Cam_jt808_timeout, Cam_jt808_0x0801_response, p_para );
    }
    else
    {
        rt_free( p_para );
        p_para = RT_NULL;
        rt_free( pnodedata );
        rt_kprintf( "\r\n没有找到图片:%s", __func__ );
    }
    return RT_EOK;
}

/*********************************************************************************
  *函数名称:JT808_MSG_STATE Cam_jt808_0x801_response( JT808_TX_NODEDATA * nodedata , uint8_t *pmsg )
  *功能描述:在jt808中处理照片数据传输ACK_ok函数，该函数是 JT808_TX_NODEDATA 的 cb_tx_response 回调函数
  *输 入:	nodedata	:正在处理的发送链表
  *输 出: none
  *返 回 值:JT808_MSG_STATE
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static JT808_MSG_STATE Cam_jt808_0x0800_response( JT808_TX_NODEDATA *nodedata, uint8_t *pmsg )
{
    //	JT808_TX_NODEDATA		* iterdata = nodedata;
    TypePicMultTransPara	 *p_para;
    //	TypeDF_PackageHead		TempPackageHead;
    //	uint32_t				TempAddress;
    uint16_t	temp_msg_id;
    //	uint16_t				body_len; /*消息体长度*/
    uint8_t		 *msg;

    if( pmsg == RT_NULL )
    {
        return IDLE;
    }

    temp_msg_id = buf_to_data( pmsg, 2 );
    //	body_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
    msg = pmsg + 12;
    if( 0x8001 == temp_msg_id ) ///通用应答
    {
        if( ( nodedata->head_sn == buf_to_data( msg, 2 ) ) &&
                ( nodedata->head_id == buf_to_data( msg + 2, 2 ) ) &&
                ( msg[4] == 0 ) )
        {
            p_para = nodedata->user_para;
            Cam_jt808_0x0801( nodedata->linkno, p_para->Data_ID, p_para->Delete );
            nodedata->state = ACK_OK;
            return ACK_OK;


            /*
               p_para = nodedata->user_para;
               rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
               TempAddress = Cam_Flash_FindPicID( p_para->Data_ID, &TempPackageHead );
               rt_sem_release( &sem_dataflash );
               if( TempAddress == 0xFFFFFFFF )
               {
               return ACK_OK;
               }
               nodedata->size			= TempPackageHead.Len - 64 + 36;
               nodedata->multipacket	= 1;
               nodedata->type			= SINGLE_CMD;
               nodedata->state			= IDLE;
               nodedata->retry			= 0;
               nodedata->packet_num	= ( nodedata->size / JT808_PACKAGE_MAX_MEDIA );
               if( nodedata->size % JT808_PACKAGE_MAX_MEDIA )
               {
               nodedata->packet_num++;
               }
               nodedata->packet_no			= 0;
               nodedata->msg_len			= 0;
               nodedata->head_id			= 0x801;
               nodedata->head_sn			= 0xF001;
               nodedata->timeout			= 0;
               nodedata->cb_tx_timeout		= Cam_jt808_timeout;
               nodedata->cb_tx_response	= Cam_jt808_0x801_response;

               return IDLE;
             */
        }
    }
    return IDLE;
}

/*********************************************************************************
  *函数名称:rt_err_t Cam_jt808_0x0800(uint8_t linkno, u32 mdeia_id, u8 media_delete )
  *功能描述:多媒体事件信息上传_发送照片多媒体信息
  *输	入:	linkno		:链路编号
  			mdeia_id	:多媒体ID
  			media_delete:表示发送成功后是否删除多媒体体，非0表示删除，0表示不删除
  			send_meda	:非0表示收到0x0800通用应答后要发送0801数据，0表示不做处理
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2014-11-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
rt_err_t Cam_jt808_0x0800_ex(uint8_t linkno, u32 mdeia_id, u8 media_delete , u8 send_meda )
{
    u8						ptempbuf[32];
    u16						datalen = 0;
    //	u16						i;
    u32						TempAddress;
    TypePicMultTransPara	 *p_para;
    TypeDF_PackageHead		TempPackageHead;
    //	rt_err_t				rt_ret;
    JT808_TX_NODEDATA		 *pnodedata;

    ///查找多媒体ID是否存在
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    TempAddress = Cam_Flash_FindPicID( mdeia_id, &TempPackageHead, 0x00 );
    rt_sem_release( &sem_dataflash );
    if( TempAddress == 0xFFFFFFFF )
    {
        return RT_ERROR;
    }

    p_para = rt_malloc( sizeof( TypePicMultTransPara ) );
    if( p_para == NULL )
    {
        return RT_ENOMEM;
    }
    ///填充用户区数据
    memset( p_para, 0xFF, sizeof( TypePicMultTransPara ) );
    p_para->Address = TempAddress;
    p_para->Data_ID = mdeia_id;
    p_para->Delete	= media_delete;

    datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.id, 4 );
    datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Media_Style, 1 );
    datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Media_Format, 1 );
    datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.TiggerStyle, 1 );
    datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Channel_ID, 1 );

    //pnodedata = node_begin( linkno, SINGLE_CMD, 0x0800, -1, 512 + 32 ); /*0x0800是单包*/
    pnodedata = node_begin( linkno, SINGLE_CMD, 0x0800, -1, datalen ); /*0x0800是单包*/
    if( pnodedata == RT_NULL )
    {
        rt_free( p_para );
        return RT_ENOMEM;
    }

    //pnodedata->size			= TempPackageHead.Len - 64 + 36;
    //pnodedata->packet_num	= ( pnodedata->size + JT808_PACKAGE_MAX_MEDIA - 1 ) / JT808_PACKAGE_MAX_MEDIA;
    //pnodedata->packet_no	= 0;

    node_data( pnodedata, ptempbuf, datalen );
    if(send_meda)
    {
        node_end( SINGLE_CMD, pnodedata, Cam_jt808_timeout, Cam_jt808_0x0800_response, p_para );
    }
    else
    {
        node_end( SINGLE_CMD, pnodedata, RT_NULL, RT_NULL, p_para );
    }

    return RT_EOK;
}

/*********************************************************************************
  *函数名称:rt_err_t Cam_jt808_0x0800(uint8_t linkno, u32 mdeia_id, u8 media_delete )
  *功能描述:多媒体事件信息上传_发送照片多媒体信息
  *输	入:	linkno		:链路编号
  			mdeia_id	:多媒体ID
  			media_delete:表示发送成功后是否删除多媒体体，非0表示删除，0表示不删除
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
rt_err_t Cam_jt808_0x0800(uint8_t linkno, u32 mdeia_id, u8 media_delete )
{
    return Cam_jt808_0x0800_ex( linkno, mdeia_id, media_delete, 1);
}

/*********************************************************************************
  *函数名称:void Cam_jt808_0x8801_cam_ok( struct _Style_Cam_Requset_Para *para,uint32_t pic_id )
  *功能描述:平台下发拍照命令处理函数的回调函数_单张照片拍照OK
  *输	入:	para	:拍照处理结构体
   pic_id	:图片ID
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void cam_ok( struct _Style_Cam_Requset_Para *para, uint32_t pic_id )
{
    u8	*pdestbuf;
    u16 datalen = 0;

    pdestbuf = (u8 *)para->user_para;

    if( ( para->PhotoNum <= para->PhotoTotal ) && ( para->PhotoNum ) && ( para->PhotoNum <= 32 ) )
    {
        datalen = ( para->PhotoNum - 1 ) * 4 + 5;
    }
    else
    {
        return;
    }
    if( pdestbuf != RT_NULL )		/*并不是都要上报0x0805*/
    {
        data_to_buf( pdestbuf + datalen, pic_id, 4 ); ///写入应答流水号
    }
    /*
    if( para->SendPhoto )
    {
    	rt_kprintf( "\n>(%s) pic_id=%d", __func__, pic_id );
    	//Cam_jt808_0x0801( RT_NULL, pic_id, !para->SavePhoto );
    	Cam_jt808_0x0800( pic_id, !para->SavePhoto );
    }
    */
}

/*********************************************************************************
  *函数名称:void Cam_jt808_0x8801_cam_end( struct _Style_Cam_Requset_Para *para )
  *功能描述:平台下发拍照命令处理函数的回调函数
  *输	入:	para	:拍照处理结构体
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void cam_end( struct _Style_Cam_Requset_Para *para )
{
    u8	*pdestbuf;
    u16 datalen;

    pdestbuf = (u8 *)para->user_para;

    if( ( para->PhotoNum <= para->PhotoTotal ) && ( para->PhotoNum ) && ( para->PhotoNum <= 32 ) )
    {
        pdestbuf[2] = 0;
        datalen		= para->PhotoNum * 4 + 5;
    }
    else  /*拍照失败*/
    {
        pdestbuf[2] = 1;
        datalen		= 3;
    }
    //data_to_buf( pdestbuf + 3, para->PhotoNum, 2 ); ///写入应答流水号
    pdestbuf[3] = para->PhotoNum >> 8;
    pdestbuf[4] = para->PhotoNum & 0xFF;
    //jt808_tx_ack( 0x0805, pdestbuf, datalen );  /*不把它放到发送头*/
    jt808_tx_ack_ex(mast_socket->linkno, 0x0805, pdestbuf, datalen );
    if( para->user_para != RT_NULL )
    {
        rt_free( para->user_para );
    }
    para->user_para = RT_NULL;
    rt_kprintf( "\nCam_jt808_0x8801_cam_end" );
    return;
}

/*********************************************************************************
   *函数名称:rt_err_t Cam_jt808_0x8801(uint8_t linkno,uint8_t *pmsg)
   *功能描述:平台下发拍照命令处理函数
   *输	入:	pmsg	:808消息体数据
   msg_len	:808消息体长度
   *输	出:	none
   *返 回 值:rt_err_t
   *作	者:白养民
   *创建日期:2013-06-17
*--------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
rt_err_t Cam_jt808_0x8801( uint8_t linkno, uint8_t *pmsg )
{
    u8						*pdestbuf;
    u16						datalen;
    Style_Cam_Requset_Para	cam_para;
    u32						Tempu32data;
    u16						msg_len;
    msg_len = buf_to_data( pmsg + 2, 2 ) & 0x3FF;

    if( msg_len < 12 )
    {
        return RT_ERROR;
    }
    memset( &cam_para, 0, sizeof( cam_para ) );
    cam_para.TiggerStyle = Cam_TRIGGER_PLANTFORM; ///设置触发类型为平台触发

    ///通道 ID 1BYTE
    datalen				= 0;
    cam_para.Channel_ID = pmsg[12];
    ///拍摄命令 2BYTE
    Tempu32data = ( pmsg[13] << 8 ) | pmsg[14];
    if( ( Tempu32data ) && ( Tempu32data != 0xFFFF ) )
    {
        cam_para.PhotoTotal = Tempu32data;
        if( cam_para.PhotoTotal > 32 )
        {
            cam_para.PhotoTotal = 32;
        }
    }
    else
    {
        return RT_ERROR;
    }
    ///拍照间隔/录像时间 2BYTE second
    Tempu32data			= ( pmsg[15] << 8 ) | pmsg[16];
    datalen				+= 2;
    cam_para.PhotoSpace = Tempu32data * RT_TICK_PER_SECOND;
    ///保存标志
    if( pmsg[17] )
    {
        cam_para.SavePhoto	= 1;
        cam_para.SendPhoto	= 0;
    }
    else
    {
        cam_para.SavePhoto	= 0;
        cam_para.SendPhoto	= 1;
    }
    ///和用户回调函数相关的数据参数
    datalen		= cam_para.PhotoTotal * 4 + 5;  /*5字节头+4*n*/
    pdestbuf	= rt_malloc( datalen );
    if( pdestbuf == RT_NULL )
    {
        return RT_ERROR;
    }
    memset( pdestbuf, 0, datalen );             ///清空数据

    pdestbuf[0]						= pmsg[10];
    pdestbuf[1]						= pmsg[11];
    cam_para.user_para				= (void *)pdestbuf;
    cam_para.cb_response_cam_ok		= cam_ok;   ///一张照片拍照成功回调函数
    cam_para.cb_response_cam_end	= cam_end;  ///所有照片拍照结束回调函数
    take_pic_request( &cam_para );              ///发送拍照请求
    return RT_EOK;
}

/*多媒体信息检索*/
rt_err_t Cam_jt808_0x8802( uint8_t linkno, uint8_t *pmsg )
{
    u8					*pdestbuf;
    u16					datalen = 0;
    u16					i, mediatotal;
    TypeDF_PackageHead	*pHead;
    u16					seq;
    uint8_t				buf[16];
    MYTIME				cam_time_end;

    datalen = buf_to_data( pmsg + 2, 2 ) & 0x3FF;
    seq		= buf_to_data( pmsg + 10, 2 );
    pmsg	+= 12;

    if( pmsg[0] )
    {
        return RT_ERROR;
    }
    ///查找符合条件的图片，并将图片地址存入ptempbuf中
    mediatotal = 13;
    /*平台下发消息
    7E8802000F013602069002	878D
    000000
    000000000000
    000000000000
    2E7E
    */
    /*无数据应答
    7E080200040136020690020014
    878D
    0000B17E
    */
    /*有4个图片应答
    7E080200900136020690020002
    92AF
    0004
    0000000100010000000000000020010000000000000000000000000000140224053654
    000000030001000000000000002003026259F906EA0500000000000120130305180529
    000000040001000000000000002003026259F906EA0500000000000120130305180623
    0000000500010000000000000020030261A56006EA04FC0000043700B3130305181551
    267E
    */
    cam_time_end =  mytime_from_bcd( pmsg + 9 );
    if(cam_time_end == 0)
    {
        cam_time_end = 0xFFFFFFFF;
    }
    pdestbuf = Cam_Flash_SearchPicHead( mytime_from_bcd( pmsg + 3 ),
                                        cam_time_end,
                                        pmsg[1],
                                        pmsg[2],
                                        &mediatotal,
                                        (BIT(0) + BIT(1)));

    if( mediatotal == 0 )
    {
        data_to_buf( buf, seq, 2 );
        data_to_buf( buf + 2, mediatotal, 2 );
        jt808_tx_ack_ex(linkno, 0x0802, buf, 4 );
        return RT_EOK;
    }

    /*整理上报数据格式*/
    datalen = 4; /*跳过开始的应答流水号word 和 多媒体数据总项数word*/

    for( i = 0; i < mediatotal; i++ )
    {
        pHead	= (TypeDF_PackageHead *)( pdestbuf + i * sizeof( TypeDF_PackageHead ) );
        datalen += data_to_buf( pdestbuf + datalen, pHead->id, 4 );
        datalen += data_to_buf( pdestbuf + datalen, pHead->Media_Style, 1 );
        datalen += data_to_buf( pdestbuf + datalen, pHead->Channel_ID, 1 );
        datalen += data_to_buf( pdestbuf + datalen, pHead->TiggerStyle, 1 );
        memcpy( pdestbuf + datalen, pHead->position, 28 ); ///位置信息汇报
        datalen += 28;
    }

    data_to_buf( pdestbuf, seq, 2 );
    data_to_buf( pdestbuf + 2, mediatotal, 2 );
    jt808_tx_ack_ex(linkno, 0x0802, pdestbuf, datalen );

    rt_free( pdestbuf );
    return RT_EOK;
}

/*查找并上报多媒体数据*/
rt_err_t Cam_jt808_0x8803( uint8_t linkno, uint8_t *pmsg )
{
    u8					*ptempbuf;
    u16					i, mediatotal;
    TypeDF_PackageHead	 *pHead;
    u32					TempAddress;

    pmsg += 12;

    if( pmsg[0] )
    {
        return RT_ERROR;
    }
    if( Cam_get_param( )->Number == 0 )
    {
        return RT_ERROR;
    }
    mediatotal = 10;
    ptempbuf = Cam_Flash_SearchPicHead( mytime_from_bcd( pmsg + 3 ),
                                        mytime_from_bcd( pmsg + 9 ),
                                        pmsg[1],
                                        pmsg[2],
                                        &mediatotal,
                                        BIT(0));

    if( mediatotal == 0 ) 		///没有图片
    {
        return RT_EOK;
    }
    for( i = 0; i < mediatotal; i++ )
    {
        pHead = (TypeDF_PackageHead *)( ptempbuf + i * sizeof( TypeDF_PackageHead ) );
        //Cam_jt808_0x0801(linkno, pHead->id, pmsg[15] );
        Cam_jt808_0x0800(linkno, pHead->id, pmsg[15] );
    }
    rt_free( ptempbuf );
    return RT_EOK;
}

/*单条多媒体检索上传*/
rt_err_t Cam_jt808_0x8805( uint8_t linkno, uint8_t *pmsg )
{
    uint32_t	id		= buf_to_data(pmsg + 12, 4);
    uint8_t		del		= pmsg[16];

    //Cam_jt808_0x0801(linkno, id, del );
    Cam_jt808_0x0800(linkno, id, del );
    return RT_EOK;
}



/*********************************************************************************
  *函数名称:uint8_t Cam_not_send_get( void )
  *功能描述:检查是否有需要发送的图片数据,如果有图片，则发送。
  *输	入:	none
  *输	出: none
  *返 回 值:uint8_t		:	为非0表示产生了发送数据列表
  *作	者:白养民
  *创建日期:2013-11-18
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t Cam_jt808_get( void )
{
    u8					*ptempbuf;
    u16					mediatotal;
    TypeDF_PackageHead	 *pHead;

    //if(( mast_socket->state != CONNECTED )||(jt808_state != JT808_REPORT))  /*不在线*/
    //{
    //	return 0;
    //}


    if(( Cam_get_param( )->Number == 0 ) || ( Cam_get_param( )->NotSendCounter == 0 ))
    {
        return 0;
    }
    mediatotal = 1;
    ptempbuf = Cam_Flash_SearchPicHead( 0x00000000, 0xFFFFFFFF, 0, 0xFF, &mediatotal , BIT(0) | BIT(1) | BIT(2));
    if( mediatotal )
    {
        pHead = (TypeDF_PackageHead *)( ptempbuf );
        Cam_jt808_0x0800(mast_socket->linkno, pHead->id, 0 );
        rt_free(ptempbuf);
        return 1;
    }
    Cam_get_param( )->NotSendCounter = 0;
    return 0;
}

/************************************** The End Of File **************************************/
