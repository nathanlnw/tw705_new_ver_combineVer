#ifndef _H_COMMON
#define _H_COMMON

#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
//#include <dfs_posix.h>

#include "include.h"

#ifndef BIT
#define BIT( i ) ( (unsigned long)( 1 << i ) )
#endif

extern const unsigned char tbl_hex_to_assic[]; 		//���ڴ��д洢��16����������ת��ΪASSIC�ɼ��ַ����ݣ���Ҫ�õ��ı�
extern const unsigned char tbl_assic_to_hex[24];	//��ASSIC�ɼ��ַ�������ת��Ϊ���ڴ��д洢��16�������ݣ���Ҫ�õ��ı�


extern unsigned int Hex_To_Ascii( unsigned char *pDst, const unsigned char *pSrc, unsigned int nSrcLength );
extern unsigned int Ascii_To_Hex(unsigned char *src_dest, char *src_buf, unsigned int max_rx_len);
extern unsigned long AssicBufToUL(char *buf, unsigned int num);
extern void printf_hex_data( const u8 *pSrc, u16 nSrcLength );
extern u8 Get_Month_Day(u8 month, u8 leapyear);
extern void strtrim( u8 *s, u8 c );

#endif
