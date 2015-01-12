#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "include.h"


const unsigned char	tbl_hex_to_assic[] = "0123456789ABCDEF"; 	// 0x0-0xf���ַ����ұ�
const unsigned char tbl_assic_to_hex[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };


/*********************************************************************************
*��������:unsigned int Hex_To_Ascii( unsigned char* pDst, const unsigned char* pSrc, unsigned int nSrcLength )
*��������:��hex����ת��Ϊ���ڴ��д洢��16����ASSIC���ַ���
*��    ��:	pDst	:�洢ת��ASSIC���ַ����Ľ��
			pSrc	:ԭʼ����
			nSrcLength	:pSrc����
*��    ��:unsigned int �����ݣ���ʾת����ASSIC��pDst�ĳ���
*�� �� ֵ:
*��    ��:������
*��������:2013-2-19
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
unsigned int Hex_To_Ascii( unsigned char *pDst, const unsigned char *pSrc, unsigned int nSrcLength )
{
    u16			i;

    for( i = 0; i < nSrcLength; i++ )
    {
        // �����4λ
        *pDst++ = tbl_hex_to_assic[*pSrc >> 4];

        // �����4λ
        *pDst++ = tbl_hex_to_assic[*pSrc & 0x0f];

        pSrc++;
    }

    // ����ַ����Ӹ�������
    *pDst = '\0';

    // ����Ŀ���ַ�������
    return ( nSrcLength << 1 );
}


/*********************************************************************************
*��������:unsigned int Ascii_To_Hex(unsigned char *src_dest,char * src_buf,unsigned int max_rx_len)
*��������:��16����ASSIC���ַ���ת��Ϊ���ڴ��д洢��hex����
*��    ��:	src_dest:�洢ת���Ľ��
			src_buf	:ASSIC���ַ���
			max_rx_len		:src_dest�����Խ��յĳ���
*��    ��:unsigned int �����ݣ���ʾת����src_dest�ĳ���
*�� �� ֵ:
*��    ��:������
*��������:2013-2-19
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:unsigned long AssicBufToUL(char * buf,unsigned int num)
*��������:��10����ASSIC���ַ�����ָ����󳤶�Ϊnum���ַ���תΪunsigned long������
*��    ��:	* buf	:ASSIC���ַ���
			num		:�ַ�������󳤶�
*��    ��:unsigned long ������
*�� �� ֵ:
*��    ��:������
*��������:2013-2-19
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:void printf_hex_data( const u8* pSrc, u16 nSrcLength )
*��������:��hex����ת��Ϊ���ڴ��д洢��16����ASSIC���ַ���Ȼ���ӡ����
*��    ��:	pSrc	:ԭʼ����
			nSrcLength	:pSrc����
*��    ��:none
*�� �� ֵ:
*��    ��:������
*��������:2013-2-19
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
void printf_hex_data( const u8 *pSrc, u16 nSrcLength )
{
    char 		pDst[3];
    u16			i;


    pDst[2]  = 0;
    for( i = 0; i < nSrcLength; i++ )
    {
        // �����4λ
        pDst[0] = tbl_hex_to_assic[*pSrc >> 4];

        // �����4λ
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
// �ú�������Ϊȥ���ַ���s��ǰ��Ϊc���ַ�
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


