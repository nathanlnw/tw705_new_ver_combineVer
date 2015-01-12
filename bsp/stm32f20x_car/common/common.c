#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "include.h"


const unsigned char	tbl_hex_to_assic[] = "0123456789ABCDEF"; 	// 0x0-0xf的字符查找表
const unsigned char tbl_assic_to_hex[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };


/*********************************************************************************
*函数名称:unsigned int Hex_To_Ascii( unsigned char* pDst, const unsigned char* pSrc, unsigned int nSrcLength )
*功能描述:将hex数据转换为在内存中存储的16进制ASSIC码字符串
*输    入:	pDst	:存储转换ASSIC码字符串的结果
			pSrc	:原始数据
			nSrcLength	:pSrc长度
*输    出:unsigned int 型数据，表示转换的ASSIC码pDst的长度
*返 回 值:
*作    者:白养民
*创建日期:2013-2-19
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
unsigned int Hex_To_Ascii( unsigned char *pDst, const unsigned char *pSrc, unsigned int nSrcLength )
{
    u16			i;

    for( i = 0; i < nSrcLength; i++ )
    {
        // 输出低4位
        *pDst++ = tbl_hex_to_assic[*pSrc >> 4];

        // 输出高4位
        *pDst++ = tbl_hex_to_assic[*pSrc & 0x0f];

        pSrc++;
    }

    // 输出字符串加个结束符
    *pDst = '\0';

    // 返回目标字符串长度
    return ( nSrcLength << 1 );
}


/*********************************************************************************
*函数名称:unsigned int Ascii_To_Hex(unsigned char *src_dest,char * src_buf,unsigned int max_rx_len)
*功能描述:将16进制ASSIC码字符串转换为在内存中存储的hex数据
*输    入:	src_dest:存储转换的结果
			src_buf	:ASSIC码字符串
			max_rx_len		:src_dest最大可以接收的长度
*输    出:unsigned int 型数据，表示转换的src_dest的长度
*返 回 值:
*作    者:白养民
*创建日期:2013-2-19
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
unsigned int Ascii_To_Hex(unsigned char *src_dest, char *src_buf, unsigned int max_rx_len)
{
    char		c;
    char 		*p;
    unsigned int i, infolen;

    if((unsigned long)src_dest == 0)
        return 0;
    infolen = strlen(src_buf) / 2;
    p = src_buf;
    for( i = 0; i < infolen; i++ )
    {
        c		= tbl_assic_to_hex[*p++ - '0'] << 4;
        c		|= tbl_assic_to_hex[*p++ - '0'];
        src_dest[i] = c;
        if(i >= max_rx_len)
            break;
    }
    return i;
}





/*********************************************************************************
*函数名称:unsigned long AssicBufToUL(char * buf,unsigned int num)
*功能描述:将10进制ASSIC码字符串或指定最大长度为num的字符串转为unsigned long型数据
*输    入:	* buf	:ASSIC码字符串
			num		:字符串的最大长度
*输    出:unsigned long 型数据
*返 回 值:
*作    者:白养民
*创建日期:2013-2-19
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
unsigned long AssicBufToUL(char *buf, unsigned int num)
{
    unsigned char tempChar;
    unsigned int i, j;
    unsigned long retLong = 0;

    for(i = 0; i < num; i++)
    {
        tempChar = (unsigned char)buf[i];
        if((tempChar >= '0') && (tempChar <= '9'))
        {
            retLong *= 10;
            retLong += tempChar - '0';
        }
        else
        {
            return retLong;
        }
    }
    return retLong;
}

/*********************************************************************************
*函数名称:void printf_hex_data( const u8* pSrc, u16 nSrcLength )
*功能描述:将hex数据转换为在内存中存储的16进制ASSIC码字符串然后打印出来
*输    入:	pSrc	:原始数据
			nSrcLength	:pSrc长度
*输    出:none
*返 回 值:
*作    者:白养民
*创建日期:2013-2-19
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void printf_hex_data( const u8 *pSrc, u16 nSrcLength )
{
    char 		pDst[3];
    u16			i;


    pDst[2]  = 0;
    for( i = 0; i < nSrcLength; i++ )
    {
        // 输出低4位
        pDst[0] = tbl_hex_to_assic[*pSrc >> 4];

        // 输出高4位
        pDst[1] = tbl_hex_to_assic[*pSrc & 0x0f];

        pSrc++;

        rt_kprintf(pDst);
    }
}



u8 Get_Month_Day(u8 month, u8 leapyear)
{
    u8 day;
    switch(month)
    {
    case 12 :
        day = 31;
        break;
    case 11 :
        day = 30;
        break;
    case 10 :
        day = 31;
        break;
    case 9 :
        day = 30;
        break;
    case 8 :
        day = 31;
        break;
    case 7 :
        day = 31;
        break;
    case 6 :
        day = 30;
        break;
    case 5 :
        day = 31;
        break;
    case 4 :
        day = 30;
        break;
    case 3 :
        day = 31;
        break;
    case 2 :
    {
        day = 28;
        day += leapyear;
        break;
    }
    case 1 :
        day = 31;
        break;
    default :
        break;
    }
    return day;
}



// **************************************************************************
// trim a string of it's leading and trailing characters
// 该函数功能为去掉字符串s中前后为c的字符
void strtrim( u8 *s, u8 c )
{
    u8	i, j, * p1, * p2;

    if ( s == 0 )
    {
        return;
    }

    // delete the trailing characters
    if ( *s == 0 )
    {
        return;
    }
    j = strlen( (const char *)s );
    p1 = s + j;
    for ( i = 0; i < j; i++ )
    {
        p1--;
        if ( *p1 != c )
        {
            break;
        }
    }
    if ( i < j )
    {
        p1++;
    }
    *p1 = 0;	// null terminate the undesired trailing characters

    // delete the leading characters
    p1 = s;
    if ( *p1 == 0 )
    {
        return;
    }
    for ( i = 0; *p1++ == c; i++ )
        ;
    if ( i > 0 )
    {
        p2 = s;
        p1--;
        for ( ; *p1 != 0; )
        {
            *p2++ = *p1++;
        }
        *p2 = 0;
    }
}


