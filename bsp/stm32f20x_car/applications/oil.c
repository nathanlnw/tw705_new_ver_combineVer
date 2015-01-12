/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��:����Ϊ���ͺ����ݱ�����flash�У����������U�̲���Ļ�ͨ����ʾ�������ͺ����ݵ��뵽U
 										���У���Ҫע����ǣ��������ݱ����ڳ�̨�ӵ�30������������ֻ�в���JT808_param.id_0xF04B��Чʱ���ܵ�������
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
#include "ff.h"       /* FATFS */
#include "include.h"
#include "usbh_usr.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "jt808_param.h"
#include "math.h"
#include "jt808_config.h"
#include "rs232.h"
#include "Menu_Include.h"
#include "oil.h"

#define OIL_DATA_SAVE_SPACE			300				///�������ݴ洢�������λΪ�룬Ĭ��Ϊ5���ӣ���300��

#define OIL_PACK_SECT_SIZE			16
#define PACK_HEAD_OIL				0xABCDAAA2
#define OIL_DATA_LEN				8


#ifndef PACK_NONET_NOREPORT
#define PACK_NONET_NOREPORT 		0xFF    /*δ���� δ�ϱ� */
#define PACK_REPORTED_OK			0x7F    /*���ϱ� OK		*/
#endif


//#define ADDR_DF_OIL_PACK_END		0x37E000				///gpsԭʼѹ�����ݴ洢�������λ��
//#define ADDR_DF_OIL_PACK_SECT		10						///����������

typedef __packed struct
{
    uint32_t	mn;
    uint32_t	id;
    //uint32_t	msg_id;		/*�ϱ�����Ϣͷ0x0200*/
} OIL_PACK_HEAD;

typedef struct
{
    uint32_t	addr;
    uint32_t	id;
    uint8_t		attr;
    uint16_t	len;
    uint16_t	pack_count;		//��û���ϱ��ļ�¼��
    uint16_t	get_count;    	//��ȡ��δ�ϱ���¼��
    uint32_t 	get_addr;		//��ȡ��δ�ϱ���¼��Сid�Ĵ洢��ַ
    uint32_t	pack_addr[4];	//�ϱ������¼�ĵ�ַ
} OIL_PACK_INFO;


typedef struct
{
    uint32_t 	time;			//ʵʱʱ�䣬��ʽΪmytime
    uint16_t	value1;			//���������Һ��߶�ֵ
    uint16_t	value2;			//ʵʱֵҺ��߶�ֵ
} OIL_PACK;
extern uint8_t				debug_stop;			///�ò�����Menu_5_9_debug�У���ʾ�Ƿ�ˢ����Ļ
OIL_PACK_INFO		oil_pack_curr = { 0, 0, 0, 0, 0, 0 };

/*********************************************************************************
  *��������:void gps_pack_init( void )
  *��������:�������һ��д���λ��,  ����δ�ϱ���¼��Сid��λ��,
  			��ʼ�����ݴ洢��ز���
  			ע��:�ú���ֻ��������ʼ���洢���򣬲�������ֻ�ܳ�ʼ��һ�εĴ��룬���粻�ܳ�ʼ���ź���
  			���ܳ�ʼ���̵߳�
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-03-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void oil_pack_init( void )
{
    uint32_t		addr;
    OIL_PACK_HEAD 	head;
    uint32_t		read_id = 0xFFFFFFFF;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    oil_pack_curr.pack_count		= 0; /*���²���*/
    oil_pack_curr.addr		= 0;
    oil_pack_curr.id		= 0;
    oil_pack_curr.len		= 0;

    memset( oil_pack_curr.pack_addr, 0, sizeof( oil_pack_curr.pack_addr ) );

    for( addr = ADDR_DF_OIL_PACK_START; addr < ADDR_DF_OIL_PACK_END; addr += OIL_PACK_SECT_SIZE )
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( OIL_PACK_HEAD ) );
        if( head.mn == PACK_HEAD_OIL )         /*��Ч��¼*/
        {
            if( head.id >= oil_pack_curr.id ) /*���������¼,��ǰд���λ��*/
            {
                oil_pack_curr.id		= head.id;
                oil_pack_curr.addr		= addr; /*��ǰ��¼�ĵ�ַ*/
                oil_pack_curr.len		= OIL_DATA_LEN;
            }
            if( read_id > head.id )     /*Ҫ�ҵ���Сid�ĵ�ַ*/
            {
                oil_pack_curr.get_addr = addr;
                read_id			= head.id;
            }
            oil_pack_curr.pack_count++;             /*ͳ�Ƽ�¼��*/
        }
    }
    if(oil_pack_curr.pack_count == 0)
    {
        oil_pack_curr.get_addr =  oil_pack_curr.addr = ADDR_DF_OIL_PACK_START;
    }
    rt_sem_release( &sem_dataflash );
    rt_kprintf( "\n%d>(%d)OIL_PACKд��λ��:%x id=%d ����λ��:%x id=%d",
                rt_tick_get( ),
                oil_pack_curr.pack_count,
                oil_pack_curr.addr,
                oil_pack_curr.id,
                oil_pack_curr.get_addr,
                read_id );
}

/*�����ͺ���������*/
void oil_pack_put( uint8_t *pinfo, uint16_t len )
{
    OIL_PACK_HEAD head;
    uint32_t  last_addr;

    if( oil_pack_curr.addr )                  /*���ȵ��ϱ���ַ*/
    {
        last_addr			= oil_pack_curr.addr;
        oil_pack_curr.addr	+= ( sizeof( OIL_PACK_HEAD ) + OIL_DATA_LEN + OIL_PACK_SECT_SIZE - 1 );	//��63��Ϊ��ȡһ���Ƚϴ��ֵ
        //gps_pack_curr.addr	&= 0xFFFFFFC0;  /*ָ����һ����ַ*/
        oil_pack_curr.addr	= oil_pack_curr.addr / OIL_PACK_SECT_SIZE * OIL_PACK_SECT_SIZE;  /*ָ����һ����ַ*/
        //Ŀ���ǵ�д���λ�ó�����Ҫ��ȡ��λ���ǽ���ȡ��λ�ú���һ������λ
        if(( oil_pack_curr.get_addr > last_addr) && (oil_pack_curr.get_addr <= oil_pack_curr.addr) && ( oil_pack_curr.pack_count ))
        {
            oil_pack_curr.get_addr	= oil_pack_curr.addr + OIL_PACK_SECT_SIZE;
            oil_pack_curr.pack_count--;
            if( oil_pack_curr.get_addr >= ADDR_DF_OIL_PACK_END )
            {
                oil_pack_curr.get_addr = oil_pack_curr.get_addr - ADDR_DF_OIL_PACK_END + ADDR_DF_OIL_PACK_START;
            }
        }
    }
    else                                    /*��û�м�¼*/
    {
        oil_pack_curr.addr = ADDR_DF_OIL_PACK_START;
    }

    if( oil_pack_curr.addr >= ADDR_DF_OIL_PACK_END )
    {
        oil_pack_curr.addr = oil_pack_curr.addr - ADDR_DF_OIL_PACK_END + ADDR_DF_OIL_PACK_START;
        //Ŀ���ǵ�д���λ�ó�����Ҫ��ȡ��λ���ǽ���ȡ��λ�ú���һ������λ
        if(( oil_pack_curr.get_addr <= oil_pack_curr.addr) && (oil_pack_curr.pack_count))
        {
            oil_pack_curr.get_addr	= oil_pack_curr.addr + OIL_PACK_SECT_SIZE;
            oil_pack_curr.pack_count--;
        }
    }
    /*
    if( ( gps_pack_curr.addr & 0xFFF ) == 0 )
    {
    	sst25_erase_4k( gps_pack_curr.addr );
    }
    */

    oil_pack_curr.id++;
    oil_pack_curr.len = len;

    head.mn		= PACK_HEAD_OIL;
    head.id		= oil_pack_curr.id;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    //sst25_write_through( gps_pack_curr.addr, (uint8_t*)&head, sizeof( GPS_PACK_HEAD ) );
    //sst25_write_through( gps_pack_curr.addr + sizeof( GPS_PACK_HEAD ), pinfo, len );
    sst25_write_auto( oil_pack_curr.addr, (uint8_t *)&head, sizeof( OIL_PACK_HEAD ) , ADDR_DF_OIL_PACK_START, ADDR_DF_OIL_PACK_END);
    sst25_write_auto( oil_pack_curr.addr + sizeof( OIL_PACK_HEAD ), pinfo, len , ADDR_DF_OIL_PACK_START, ADDR_DF_OIL_PACK_END);
    rt_sem_release( &sem_dataflash );

    rt_kprintf("\n oil_save_addr=0x%08X,id=%d", oil_pack_curr.addr, oil_pack_curr.id);
    oil_pack_curr.pack_count++;                     /*����δ�ϱ���¼��*/
    if( oil_pack_curr.pack_count == 1 )
    {
        oil_pack_curr.get_addr = oil_pack_curr.addr;
    }
    //rt_kprintf("\n PUT_gps_pack:get_addr=0x%08X,put_addr=0x%08X,id=%05d,count=%05d",gps_pack_curr.get_addr,gps_pack_curr.addr,head.id,gps_pack_curr.pack_count);

}

void redraw_debug_lcd(void)
{
    pMenuItem = &Menu_5_9_debug;
    pMenuItem->tick = rt_tick_get( );
    pMenuItem->show( );
}
/*����ԭʼ��λ����*/
void oil_pack_proc( void )
{
    uint8_t 		i;
    static rt_tick_t last_put_tick = 0;
    static uint16_t last_oil_high = 0;
    uint16_t		temp_u16data = 0;
    OIL_PACK 		pack;
    static uint8_t	int_ok = 0;
    static uint8_t 	usb_insert_ok = 0;

    ///��ʼ����������ز���
    if(int_ok == 0)
    {
        int_ok = 1;
        oil_pack_init();
    }

    ///���ͺ�����д��flash��
    temp_u16data = abs(last_oil_high - oil_high_average);
    if((temp_u16data > 50) || (rt_tick_get() - last_put_tick > RT_TICK_PER_SECOND * OIL_DATA_SAVE_SPACE))
    {
        pack.time = mytime_now;
        pack.value1 = oil_high_average;
        pack.value2 = oil_high;
        oil_pack_put((uint8_t *)&pack, OIL_DATA_LEN);
        if(rt_tick_get() - last_put_tick > RT_TICK_PER_SECOND * (OIL_DATA_SAVE_SPACE + 30) )
            last_put_tick = rt_tick_get();
        else
            last_put_tick += RT_TICK_PER_SECOND * OIL_DATA_SAVE_SPACE;
        last_oil_high = oil_high_average;
    }
    ///����Ƿ���U�̲��룬Ȼ������û����������Ƿ�д���ͺ�����
    if((rt_tick_get() > 30 * RT_TICK_PER_SECOND) && (diskinited) && (usb_insert_ok != diskinited))
    {
        debug_write("��⵽U�̲���");
        debug_write("��'ȷ��'�������ͺ�");
        redraw_debug_lcd();
        for(i = 0; i < 50; i++)
        {
            if(debug_stop)
                break;
            rt_thread_delay( RT_TICK_PER_SECOND / 10 );
        }
        if(debug_stop)
        {
            debug_stop = 0;
            debug_write("�ͺ����ݵ�����...");
            redraw_debug_lcd();
            debug_stop = 0;
            if(oil_packt_get())
            {
                debug_write("�ͺ����ݵ���ʧ��");
            }
            //redraw_debug_lcd();
        }
        else
        {
            debug_write("����ʱ�䳬ʱ���ͺ�����û�е���!");
            pMenuItem = &Menu_1_Idle;
            pMenuItem->tick = rt_tick_get( );
            pMenuItem->show( );
        }
    }
    usb_insert_ok = diskinited;
}

/*
   ��ȡԭʼ��λ����

   ʹ��0900�ϱ�
 */
uint8_t oil_packt_get( void )
{
    OIL_PACK_HEAD 	head;
    uint8_t			databuf[256];              /*�����ϱ����ֽڲ�����256*/
    uint8_t			writebuf[64];              /*�����ϱ����ֽڲ�����256*/
    uint32_t		i, j;
    uint32_t		addr;
    uint16_t		pos;
    uint16_t		read_count;             /*Ҫ��ȡ�ļ�¼��*/
    uint16_t		out_pack_num = 0;
    OIL_PACK		*pack;
    FRESULT 		res;
    uint32_t  		len;
    uint32_t		usb_error = 0;
    uint8_t 		usb_insert_ok = diskinited;

    oil_pack_init();
    if( oil_pack_curr.pack_count == 0 )                 /*û����δ�ϱ���¼*/
    {
        return 0;
    }
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    ///OPEN�ļ�
    if(usb_insert_ok)
    {
        rt_enter_critical( );
        res = f_open(&file_USB, "1:����.txt", FA_READ | FA_WRITE | FA_OPEN_ALWAYS );
        if(res == 0)
        {
            f_lseek( &file_USB, file_USB.fsize);
        }
        else
        {
            usb_error += res;
            rt_kprintf("\r\n f_open = %d", res);
        }
    }
    else
    {
        usb_error++;
        rt_kprintf("\n U��û�в���!");
    }

    /*�������Ҽ�¼*/
    oil_pack_curr.get_count	= 0;
    addr				= oil_pack_curr.get_addr;
    pos					= 0;
    read_count			= 0;                 /*��û�б��ļ�¼����*/
    rt_kprintf("\n");
    sprintf(writebuf, "���:    ���ݼ�¼ʱ��   ,�� ��,ʵʱ�߶�\r\n");
    if(usb_insert_ok == 0)
        rt_kprintf(writebuf);
    if(usb_error == 0)
    {
        res = f_write(&file_USB, writebuf, strlen(writebuf), &len);
        usb_error += res;
        if(res)
        {
            usb_error += res;
            rt_kprintf("\r\n f_write = %d", res);
        }

    }
    for( i = 0; i < ADDR_DF_OIL_PACK_SECT * (4096 / OIL_PACK_SECT_SIZE); i++ )   /*ÿ��¼20�ֽ�*/
    {
        sst25_read( addr, (uint8_t *)&head, sizeof( OIL_PACK_HEAD ) );
        if( head.mn == PACK_HEAD_OIL )                     /*��Ч��¼*/
        {
            sst25_read_auto( addr + sizeof(OIL_PACK_HEAD), databuf + pos, OIL_DATA_LEN , ADDR_DF_OIL_PACK_START, ADDR_DF_OIL_PACK_END);
            pos								+=  OIL_DATA_LEN;
            addr	+= ( sizeof(OIL_PACK_HEAD) + OIL_DATA_LEN + OIL_PACK_SECT_SIZE - 1 );	//��63��Ϊ��ȡһ���Ƚϴ��ֵ
            addr	= addr / OIL_PACK_SECT_SIZE * OIL_PACK_SECT_SIZE;			//ָ����һ����ַ
            read_count++;
            oil_pack_curr.get_count++;
            //rt_kprintf("\n GET_gps_pack:addr=0x%08X,id=%d",addr,head.id);
        }
        else
        {
            addr += OIL_PACK_SECT_SIZE;
        }
        if( addr >= ADDR_DF_OIL_PACK_END )
        {
            addr = ADDR_DF_OIL_PACK_START;
        }
        //rt_sem_release( &sem_dataflash );
        if((pos >= sizeof(databuf) - OIL_DATA_LEN) || ( oil_pack_curr.get_count >= oil_pack_curr.pack_count ))
        {
            ///�����Ϣ��U��
            for(j = 0; j < read_count; j++)
            {
                out_pack_num++;
                pack = (void *)&databuf[j * OIL_DATA_LEN];
                sprintf(writebuf, "%04d:20%02d-%02d-%02d %02d:%02d:%02d,%05d,%05d\r\n",
                        out_pack_num,
                        YEAR( pack->time ),
                        MONTH( pack->time ),
                        DAY( pack->time ),
                        HOUR( pack->time ),
                        MINUTE( pack->time ),
                        SEC( pack->time ),
                        pack->value1,
                        pack->value2
                       );
                if(usb_insert_ok == 0)
                    rt_kprintf(writebuf);
                ///WRITE�ļ�
                if(usb_error == 0)
                {
                    res = f_write(&file_USB, writebuf, strlen(writebuf), &len);
                    usb_error += res;
                    if(res)
                    {
                        usb_error += res;
                        rt_kprintf("\r\n f_write = %d", res);
                    }
                }
            }
            //�������������
            pos = 0;
            read_count = 0;
        }
        if( oil_pack_curr.get_count >= oil_pack_curr.pack_count ) /*�յ�ָ����������*/
        {
            break;
        }
    }
    ///CLOSE�ļ�
    if(usb_insert_ok)
    {
        res = f_close(&file_USB);
        if(res)
        {
            rt_kprintf("\n f_close = %d", res);
            usb_error += res;
        }
        if(0 == usb_error)
        {
            debug_write("�ͺ����ݵ������");
            redraw_debug_lcd();
        }
        ///�����̵߳���
        rt_exit_critical( );
    }

    rt_kprintf("\n USB_WRITE_ERROR = %d", usb_error);
    return usb_error;
    //rt_kprintf( "\n%d>(%d)����λ��:%x,��¼��:%d,����:%d", rt_tick_get( ), gps_pack_curr.pack_count, gps_pack_curr.get_addr, gps_pack_curr.get_count, pos );
}

//FINSH_FUNCTION_EXPORT( oil_packt_get, oil_packt_get );
/************************************** The End Of File **************************************/
