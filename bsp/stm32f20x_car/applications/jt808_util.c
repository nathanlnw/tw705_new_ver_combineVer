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
#include "jt808_util.h"
#include <rtthread.h>
#include <finsh.h>

const unsigned short tbl_crc[256] =   /* CRC ��ʽ�� MODBUS Э����ʽ��*/
{
    0x0000, 0xC1C0, 0x81C1, 0x4001, 0x01C3, 0xC003, 0x8002, 0x41C2,
    0x01C6, 0xC006, 0x8007, 0x41C7, 0x0005, 0xC1C5, 0x81C4, 0x4004,
    0x01CC, 0xC00C, 0x800D, 0x41CD, 0x000F, 0xC1CF, 0x81CE, 0x400E,
    0x000A, 0xC1CA, 0x81CB, 0x400B, 0x01C9, 0xC009, 0x8008, 0x41C8,
    0x01D8, 0xC018, 0x8019, 0x41D9, 0x001B, 0xC1DB, 0x81DA, 0x401A,
    0x001E, 0xC1DE, 0x81DF, 0x401F, 0x01DD, 0xC01D, 0x801C, 0x41DC,
    0x0014, 0xC1D4, 0x81D5, 0x4015, 0x01D7, 0xC017, 0x8016, 0x41D6,
    0x01D2, 0xC012, 0x8013, 0x41D3, 0x0011, 0xC1D1, 0x81D0, 0x4010,
    0x01F0, 0xC030, 0x8031, 0x41F1, 0x0033, 0xC1F3, 0x81F2, 0x4032,
    0x0036, 0xC1F6, 0x81F7, 0x4037, 0x01F5, 0xC035, 0x8034, 0x41F4,
    0x003C, 0xC1FC, 0x81FD, 0x403D, 0x01FF, 0xC03F, 0x803E, 0x41FE,
    0x01FA, 0xC03A, 0x803B, 0x41FB, 0x0039, 0xC1F9, 0x81F8, 0x4038,
    0x0028, 0xC1E8, 0x81E9, 0x4029, 0x01EB, 0xC02B, 0x802A, 0x41EA,
    0x01EE, 0xC02E, 0x802F, 0x41EF, 0x002D, 0xC1ED, 0x81EC, 0x402C,
    0x01E4, 0xC024, 0x8025, 0x41E5, 0x0027, 0xC1E7, 0x81E6, 0x4026,
    0x0022, 0xC1E2, 0x81E3, 0x4023, 0x01E1, 0xC021, 0x8020, 0x41E0,
    0x01A0, 0xC060, 0x8061, 0x41A1, 0x0063, 0xC1A3, 0x81A2, 0x4062,
    0x0066, 0xC1A6, 0x81A7, 0x4067, 0x01A5, 0xC065, 0x8064, 0x41A4,
    0x006C, 0xC1AC, 0x81AD, 0x406D, 0x01AF, 0xC06F, 0x806E, 0x41AE,
    0x01AA, 0xC06A, 0x806B, 0x41AB, 0x0069, 0xC1A9, 0x81A8, 0x4068,
    0x0078, 0xC1B8, 0x81B9, 0x4079, 0x01BB, 0xC07B, 0x807A, 0x41BA,
    0x01BE, 0xC07E, 0x807F, 0x41BF, 0x007D, 0xC1BD, 0x81BC, 0x407C,
    0x01B4, 0xC074, 0x8075, 0x41B5, 0x0077, 0xC1B7, 0x81B6, 0x4076,
    0x0072, 0xC1B2, 0x81B3, 0x4073, 0x01B1, 0xC071, 0x8070, 0x41B0,
    0x0050, 0xC190, 0x8191, 0x4051, 0x0193, 0xC053, 0x8052, 0x4192,
    0x0196, 0xC056, 0x8057, 0x4197, 0x0055, 0xC195, 0x8194, 0x4054,
    0x019C, 0xC05C, 0x805D, 0x419D, 0x005F, 0xC19F, 0x819E, 0x405E,
    0x005A, 0xC19A, 0x819B, 0x405B, 0x0199, 0xC059, 0x8058, 0x4198,
    0x0188, 0xC048, 0x8049, 0x4189, 0x004B, 0xC18B, 0x818A, 0x404A,
    0x004E, 0xC18E, 0x818F, 0x404F, 0x018D, 0xC04D, 0x804C, 0x418C,
    0x0044, 0xC184, 0x8185, 0x4045, 0x0187, 0xC047, 0x8046, 0x4186,
    0x0182, 0xC042, 0x8043, 0x4183, 0x0041, 0xC181, 0x8180, 0x4040
};



uint8_t HEX2BCD( uint8_t x )
{
    return ( ( ( x ) / 10 ) << 4 | ( ( x ) % 10 ) );
}


uint8_t BCD2HEX( uint8_t x )
{
    return ( ( ( ( x ) >> 4 ) * 10 ) + ( ( x ) & 0x0f ) );
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
__inline MYTIME buf_to_mytime( uint8_t *p )
{
    uint32_t ret;
    ret = (uint32_t)( ( *p++ ) << 26 );
    ret |= (uint32_t)( ( *p++ ) << 22 );
    ret |= (uint32_t)( ( *p++ ) << 17 );
    ret |= (uint32_t)( ( *p++ ) << 12 );
    ret |= (uint32_t)( ( *p++ ) << 6 );
    ret |= ( *p );
    return ret;
}

/*
   ��buf�л�ȡʱ����Ϣ
   �����0xFF ��ɵ�!!!!!!!����Դ�

 */
MYTIME mytime_from_hex( uint8_t *buf )
{
    MYTIME	ret = 0;
    uint8_t *p	= buf;
    if( *p == 0xFF ) /*������Ч������*/
    {
        return 0xFFFFFFFF;
    }
    ret = (uint32_t)( ( *p++ ) << 26 );
    ret |= (uint32_t)( ( *p++ ) << 22 );
    ret |= (uint32_t)( ( *p++ ) << 17 );
    ret |= (uint32_t)( ( *p++ ) << 12 );
    ret |= (uint32_t)( ( *p++ ) << 6 );
    ret |= ( *p );
    return ret;
}

/*ת��bcdʱ��*/
MYTIME mytime_from_bcd( uint8_t *buf )
{
    uint32_t year, month, day, hour, minute, sec;
    uint8_t *psrc = buf;
    year	= BCD2HEX( *psrc++ );
    month	= BCD2HEX( *psrc++ );
    day		= BCD2HEX( *psrc++ );
    hour	= BCD2HEX( *psrc++ );
    minute	= BCD2HEX( *psrc++ );
    sec		= BCD2HEX( *psrc );
    return MYDATETIME( year, month, day, hour, minute, sec );
}

/*ת��Ϊʮ�����Ƶ�ʱ�� ���� 2013/07/18 => 0x0d 0x07 0x12*/
void mytime_to_hex( uint8_t *buf, MYTIME time )
{
    uint8_t *psrc = buf;
    *psrc++ = YEAR( time );
    *psrc++ = MONTH( time );
    *psrc++ = DAY( time );
    *psrc++ = HOUR( time );
    *psrc++ = MINUTE( time );
    *psrc	= SEC( time );
}

/*ת��Ϊbcd�ַ���Ϊ�Զ���ʱ�� ���� 0x13 0x07 0x12=>���� 13��7��12��*/
void mytime_to_bcd( uint8_t *buf, MYTIME time )
{
    uint8_t *psrc = buf;
    *psrc++ = HEX2BCD( YEAR( time ) );
    *psrc++ = HEX2BCD( MONTH( time ) );
    *psrc++ = HEX2BCD( DAY( time ) );
    *psrc++ = HEX2BCD( HOUR( time ) );
    *psrc++ = HEX2BCD( MINUTE( time ) );
    *psrc	= HEX2BCD( SEC( time ) );
}



/*********************************************************************************
  *��������:unsigned long mytime_to_utc(MYTIME	time)
  *��������:����ʽΪMYTIME��ʱ��ת��ΪUTCʱ��
  *��	��:	time	:MYTIMEʱ��
  *��	��:	none
  *�� �� ֵ:unsigned long����ʾ�����UTCʱ��
  *��	��:������
  *��������:2013-12-18
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
unsigned long mytime_to_utc(MYTIME	time)
{
    unsigned long	utc;
    unsigned int 	year;
    unsigned int 	month;
    unsigned int 	day;
    unsigned int 	hour;
    unsigned int 	minute;
    unsigned int 	sec;

    year	= YEAR(time) + 2000;
    month	= MONTH(time);
    day		= DAY(time);
    hour	= HOUR(time);
    minute	= MINUTE(time);
    sec		= SEC(time);

    if( 0 >= (int)( month -= 2 ) )    /**//* 1..12 -> 11,12,1..10 */
    {
        month	+= 12;              /**//* Puts Feb last since it has leap day */
        year	-= 1;
    }
    utc = ( ( ( (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * month / 12 + day ) +
                year * 365 - 719499
              ) * 24 + hour		/**//* now have hours */
            ) * 60 + minute		   /**//* now have minutes */
          ) * 60 + sec;			/**//* finally seconds */
    return utc;
}


/*********************************************************************************
  *��������:MYTIME utc_to_mytime(unsigned long utc)
  *��������:��utcʱ��ת��ΪMYTIMEʱ���ʽ
  *��	��:	utc	:UTCʱ��
  *��	��:	none
  *�� �� ֵ:MYTIMEʱ���ʽ��ʱ��
  *��	��:������
  *��������:2013-12-18
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
MYTIME utc_to_mytime(unsigned long utc)
{
    int i, leapyear, day2;
    uint32_t day1;
    unsigned int 	year;
    unsigned int 	month;
    unsigned int 	day;
    unsigned int 	hour;
    unsigned int 	minute;
    unsigned int 	sec;
    MYTIME 			time;

    ///10957   10988

    sec		= utc % 60;
    minute	= utc % 3600 / 60;
    hour	= utc % 86400 / 3600;
    utc		/= 86400;
    if(utc < 10957)
        utc	= 10967;
    year	= 2000 + (utc - 10957) / 1461 * 4;		///10957��ʾΪ2000��1��1�յ�UTC������1461��ʾ4��Ϊ��λ����������Ϊ��2000�꿪ʼ���㣬���Ե�һ��Ϊ366�죬����3��Ϊ365��
    day1 = (utc - 10957) % 1461;
    if(day1 >= 366)
    {
        year++;
        day1 -= 366;
        year	+= day1 / 365;

        day1	%= 365;
        leapyear	= 0;
    }
    else
    {
        leapyear	= 1;
    }
    day2 = 0;
    for(i = 1; i <= 12; i++)
    {
        day = Get_Month_Day(i, leapyear);
        day2 += day;
        //�����ǰ�µ�������С�ڼ���������õ����·�
        if(day2 > day1)
        {
            day2 -= day;
            break;
        }
    }
    month = i;
    day = day1 - day2 + 1;
    time	= MYDATETIME(year - 2000, month, day, hour, minute, sec);
    return time;
}

void time_test(char *str)
{
    uint8_t i;
    uint8_t time[6];
    unsigned int 	year;
    unsigned int 	month;
    unsigned int 	day;
    unsigned int 	hour;
    unsigned int 	minute;
    unsigned int 	sec;
    uint32_t		utc;
    MYTIME 		my_time;

    for(i = 0; i < 12; i += 2)
    {
        time[i / 2] = (str[i] - '0') * 10;
        time[i / 2] += str[i + 1] - '0';
    }
    rt_kprintf("\n time=%02d-%02d-%02d %02d:%02d:%02d", time[0], time[1], time[2], time[3], time[4], time[5]);

    my_time	= MYDATETIME(time[0], time[1], time[2], time[3], time[4], time[5]);
    rt_kprintf("\n my_time1=%u", my_time);
    utc = mytime_to_utc(my_time);
    my_time = utc_to_mytime(utc);
    rt_kprintf("\n my_time2=%u", my_time);
    rt_kprintf("\n utc_time=%u\n", utc);
}
//FINSH_FUNCTION_EXPORT( time_test, time_test );


void time_test_2(uint32_t value)
{
    uint32_t i;
    uint8_t time[6];
    unsigned int 	year = 99;
    MYTIME 		my_time1, my_time2;
    uint32_t 		utc_time = 978307201;
    uint32_t 		utc;

    //for(i=0;i<36500;i++)
    while(utc_time < 0xFFFF0000)
    {
        my_time1 = utc_to_mytime(utc_time);
        utc = mytime_to_utc(my_time1);
        my_time2 = utc_to_mytime(utc);
        if(my_time1 != my_time2)
        {
            time[0]	= YEAR(my_time1);
            time[1]	= MONTH(my_time1);
            time[2]	= DAY(my_time1);
            time[3]	= HOUR(my_time1);
            time[4]	= MINUTE(my_time1);
            time[5]	= SEC(my_time1);
            rt_kprintf("\n my_time_error=%02d-%02d-%02d %02d:%02d:%02d", time[0], time[1], time[2], time[3], time[4], time[5]);
            return;
        }
        time[0]	= YEAR(my_time1);
        if(time[0] != year)
        {
            year	= time[0];
            time[0]	= YEAR(my_time1);
            time[1]	= MONTH(my_time1);
            time[2]	= DAY(my_time1);
            time[3]	= HOUR(my_time1);
            time[4]	= MINUTE(my_time1);
            time[5]	= SEC(my_time1);
            rt_kprintf("\n my_time=%02d-%02d-%02d %02d:%02d:%02d", time[0], time[1], time[2], time[3], time[4], time[5]);
        }
        utc_time += value;
    }
    time[0]	= YEAR(my_time1);
    time[1]	= MONTH(my_time1);
    time[2]	= DAY(my_time1);
    time[3]	= HOUR(my_time1);
    time[4]	= MINUTE(my_time1);
    time[5]	= SEC(my_time1);
    rt_kprintf("\n my_time_end=%02d-%02d-%02d %02d:%02d:%02d", time[0], time[1], time[2], time[3], time[4], time[5]);

    rt_kprintf("\n !!!!time_test_2_OK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    beep(5, 5, 5);
}
//FINSH_FUNCTION_EXPORT( time_test_2, time_test TIME  );


/*********************************************************************************
  *��������:uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width )
  *��������:����ͬ���͵����ݴ���buf�У�������buf��Ϊ���ģʽ
  *��	��:	pdest:  ������ݵ�buffer
   data:	������ݵ�ԭʼ����
   width:	��ŵ�ԭʼ����ռ�õ�buf�ֽ���
  *��	��:
  *�� �� ֵ:������ֽ���
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint16_t data_to_buf( uint8_t *pdest, uint32_t data, uint8_t width )
{
    uint8_t *buf;
    buf = pdest;

    switch( width )
    {
    case 1:
        *buf++ = data & 0xff;
        break;
    case 2:
        *buf++	= data >> 8;
        *buf++	= data & 0xff;
        break;
    case 4:
        *buf++	= data >> 24;
        *buf++	= data >> 16;
        *buf++	= data >> 8;
        *buf++	= data & 0xff;
        break;
    }
    return width;
}

/*********************************************************************************
  *��������:uint16_t buf_to_data( uint8_t * psrc, uint8_t width )
  *��������:����ͬ���͵����ݴ�buf��ȡ������������buf��Ϊ���ģʽ
  *��	��:	psrc:   ������ݵ�buffer
   width:	��ŵ�ԭʼ����ռ�õ�buf�ֽ���
  *��	��:	 none
  *�� �� ֵ:uint32_t ���ش洢������
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint32_t buf_to_data( uint8_t *psrc, uint8_t width )
{
    uint8_t		i;
    uint32_t	outData = 0;

    for( i = 0; i < width; i++ )
    {
        outData <<= 8;
        outData += *psrc++;
    }
    return outData;
}

/*********************************************************************************
  *��������:unsigned int CRC16_ModBusEx(unsigned char *ptr, unsigned int len,unsigned int crc)
  *��������:�����ݽ���CRCУ�飬
  *��	��:	ptr	:����
   len	:���ݳ���
   crc	:��ʼCRCֵ��ע����һ��ʱΪOxFFFF;
  *��	��:	none
  *�� �� ֵ:unsigned int CRCУ����
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
unsigned short CalcCRC16( unsigned char *ptr, int pos, unsigned int len, unsigned short crc )
{
    unsigned char	da;
    unsigned char	*p = ptr + pos;
    while( len-- != 0 )
    {
        da	= (unsigned char)( crc / 256 ); /* �� 8 λ������������ʽ�ݴ�CRC �ĸ�8 λ */
        crc <<= 8;                          /* ���� 8 λ���൱��CRC �ĵ�8 λ����28 */
        crc ^= tbl_crc[da ^ *p];            /* �� 8 λ�͵�ǰ�ֽ���Ӻ��ٲ����CRC ���ټ�����ǰ��CRC */
        p++;
    }
    return ( crc );
}

/*********************************************************************************
  *��������:uint8_t BkpSram_write(uint32_t addr,uint8_t *data, uint16_t len)
  *��������:backup sram ����д��
  *��	��:	addr	:д��ĵ�ַ
   data	:д�������ָ��
   len		:д��ĳ���
  *��	��:	none
  *�� �� ֵ:uint8_t	:	1:��ʾ����ʧ�ܣ�	0:��ʾ�����ɹ�
  *��	��:������
  *��������:2013-06-18
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t bkpsram_write( uint32_t addr, uint8_t *data, uint16_t len )
{
    uint32_t i;
    //addr &= 0xFFFC;
    //*(__IO uint32_t *) (BKPSRAM_BASE + addr) = data;
    for( i = 0; i < len; i++ )
    {
        if( addr < 0x1000 )
        {
            *(__IO uint8_t *)( BKPSRAM_BASE + addr ) = *data++;
        }
        else
        {
            return 1;
        }
        ++addr;
    }
    return 0;
}

/*********************************************************************************
  *��������:uint16_t bkpSram_read(uint32_t addr,uint8_t *data, uint16_t len)
  *��������:backup sram ���ݶ�ȡ
  *��	��:	addr	:��ȡ�ĵ�ַ
   data	:��ȡ������ָ��
   len		:��ȡ�ĳ���
  *��	��:	none
  *�� �� ֵ:uint16_t	:��ʾʵ�ʶ�ȡ�ĳ���
  *��	��:������
  *��������:2013-06-18
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint16_t bkpsram_read( uint32_t addr, uint8_t *data, uint16_t len )
{
    uint32_t i;
    //addr &= 0xFFFC;
    //data = *(__IO uint32_t *) (BKPSRAM_BASE + addr);
    for( i = 0; i < len; i++ )
    {
        if( addr < 0x1000 )
        {
            *data++ = *(__IO uint8_t *)( BKPSRAM_BASE + addr );
        }
        else
        {
            break;
        }
        ++addr;
    }
    return i;
}

/*��ʼ��bkp sram �ɹ�����0��ʧ�ܻ�1*/
uint8_t bkpsram_init( void )
{
    __IO uint32_t	tmp;
    uint8_t			buf[8];

    RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );
    PWR_BackupAccessCmd( ENABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_BKPSRAM, ENABLE );
    PWR_BackupRegulatorCmd( ENABLE );
    while( PWR_GetFlagStatus( PWR_FLAG_BRR ) == RESET )
    {
        tmp++;
        if( tmp > 0xFFFFFF )
        {
            debug_write("BKP_SRAM����");
            break;
        }
    }
    /*����־,�Ƿ���ĩ�����*/
    tmp = param_load_bksram();
    if( tmp == 0 )
    {
        debug_write("BKP�洢OK!");
        return 0;
    }
    else
    {
        debug_write("BKP_SRAM������ʧ");
        return 2;	/*������ʧ*/
    }
}

#define BKPSRAM_TEST
#ifdef BKPSRAM_TEST
/**/
void bkpsram_wr( uint32_t addr, char *psrc )
{
    char pstr[128];
    memset( pstr, 0, sizeof( pstr ) );
    memcpy( pstr, psrc, strlen( psrc ) );
    bkpsram_write( addr, (uint8_t *)pstr, strlen( pstr ) + 1 );
}

FINSH_FUNCTION_EXPORT( bkpsram_wr, write from backup sram );

/**/
void bkpsram_rd( uint32_t addr, uint16_t count )
{
    uint16_t i;
    for( i = 0; i < count; i++ )
    {
        if(i % 16 == 0) rt_kprintf("\n");
        rt_kprintf("%02x ", *(__IO uint8_t *)( BKPSRAM_BASE + addr + i ));
    }
    rt_kprintf("\n");


}

FINSH_FUNCTION_EXPORT( bkpsram_rd, read from backup sram );
#endif

/*log����*/
void trace_log(uint8_t model, uint8_t level, char *fmt, ...)
{
    va_list args;
    rt_size_t length;
    static char log_buf[4096];

    va_start(args, fmt);
    length = vsnprintf(log_buf, sizeof(log_buf), fmt, args);
    rt_hw_console_output(log_buf);

    va_end(args);


}



/************************************** The End Of File **************************************/
