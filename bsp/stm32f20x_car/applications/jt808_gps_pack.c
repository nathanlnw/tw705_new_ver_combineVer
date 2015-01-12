/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// ÎÄ¼şÃû
 * Author:			// ×÷Õß
 * Date:			// ÈÕÆÚ
 * Description:		// Ä£¿éÃèÊö
 * Version:			// °æ±¾ĞÅÏ¢
 * Function List:	// Ö÷Òªº¯Êı¼°Æä¹¦ÄÜ
 *     1. -------
 * History:			// ÀúÊ·ĞŞ¸Ä¼ÇÂ¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include "jt808_gps.h"
#include <string.h>
#include "jt808_config.h"
#include <finsh.h>


#define PACK_SECT_SIZE			256
#define PACK_HEAD				0xABCDAAA0


#define PACK_NONET_NOREPORT 0xFF    /*Î´µÇÍø Î´ÉÏ±¨ */
#define PACK_REPORTED_OK	0x7F    /*ÒÑÉÏ±¨ OK		*/

//#define ADDR_DF_GPS_PACK_END		0x37E000				///gpsÔ­Ê¼Ñ¹ËõÊı¾İ´æ´¢ÇøÓò½áÊøÎ»ÖÃ
//#define ADDR_DF_GPS_PACK_SECT		10						///ÉÈÇø×ÜÊıÁ¿




#if 0
/*
   Ã¿4kĞ´ÈëÒ»´ÎdataFlash
   ÉÈÇøÍ·°üÀ¨  uint32_t id;		µİÔöĞòºÅ
 */
static uint8_t	pack_buf[4096];
static uint16_t pack_buf_pos;   /*Ğ´ÈëµÄÆ«ÒÆ*/
static uint32_t pack_buf_id;    /*id*/
static uint8_t	sect_index;


/*Ñ¹ËõgpsĞÅÏ¢
   ÎåÖÖÖÖÓï¾ä
   GGA
   GSA
   GSV
   RMC
   GLL


   ÈıÖÖÀàĞÍ
   GN
   BD
   GP

   TXTÓï¾äµ¥¶À´¦Àí
   $GNTXT,

 */

#define MN 0xbadbad00

static char *start[15] =
{
    "$GNGSV,",
    "$GNGSA,",
    "$GNGGA,",
    "$GNRMC,",
    "$GNGLL,",
    "$GPGSV,",
    "$GPGSA,",
    "$GPGGA,",
    "$GPRMC,",
    "$GPGLL,",
    "$BDGSV,",
    "$BDGSA,",
    "$BDGGA,",
    "$BDRMC,",
    "$BDGLL,",
};


/*
   static char* gntxt	= "$GNTXT,01,01,01,ANTENNA OK*2B";
   static char * gptxt = "$GPTXT,01,01,01,ANTENNA OK*35";
   static char * bdtxt = "$BDTXT,01,01,01,ANTENNA OK*24";
 */

static char *txt[] =
{
    { "$GNTXT,01,01,01,ANTENNA OK*2B"	 },
    { "$GPTXT,01,01,01,ANTENNA OK*35"	 },
    { "$BDTXT,01,01,01,ANTENNA OK*24"	 },
    { "$GNTXT,01,01,01,ANTENNA OPEN*3B"	 },
    { "$GPTXT,01,01,01,ANTENNA OPEN*25"	 },
    { "$BDTXT,01,01,01,ANTENNA OPEN*34"	 },
    { "$GNTXT,01,01,01,ANTENNA SHORT*7D" },
    { "$GPTXT,01,01,01,ANTENNA SHORT*63" },
    { "$BDTXT,01,01,01,ANTENNA SHORT*72" },
};

/*»¹Ô­Ñ¹ËõµÄÊı¾İ£¬²¢Ôö¼ÓĞ£Ñé*/
uint8_t jt808_gps_unpack( uint8_t *pinfo, char *pout )
{
    uint8_t			*psrc = pinfo;
    char			*pdst;
    uint8_t			count;
    char			buf[80];
    unsigned char	i, id, len, c[2];
    unsigned char	fcs;
    char			tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    id = psrc[3];

    if( id > 0x0F )
    {
        strcpy( pout, txt[*psrc - 0x10] );
        return 29;
    }

    memcpy( buf, start[id], 7 );

    len = psrc[4];

    psrc	= pinfo + 5;
    pdst	= buf + 7;
    count	= 0;

    while( count <= len )
    {
        c[0]	= *psrc >> 4;
        c[1]	= *psrc & 0xf;
        psrc++;
        for( i = 0; i < 2; i++ )
        {
            switch( c[i] )
            {
            case 0x0a:                  /* A »ò -*/
                if( ( id % 5 ) == 2 )   /*GGA*/
                {
                    *pdst++ = '-';
                }
                else  /*RMC GSA*/
                {
                    *pdst++ = 'A';
                }
                break;
            case 0x0b:
                if( ( id % 5 ) == 2 )   /*GGA*/
                {
                    *pdst++ = 'M';
                }
                else
                {
                    *pdst++ = 'V';      /*RMC GLL*/
                }
                break;
            case 0x0c:
                *pdst++ = 'N';
                break;
            case 0x0d:
                *pdst++ = '.';
                break;
            case 0x0e:
                *pdst++ = 'E';
                break;
            case 0x0f:
                *pdst++ = ',';
                break;
            default:
                *pdst++ = ( c[i] + '0' );
                break;
            }
            count++;
            if( count > len )
            {
                break;
            }
        }
    }

    fcs = 0;
    for( i = 1; i < ( count + 6 ); i++ )
    {
        fcs ^= buf[i];
    }
    buf[count + 6]	= '*';
    buf[count + 7]	= tbl[fcs >> 4];
    buf[count + 8]	= tbl[fcs & 0xF];
    buf[count + 9]	= 0;
#if 0
    rt_kprintf( "\ndecode>%s\n", buf );
#endif
    return count + 10;
}

/*±£´æ¼ÇÂ¼*/
static void save_rec( uint8_t *p, uint8_t len )
{
    if( ( 4096 - pack_buf_pos ) < len ) /*²»×ãÒÔ±£ÁôÕû¸öĞÅÏ¢,*/
    {
        rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY_2 );
        sst25_write_through(ADDR_DF_GPS_PACK_START + sect_index * 4096, pack_buf, 4096);
        sect_index++;
        sect_index %= ADDR_DF_GPS_PACK_SECT;
        sst25_erase_4k(ADDR_DF_GPS_PACK_START + sect_index * 4096);
        rt_sem_release( &sem_dataflash );
        memset(pack_buf, 0xFF, 4096);
        pack_buf_id++;
        pack_buf[0] = pack_buf_id >> 24;
        pack_buf[1] = pack_buf_id >> 16;
        pack_buf[2] = pack_buf_id >> 8;
        pack_buf[3] = pack_buf_id & 0xFF;
        pack_buf_pos = 4;
    }
    else
    {
        memcpy( pack_buf + pack_buf_pos, p, len );
        pack_buf_pos += len;
    }
}

/*Ñ¹ËõÊı¾İ*/
uint8_t jt808_gps_pack( char *pinfo, uint8_t len )
{
    char		*p = pinfo;
    uint8_t		*pdst;
    uint8_t		buf[80];
    uint8_t		count;
    uint8_t		i, j, c;
    uint32_t	id;
    i = 0;
    if( p[5] == 'T' )                       /*TXTµ¥¶À´¦Àí*/
    {
        switch( p[2] )
        {
        case 'N':
            if( p[25] == 'K' )          /*OK*/
            {
                i = 0x10;
            }
            else if( p[25] == 'P' )     /*OPEN*/
            {
                i = 0x13;
            }
            else if( p[25] == 'H' )     /*SHORT*/
            {
                i = 0x16;
            }
            break;
        case 'P':

            if( p[25] == 'K' )          /*OK*/
            {
                i = 0x11;
            }
            else if( p[25] == 'P' )     /*OPEN*/
            {
                i = 0x14;
            }
            else if( p[25] == 'H' )     /*SHORT*/
            {
                i = 0x17;
            }
            break;
        case 'D':

            if( p[25] == 'K' )          /*OK*/
            {
                i = 0x12;
            }
            else if( p[25] == 'P' )     /*OPEN*/
            {
                i = 0x15;
            }
            else if( p[25] == 'H' )     /*SHORT*/
            {
                i = 0x18;
            }
            break;
        default:
            return 0;
        }
    }
    if( i )
    {
        memcpy( buf, "\xba\xdb\xad\x00\x00", 5 );
        buf[4] = i;
        save_rec( buf, 5 );
        return 4;
    }

    switch( p[2] )
    {
    case 'N':
        i = 0;
        break;
    case 'P':
        i = 1;
        break;
    case 'D':
        i = 2;
        break;
    default:
        rt_kprintf( "\nunknow>%s", pinfo ); /*unknow>ÿ$GNRMC,074127.90,V,,,,,,,200713,,,N*6A*/
        return 0;
    }
    switch( p[4] )
    {
    case 'S':
        if( p[5] == 'V' )           /* GSV*/
        {
            j = 0;
        }
        else if( p[5] == 'A' )      /* GSA*/
        {
            j = 1;
        }
        else
        {
            rt_kprintf( "\nunknow" );
            return 0;
        }
        break;
    case 'G':
        j = 2;
        break; /*GGA*/
    case 'M':
        j = 3;
        break; /*RMC*/
    case 'L':
        j = 4;
        break; /*GLL*/
    default:
        rt_kprintf( "\ngps_packÎ´Öª:%02x", p[4] );
        return 0;
    }
    id		= MN | ( i * 5 + j );
    buf[0]	= id >> 24;
    buf[1]	= id >> 16;
    buf[2]	= id >> 8;
    buf[3]	= id & 0xFF;

    pdst	= buf + 5;      /*buf+4 Áô×÷Êı¾İ³¤¶È*/
    p		= pinfo + 7;    /*Ìø¹ıÓï¾äÇ°µÄ,*/
    count	= 0;
    c		= 0;
    while( *p != '*' )
    {
        c <<= 4;
        if( ( *p >= '0' ) && ( *p <= '9' ) )
        {
            c |= ( *p - '0' );
        }
        else
        {
            switch( *p )
            {
            case 'A':
            case '-':
                c |= 0xa;
                break;
            case 'V':
            case 'M':
                c |= 0xb;
                break;
            case 'N':
                c |= 0xc;
                break;
            case '.':
                c |= 0xd;
                break;
            case 'E':
                c |= 0xe;
                break;
            case ',':
                c |= 0xf;
                break;
            default:
                rt_kprintf( "\ngps_packÎ´Öª[%02x]", *p );
                return 0;
            }
        }
        count++;
        if( ( count & 0x01 ) == 0 ) /*Ã¿Á½¸öºÏ²¢Ò»ÏÂ*/
        {
            *pdst++ = c;
            c		= 0;
        }
        p++;
    }

    if( count & 0x01 )  /*»¹Ê£Ò»¸ö*/
    {
        *pdst = ( c << 4 );
    }
    buf[4] = count;     /*Ô­Ê¼µÄÊı¾İ³¤¶È*/

#if 0
    j = ( count >> 1 ) + ( count & 0x01 );
    rt_kprintf( "Encode>" );
    for( i = 0; i < j + 5; i++ )
    {
        rt_kprintf( "%02x", buf[i] );
    }
    rt_kprintf( "\n" );

    {
        char buf_out[80];
        jt808_gps_unpack( buf, buf_out );
    }
#endif
    save_rec( buf, count + 5 );
    return count + 5;
}

/*
³õÊ¼»¯£¬
ÊÇÉÏµçºó¾Í¿ªÊ¼×ö
»¹ÊÇ½øÈëÒªÉÏ±¨Ô­Ê¼ĞÅÏ¢µÄÇøÓò²Å¿ªÊ¼
*/
void gps_pack_init( void )
{
    uint32_t	i;
    uint8_t		buf[12];
    uint32_t	addr, id;
    uint16_t	pos;

    pack_buf_id = 0;
    sect_index	= 0xFF;
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    for( i = 0; i < ADDR_DF_GPS_PACK_SECT; i++ )
    {
        addr = ADDR_DF_GPS_PACK_START + i * 4096;
        sst25_read( addr, buf, 4 );
        id = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
        if( id != 0xFFFFFFFF )
        {
            if( id > pack_buf_id )
            {
                sect_index	= i;
                pack_buf_id = id;
            }
        }
    }

    memset(pack_buf, 0xFF, sizeof(pack_buf));


    if( sect_index == 0xFF )            /*Ã»ÓĞÕÒµ½ÓĞÊı¾İµÄÉÈÇø*/
    {
        sect_index		= 0;            /**/
        pack_buf_pos	= 4;            /*¿Õ³ö¿ªÊ¼µÄËÄ¸ö×Ö½Ú±£´æid*/
        pack_buf[0] = 0;
        pack_buf[1] = 0;
        pack_buf[2] = 0;
        pack_buf[3] = 0;

    }
    else  /*ÒªÔÚÌØ¶¨µÄÉÈÇøÖĞ²éÕÒ£¬²¢²»ÊÇÌ«·½±ã,ÒòÎªÊÇ±ä³¤µÄ*/
    {
        /*¶ÁÈëµ½4kµÄram*/

        sst25_read( addr + sect_index * 4096, pack_buf, 4096 );
        pos = 4;
        while( pos < 4096 )
        {
            if( ( buf[pos] == 0xba ) && ( buf[pos + 1] == 0xdb ) && ( buf[pos + 2] == 0xad ) )
            {
                pos += ( buf[pos + 4] + 5 );
            }
            else
            {
                break;
            }
        }
        if( pos > 4095 ) /*³ö´íÁË? why?*/
        {
            rt_kprintf( "\ngps_pack_init fault" );
            for( i = 0; i < ADDR_DF_GPS_PACK_SECT; i++ )
            {
                sst25_erase_4k( ADDR_DF_GPS_PACK_START + i * 4096 );
            }
            pack_buf_id		= 0;
            pack_buf_pos	= 4;
            sect_index		= 0;
        }
        else
        {
            pack_buf_pos = pos;
        }
    }
    rt_sem_release( &sem_dataflash );
    rt_kprintf( "\nsect=%d id=%d pos=%d", sect_index, pack_buf_id, pack_buf_pos );
}
#endif

typedef __packed struct
{
    uint32_t	mn;
    uint32_t	id;
    uint8_t		attr;
    uint8_t		unused;
    uint16_t	len;
    //uint32_t	msg_id;		/*ÉÏ±¨µÄÏûÏ¢Í·0x0200*/
} GPS_PACK_HEAD;

typedef struct
{
    uint32_t	addr;
    uint32_t	id;
    uint8_t		attr;
    uint16_t	len;
    uint16_t	pack_count;		//»¹Ã»ÓĞÉÏ±¨µÄ¼ÇÂ¼Êı
    uint8_t		get_count;    	//»ñÈ¡µÄÎ´ÉÏ±¨¼ÇÂ¼Êı
    uint32_t 	get_addr;		//»ñÈ¡µÄÎ´ÉÏ±¨¼ÇÂ¼×îĞ¡idµÄ´æ´¢µØÖ·
    uint32_t	pack_addr[4];	//ÉÏ±¨¶à¸ö¼ÇÂ¼µÄµØÖ·
} GPS_PACK_INFO;

GPS_PACK_INFO		gps_pack_curr = { 0, 0, 0, 0, 0, 0 };

//static uint32_t report_addr[20];            /*ÉÏ±¨¼ÇÂ¼µÄµØÖ·*/
//static uint8_t	report_get_count	= 0;    /*»ñÈ¡µÄÎ´ÉÏ±¨¼ÇÂ¼Êı*/
//static uint32_t report_get_addr		= 0;    /*»ñÈ¡µÄÎ´ÉÏ±¨¼ÇÂ¼×îĞ¡idµÄµØÖ·*/

//static uint16_t report_count = 0;           /*»¹Ã»ÓĞÉÏ±¨µÄ¼ÇÂ¼Êı*/


/*
   ÉÏ±¨ĞÅÏ¢ÊÕµ½ÏìÓ¦,Í¨ÓÃÓ¦´ğ 0x8001
 */
static JT808_MSG_STATE gps_pack_response( JT808_TX_NODEDATA *nodedata, uint8_t *pmsg )
{
    uint8_t			 *msg = pmsg + 12;
    uint16_t		ack_id;
    uint16_t		ack_seq;
    uint8_t			ack_res;
    uint32_t		addr;
    GPS_PACK_HEAD head;
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
        if( ack_id == 0x0900 )              /*ÅúÁ¿ÉÏ±¨*/
        {
            for( i = 0; i < gps_pack_curr.get_count; i++ )
            {
                sst25_read( gps_pack_curr.pack_addr[i], (uint8_t *)&head, sizeof( GPS_PACK_HEAD ) );
                if(( head.mn == PACK_HEAD) && (head.attr == PACK_NONET_NOREPORT))
                {

                    head.attr = PACK_REPORTED_OK;      /*×î¸ßÎ»ÖÃ0*/
                    //rt_kprintf("\n DEL_gps_pack:addr=0x%08X,id=%d",gps_pack_curr.pack_addr[i],head.id);
                    sst25_write_through( gps_pack_curr.pack_addr[i], (uint8_t *)&head, sizeof( GPS_PACK_HEAD ) );
                    gps_pack_curr.get_addr = gps_pack_curr.pack_addr[i];
                    if( gps_pack_curr.pack_count )
                    {
                        gps_pack_curr.pack_count--;
                    }
                }
            }
        }
        rt_sem_release( &sem_dataflash );
    }

    return ACK_OK;
}

/*********************************************************************************
  *º¯ÊıÃû³Æ:void gps_pack_init( void )
  *¹¦ÄÜÃèÊö:²éÕÒ×îºóÒ»¸öĞ´ÈëµÄÎ»ÖÃ,  ²éÕÒÎ´ÉÏ±¨¼ÇÂ¼×îĞ¡idµÄÎ»ÖÃ,
  			³õÊ¼»¯Êı¾İ´æ´¢Ïà¹Ø²ÎÊı
  			×¢Òâ:¸Ãº¯ÊıÖ»ÄÜÓÃÀ´³õÊ¼»¯´æ´¢ÇøÓò£¬²»¿ÉÔö¼ÓÖ»ÄÜ³õÊ¼»¯Ò»´ÎµÄ´úÂë£¬²»Èç²»ÄÜ³õÊ¼»¯ĞÅºÅÁ¿
  			²»ÄÜ³õÊ¼»¯Ïß³ÌµÈ
  *Êä	Èë:none
  *Êä	³ö:none
  *·µ »Ø Öµ:none
  *×÷	Õß:°×ÑøÃñ
  *´´½¨ÈÕÆÚ:2014-03-5
  *---------------------------------------------------------------------------------
  *ĞŞ ¸Ä ÈË:
  *ĞŞ¸ÄÈÕÆÚ:
  *ĞŞ¸ÄÃèÊö:
*********************************************************************************/
void gps_pack_init( void )
{
    uint32_t		addr;
    GPS_PACK_HEAD head;
    uint32_t		read_id = 0xFFFFFFFF;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    gps_pack_curr.pack_count		= 0; /*ÖØĞÂ²éÕÒ*/
    gps_pack_curr.addr	= 0;
    gps_pack_curr.id		= 0;
    gps_pack_curr.len		= 0;

    memset( gps_pack_curr.pack_addr, 0, sizeof( gps_pack_curr.pack_addr ) );

    for( addr = ADDR_DF_GPS_PACK_START; addr < ADDR_DF_GPS_PACK_END; addr += PACK_SECT_SIZE )
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( GPS_PACK_HEAD ) );
        if( head.mn == PACK_HEAD )         /*ÓĞĞ§¼ÇÂ¼*/
        {
            if( head.id >= gps_pack_curr.id ) /*²éÕÒ×î½ü¼ÇÂ¼,µ±Ç°Ğ´ÈëµÄÎ»ÖÃ*/
            {
                gps_pack_curr.id		= head.id;
                gps_pack_curr.addr		= addr; /*µ±Ç°¼ÇÂ¼µÄµØÖ·*/
                gps_pack_curr.len		= head.len;
            }
            if( head.attr == PACK_NONET_NOREPORT )          /*bit7=1 »¹Ã»ÓĞ·¢ËÍ*/
            {
                gps_pack_curr.pack_count++;             /*Í³¼ÆÎ´ÉÏ±¨¼ÇÂ¼Êı*/
                if( read_id > head.id )     /*ÒªÕÒµ½×îĞ¡idµÄµØÖ·*/
                {
                    gps_pack_curr.get_addr = addr;
                    read_id			= head.id;
                }
            }
        }
    }
    if(gps_pack_curr.pack_count == 0)
    {
        gps_pack_curr.get_addr =  gps_pack_curr.addr;
    }
    rt_sem_release( &sem_dataflash );
    rt_kprintf( "\n%d>(%d)GPS_PACKĞ´ÈëÎ»ÖÃ:%x id=%d ¶Á³öÎ»ÖÃ:%x id=%d",
                rt_tick_get( ),
                gps_pack_curr.pack_count,
                gps_pack_curr.addr,
                gps_pack_curr.id,
                gps_pack_curr.get_addr,
                read_id );
}

/*±£´æÔ­Ê¼¶¨Î»Êı¾İ*/
void gps_pack_put( uint8_t *pinfo, uint16_t len )
{
    GPS_PACK_HEAD head;
    uint32_t *p_addr;
    uint32_t  last_addr;

    if( gps_pack_curr.addr )                  /*µ±ÏÈµÄÉÏ±¨µØÖ·*/
    {
        last_addr			= gps_pack_curr.addr;
        gps_pack_curr.addr	+= ( sizeof( GPS_PACK_HEAD ) + gps_pack_curr.len + PACK_SECT_SIZE - 1 );	//¼Ó63ÊÇÎªÁËÈ¡Ò»¸ö±È½Ï´óµÄÖµ
        //gps_pack_curr.addr	&= 0xFFFFFFC0;  /*Ö¸ÏòÏÂÒ»¸öµØÖ·*/
        gps_pack_curr.addr	= gps_pack_curr.addr / PACK_SECT_SIZE * PACK_SECT_SIZE;  /*Ö¸ÏòÏÂÒ»¸öµØÖ·*/
        //Ä¿µÄÊÇµ±Ğ´ÈëµÄÎ»ÖÃ³¬¹ıÁËÒª¶ÁÈ¡µÄÎ»ÖÃÊÇ½«¶ÁÈ¡µÄÎ»ÖÃºóÒÆÒ»¸öÊı¾İÎ»
        if(( gps_pack_curr.get_addr > last_addr) && (gps_pack_curr.get_addr <= gps_pack_curr.addr) && ( gps_pack_curr.pack_count ))
        {
            gps_pack_curr.get_addr	= gps_pack_curr.addr + PACK_SECT_SIZE;
            gps_pack_curr.pack_count--;
            if( gps_pack_curr.get_addr >= ADDR_DF_GPS_PACK_END )
            {
                gps_pack_curr.get_addr = gps_pack_curr.get_addr - ADDR_DF_GPS_PACK_END + ADDR_DF_GPS_PACK_START;
            }
        }
    }
    else                                    /*»¹Ã»ÓĞ¼ÇÂ¼*/
    {
        gps_pack_curr.addr = ADDR_DF_GPS_PACK_START;
    }

    if( gps_pack_curr.addr >= ADDR_DF_GPS_PACK_END )
    {
        gps_pack_curr.addr = gps_pack_curr.addr - ADDR_DF_GPS_PACK_END + ADDR_DF_GPS_PACK_START;
        //Ä¿µÄÊÇµ±Ğ´ÈëµÄÎ»ÖÃ³¬¹ıÁËÒª¶ÁÈ¡µÄÎ»ÖÃÊÇ½«¶ÁÈ¡µÄÎ»ÖÃºóÒÆÒ»¸öÊı¾İÎ»
        if(( gps_pack_curr.get_addr <= gps_pack_curr.addr) && (gps_pack_curr.pack_count))
        {
            gps_pack_curr.get_addr	= gps_pack_curr.addr + PACK_SECT_SIZE;
            gps_pack_curr.pack_count--;
        }
    }
    /*
    if( ( gps_pack_curr.addr & 0xFFF ) == 0 )
    {
    	sst25_erase_4k( gps_pack_curr.addr );
    }
    */

    gps_pack_curr.id++;
    gps_pack_curr.len = len;

    head.mn		= PACK_HEAD;
    head.id		= gps_pack_curr.id;
    head.len	= len;
    head.attr 	= 0xFF;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    //sst25_write_through( gps_pack_curr.addr, (uint8_t*)&head, sizeof( GPS_PACK_HEAD ) );
    //sst25_write_through( gps_pack_curr.addr + sizeof( GPS_PACK_HEAD ), pinfo, len );
    sst25_write_auto( gps_pack_curr.addr, (uint8_t *)&head, sizeof( GPS_PACK_HEAD ) , ADDR_DF_GPS_PACK_START, ADDR_DF_GPS_PACK_END);
    sst25_write_auto( gps_pack_curr.addr + sizeof( GPS_PACK_HEAD ), pinfo, len , ADDR_DF_GPS_PACK_START, ADDR_DF_GPS_PACK_END);
    rt_sem_release( &sem_dataflash );

    gps_pack_curr.pack_count++;                     /*Ôö¼ÓÎ´ÉÏ±¨¼ÇÂ¼Êı*/
    if( gps_pack_curr.pack_count == 1 )
    {
        gps_pack_curr.get_addr = gps_pack_curr.addr;
    }
    rt_kprintf("\n PUT_gps_pack:get_addr=0x%08X,put_addr=0x%08X,id=%05d,count=%05d", gps_pack_curr.get_addr, gps_pack_curr.addr, head.id, gps_pack_curr.pack_count);

}
/*±£´æÔ­Ê¼¶¨Î»Êı¾İ*/
void gps_pack_proc( uint8_t *pinfo, uint16_t len )
{
    static uint8_t data_buf[1000];
    static uint16_t data_len = 0;
    if(!area_save_gps_pack)
    {
        data_len = 0;
        return;
    }
    if((gps_pack_curr.id == 0) && ((jt808_status & BIT_STATUS_FIXED) == 0))
    {
        return;
    }
    if( strncmp( pinfo + 3, "GGA,", 4 ) == 0 )
    {
        if(data_len)
        {
            gps_pack_put(data_buf, data_len);
            data_len = 0;
        }
    }
    if(data_len + len > sizeof(data_buf))
    {
        gps_pack_put(data_buf, data_len);
        data_len = 0;
    }
    memcpy(data_buf + data_len, pinfo, len);
    data_len += len;
}

/*
   »ñÈ¡Ô­Ê¼¶¨Î»Êı¾İ

   Ê¹ÓÃ0900ÉÏ±¨
 */
uint8_t jt808_packt_get( void )
{
    GPS_PACK_HEAD 	head;
    uint8_t			buf[1024];              /*µ¥°üÉÏ±¨µÄ×Ö½Ú²»´óÓÚ256*/
    uint32_t		i;
    uint32_t		addr;
    uint16_t		get_size;               /*»ñÈ¡¼ÇÂ¼µÄ´óĞ¡*/
    uint16_t		pos;
    uint8_t			read_count;             /*Òª¶ÁÈ¡µÄ¼ÇÂ¼Êı*/
    uint32_t 		*p_addr;

    if( gps_pack_curr.pack_count == 0 )                 /*Ã»ÓĞÓĞÎ´ÉÏ±¨¼ÇÂ¼*/
    {
        return 0;
    }

    //if(( mast_socket->state != CONNECTED )||(jt808_state != JT808_REPORT))  /*²»ÔÚÏß*/
    //{
    //	return 0;
    //}


    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    /*ÅúÁ¿²éÕÒ¼ÇÂ¼*/
    gps_pack_curr.get_count	= 0;
    addr				= gps_pack_curr.get_addr;
    get_size			= 3;
    pos					= 1;                            /*0x0704Êı¾İ°ü£¬¿ªÊ¼µÄÆ«ÒÆ*/
    read_count			= gps_pack_curr.pack_count;                 /*»¹Ã»ÓĞ±¨µÄ¼ÇÂ¼×ÜÊı*/
    if( gps_pack_curr.pack_count > sizeof(gps_pack_curr.pack_addr) / sizeof(uint32_t) )                           /*×î¶à1´ÎÉÏ±¨10Ìõ¼ÇÂ¼ 303×Ö½Ú*/
    {
        read_count = sizeof(gps_pack_curr.pack_addr) / sizeof(uint32_t);
    }
    read_count = 1;		///»ñÈ¡Ò»°üÊı¾İ£¬·ñÔòÆÁ±Î¸ÃĞĞ
    for( i = 0; i < ADDR_DF_GPS_PACK_SECT * ( 4096 / PACK_SECT_SIZE ); i++ )   /*Ã¿¼ÇÂ¼64×Ö½Ú,Ã¿sector(4096Byte)ÓĞ64¼ÇÂ¼*/
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( GPS_PACK_HEAD ) );
        if( head.mn == PACK_HEAD )                     /*ÓĞĞ§¼ÇÂ¼*/
        {
            if( head.attr == PACK_NONET_NOREPORT)      /*»¹Ã»ÓĞ·¢ËÍ*/
            {
                //rt_kprintf("\r\njt808_report_get addr=%X",addr);
                //rt_kprintf("   head=");
                //printf_hex_data((const u8 *)&head,sizeof(head));
                //ÅĞ¶ÏĞèÒª·¢ËÍµÄ×Ü³¤¶ÈÊÇ·ñ³¬¹ıµ¥°ü·¢ËÍµÄ×î´ó³¤¶È
                if( ( pos + head.len  ) >= JT808_PACKAGE_MAX )
                {
                    break;
                }
                //sst25_read( addr + sizeof(GPS_PACK_HEAD), buf + pos, head.len );
                sst25_read_auto( addr + sizeof(GPS_PACK_HEAD), buf + pos, head.len , ADDR_DF_GPS_PACK_START, ADDR_DF_GPS_PACK_END);
                pos								+=  head.len;
                gps_pack_curr.pack_addr[gps_pack_curr.get_count++]	= addr;

                //rt_kprintf("\n GET_gps_pack:addr=0x%08X,id=%d",addr,head.id);
                if( gps_pack_curr.get_count >= read_count ) /*ÊÕµ½Ö¸¶¨ÌõÊıÊı¾İ*/
                {
                    break;
                }
            }
            addr	+= ( sizeof(GPS_PACK_HEAD) + head.len + PACK_SECT_SIZE - 1 );	//¼Ó63ÊÇÎªÁËÈ¡Ò»¸ö±È½Ï´óµÄÖµ
            //addr	&= 0xFFFFFFC0;  			//Ö¸ÏòÏÂÒ»¸öµØÖ·
            addr	= addr / PACK_SECT_SIZE * PACK_SECT_SIZE;			//Ö¸ÏòÏÂÒ»¸öµØÖ·
        }
        else
        {
            addr += PACK_SECT_SIZE;
        }
        if( addr >= ADDR_DF_GPS_PACK_END )
        {
            addr = ADDR_DF_GPS_PACK_START;
        }
    }
    rt_sem_release( &sem_dataflash );

    if( gps_pack_curr.get_count ) /*µÃµ½µÄ¼ÇÂ¼Êı*/
    {
        buf[0] = 0x00;
        jt808_add_tx( mast_socket->linkno, SINGLE_CMD, 0x0900, -1, RT_NULL, gps_pack_response, pos, buf, RT_NULL);
        return 1;

    }
    else
    {
        gps_pack_curr.pack_count = 0; /*Ã»ÓĞµÃµ½ÒªÉÏ±¨µÄÊı¾İ*/
    }
    return 0;
    //rt_kprintf( "\n%d>(%d)¶Á³öÎ»ÖÃ:%x,¼ÇÂ¼Êı:%d,³¤¶È:%d", rt_tick_get( ), gps_pack_curr.pack_count, gps_pack_curr.get_addr, gps_pack_curr.get_count, pos );
}

void save_pack(uint8_t i)
{
    area_save_gps_pack = i;
}
FINSH_FUNCTION_EXPORT( save_pack, save_pack([1 | 0] ) );

/************************************** The End Of File **************************************/
