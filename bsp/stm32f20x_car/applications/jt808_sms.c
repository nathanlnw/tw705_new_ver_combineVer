/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		//  jt808_sms.c
 * Author:			//  baiyangmin
 * Date:			//  2013-07-08
 * Description:		//  短信处理及发送，接收，修改参数等功能
 * Version:			//  V0.01
 * Function List:	//  主要函数及其功能
 *     1. -------
 * History:			//  历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/

#include <rtthread.h>
#include <rthw.h>
#include "include.h"
#include <finsh.h>

#include  <stdlib.h> //数字转换成字符串
#include  <stdio.h>
#include  <string.h>
#include  "jt808_sms.h"
#include "jt808.h"
#include "jt808_param.h"
#include "jt808_gps.h"
#include "m66.h"
#include "gps.h"

#define SMS_CMD_RESET		0x01			///复位设备
#define SMS_CMD_RELINK		0x02		///重新连接模块
#define SMS_CMD_FACTSET		0x04		///恢复出厂设置
#define SMS_CMD_SAVEPARAM	0x08		///保存参数到flash中
#define SMS_CMD_FACTSETDATA	0x10		///恢复出厂数据


#define HALF_BYTE_SWAP( a ) ( ( ( ( a ) & 0x0f ) << 4 ) | ( ( a ) >> 4 ) )
#define GSM_7BIT	0x00
#define GSM_UCS2	0x08

static char sms_buf[400];
static const char	sms_gb_data[]	= "冀京津沪宁渝琼藏川粤青贵闽吉陕蒙晋甘桂鄂赣浙苏新鲁皖湘黑辽云豫";
static uint16_t		sms_ucs2[31]	= { 0x5180, 0x4EAC, 0x6D25, 0x6CAA, 0x5B81, 0x6E1D, 0x743C, 0x85CF, 0x5DDD, 0x7CA4, 0x9752,
                                        0x8D35,		   0x95FD, 0x5409, 0x9655, 0x8499, 0x664B, 0x7518, 0x6842, 0x9102, 0x8D63,
                                        0x6D59,		   0x82CF, 0x65B0, 0x9C81, 0x7696, 0x6E58, 0x9ED1, 0x8FBD, 0x4E91, 0x8C6B,
                                };
static char			sender_tel[24];		///接收到的短信源号码，该号码为普通格式号码
static char			sender[32];			///接收到的短信源号码，该号码为PDU格式号码
static char			smsc[32];			///接收到的短信中心号码，该号码为PDU格式号码
static char 		sms_command = 0;		///表示接收到命令后的相关操作，因为需要发送短信成功后才执行操作，所以需要等待一段时间

/**/
u16  gsmencode8bit( const char *pSrc, char *pDst, u16 nSrcLength )
{
    u16 m;
    // 简单复制
    for( m = 0; m < nSrcLength; m++ )
    {
        *pDst++ = *pSrc++;
    }

    return nSrcLength;
}

/**/
static uint16_t ucs2_to_gb2312( uint16_t uni )
{
    uint8_t i;
    for( i = 0; i < 31; i++ )
    {
        if( uni == sms_ucs2[i] )
        {
            return ( sms_gb_data[i * 2] << 8 ) | sms_gb_data[i * 2 + 1];
        }
    }
    return 0xFF20;
}

/**/
static uint16_t gb2312_to_ucs2( uint16_t gb )
{
    uint8_t i;
    for( i = 0; i < 31; i++ )
    {
        if( gb == ( ( sms_gb_data[i * 2] << 8 ) | sms_gb_data[i * 2 + 1] ) )
        {
            return sms_ucs2[i];
        }
    }
    return 0;
}

/*ucs2过来的编码*/
u16 gsmdecodeucs2( char *pSrc, char *pDst, u16 nSrcLength )
{
    u16			nDstLength = 0; // UNICODE宽字符数目
    u16			i;
    uint16_t	uni, gb;
    char		 *src	= pSrc;
    char		 *dst	= pDst;

    for( i = 0; i < nSrcLength; i += 2 )
    {
        uni = ( src[i] << 8 ) | src[i + 1];
        if( uni > 0x80 ) /**/
        {
            gb			= ucs2_to_gb2312( uni );
            *dst++		= ( gb >> 8 );
            *dst++		= ( gb & 0xFF );
            nDstLength	+= 2;
        }
        else  /*也是双字节的 比如 '1' ==> 0031*/
        {
            *dst++ = uni;
            nDstLength++;
        }
    }
    return nDstLength;
}

/**/
u16 gsmencodeucs2( u8 *pSrc, u8 *pDst, u16 nSrcLength )
{
    uint16_t	len = nSrcLength;
    uint16_t	gb, ucs2;
    uint8_t		*src	= pSrc;
    uint8_t		*dst	= pDst;

    while( len )
    {
        gb = *src++;
        if( gb > 0x7F )
        {
            gb		|= *src++;
            ucs2	= gb2312_to_ucs2( gb ); /*有可能没有找到返回00*/
            *dst++	= ( ucs2 >> 8 );
            *dst++	= ( ucs2 & 0xFF );
            len		-= 2;
        }
        else
        {
            *dst++	= 0x00;
            *dst++	= ( gb & 0xFF );
            len--;
        }
    }
    // 返回目标编码串长度
    return ( nSrcLength << 1 );
}

/**/
u16 gsmdecode7bit( char *pSrc, char *pDst, u16 nSrcLength )
{
    u16 nSrc;   // 源字符串的计数值
    u16 nDst;   // 目标解码串的计数值
    u16 nByte;  // 当前正在处理的组内字节的序号，范围是0-6
    u8	nLeft;  // 上一字节残余的数据

    // 计数值初始化
    nSrc	= 0;
    nDst	= 0;

    // 组内字节序号和残余数据初始化
    nByte	= 0;
    nLeft	= 0;

    // 将源数据每7个字节分为一组，解压缩成8个字节
    // 循环该处理过程，直至源数据被处理完
    // 如果分组不到7字节，也能正确处理
    while( nSrc < nSrcLength )
    {
        *pDst	= ( ( *pSrc << nByte ) | nLeft ) & 0x7f;    // 将源字节右边部分与残余数据相加，去掉最高位，得到一个目标解码字节
        nLeft	= *pSrc >> ( 7 - nByte );                   // 将该字节剩下的左边部分，作为残余数据保存起来
        pDst++;                                             // 修改目标串的指针和计数值
        nDst++;
        nByte++;                                            // 修改字节计数值
        if( nByte == 7 )                                    // 到了一组的最后一个字节
        {
            *pDst = nLeft;                                  // 额外得到一个目标解码字节
            pDst++;                                         // 修改目标串的指针和计数值
            nDst++;
            nByte	= 0;                                    // 组内字节序号和残余数据初始化
            nLeft	= 0;
        }
        pSrc++;                                             // 修改源串的指针和计数值
        nSrc++;
    }
    *pDst = 0;
    return nDst;                                            // 返回目标串长度
}


/*信息编码为7bit pdu模式,并发送*/


/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void sms_encode_pdu_7bit( char *info , char *sendnum )
{
#define BITMASK_7BITS 0x7F

    //char			buf[200];
    uint16_t		len;
    char			*pSrc, *pDst;
    unsigned char	c;
    char			ASC[]	= "0123456789ABCDEF";
    uint8_t			count	= 0;

    unsigned char	nSrc	= 0, nDst = 0, nChar = 0;
    uint8_t			nLeft	= 0;
    char			DA_buf[32];

    memset(sms_buf, 0, sizeof(sms_buf));
    memset(DA_buf, 0, sizeof(DA_buf));
    if(((uint32_t)sendnum == 0) || (strlen(sendnum) == 0))
    {
        strcpy(DA_buf, sender);
    }
    else
    {
        strcpy(DA_buf, sendnum);
    }
    memset(sms_buf, 0, sizeof(sms_buf));
    len = strlen( info );
    rt_kprintf( "\nSMS>ENCODE(%d):%s", len, info );

    //strcpy( buf, "0891683108200205F01100" );
    strcpy( sms_buf, "001100" );
    count = strlen( DA_buf );
    strcat( sms_buf, DA_buf );
    strcat( sms_buf, "000000" );                                //	7bit编码

    pSrc	= info;
    pDst	= sms_buf + ( count + 12 + 2 );                     //sender长度+12ASC+2预留TP长度

    while( nSrc < ( len + 1 ) )
    {
        nChar = nSrc & 0x07;                                // 取源字符串的计数值的最低3位
        if( nChar == 0 )                                    // 处理源串的每个字节
        {
            nLeft = *pSrc;                                  // 组内第一个字节，只是保存起来，待处理下一个字节时使用
        }
        else
        {
            c		= ( *pSrc << ( 8 - nChar ) ) | nLeft;   // 组内其它字节，将其右边部分与残余数据相加，得到一个目标编码字节
            *pDst++ = ASC[c >> 4];
            *pDst++ = ASC[c & 0x0f];
            nLeft	= *pSrc >> nChar;                       // 将该字节剩下的左边部分，作为残余数据保存起来
            nDst++;
        }
        pSrc++;
        nSrc++;                                             // 修改源串的指针和计数值
    }
    *pDst = 0;
    //buf[count + 28] = ASC[len >> 4];
    //buf[count + 29] = ASC[len & 0x0F];
    sms_buf[count + 12] = ASC[len >> 4];
    sms_buf[count + 13] = ASC[len & 0x0F];

    //rt_kprintf( "\nSMS>encode 7BIT:%s", buf );
    //sms_tx( buf, ( strlen( buf ) - 18 ) >> 1 );
    sms_tx( sms_buf, ( strlen( sms_buf ) - 2 ) >> 1 );				//因为CMGS发送的长度不包括SCA的长度，在该发送短信中SCA为"00"，所以减2
}

/*信息编码为uscs2 pdu模式,并发送*/
void sms_encode_pdu_ucs2( char *info , char *sendnum)
{
    //char		buf[200];
    uint16_t	len, gb, ucs2;
    char		*pSrc, *pDst;
    char		ASC[]	= "0123456789ABCDEF";
    uint8_t		count	= 0;
    uint8_t		TP_UL	= 0;
    char		DA_buf[32];

    memset(sms_buf, 0, sizeof(sms_buf));
    memset(DA_buf, 0, sizeof(DA_buf));
    if(((uint32_t)sendnum == 0) || (strlen(sendnum) == 0))
    {
        strcpy(DA_buf, sender);
    }
    else
    {
        strcpy(DA_buf, sendnum);
    }

    len = strlen( info );
    rt_kprintf( "\nSMS>ENCODE(%d):%s", len, info );

    //	strcpy(buf,smsc,strlen(smsc));
    strcpy( sms_buf, "001100" );	//不设置短信中心方式发送，默认均采用该方法发送
    count = strlen( DA_buf );
    strcat( sms_buf, DA_buf );
    strcat( sms_buf, "00088F" );	//从左往右含义，第一个00是PID，默认均为00；08为DCS表示UCS2编码；8F为VP表示消息有效期 (8F+1)*5 分钟。
    //	ucs2bit编码
    pSrc	= info;
    pDst	= sms_buf + ( count + 12 + 2 ); //sender长度+12ASC+2预留TP长度
    while( *pSrc )
    {
        gb = *pSrc++;
        if( gb > 0x7F )
        {
            gb		<<= 8;
            gb		|= *pSrc++;
            ucs2	= gb2312_to_ucs2( gb ); /*有可能没有找到返回00*/
            *pDst++ = ASC[( ucs2 & 0xF000 ) >> 12];
            *pDst++ = ASC[( ucs2 & 0x0F00 ) >> 8];
            *pDst++ = ASC[( ucs2 & 0x00F0 ) >> 4];
            *pDst++ = ASC[( ucs2 & 0x000F )];
        }
        else
        {
            *pDst++ = '0';
            *pDst++ = '0';
            *pDst++ = ASC[gb >> 4];
            *pDst++ = ASC[gb & 0x0F];
        }
        TP_UL += 2;
    }
    *pDst			= 0;
    sms_buf[count + 12] = ASC[TP_UL >> 4];
    sms_buf[count + 13] = ASC[TP_UL & 0x0F];

    //rt_kprintf( "\nSMS>encode UCS2:%s", buf );
    sms_tx( sms_buf, ( strlen( sms_buf ) - 2 ) >> 1 );						//因为CMGS发送的长度不包括SCA的长度，在该发送短信中SCA为"00"，所以减2
}


/*********************************************************************************
  *函数名称:void set_phone_num(char * str)
  *功能描述:设置要发送的目的移动设备号码
  *输	入:	str	:目的手机号码
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-11-25
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void set_phone_num(char *str)
{
    uint8_t		i, j;
    uint8_t 	len = strlen(str);
    char *pbuf;

    memset(sender, 0, sizeof(sender));
    //memcpy(sender,"0D9068",6);
    sprintf(sender, "%02X9168", len + 2);
    //memcpy(sender,"0D9168",6);
    memcpy(sender + 6, str, len);
    if(len % 2)
        sender[6 + len] = 'F';
    pbuf = sender + 6;
    for(i = 0; i < len; i += 2)
    {
        j = pbuf[i];
        pbuf[i] = pbuf[i + 1];
        pbuf[i + 1] = j;
    }
    rt_kprintf("\n phont_num=%s", sender);
}
FINSH_FUNCTION_EXPORT( set_phone_num, sms set_phone_num );


/*********************************************************************************
  *函数名称:void sms_send(char * phone_num,char *str)
  *功能描述:发送短信，自动识别采用7BIT编码还是UCS2编码
  *输	入:	phone_num	:目的手机号码
  			str			:短信内容
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-11-25
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void sms_send(char *str, char *phone_num)
{
    uint8_t i;
    char	buf[162];
    int	len = 0;

    if(((uint32_t)phone_num ) && (strlen(phone_num)))
    {
        set_phone_num(phone_num);
    }

    for(i = 0; i < strlen(str); i++)
    {
        if(str[i] > 0x7F)
        {
            sms_encode_pdu_ucs2(str, 0);
            return;
        }
    }
    while(strlen(str))
    {
        memset(buf, 0, sizeof(buf));

        if(strlen(str) > 160)
        {
            len	= 160;
        }
        else
        {
            len	= strlen(str);
        }

        memcpy(buf, str, len);
        sms_encode_pdu_7bit(str, 0);
        str += len;
    }
}
FINSH_FUNCTION_EXPORT( sms_send, sms_send );


/*输入命令,按要求返回
   对于查询命令直接发送 返回0，不需要应答或内部已应答
   其他命令返回1,交由调用方处理,需要通用应答


 */
uint8_t analy_param( char *cmd, char *value , uint8_t send_sms)
{
    const char *gps_mode_string[] =
    {
        "G0",		///表示模块错误
        "GP",		///BD2
        "BD",		///GPS
        "GN",		///BDGPS
    };
    const char gps_fixed[2] = {'V', 'A'};
    uint16_t i, j;
    char buf[160];
    char ip[32];
    char *p;
    int port;
    uint32_t id;

    if( strlen( cmd ) == 0 )
    {
        return 0;
    }
    rt_kprintf( "\nSMS>cmd=%s,value=%s", cmd, value );
    /*先找不带参数的*/
    if( strncmp( cmd, "TIREDCLEAR", 10 ) == 0 )
    {
        jt808_param_bk.utc_over_time_start = 0;
        return 0;
    }
    if( strncmp( cmd, "DISCLEAR", 8 ) == 0 )
    {
        jt808_param_bk.car_mileage = 0;
        jt808_param_bk.mileage_pulse = 0;
        jt808_data.id_0xFA01 = 0;
        jt808_data.id_0xFA03 = 0;
        data_save();
        return 0;
    }
    if( strncmp( cmd, "RESET", 5 ) == 0 )               /*要求复位,发送完成后复位，如何做?*/
    {
        sms_command |= SMS_CMD_RESET;
        return 0;
    }
    else if( strncmp( cmd, "CLEARREGIST", 11 ) == 0 )   /*清除注册*/
    {
        jt808_param_bk.updata_utctime = 0;
        memset( jt808_param.id_0xF003, 0, 32 );         /*清除鉴权码*/
        sms_command |= SMS_CMD_SAVEPARAM | SMS_CMD_RELINK;
        return 0;
    }
    else if( strncmp( cmd, "ACK", 3 ) == 0 )               /*要求短信应答*/
    {
        return 1;
    }


    /*带参数的，先判有没有参数*/
    if( strlen( value ) == 0 )
    {
        return 0;
    }
    /*查找对应的参数*/
    if( strncmp( cmd, "DNSR", 4 ) == 0 )
    {
        if( cmd[4] == '1' )
        {
            strcpy( jt808_param.id_0x0013, value );
        }
        if( cmd[4] == '2' )
        {
            strcpy( jt808_param.id_0x0017, value );
        }
        sms_command |= SMS_CMD_SAVEPARAM | SMS_CMD_RELINK;
        return 0;
    }
    else if( strncmp( cmd, "PORT", 4 ) == 0 )
    {
        if( cmd[4] == '1' )
        {
            jt808_param.id_0x0018 = atoi( value );
        }
        else if( cmd[4] == '2' )
        {
            jt808_param.id_0xF031 = atoi( value );
        }
        else
        {
            return 0;
        }
        sms_command |= SMS_CMD_SAVEPARAM | SMS_CMD_RELINK;
        return 0;
    }
    else if( strncmp( cmd, "IP", 2 ) == 0 )
    {
        jt808_param_bk.updata_utctime = 0;
        if( cmd[2] == '1' )
        {
            strcpy( jt808_param.id_0x0013, value );
        }
        else if( cmd[2] == '2' )
        {
            strcpy( jt808_param.id_0x0017, value );
        }
        else
        {
            return 0;
        }
        sms_command |= SMS_CMD_SAVEPARAM | SMS_CMD_RELINK;
        param_save_bksram();
        return 0;
    }
    else if( strncmp( cmd, "DUR", 3 ) == 0 )
    {
        jt808_param.id_0x0029 = atoi(value);
        sms_command |= SMS_CMD_SAVEPARAM;
        return 0;
    }
    else if( strncmp( cmd, "SIMID", 6 ) == 0 )
    {
        memset(jt808_param.id_0xF006, 0, sizeof(jt808_param.id_0xF006));
        strcpy( jt808_param.id_0xF006, value );
        sms_command |= SMS_CMD_SAVEPARAM | SMS_CMD_RELINK;
        return 0;
    }
    else if( strncmp( cmd, "DEVICEID", 8 ) == 0 )
    {
        memset(jt808_param.id_0xF002, 0, sizeof(jt808_param.id_0xF002));
        strcpy( jt808_param.id_0xF002, value ); /*终端ID 大写字母+数字*/
        sms_command |= SMS_CMD_SAVEPARAM;
        return 0;
    }
    else if( strncmp( cmd, "MODE", 4 ) == 0 )        ///6. 设置定位模式
    {
        if( strncmp( value, "BD", 2 ) == 0 )
        {
            jt808_param.id_0x0090 = 0x02;
        }
        else if( strncmp( value, "GP", 2 ) == 0 )
        {
            jt808_param.id_0x0090 = 0x01;
        }
        else if( strncmp( value, "GN", 2 ) == 0 )
        {
            jt808_param.id_0x0090 = 0x03;
        }
        sms_command |= SMS_CMD_SAVEPARAM;
        return 0;
    }
    else if( strncmp( cmd, "VIN", 3 ) == 0 )
    {
        memset(jt808_param.id_0xF005, 0, sizeof(jt808_param.id_0xF005));
        strcpy( jt808_param.id_0xF005, value ); /*VIN(TJSDA09876723424243214324)*/
        sms_command |= SMS_CMD_SAVEPARAM;
        return 0;
    }
    /*	RELAY(0)关闭继电器 继电器通 RELAY(1) 断开继电器 继电器断*/
    else if( strncmp( cmd, "RELAY", 5 ) == 0 )
    {
        return 0;
    }
    else if( strncmp( cmd, "TAKE", 4 ) == 0 )    /*拍照 1.2.3.4路*/
    {
        Cam_takepic(value[0] - '0', 0, 1, 8);
        return 0;
    }
    else if( strncmp( cmd, "PLAY", 4 ) == 0 )    /*语音播报*/
    {
        tts_write( value, strlen( value ) );
        return 0;
    }
    else if(( strncmp( cmd, "QUERY", 5 ) == 0 ) && (send_sms)) /*0 车辆相关信息 1 网络系统配置信息 2位置信息*/
    {
        /*直接发送信息*/
        if( *value == '0' )
        {
            sprintf( buf, "TW705#%s#%s#%s#%d#%d#%d",
                     jt808_param.id_0x0083,
                     jt808_param.id_0xF002,
                     jt808_param.id_0xF005,
                     jt808_param.id_0x0029,
                     jt808_param.id_0x0028,
                     jt808_param.id_0x0027 );
            sms_send( buf, 0  );
        }

        else if( *value == '1' )
        {
            sprintf( buf, "TW705#%s#%s#%s#%d",
                     jt808_param.id_0x0083 + 2,
                     jt808_param.id_0xF002,
                     jt808_param.id_0x0013,
                     jt808_param.id_0x0018,
                     jt808_param.id_0x0017,
                     jt808_param.id_0xF031);
            sms_send( buf, 0  );
        }
        else if( *value == '2' )
        {
            i = 2;
            if(gps_status.mode <= MODE_BDGPS)
            {
                i = gps_status.mode;
            }
            j = 0;
            if(jt808_status & BIT_STATUS_FIXED)
            {
                j = 1;
            }
            sprintf( buf, "TW705#%s#%s#%s#%c#E#%d#N#%d#CSQ%02d",
                     jt808_param.id_0x0083 + 2,
                     jt808_param.id_0xF006,
                     gps_mode_string[i],
                     gps_fixed[j],
                     gps_longi,
                     gps_lati,
                     gsm_param.csq);
            sms_send( buf, 0  );
        }
        else if( *value == '3' )
        {
#ifdef TJ_MIXERS_V01
            sprintf( buf, "TW705#%s#HV%d#SV%d.%02d#BYM_JBC",
                     jt808_param.id_0x0083,
                     HARD_VER, SOFT_VER / 100, SOFT_VER % 100);
#else
            sprintf( buf, "TW705#%s#HV%d#SV%d.%02d#BYM",
                     jt808_param.id_0x0083,
                     HARD_VER, SOFT_VER / 100, SOFT_VER % 100);
#endif
            sms_send( buf, 0  );
        }

        return 0;
    }
    else if( strncmp( cmd, "ISP", 3 ) == 0 )         /*	ISP(202.89.23.210：9000)*/
    {
        p = strchr(value, ':');
        if( p )
        {
            *p++ = 0;
            memset( jt808_param_bk.update_ip, 0, sizeof(jt808_param_bk.update_ip) );
            strcpy( jt808_param_bk.update_ip, value );
            if(sscanf(p, "%d", &port))
            {
                jt808_param_bk.update_port = port;
                jt808_param_bk.updata_utctime = 7200;
                param_save_bksram();
                sms_command |= SMS_CMD_RELINK;
                return 0;
            }
        }
#if 0
        sscanf(value, "%s:%d", ip, &port);
        /*
        gsm_socket[2].index=2;
        strcpy(gsm_socket[2].ipstr,ip);
        gsm_socket[2].port=port;
        gsm_socket[2].state=CONNECT_IDLE;
        */
        jt808_param_bk.update_port = port;
        strcpy( jt808_param_bk.update_ip, ip );
        jt808_param_bk.updata_utctime = 7200;
        //gsmstate( GSM_POWEROFF );
        param_save_bksram();
        sms_command |= SMS_CMD_RELINK;
        return 1;
#endif
        return 0;
    }
    else if( strncmp( cmd, "PLATENUM", 8 ) == 0 )    /*PLATENUM(津A8888)	*/
    {
        strcpy(jt808_param.id_0x0083, value);
        sms_command |= SMS_CMD_SAVEPARAM;
        return 0;
    }
    else if( strncmp( cmd, "COLOR", 5 ) == 0 )       /* COLOR(0) 1： 蓝色  2： 黄色    3： 黑色    4： 白色    9：  其他*/
    {
        jt808_param.id_0x0084 = atoi(value);
        sms_command |= SMS_CMD_SAVEPARAM;
        return 0;
    }
    else if( strncmp( cmd, "CONNECT", 7 ) == 0 )     /*CONNECT(0)  0:  DNSR   优先     1：  MainIP   优先 */
    {
        jt808_param_bk.updata_utctime = 0;
        sms_command |= SMS_CMD_RELINK;
        return 0;
    }
    else if( strncmp( cmd, "PASSWORD", 7 ) == 0 )    /*PASSWORD（0） 1： 通过   0： 不通过*/
    {
        jt808_param.id_0xF00F = 0;
        i = value[0] - '0';
        if( i <= 3 )
        {
            jt808_param.id_0xF00F = i;
        }
        sms_command |= SMS_CMD_SAVEPARAM;
        return 0;
    }
    else if( strncmp( cmd, "FACT", 4 ) == 0 )   	/*1 清空参数，2清空数据*/
    {
        /*直接发送信息*/
        if( *value == '1' )
        {
            //factory_ex(1,0);
            sms_command |= SMS_CMD_FACTSET;
        }

        else if( *value == '2' )
        {
            //factory_ex(2,0);
            sms_command |= SMS_CMD_FACTSETDATA;
        }

        else if( *value == '3' )
        {
            //factory_ex(2,0);
            sms_command |= SMS_CMD_FACTSET;
            sms_command |= SMS_CMD_FACTSETDATA;
        }
        else if( *value == '4' )
        {
            factory_bksram();
        }

        return 0;
    }
    else if(( strncmp( cmd, "0x", 2 ) == 0 ) || ( strncmp( cmd, "0X", 2 ) == 0 ))  /*设置808协议0x8103对应的参数*/
    {
        if(cmd[1] == 'X')
        {
            cmd[1] = 'x';
        }
        if(sscanf( cmd, "%x", &id ) == 1)
        {
            if(jt808_param_set(id, value))
            {
                sms_command |= SMS_CMD_SAVEPARAM;
                return 0;
            }
        }
    }
    else if( strncmp( cmd, "ALARM", 5 ) == 0 )               /*短信命令，清空报警参数为0*/
    {
        /*直接发送信息*/
        if( *value == '0' )
        {
            jt808_param_bk.car_alarm = 0;
        }
    }
    return 0;
}

/*********************************************************************************
  *函数名称:void sms_param_proc(char *info_msg,uint8_t send_sms)
  *功能描述:处理收到的短信内容，需要回复短信的话需要send_sms为非0值
  *输	入:	info_msg:收到的短信内容，该内容是已经解码的短信内容
  			send_sms:为0表示不回复短信，非0表示需要回复短信
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-12-13
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t sms_param_proc(char *info_msg, uint8_t send_sms)
{
    uint16_t	i;
    uint16_t	count;
    uint8_t		cmd_step = 0;
    char		*p;
    char		*psave;
    char		cmd[32];
    char		value[64];
    char		tmpbuf[180];        /*解码后的信息*/

    ///输出信息内容到LCD start
    debug_write("短信内容为:");
    debug_write(info_msg);
    ///输出信息内容到LCD end
    /*decode_msg保存解码后的信息*/
    p = info_msg;
    if(( strncmp( p, "TW703", 5 ) != 0 ) && ( strncmp( p, "TW705", 5 ) != 0 ))
    {
        return 0;
    }
    sprintf(tmpbuf, "SMS(%d,%s):%s", send_sms, sender, info_msg);
    sd_write_console(tmpbuf);
    p += 5;
    memset( cmd, 0, sizeof(cmd) );
    memset( value, 0, sizeof(value) );
    count	= 0;
    psave	= cmd;
    i		= 0;
    /*tmpbuf用作应答的信息 TW703#*/
    while( *p != 0 )
    {
        switch( *p )
        {
        case '#':
            cmd_step = 1;
            memset( cmd, 0, sizeof(cmd) );
            memset( value, 0, sizeof(value) );
            psave	= cmd;		/*先往cmd缓冲中存*/
            count	= 0;
            break;
        case '(':
            if(cmd_step == 1)
                cmd_step = 2;
            psave	= value;	/*向取值区保存*/
            count	= 0;
            break;
        case ')':				/*表示结束,不保存*/
            if(cmd_step == 2)
            {
                i += analy_param( cmd, value, send_sms );
                cmd_step = 0;
            }
            break;
        default:
            if( count < 64 )	/*正常，保存*/
            {
                *psave++ = *p;
                count++;
            }
            break;
        }
        p++;
    }
    if((i) && (send_sms))
    {
        memset(tmpbuf, 0, sizeof(tmpbuf));
        sprintf( tmpbuf, "%s#%s#%s", jt808_param.id_0x0083, jt808_param.id_0xF002, info_msg + 6 );

        sms_send(tmpbuf, 0);
    }
    if(sms_command)
    {
        if(sms_command & SMS_CMD_RESET)
        {
            //reset(4);
            control_device(DEVICE_RESET_DEVICE, 0xFFFFFFFF);
            if(send_sms)
                device_control.delay_counter = 60;
        }
        if(sms_command & SMS_CMD_RELINK)
        {
            modem_poweron_count = 0;
            sock_proc(mast_socket, SOCK_RECONNECT);
            control_device(DEVICE_NORMAL, 0);
            gsmstate( GSM_POWEROFF );
        }
        if(sms_command & SMS_CMD_FACTSET)
        {
            factory_ex(1, 0);
            control_device(DEVICE_RESET_DEVICE, 0xFFFFFFFF);
        }
        if(sms_command & SMS_CMD_SAVEPARAM)
        {
            param_save( 1 );
        }
        if(sms_command & SMS_CMD_FACTSETDATA)
        {
            factory_ex(2, 0);
            control_device(DEVICE_RESET_DEVICE, 0xFFFFFFFF);
        }
        sms_command = 0;
    }
    return 1;

}
FINSH_FUNCTION_EXPORT( sms_param_proc, sms_param_proc );


/*********************************************************************************
  *函数名称:void jt808_sms_rx( char *info, uint16_t size )
  *功能描述:收到短信息处理,并且回复短信内容
  *输	入:	info	:收到的短信内容，该内容是PDU编码的短信内容
  			size	:info的长度
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-12-13
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void jt808_sms_rx_old( char *info, uint16_t size )
{
    uint16_t	i;
    char		*p, *pdst;
    char		c, pid;
    uint16_t	len;
    uint8_t		pdu_type, msg_len;
    uint8_t		dcs;

    char		decode_msg[180];    /*解码后的信息*/
    uint16_t	decode_msg_len = 0;
    char		tmpbuf[180];        /*解码后的信息*/

    char		cmd[64];
    char		value[64];
    uint8_t		count;
    char		*psave;
    uint8_t		cmd_step = 0;

    memset( sender, 0, sizeof(sender) );
    memset( smsc, 0, sizeof(smsc) );
    p = info;                       /*重新定位到开始*/
    /*SCA*/
    len = tbl_assic_to_hex[p[0] - '0'] << 4;
    len |= tbl_assic_to_hex[p[1] - '0'];         /*服务中心号码 Length (长度)+Tosca（服务中心类型）+Address（地址）*/

    if(len > 12)
        return;
    len <<= 1;                      /*转成双字*/
    for( i = 0; i < len + 2; i++ )  /*包括长度*/
    {
        smsc[i] = *p++;
    }
    /*pdu类型*/
    pdu_type	= tbl_assic_to_hex[*p++ - '0'] << 4;
    pdu_type	|= tbl_assic_to_hex[*p++ - '0'];

    /*OA来源地址 05 A1 0180F6*/
    /*M50模块在收到的地址中地址类型为A1,发送的时候不成功，需要转换为9186(91表示为国际号码，86表示中国)才能发送ok*/
    len = tbl_assic_to_hex[p[0] - '0'] << 4;
    len |= tbl_assic_to_hex[p[1] - '0']; /*手机发送源地址 Length (长度,是数字个数)+Tosca（地址类型）+Address（地址）*/

    if(len > 21)
        return;
    if(strncmp(p + 4, "68", 2) != 0)
    {
        sprintf(sender, "%02X9168", len + 2);
        if( len & 0x01 )
        {
            len++;              /*这里是数字位数，比如10086是5位 用三个字节表示*/
        }
        strncpy(sender + 6, p + 4, len);
    }
    else
    {
        if( len & 0x01 )
        {
            len++;              /*这里是数字位数，比如10086是5位 用三个字节表示*/
        }
        strncpy(sender, p, len + 4);
    }
    p	+= 4 + len;
    rt_kprintf("\nSMS>sender=%s", sender);

    /*PID标志*/
    pid = tbl_assic_to_hex[*p++ - '0'] << 4;
    pid |= tbl_assic_to_hex[*p++ - '0'];
    /*DCS编码*/
    dcs = tbl_assic_to_hex[*p++ - '0'] << 4;
    dcs |= tbl_assic_to_hex[*p++ - '0'];
    /*SCTS服务中心时间戳 yymmddhhmmsszz 包含时区*/
    p += 14;
    /*消息长度*/
    //msg_len = tbl[*p++ - '0'] << 4;
    //msg_len |= tbl[*p++ - '0'];
    p += 2;

    if((uint32_t)info + size <= (uint32_t)p)
    {
        return;
    }
    msg_len = ((uint32_t)info + size - (uint32_t)p) / 2;
    /*是否为长信息pdu_type 的bit6 UDHI=1*/
    if( pdu_type & 0x40 )           /*跳过长信息的头*/
    {
        if( ( dcs & 0x0c ) != GSM_7BIT )		///暂时不处理7bit编码的情况，因为7bit编码对该区域也编码了
        {
            len		= tbl_assic_to_hex[*p++ - '0'] << 4;
            len		|= tbl_assic_to_hex[*p++ - '0'];
            p		+= ( len << 1 );    /*现在还是OCT模式*/
            msg_len -= ( len + 1 );     /*长度占一个字节*/
        }
    }

    rt_kprintf( "\nSMS>PDU Type=%02x PID=%02x DCS=%02x TP-UDL=%d", pdu_type, pid, dcs, msg_len );

    /*直接在info上操作有误?*/
    pdst = tmpbuf;
    /*将OCT数据转为HEX,两个字节拼成一个*/
    for( i = 0; i < msg_len; i++ )
    {
        c		= tbl_assic_to_hex[*p++ - '0'] << 4;
        c		|= tbl_assic_to_hex[*p++ - '0'];
        *pdst++ = c;
    }
    *pdst = 0;

    if( ( dcs & 0x0c ) == GSM_7BIT )
    {
        decode_msg_len = gsmdecode7bit( tmpbuf, decode_msg, msg_len );
        /*将用户数据头部分去掉*/
        len = tmpbuf[0] + 1;		//计算用户数据，包含了 用户数据头长度(1byte) + 用户数据;
        if(len * 8 % 7)
        {
            len = len * 8 / 7 + 1;
        }
        else
        {
            len =  len * 8 / 7;
        }
        if(( pdu_type & 0x40 ) && (len < decode_msg_len))
        {
            decode_msg_len	-= len;
            memcpy(tmpbuf, decode_msg + len, decode_msg_len);
            memcpy(decode_msg, tmpbuf, decode_msg_len);
        }
    }
    else if( ( dcs & 0x0c ) == GSM_UCS2 )
    {
        decode_msg_len = gsmdecodeucs2( tmpbuf, decode_msg, msg_len );
    }
    else
    {
        memcpy( decode_msg, tmpbuf, msg_len );
        decode_msg_len = msg_len;   /*简单拷贝即可*/
    }

    if( decode_msg_len == 0 )       /*对于不是合法的数据是不是直接返回0*/
    {
        return;
    }
    decode_msg[decode_msg_len] = 0;

    /*decode_msg保存解码后的信息*/
    p = decode_msg;
    rt_kprintf( "\nSMS(%dbytes)>%s", decode_msg_len, p );
    if(( strncmp( p, "TW703", 5 ) != 0 ) && ( strncmp( p, "TW705", 5 ) != 0 ))
    {
        return;
    }
    p += 5;
    memset( cmd, 0, 64 );
    memset( value, 0, 64 );
    count	= 0;
    psave	= cmd;
    i		= 0;
    /*tmpbuf用作应答的信息 TW703#*/
    while( *p != 0 )
    {
        switch( *p )
        {
        case '#':
            cmd_step = 1;
            memset( cmd, 0, 64 );
            memset( value, 0, 64 );
            psave	= cmd;      /*先往cmd缓冲中存*/
            count	= 0;
            break;
        case '(':
            if(cmd_step == 1)
                cmd_step = 2;
            psave	= value;    /*向取值区保存*/
            count	= 0;
            break;
        case ')':               /*表示结束,不保存*/
            if(cmd_step == 2)
            {
                i += analy_param( cmd, value, 1 ) ;
                cmd_step = 0;
            }
            break;
        default:
            if( count < 64 )    /*正常，保存*/
            {
                *psave++ = *p;
                count++;
            }
            break;
        }
        p++;
    }

    if(i)
    {
        memset(tmpbuf, 0, sizeof(tmpbuf));
        sprintf( tmpbuf, "%s#%-7s#%s", jt808_param.id_0x0083, jt808_param.id_0xF002, decode_msg + 6 );

        sms_send(tmpbuf, 0);
    }

    if(sms_command)
    {
        if(sms_command & SMS_CMD_RESET)
        {
            reset(4);
        }
        if(sms_command & SMS_CMD_RELINK)
        {
            gsmstate( GSM_POWEROFF );
        }
        if(sms_command & SMS_CMD_FACTSET)
        {
            rt_kprintf("SMS> 恢复出厂参数;");
        }
        if(sms_command & SMS_CMD_SAVEPARAM)
        {
            param_save( 1 );
        }
        sms_command = 0;
    }
}

/*********************************************************************************
  *函数名称:void jt808_sms_rx( char *info, uint16_t size )
  *功能描述:收到短信息处理,并且回复短信内容
  *输	入:	info	:收到的短信内容，该内容是PDU编码的短信内容
  			size	:info的长度
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-12-13
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void jt808_sms_rx_old2( char *info, uint16_t size )
{
    uint16_t	i;
    char		*p, *pdst;
    char		c, pid;
    uint16_t	len;
    uint8_t		pdu_type, msg_len;
    uint8_t		dcs;

    char		decode_msg[180];    /*解码后的信息*/
    uint16_t	decode_msg_len = 0;
    char		tmpbuf[180];        /*解码后的信息*/

    //char		cmd[64];
    //char		value[64];
    //uint8_t		count;
    //char		*psave;
    //uint8_t		cmd_step=0;

    memset( sender, 0, sizeof(sender) );
    memset( smsc, 0, sizeof(smsc) );
    p = info;                       /*重新定位到开始*/
    /*SCA*/
    len = tbl_assic_to_hex[p[0] - '0'] << 4;
    len |= tbl_assic_to_hex[p[1] - '0'];         /*服务中心号码 Length (长度)+Tosca（服务中心类型）+Address（地址）*/

    if(len > 12)
        return;
    len <<= 1;                      /*转成双字*/
    for( i = 0; i < len + 2; i++ )  /*包括长度*/
    {
        smsc[i] = *p++;
    }
    /*pdu类型*/
    pdu_type	= tbl_assic_to_hex[*p++ - '0'] << 4;
    pdu_type	|= tbl_assic_to_hex[*p++ - '0'];

    /*OA来源地址 05 A1 0180F6*/
    /*M50模块在收到的地址中地址类型为A1,发送的时候不成功，需要转换为9186(91表示为国际号码，86表示中国)才能发送ok*/
    len = tbl_assic_to_hex[p[0] - '0'] << 4;
    len |= tbl_assic_to_hex[p[1] - '0']; /*手机发送源地址 Length (长度,是数字个数)+Tosca（地址类型）+Address（地址）*/

    if(len > 21)
        return;
    if(strncmp(p + 4, "68", 2) != 0)
    {
        sprintf(sender, "%02X9168", len + 2);
        if( len & 0x01 )
        {
            len++;              /*这里是数字位数，比如10086是5位 用三个字节表示*/
        }
        strncpy(sender + 6, p + 4, len);
    }
    else
    {
        if( len & 0x01 )
        {
            len++;              /*这里是数字位数，比如10086是5位 用三个字节表示*/
        }
        strncpy(sender, p, len + 4);
    }
    p	+= 4 + len;
    rt_kprintf("\nSMS>sender=%s", sender);

    /*PID标志*/
    pid = tbl_assic_to_hex[*p++ - '0'] << 4;
    pid |= tbl_assic_to_hex[*p++ - '0'];
    /*DCS编码*/
    dcs = tbl_assic_to_hex[*p++ - '0'] << 4;
    dcs |= tbl_assic_to_hex[*p++ - '0'];
    /*SCTS服务中心时间戳 yymmddhhmmsszz 包含时区*/
    p += 14;
    /*消息长度*/
    //msg_len = tbl[*p++ - '0'] << 4;
    //msg_len |= tbl[*p++ - '0'];
    p += 2;

    if((uint32_t)info + size <= (uint32_t)p)
    {
        return;
    }
    msg_len = ((uint32_t)info + size - (uint32_t)p) / 2;
    /*是否为长信息pdu_type 的bit6 UDHI=1*/
    if( pdu_type & 0x40 )           /*跳过长信息的头*/
    {
        if( ( dcs & 0x0c ) != GSM_7BIT )		///暂时不处理7bit编码的情况，因为7bit编码对该区域也编码了
        {
            len		= tbl_assic_to_hex[*p++ - '0'] << 4;
            len		|= tbl_assic_to_hex[*p++ - '0'];
            p		+= ( len << 1 );    /*现在还是OCT模式*/
            msg_len -= ( len + 1 );     /*长度占一个字节*/
        }
    }

    rt_kprintf( "\nSMS>PDU Type=%02x PID=%02x DCS=%02x TP-UDL=%d", pdu_type, pid, dcs, msg_len );

    /*直接在info上操作有误?*/
    pdst = tmpbuf;
    /*将OCT数据转为HEX,两个字节拼成一个*/
    for( i = 0; i < msg_len; i++ )
    {
        c		= tbl_assic_to_hex[*p++ - '0'] << 4;
        c		|= tbl_assic_to_hex[*p++ - '0'];
        *pdst++ = c;
    }
    *pdst = 0;

    if( ( dcs & 0x0c ) == GSM_7BIT )
    {
        decode_msg_len = gsmdecode7bit( tmpbuf, decode_msg, msg_len );
        /*将用户数据头部分去掉*/
        len = tmpbuf[0] + 1;		//计算用户数据，包含了 用户数据头长度(1byte) + 用户数据;
        if(len * 8 % 7)
        {
            len = len * 8 / 7 + 1;
        }
        else
        {
            len =  len * 8 / 7;
        }
        if(( pdu_type & 0x40 ) && (len < decode_msg_len))
        {
            decode_msg_len	-= len;
            memcpy(tmpbuf, decode_msg + len, decode_msg_len);
            memcpy(decode_msg, tmpbuf, decode_msg_len);
        }
    }
    else if( ( dcs & 0x0c ) == GSM_UCS2 )
    {
        decode_msg_len = gsmdecodeucs2( tmpbuf, decode_msg, msg_len );
    }
    else
    {
        memcpy( decode_msg, tmpbuf, msg_len );
        decode_msg_len = msg_len;   /*简单拷贝即可*/
    }

    if( decode_msg_len == 0 )       /*对于不是合法的数据是不是直接返回0*/
    {
        return;
    }
    decode_msg[decode_msg_len] = 0;


    //////
    /*decode_msg保存解码后的信息*/
    rt_kprintf( "\nSMS(%dbytes)>%s", decode_msg_len, decode_msg );
    sms_param_proc(decode_msg, 1);
#if 0
    p = decode_msg;
    if(( strncmp( p, "TW703", 5 ) != 0 ) && ( strncmp( p, "TW705", 5 ) != 0 ))
    {
        return;
    }
    p += 5;
    memset( cmd, 0, 64 );
    memset( value, 0, 64 );
    count	= 0;
    psave	= cmd;
    i		= 0;
    /*tmpbuf用作应答的信息 TW703#*/
    while( *p != 0 )
    {
        switch( *p )
        {
        case '#':
            cmd_step = 1;
            memset( cmd, 0, 64 );
            memset( value, 0, 64 );
            psave	= cmd;      /*先往cmd缓冲中存*/
            count	= 0;
            break;
        case '(':
            if(cmd_step == 1)
                cmd_step = 2;
            psave	= value;    /*向取值区保存*/
            count	= 0;
            break;
        case ')':               /*表示结束,不保存*/
            if(cmd_step == 2)
            {
                i += analy_param( cmd, value , 1);
                cmd_step = 0;
            }
            break;
        default:
            if( count < 64 )    /*正常，保存*/
            {
                *psave++ = *p;
                count++;
            }
            break;
        }
        p++;
    }

    if(i)
    {
        memset(tmpbuf, 0, sizeof(tmpbuf));
        sprintf( tmpbuf, "%s#%-7s#%s", jt808_param.id_0x0083, jt808_param.id_0xF002, decode_msg + 6 );

        sms_send(tmpbuf, 0);
    }

    if(sms_command)
    {
        if(sms_command & SMS_CMD_RESET)
        {
            reset(4);
        }
        if(sms_command & SMS_CMD_RELINK)
        {
            gsmstate( GSM_POWEROFF );
        }
        if(sms_command & SMS_CMD_FACTSET)
        {
            rt_kprintf("SMS> 恢复出厂参数;");
        }
        if(sms_command & SMS_CMD_SAVEPARAM)
        {
            param_save( 1 );
        }
        sms_command = 0;
    }
#endif
}

/*********************************************************************************
  *函数名称:void jt808_sms_rx( char *info, uint16_t size )
  *功能描述:收到短信息处理,并且回复短信内容
  *输	入:	info	:收到的短信内容，该内容是PDU编码的短信内容
  			size	:info的长度
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-12-13
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void jt808_sms_rx( char *sms_data, uint16_t size )
{
    uint16_t	i;
    char		*p, *pdst;
    char		c, pid;
    uint16_t	len, msg_len;
    uint8_t		pdu_type;
    uint8_t		dcs;

    char		decode_msg[180];    /*解码后的信息*/
    uint16_t	decode_msg_len = 0;
    char		tmpbuf[180];        /*解码后的信息*/
    char 		*info = sms_buf;

    //char		cmd[64];
    //char		value[64];
    //uint8_t		count;
    //char		*psave;
    //uint8_t		cmd_step=0;
    memcpy( sms_buf, sms_data, size);
    memset( sender, 0, sizeof(sender) );
    memset( smsc, 0, sizeof(smsc) );
    p = info;                       /*重新定位到开始*/
    /*SCA*/
    len = tbl_assic_to_hex[p[0] - '0'] << 4;
    len |= tbl_assic_to_hex[p[1] - '0'];         /*服务中心号码 Length (长度)+Tosca（服务中心类型）+Address（地址）*/

    if(len > 12)
        return;
    len <<= 1;                      /*转成双字*/
    for( i = 0; i < len + 2; i++ )  /*包括长度*/
    {
        smsc[i] = *p++;
    }
    /*pdu类型*/
    pdu_type	= tbl_assic_to_hex[*p++ - '0'] << 4;
    pdu_type	|= tbl_assic_to_hex[*p++ - '0'];

    /*OA来源地址 05 A1 0180F6*/
    /*M50模块在收到的地址中地址类型为A1,发送的时候不成功，需要转换为9186(91表示为国际号码，86表示中国)才能发送ok*/
    len = tbl_assic_to_hex[p[0] - '0'] << 4;
    len |= tbl_assic_to_hex[p[1] - '0']; /*手机发送源地址 Length (长度,是数字个数)+Tosca（地址类型）+Address（地址）*/

    if(len > 21)
        return;
    memset(sender_tel, 0, sizeof(sender_tel));
    if(strncmp(p + 4, "68", 2) != 0)
    {
        sprintf(sender, "%02X9168", len + 2);
        if( len & 0x01 )
        {
            len++;              /*这里是数字位数，比如10086是5位 用三个字节表示*/
        }
        strncpy(sender + 6, p + 4, len);
        memcpy(sender_tel, p + 4, len);
    }
    else
    {
        if( len & 0x01 )
        {
            len++;              /*这里是数字位数，比如10086是5位 用三个字节表示*/
        }
        strncpy(sender, p, len + 4);
        memcpy(sender_tel, p, len);
    }
    for(i = 0; i < len; i += 2)
    {
        c = sender_tel[i];
        sender_tel[i] = sender_tel[i + 1];
        sender_tel[i + 1] = c;
    }
    if(sender_tel[len - 1] == 'F')
        sender_tel[len - 1] = 0;
    sprintf(tmpbuf, "短信:%s", sender_tel);		///
    debug_write(tmpbuf);
    p	+= 4 + len;
    rt_kprintf("\nSMS>sender=%s", sender);

    /*PID标志*/
    pid = tbl_assic_to_hex[*p++ - '0'] << 4;
    pid |= tbl_assic_to_hex[*p++ - '0'];
    /*DCS编码*/
    dcs = tbl_assic_to_hex[*p++ - '0'] << 4;
    dcs |= tbl_assic_to_hex[*p++ - '0'];
    /*SCTS服务中心时间戳 yymmddhhmmsszz 包含时区*/
    p += 14;
    /*消息长度*/
    //msg_len = tbl[*p++ - '0'] << 4;
    //msg_len |= tbl[*p++ - '0'];
    p += 2;

    if((uint32_t)info + size <= (uint32_t)p)
    {
        return;
    }
    msg_len = ((uint32_t)info + size - (uint32_t)p) / 2;
    /*是否为长信息pdu_type 的bit6 UDHI=1*/
    if( pdu_type & 0x40 )           /*跳过长信息的头*/
    {
        if( ( dcs & 0x0c ) != GSM_7BIT )		///暂时不处理7bit编码的情况，因为7bit编码对该区域也编码了
        {
            len		= tbl_assic_to_hex[*p++ - '0'] << 4;
            len		|= tbl_assic_to_hex[*p++ - '0'];
            p		+= ( len << 1 );    /*现在还是OCT模式*/
            msg_len -= ( len + 1 );     /*长度占一个字节*/
        }
    }

    rt_kprintf( "\nSMS>PDU Type=%02x PID=%02x DCS=%02x TP-UDL=%d", pdu_type, pid, dcs, msg_len );

    /*直接在info上操作有误?*/
    pdst = tmpbuf;
    /*将OCT数据转为HEX,两个字节拼成一个*/
    for( i = 0; i < msg_len; i++ )
    {
        c		= tbl_assic_to_hex[*p++ - '0'] << 4;
        c		|= tbl_assic_to_hex[*p++ - '0'];
        *pdst++ = c;
    }
    *pdst = 0;

    if( ( dcs & 0x0c ) == GSM_7BIT )
    {
        decode_msg_len = gsmdecode7bit( tmpbuf, decode_msg, msg_len );
        /*将用户数据头部分去掉*/
        len = tmpbuf[0] + 1;		//计算用户数据，包含了 用户数据头长度(1byte) + 用户数据;
        if(len * 8 % 7)
        {
            len = len * 8 / 7 + 1;
        }
        else
        {
            len =  len * 8 / 7;
        }
        if(( pdu_type & 0x40 ) && (len < decode_msg_len))
        {
            decode_msg_len	-= len;
            memcpy(tmpbuf, decode_msg + len, decode_msg_len);
            memcpy(decode_msg, tmpbuf, decode_msg_len);
        }
    }
    else if( ( dcs & 0x0c ) == GSM_UCS2 )
    {
        decode_msg_len = gsmdecodeucs2( tmpbuf, decode_msg, msg_len );
    }
    else
    {
        memcpy( decode_msg, tmpbuf, msg_len );
        decode_msg_len = msg_len;   /*简单拷贝即可*/
    }

    if( decode_msg_len == 0 )       /*对于不是合法的数据是不是直接返回0*/
    {
        return;
    }
    decode_msg[decode_msg_len] = 0;


    //////
    /*decode_msg保存解码后的信息*/
    rt_kprintf( "\nSMS(%dbytes)>%s", decode_msg_len, decode_msg );
    sms_param_proc(decode_msg, 1);
#if 0
    p = decode_msg;
    if(( strncmp( p, "TW703", 5 ) != 0 ) && ( strncmp( p, "TW705", 5 ) != 0 ))
    {
        return;
    }
    p += 5;
    memset( cmd, 0, 64 );
    memset( value, 0, 64 );
    count	= 0;
    psave	= cmd;
    i		= 0;
    /*tmpbuf用作应答的信息 TW703#*/
    while( *p != 0 )
    {
        switch( *p )
        {
        case '#':
            cmd_step = 1;
            memset( cmd, 0, 64 );
            memset( value, 0, 64 );
            psave	= cmd;      /*先往cmd缓冲中存*/
            count	= 0;
            break;
        case '(':
            if(cmd_step == 1)
                cmd_step = 2;
            psave	= value;    /*向取值区保存*/
            count	= 0;
            break;
        case ')':               /*表示结束,不保存*/
            if(cmd_step == 2)
            {
                i += analy_param( cmd, value , 1);
                cmd_step = 0;
            }
            break;
        default:
            if( count < 64 )    /*正常，保存*/
            {
                *psave++ = *p;
                count++;
            }
            break;
        }
        p++;
    }

    if(i)
    {
        memset(tmpbuf, 0, sizeof(tmpbuf));
        sprintf( tmpbuf, "%s#%-7s#%s", jt808_param.id_0x0083, jt808_param.id_0xF002, decode_msg + 6 );

        sms_send(tmpbuf, 0);
    }

    if(sms_command)
    {
        if(sms_command & SMS_CMD_RESET)
        {
            reset(4);
        }
        if(sms_command & SMS_CMD_RELINK)
        {
            gsmstate( GSM_POWEROFF );
        }
        if(sms_command & SMS_CMD_FACTSET)
        {
            rt_kprintf("SMS> 恢复出厂参数;");
        }
        if(sms_command & SMS_CMD_SAVEPARAM)
        {
            param_save( 1 );
        }
        sms_command = 0;
    }
#endif
}




#if 1
/*
   0891683108200205F0000D91683106029691F10008318011216584232C0054005700370030003300230050004C004100540045004E0055004D00286D25005400540036003500320029
   0891683108200205F0000D91683106029691F100003180112175222332D4EB0D361B0D9F4E67714845C15223A2732A8DA1D4F498EB7C46E7E17497BB4C4F8DA04F293586BAC160B814

   在线验证

   http://www.diafaan.com/sms-tutorials/gsm-modem-tutorial/online-sms-submit-pdu-decoder/
 */
void sms_test( uint8_t index )
{
    char *s[] =
    {
        "0891683108200205F0000D91685120623844F00000416092311512232ED4EB0D561BC1F030180D868ACD703258AD46C3D96629D0088687C16838144C26B3D566389C2C664B01",
        "0891683108200205F0000D91683128504568F30008318011216584232C0054005700370030003300230050004C004100540045004E0055004D00286D25005400540036003500320029",
        "0891683108200205F0000D91683128504568F300003180112175222332D4EB0D361B0D9F4E67714845C15223A2732A8DA1D4F498EB7C46E7E17497BB4C4F8DA04F293586BAC160B814",
        "0891683108200245F3240D90683128504568F30008312132513452232E6D259655004000280029004000230062006100630063006100620061003100310031003200320032003300330033"
    };
    jt808_sms_rx( s[index], strlen( s[index] ) );
}

FINSH_FUNCTION_EXPORT( sms_test, test sms );


void oiap_test(char *ip, uint16_t port)
{
    iccard_socket->index = 2;
    strcpy(iccard_socket->ipstr, ip);
    iccard_socket->port = port;
    iccard_socket->state = CONNECT_IDLE;
}
FINSH_FUNCTION_EXPORT( oiap_test, conn onair iap );

#endif

/************************************** The End Of File **************************************/
