/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		//  camera.c
 * Author:			//  baiyangmin
 * Date:			//  2013-07-08
 * Description:		//  ���չ��ܵ�ʵ�֣�������:���գ��洢���Լ���ؽӿں��������ļ�ʵ�����ļ�jt808_camera.c�����ղ�����������
 * Version:			//  V0.01
 * Function List:	//  ��Ҫ�������书��
 *     1. -------
 * History:			//  ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "include.h"
#include "jt808_param.h"
#include "rs485.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "jt808_util.h"
#include "camera.h"
#include "jt808_camera.h"
#include <finsh.h>
#include "sst25.h"
#include "jt808_config.h"

typedef enum
{
    CAM_NONE = 0, /*����*/
    CAM_IDLE,
    CAM_START,
    CAM_GET_PHOTO,
    CAM_RX_PHOTO,
    CAM_OK,
    CAM_FALSE,
    CAM_END
} CAM_STATE;

/*����cam��Ϣ��ʽ*/
typedef enum
{
    RX_IDLE = 0,
    RX_SYNC1,
    RX_SYNC2,
    RX_HEAD,
    RX_DATA,
    RX_FCS,
    RX_0D,
    RX_0A,
} CAM_RX_STATE;

typedef  __packed struct
{
    CAM_STATE				State;      ///�������״̬��
    CAM_RX_STATE			Rx_State;   ///����״̬
    u8						Retry;      ///�ظ��������
    Style_Cam_Requset_Para	Para;       ///������ǰ���������Ϣ
} Style_Cam_Para;


#define DF_CAM_REC_COUNT	4096        ///ͼƬ���ݼ�¼��Ԫ��С
#define DF_CAM_REC_MASK		0xFFFFF000  //ÿ��¼������ ~(count-1)
#define DF_SECTOR_MASK 		0xFFFFF000       /*4K�ĵ�ַ����*/

extern rt_device_t _console_device;



static Style_Cam_Para	Current_Cam_Para;
TypeDF_PICPara			DF_PicParam;    ///FLASH�洢��ͼƬ��Ϣ
ENUM_MEDIA_STATE		media_state = MEDIA_NONE;	///��ʾ��ǰ��ý�崦�����


/* ��Ϣ���п��ƿ�*/
struct rt_messagequeue	mq_Cam;
/* ��Ϣ�������õ��ķ�����Ϣ���ڴ��*/
static char				msgpool_cam[256];

#define PHOTO_RX_SIZE 600

static uint8_t	photo_rx[PHOTO_RX_SIZE];
static uint16_t photo_rx_wr = 0;

/*���浥��ͼƬ*/
uint8_t photo_ram[4 * 1024];
//static uint8_t photo_ram[32 * 1024] __attribute__( ( section( "CCM_RT_STACK" ) ) );


/*********************************************************************************
  *��������:u32 Cam_Flash_AddrCheck(u32 pro_Address)
  *��������:��鵱ǰ��λ���Ƿ�Ϸ������Ϸ����޸�Ϊ�Ϸ����ݲ�����
  *��	��:none
  *��	��:none
  *�� �� ֵ:��ȷ�ĵ�ֵַ
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static u32 Cam_Flash_AddrCheck( u32 pro_Address )
{
    while( pro_Address >= ADDR_DF_CAM_END )
    {
        pro_Address = pro_Address + ADDR_DF_CAM_START - ADDR_DF_CAM_END;
    }
    if( pro_Address < ADDR_DF_CAM_START )
    {
        pro_Address = ADDR_DF_CAM_START;
    }
    return pro_Address;
}


/*********************************************************************************
  *��������:u16 Cam_Flash_InitPara( u8 printf_info )
  *��������:��ʼ��������ش洢����
  			ע��:�ú���ֻ��������ʼ���洢���򣬲�������ֻ�ܳ�ʼ��һ�εĴ��룬���粻�ܳ�ʼ���ź���
  			���ܳ�ʼ���̵߳�
  *��	��:printf_info	:0.��ʾ����ӡͼƬ��Ϣ��1.��ӡͼƬ��Ϣ
  *��	��:none
  *�� �� ֵ:��Ч��ͼƬ����
  *��	��:������
  *��������:2014-03-05
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u16 Cam_Flash_InitPara( u8 printf_info )
{
    u32					TempAddress;
    TypeDF_PackageHead	TempPackageHead;

    DF_PicParam.Number			= 0;
    DF_PicParam.NotSendCounter	= 0;
    DF_PicParam.First.Address	= ADDR_DF_CAM_START; /*û����Ч��ַ*/
    DF_PicParam.First.Data_ID	= 0xFFFFFFFF;
    DF_PicParam.First.Len		= 0;

    DF_PicParam.Last.Address	= ADDR_DF_CAM_START;
    DF_PicParam.Last.Data_ID	= 0;
    DF_PicParam.Last.Len		= 0;
    if( printf_info )
    {
        rt_kprintf( "\nNO    ADDR        ID     LEN   STATE   trige\n" );
    }
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    for( TempAddress = ADDR_DF_CAM_START; TempAddress < ADDR_DF_CAM_END; )
    {
        sst25_read( TempAddress, (u8 *)&TempPackageHead, sizeof( TempPackageHead ) );
        if( TempPackageHead.Head == CAM_HEAD )                      /*��Ч����*/
        {
            DF_PicParam.Number++;
            if((TempPackageHead.State & 0x03) == 0x03)
                DF_PicParam.NotSendCounter++;
            if( DF_PicParam.First.Data_ID >= TempPackageHead.id )   /*��һ��Ӧ����id��С��*/
            {
                DF_PicParam.First.Address	= TempAddress;
                DF_PicParam.First.Len		= TempPackageHead.Len;
                DF_PicParam.First.Data_ID	= TempPackageHead.id;
            }

            if( DF_PicParam.Last.Data_ID <= TempPackageHead.id )    /*���һ��Ӧ����id����*/
            {
                DF_PicParam.Last.Address	= TempAddress;
                DF_PicParam.Last.Len		= TempPackageHead.Len;
                DF_PicParam.Last.Data_ID	= TempPackageHead.id;
            }

            if( printf_info )
            {
                rt_kprintf( "%05d 0x%08x  %05d  %04d  %d    %d\n", DF_PicParam.Number, TempAddress, TempPackageHead.id, TempPackageHead.Len, TempPackageHead.State, TempPackageHead.TiggerStyle);
            }
            TempAddress += ( TempPackageHead.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK;
        }
        else
        {
            TempAddress += DF_CAM_REC_COUNT;
        }
    }
    rt_sem_release( &sem_dataflash );
    return DF_PicParam.Number;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_WrPic(u8 *pData,u16 len, TypeDF_PackageHead *pHead)
  *��������:��FLASH��д��ͼƬ���ݣ�������ݵ����ݵ�pHead->lenΪ��0ֵ��ʾΪ���һ������
  *��	��:	pData:д�������ָ�룬ָ������buf��
   len:���ݳ��ȣ�ע�⣬�ó��ȱ���С��4096
   pHead:���ݰ�ͷ��Ϣ���ð�ͷ��Ϣ��Ҫ����ʱ��Timer����ֵÿ�ζ����봫�ݣ�����ͬ���İ���ֵ���ܱ仯��
    ���һ�������轫len����Ϊ���ݳ���len�ĳ��Ȱ�����ͷ���֣���ͷ����Ϊ�̶�64�ֽڣ�������
    lenΪ0.
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/

#define my_printf( X ) rt_kprintf( "\n%s=%d", # X, X )


/*********************************************************************************
  *��������:static rt_err_t Cam_Flash_SavePic( TypeDF_PackageHead * phead )
  *��������:ֱ�ӽ�һ��������ͼƬ������flash��
  *��	��:	p_head		:��Ҫ�����ͼƬ���������ݰ�ͷ���������ݡ�
  *��	��: none
  *�� �� ֵ:RT_EOK ��ʾ�洢�ɹ���������ʾʧ��
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:bai
  *�޸�����:2013-06-23
  *�޸�����:�޸����������ݴ洢�쳣ʱ�ļ�������
*********************************************************************************/
static rt_err_t Cam_Flash_SavePic( TypeDF_PackageHead *phead )
{
    uint32_t	len;
    uint32_t	addr;
    uint8_t		 *p;

    len		= phead->Len;
    addr	= ( DF_PicParam.Last.Address + DF_PicParam.Last.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK; /*4096����*/

    DF_PicParam.Last.Len	= len;
    DF_PicParam.Last.Address = Cam_Flash_AddrCheck(addr);
    DF_PicParam.Last.Data_ID++;
    DF_PicParam.Number++;
    DF_PicParam.NotSendCounter++;

    phead->id = DF_PicParam.Last.Data_ID;
    memcpy( photo_ram, (uint8_t *)phead, sizeof(TypeDF_PackageHead) );
    p = photo_ram;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    while( 1 )
    {
        addr = Cam_Flash_AddrCheck( addr );
        sst25_erase_4k( addr );
        if( len > 4096 )
        {
            sst25_write_through( addr, p, 4096 );
            p		+= 4096;
            addr	+= 4096;
            len		-= 4096;
        }
        else
        {
            sst25_write_through( addr, p, len );
            break;
        }
    }
    rt_sem_release( &sem_dataflash );
    Cam_Flash_InitPara( 0 );
    return RT_EOK;
}



/*********************************************************************************
  *��������:rt_err_t Cam_Flash_SaveMediaData(uint32_t file_addr, uint8_t *p, uint16_t len)
  *��������:����ͼƬ���ݵ�flash�У��ú���ÿ��ֻ�ܱ���4096�ֽڵ�ͼƬ���ݣ����߱���64�ֽڵİ�ͷ��Ϣ
  *��	��:	file_addr	:��Ҫ�����ͼƬ���ݵ����λ�ã�ÿ����ý������λ�ô�0��ʼ��ǰ64�ֽ�Ϊ��ͷ��Ϣ
  			p			:���������ָ�룬��һ��������Ҫ�ճ�64�ֽڵİ�ͷ���֣���ǰ64�ֽ�����Ϊ0xFF
  			len			:��������ݳ��ȣ���ý������Ϊ4096�ֽڣ���ý���ͷΪ64�ֽڣ������ý�����ݲ���4096�ֽڣ�����Ҫ���������ֶ���ֵΪ0xFF
  *��	��: none
  *�� �� ֵ:RT_EOK ��ʾ�洢�ɹ���������ʾʧ��
  *��	��:������
  *��������:2013-12-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_Flash_SaveMediaData(uint32_t file_addr, uint8_t *p, uint16_t len)
{
    uint32_t	addr;

    addr	= ( DF_PicParam.Last.Address + DF_PicParam.Last.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK; /*4096����*/
    if(len > 4096)
        return RT_ERROR;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    addr	+= file_addr;
    addr	= Cam_Flash_AddrCheck(addr);
    if((file_addr == 0) && (len == sizeof(TypeDF_PackageHead)))
    {
        sst25_write_through( addr, p, len );
        rt_sem_release( &sem_dataflash );
        Cam_Flash_InitPara( 0 );
    }
    else
    {
        sst25_erase_4k( addr );
        sst25_write_through( addr, p, len );
        rt_sem_release( &sem_dataflash );
    }
    return RT_EOK;
}


/*********************************************************************************
  *��������:u32 Cam_Flash_FindPicID(u32 id,TypeDF_PackageHead *p_head)
  *��������:��FLASH�в���ָ��ID��ͼƬ������ͼƬ��ͷ���´洢��p_head��
  *��	��:	id			:��ý��ID��
   			p_head		:������������ΪͼƬ��ý�����ݰ�ͷ��Ϣ(����ΪTypeDF_PackageHead)
   			search_state:��ʾ���ҵ�����������BIT0 ��ʾ������ɾ������Ƭ��BIT1��ʾ�������Ѿ��ɹ��ϴ�����Ƭ
  *��	��:p_head
  *�� �� ֵ:u32 0xFFFFFFFF��ʾû���ҵ�ͼƬ��������ʾ�ҵ���ͼƬ������ֵΪͼƬ��flash�еĵ�ַ
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:bai
  *�޸�����:2013-06-23
  *�޸�����:�޸����������ݴ洢�쳣ʱ�ļ�������
*********************************************************************************/
u32 Cam_Flash_FindPicID( u32 id, TypeDF_PackageHead *p_head , uint8_t search_state)
{
    u32					TempAddress;
    TypeDF_PackageHead	TempPackageHead;

    if( DF_PicParam.Number == 0 )
    {
        return 0xFFFFFFFF;
    }
    if( ( id < DF_PicParam.First.Data_ID ) || ( id > DF_PicParam.Last.Data_ID ) )
    {
        return 0xFFFFFFFF;
    }

    for( TempAddress = ADDR_DF_CAM_START; TempAddress < ADDR_DF_CAM_END; )
    {
        sst25_read( TempAddress, (u8 *)&TempPackageHead, sizeof( TempPackageHead ) );
        if( TempPackageHead.Head == CAM_HEAD ) /*��Ч����*/
        {
            if(( id == TempPackageHead.id ) && (TempPackageHead.State & search_state) == search_state)
            {
                memcpy( p_head, (u8 *)&TempPackageHead, sizeof( TempPackageHead ) );
                return TempAddress;
            }
            TempAddress += ( TempPackageHead.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK;
        }
        else
        {
            TempAddress += DF_CAM_REC_COUNT;
        }
    }
    return 0xFFFFFFFF;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_RdPic(void *pData,u16 *len, u32 id,u8 offset )
  *��������:��FLASH�ж�ȡͼƬ����
  *��	��:	pData:д�������ָ�룬ָ������buf��
   len:���ص����ݳ���ָ��ע�⣬�ó������Ϊ512
   id:��ý��ID��
   offset:��ý������ƫ��������0��ʼ��0��ʾ��ȡ��ý��ͼƬ��ͷ��Ϣ����ͷ��Ϣ���ȹ̶�Ϊ64�ֽڣ�����
    �ṹ��TypeDF_PackageHead��ʽ�洢
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_Flash_RdPic( void *pData, u16 *len, u32 id, u8 offset )
{
    u32					TempAddress;
    u32					temp_Len;
    TypeDF_PackageHead	TempPackageHead;
    u8					ret;

    *len = 0;
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    TempAddress = Cam_Flash_FindPicID( id, &TempPackageHead , 0x00);
    if( TempAddress == 0xFFFFFFFF )
    {
        ret = RT_ERROR;
        goto lbl_cam_flash_rdpic_ret;
    }

    if( offset > ( TempPackageHead.Len - 1 ) / 512 )
    {
        ret = RT_ENOMEM;
        goto lbl_cam_flash_rdpic_ret;
    }
    if( offset == ( TempPackageHead.Len - 1 ) / 512 )
    {
        temp_Len = TempPackageHead.Len - ( offset * 512 );
    }
    else
    {
        temp_Len = 512;
    }
    sst25_read( Cam_Flash_AddrCheck(TempAddress + offset * 512), (u8 *)pData, temp_Len );
    *len = temp_Len;

    ret = RT_EOK;

lbl_cam_flash_rdpic_ret:
    rt_sem_release( &sem_dataflash );
    return ret;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_DelPic(u32 id)
  *��������:��FLASH��ɾ��ͼƬ��ʵ���ϲ�û��ɾ�������ǽ�ɾ�������0�������´ξ���Ҳ����ѯ��ͼƬ
  *��	��:	id:��ý��ID��
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_Flash_DelPic( u32 id )
{
    u32					TempAddress;
    TypeDF_PackageHead	TempPackageHead;
    u8					ret;

    if( DF_PicParam.Number == 0 ) /*û��ͼƬ*/
    {
        return RT_EOK;
    }

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    TempAddress = Cam_Flash_FindPicID( id, &TempPackageHead , BIT(0));
    if( TempAddress == 0xFFFFFFFF )
    {
        ret = RT_ERROR;
        goto FUNC_RET;
    }
    TempPackageHead.State &= ~( BIT( 0 ) +  BIT( 1 ));
    sst25_write_through( TempAddress, (u8 *)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
    ret = RT_EOK;

FUNC_RET:
    rt_sem_release( &sem_dataflash );
    return ret;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_TransOkSet(u32 id)
  *��������:����ID��ͼƬ���Ϊ���ͳɹ�
  *��	��:	id:��ý��ID��
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_Flash_TransOkSet( u32 id )
{
    u32					TempAddress;
    TypeDF_PackageHead	TempPackageHead;
    u8					ret;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    TempAddress = Cam_Flash_FindPicID( id, &TempPackageHead , BIT(0) | BIT(1));
    if( TempAddress == 0xFFFFFFFF )
    {
        ret = RT_ERROR;
        goto FUNC_RET;
    }
    TempPackageHead.State &= ~( BIT( 1 ) );
    sst25_write_through( TempAddress, (u8 *)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
    ret = RT_EOK;
    if(DF_PicParam.NotSendCounter)
        DF_PicParam.NotSendCounter--;
FUNC_RET:
    rt_sem_release( &sem_dataflash );
    return ret;
}


/*********************************************************************************
  *��������:void* Cam_Flash_SearchPicHead( MYTIME start_time, MYTIME end_time, uint8_t channel, uint8_t trigger, uint16_t *findcount ,uint8_t search_state)
  *��������:����ָ��ʱ�䷶Χ�ڵ�ͼƬͷ�����γɶ�̬����ļ�¼���ܵļ�¼��
  			�������Ĵ洢��ý����Ϣ�������ն���ʾͼƬ��Ϣ
			ע���������
			ע�����˳��
			ע���м���ܼ��;
  *��	��: start_time	:	���������Ŀ�ʼʱ��
  			end_time	:	���������Ľ���ʱ��
  			channel		:	����ͨ����0��ʾ������ͨ��
  			trigger		:	�������յ�ԭ��
  			findcount	:	Ϊ����ʱ��ʾ��Ҫ���ҵ�������
  			search_state:	��ʾ���ҵ�����������BIT0 ��ʾ������ɾ������Ƭ��BIT1��ʾ�������Ѿ��ɹ��ϴ�����Ƭ
  *��	��: void*		:	������ҵ�ͼƬ�򷵻ظ�ͼƬ������ͷָ�룬�������˿ռ䣬���øú���ʱ��Ҫ�ͷŸ���Դ�����û���ҵ�ͼƬ������Ҫ�ͷţ���Ϊ������û�з�����Դ
  			findcount	:	��Ϊ���ʱ��ʾʵ�ʲ��ҵ���ͼƬ����
  *�� �� ֵ:void*
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:������
  *�޸�����:2013-11-18
  *�޸�����:���������������search_state��ͬʱ�����˲���findcount�����빦��
*********************************************************************************/
void *Cam_Flash_SearchPicHead( MYTIME start_time, MYTIME end_time, uint8_t channel, uint8_t trigger, uint16_t *findcount , uint8_t search_state)
{
    u16					i, j;
    TypeDF_PackageHead	TempPackageHead;

    uint16_t			search_counter = *findcount;
    uint16_t			count = 0;
    uint32_t			addr;
    uint32_t			find_addr[20];         /*һ�ٸ�ͼƬ������*/

    uint8_t				*p = RT_NULL;
    if(search_counter > 20)
        search_counter = 20;
    if(search_counter == 0)
        search_counter = 20;


    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    addr = DF_PicParam.First.Address;           /*�����µĿ�ʼ*/
    //rt_kprintf( "\nCAM���� Number=%05d ,Firstaddr=0x%08x ", DF_PicParam.Number, addr);
    for( i = 0, j = 0; i < DF_PicParam.Number; )
    {
        addr = Cam_Flash_AddrCheck(addr);
        sst25_read( addr, (u8 *)&TempPackageHead, sizeof( TempPackageHead ) );
        if( TempPackageHead.Head == CAM_HEAD )  /*��Ч����*/
        {
            i++;
            if( ( TempPackageHead.Time >= start_time ) && ( TempPackageHead.Time <= end_time ) )
            {
                if( ( channel == 0 ) || ( channel == TempPackageHead.Channel_ID ) )
                {
                    if( ( trigger == 0xFF ) || ( trigger == TempPackageHead.TiggerStyle ) )
                    {
                        if((TempPackageHead.State & search_state) == search_state)				///�������е�û��ɾ���Ķ�ý����
                            //if((TempPackageHead.State & (search_state|(BIT(2)))) == search_state)		///ֻ�������̵Ķ�ý��
                        {
                            find_addr[count] = addr;
                            count++;
                            if(count >= search_counter)
                                break;
                        }//else{rt_kprintf("\n CAM_STATE_OUT_RANGE=0x%02X",TempPackageHead.State);}
                    }//else{rt_kprintf("\n CAM_TRIGE_OUT_RANGE=0x%02X",TempPackageHead.TiggerStyle);}
                }//else{rt_kprintf("\n CAM_CHANNEL_OUT_RANGE=0x%02X",TempPackageHead.Channel_ID);}
            }//else{rt_kprintf("\n CAM_TIME_OUT_RANGE=0x%08X",TempPackageHead.Time);}
            //rt_kprintf( "\nCAM����OK   num=%05d ,addr=0x%08x ,head=0x%08x  ", i, addr, TempPackageHead.Head);
            addr += ( TempPackageHead.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK;
        }
        else
        {
            j++;
            if( j >= DF_PicParam.Number)
                break;
            //rt_kprintf( "\nCAM���ݴ��� num=%05d ,addr=0x%08x ,head=0x%08x  ", i, addr, TempPackageHead.Head);
            addr += DF_CAM_REC_COUNT;
        }
    }

    if( count == 0 ) /*û���ҵ���¼*/
    {
        goto lbl_search_pichead_end;
    }
    p = rt_malloc( count * sizeof( TypeDF_PackageHead ) );
    if( p == RT_NULL )
    {
        goto lbl_search_pichead_end;
    }
    /*˳����д�ҵ��ļ�¼*/
    for( i = 0; i < count; i++ )
    {
        sst25_read( find_addr[i], p + i * sizeof( TempPackageHead ), sizeof( TempPackageHead ) );
    }

lbl_search_pichead_end:
    rt_sem_release( &sem_dataflash );
    *findcount = count;
    return p;
}


/*********************************************************************************
  *��������:void Cam_Device_init( void )
  *��������:��ʼ��CAMģ����ؽӿ�
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_Device_init( void )
{
    ///����
    Power_485CH1_ON;

    ///��ʼ����Ϣ����
    rt_mq_init( &mq_Cam, "mq_cam", &msgpool_cam[0], sizeof( Style_Cam_Requset_Para ), sizeof( msgpool_cam ), RT_IPC_FLAG_FIFO );

    ///��ʼ��flash����
    Cam_Flash_InitPara( 0 );

    ///��ʼ������״̬����
    memset( (u8 *)&Current_Cam_Para, 0, sizeof( Current_Cam_Para ) );
    Current_Cam_Para.State = CAM_NONE;
}



/*********************************************************************************
  *��������:void cam_fact_set( void )
  *��������:��ʽ�����մ洢����
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-11-18
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void cam_fact_set( void )
{
    u32					TempAddress;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    for( TempAddress = ADDR_DF_CAM_START; TempAddress < ADDR_DF_CAM_END; )
    {
        sst25_erase_4k( TempAddress );
        TempAddress += 4096;
    }
    rt_sem_release( &sem_dataflash );
}
FINSH_FUNCTION_EXPORT( cam_fact_set, cam_fact_set );


/*********************************************************************************
  *��������:static void Cam_Start_Cmd(u16 Cam_ID)
  *��������:��cameraģ�鷢�Ϳ�ʼ����ָ��
  *��	��:Cam_ID	��Ҫ���յ�camera�豸ID
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static void Cam_Start_Cmd( u16 Cam_ID )
{
    u8 Take_photo[10] = { 0x40, 0x40, 0x61, 0x81, 0x02, 0X00, 0X00, 0X02, 0X0D, 0X0A };                             //----  ������������
    Take_photo[4]	= (u8)Cam_ID;
    Take_photo[5]	= (u8)( Cam_ID >> 8 );
    RS485_write( Take_photo, 10 );
    //uart_rs485_rxbuf_rd = uart_rs485_rxbuf_wr;
    rt_kprintf( "\n%d>����=%d ", rt_tick_get( ), Cam_ID );
}

/*********************************************************************************
  *��������:static void Cam_Read_Cmd(u16 Cam_ID)
  *��������:��cameraģ�鷢�Ͷ�ȡ��Ƭ����ָ��
  *��	��:Cam_ID	��Ҫ���յ�camera�豸ID
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static void Cam_Read_Cmd( u16 Cam_ID )
{
    uint8_t Fectch_photo[10] = { 0x40, 0x40, 0x62, 0x81, 0x02, 0X00, 0XFF, 0XFF, 0X0D, 0X0A }; //----- ����ȡͼ����
    Fectch_photo[4] = (u8)Cam_ID;
    Fectch_photo[5] = (u8)( Cam_ID >> 8 );
    RS485_write( Fectch_photo, 10 );
    //	uart_rs485_rxbuf_rd = uart_rs485_rxbuf_wr;
    rt_kprintf( "\n%d>����=%d ", rt_tick_get( ), Cam_ID );
}

/*********************************************************************************
  *��������:TypeDF_PICPara Cam_get_param(void)
  *��������:��ȡϵͳ������ز���
  *��	��:none
  *��	��:none
  *�� �� ֵ:TypeDF_PICPara
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
TypeDF_PICPara *Cam_get_param( void )
{
    return &DF_PicParam;
}

/*********************************************************************************
  *��������:rt_err_t take_pic_request( Style_Cam_Requset_Para *para)
  *��������:��������ָ��
  *��	��:para���ղ���
  *��	��:none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t take_pic_request( Style_Cam_Requset_Para *para )
{
    return rt_mq_send( &mq_Cam, (void *)para, sizeof( Style_Cam_Requset_Para ) );
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
void getpicpara( void )
{
    rt_kprintf( "\nMedia_num=%d", DF_PicParam.Number );
    rt_kprintf( "\nFirst id=%05d(addr=0x%08X,len=%05d)\nLast  id=%05d(addr=0x%08X,len=%05d)",
                DF_PicParam.First.Data_ID,
                DF_PicParam.First.Address,
                DF_PicParam.First.Len,
                DF_PicParam.Last.Data_ID,
                DF_PicParam.Last.Address,
                DF_PicParam.Last.Len );
}

FINSH_FUNCTION_EXPORT( getpicpara, getpicpara );


/*********************************************************************************
  *��������:bool Camera_RX_Data(u16 *RxLen)
  *��������:���ս������ݴ���
  *��	��:RxLen:��ʾ���յ������ݳ���ָ��
  *��	��:RxLen:��ʾ���յ������ݳ���ָ��
  *�� �� ֵ:0��ʾ���ս�����,1��ʾ���ճɹ�
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static u8 Camera_RX_Data( u16 *RxLen )
{
    u8			ch;
    static u16	page_size		= 0;
    static u16	cam_rx_head_wr	= 0;

    /*�����Ƿ��յ�����*/
    while( RS485_read_char( &ch ) )
    {
        //rt_kprintf("%02X",ch);
        switch( Current_Cam_Para.Rx_State )
        {
        case RX_DATA: /*������Ϣ��ʽ: λ��(2B) ��С(2B) FLAG_FF ���� 0D 0A*/
            photo_rx[photo_rx_wr++] = ch;
            photo_rx_wr				%= PHOTO_RX_SIZE;
            if( photo_rx_wr == page_size )
            {
                Current_Cam_Para.Rx_State = RX_FCS;
                //rt_kprintf( "\n%d>CAM Rx_data=%d", rt_tick_get( ), photo_rx_wr );
            }
            break;
        case RX_IDLE:
            if( ch == 0x40 )
            {
                Current_Cam_Para.Rx_State = RX_SYNC1;
            }
            break;
        case RX_SYNC1:
            if( ch == 0x40 )
            {
                Current_Cam_Para.Rx_State = RX_SYNC2;
            }
            else
            {
                Current_Cam_Para.Rx_State = RX_IDLE;
            }
            break;
        case RX_SYNC2:
            if( ch == 0x63 )
            {
                cam_rx_head_wr				= 0;
                Current_Cam_Para.Rx_State	= RX_HEAD;
            }
            else
            {
                Current_Cam_Para.Rx_State = RX_IDLE;
            }
            break;
        case RX_HEAD:
            photo_rx[cam_rx_head_wr++] = ch;
            if( cam_rx_head_wr == 5 )
            {
                photo_rx_wr					= 0;
                page_size					= ( photo_rx[3] << 8 ) | photo_rx[2];
                Current_Cam_Para.Rx_State	= RX_DATA;
            }
            break;
        case RX_FCS:
            Current_Cam_Para.Rx_State = RX_0D;
            break;
        case RX_0D:
            if( ch == 0x0d )
            {
                Current_Cam_Para.Rx_State = RX_0A;
            }
            else
            {
                Current_Cam_Para.Rx_State = RX_IDLE;
            }
            break;
        case RX_0A:
            Current_Cam_Para.Rx_State = RX_IDLE;
            if( ch == 0x0a )
            {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/*********************************************************************************
  *��������:void Cam_response_ok( struct _Style_Cam_Requset_Para *para,uint32_t pic_id )
  *��������:ƽ̨�·�������������Ļص�����_������Ƭ����OK
  *��	��:	para	:���մ���ṹ��
   pic_id	:ͼƬID
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_response_ok( struct _Style_Cam_Requset_Para *para, uint32_t pic_id )
{
    /*
    if( para->SendPhoto )
    {
    	if(( gsm_socket[0].state == CONNECTED )&&(jt808_state == JT808_REPORT))
    		{
    		Cam_jt808_0x0800( pic_id, !para->SavePhoto );
    		}
    }
    */
    return;
}

/*********************************************************************************
  *��������:void Cam_response_end( struct _Style_Cam_Requset_Para *para )
  *��������:ƽ̨�·�������������Ļص�����
  *��	��:	para	:���մ���ṹ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_response_end( struct _Style_Cam_Requset_Para *para )
{
    return;
}

/*********************************************************************************
  *��������:void Camera_Process(void)
  *��������:����������ش���(������:���գ��洢��Ƭ���������ս���ָ���808)
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/

u8 Camera_Process( void )
{
    u16							i, RxLen;
    static u16					cam_photo_size;
    static u8					cam_page_num	= 0;
    u8							cam_last_page	= 0;
    static uint32_t				photo_tick;
    static TypeDF_PackageHead	pack_head;
    static uint8_t				chn_id = 0;
    static uint8_t				power_cam_on = 1;
    static uint8_t				cam_channal_used_max  = 0;	///��ʾ��ǰ��������״̬������ͷ��ͨ���ŵ����ͨ��
    static uint8_t				cam_channal_used  = 0;		///��ʾ��ǰ��������״̬������ͷ��ͨ���ţ�ÿ��BIT��ʾһ��ͨ������BIT1��ʼ
    static uint8_t				cam_channal_check = 0;		///��һ�μӵ���Ե�������ͷͨ������BIT1��ʼ
    u8							ch;
    char						temp_buf[32];

    cam_channal_used &= 0x1F;			///ֻ����5·
    switch( Current_Cam_Para.State )
    {
    case CAM_NONE:
        if( RT_EOK == rt_mq_recv( &mq_Cam, (void *)&Current_Cam_Para.Para, sizeof( Style_Cam_Requset_Para ), RT_WAITING_NO ) )
        {
            Power_485CH1_ON;
            Current_Cam_Para.State				= CAM_IDLE;
            Current_Cam_Para.Para.start_tick	= RT_TICK_PER_SECOND / 10 * 12 + rt_tick_get( );
            if( Current_Cam_Para.Para.Channel_ID == 0xFF ) /*��·����*/
            {
                chn_id = 1;
            }
            else
            {
                chn_id = Current_Cam_Para.Para.Channel_ID;
            }
            sprintf(temp_buf, "��ʼ����,CH=%d", Current_Cam_Para.Para.Channel_ID);
            debug_write(temp_buf);
        }
        else
        {
            power_cam_on = 0;
            Power_485CH1_OFF;
            if(media_state != MEDIA_RECORD)
                media_state = MEDIA_NONE;
            return 0;
        }
    case CAM_IDLE:
        if(media_state != MEDIA_RECORD)
            media_state = MEDIA_NONE;
        if( (( rt_tick_get( ) > Current_Cam_Para.Para.start_tick )) && (( rt_tick_get( ) - Current_Cam_Para.Para.start_tick ) >= (Current_Cam_Para.Para.PhotoSpace * (u32)Current_Cam_Para.Para.PhotoNum)) )
        {
            Current_Cam_Para.Retry	= 0;
            Current_Cam_Para.State	= CAM_START;
        }
        break;
    case CAM_START: /*��ʼ����*/
        if(media_state == MEDIA_RECORD)
        {
            return 0;
        }
        media_state = MEDIA_NONE;
        cam_page_num	= 0;
        cam_photo_size	= 64;		//4096�ֽڴ洢һ�Σ���Լram
        //cam_photo_size	= 0;		//�����洢ͼƬ
        photo_tick		= rt_tick_get( );

        if(( Current_Cam_Para.Para.Channel_ID == 0xFF ) && (cam_channal_used)) /*��·����*/
        {
            for(i = chn_id; i < 5; i++)
            {
                if((BIT(i)) & cam_channal_used)
                {
                    chn_id = i;
                    break;
                }
            }
        }
        Cam_Start_Cmd( chn_id );

        pack_head.Channel_ID	= chn_id;
        pack_head.TiggerStyle	= Current_Cam_Para.Para.TiggerStyle;
        pack_head.Media_Format	= 0;
        pack_head.Media_Style	= 0;
        pack_head.Time			= mytime_now;
        pack_head.Head			= CAM_HEAD;
        pack_head.State			= 0xFF;
        /////
        if(Current_Cam_Para.Para.SavePhoto == 1)
            pack_head.State &= ~( BIT( 2 ) );
        /////
        pack_head.id			= DF_PicParam.Last.Data_ID + 1;
        memcpy( (uint8_t *) & ( pack_head.position ), (uint8_t *)&gps_baseinfo, 28 );
        memset( photo_ram , 0xFF , sizeof(photo_ram));
        Current_Cam_Para.State		= CAM_RX_PHOTO;
        Current_Cam_Para.Rx_State	= RX_IDLE;
        break;
    case CAM_GET_PHOTO:
        photo_tick = rt_tick_get( );
        Cam_Read_Cmd( chn_id );
        Current_Cam_Para.State		= CAM_RX_PHOTO;
        Current_Cam_Para.Rx_State	= RX_IDLE;
        break;
    case CAM_RX_PHOTO:
        if( 1 == Camera_RX_Data( &RxLen ) )
        {
            if(media_state == MEDIA_RECORD)
            {
                Current_Cam_Para.State = CAM_START;
                return 0;
            }
            media_state = MEDIA_CAMERA;
            photo_tick = rt_tick_get( );
            if( photo_rx_wr > 512 ) ///���ݴ���512,�Ƿ�
            {
                rt_kprintf( "\nCAM%d invalided\n", chn_id );
                Current_Cam_Para.State = CAM_START;
                break;
            }
            if( photo_rx_wr == 512 )
            {
                if( ( photo_rx[510] == 0xff ) && ( photo_rx[511] == 0xD9 ) )
                {
                    cam_last_page = 1;
                }
            }
            else
            {
                cam_last_page = 1;
            }
            cam_page_num++;

            ////////4096�ֽڴ洢һ�Σ���Լram
            for(i = 0; i < photo_rx_wr; i++)
            {
                photo_ram[(cam_photo_size++) % 4096] = photo_rx[i];
                if(cam_photo_size % 4096 == 0)
                {
                    Cam_Flash_SaveMediaData(cam_photo_size - 4096, photo_ram, 4096);
                    memset(photo_ram, 0xFF, 4096);
                }
            }
            ////////
            ////////�����洢ͼƬ
            //memcpy( photo_ram + cam_photo_size + 64, photo_rx, photo_rx_wr );   /*�������ݵ�sram*/
            //cam_photo_size += photo_rx_wr;
            ////////


            if( cam_last_page )                                                 ///���һ�����ݣ���Ҫ������д�롣
            {
                ////////4096�ֽڴ洢һ�Σ���Լram
                if(cam_photo_size % 4096)
                    Cam_Flash_SaveMediaData(cam_photo_size / 4096 * 4096, photo_ram, cam_photo_size % 4096);
                pack_head.Len = cam_photo_size;
                Cam_Flash_SaveMediaData(0, (uint8_t *)&pack_head, sizeof(TypeDF_PackageHead));
                ////////
                ////////�����洢ͼƬ
                //pack_head.Len = cam_photo_size + 64;
                //Cam_Flash_SavePic( &pack_head );
                ////////
                Current_Cam_Para.State = CAM_OK;
            }
            else
            {
                Current_Cam_Para.State = CAM_GET_PHOTO;
            }
            photo_rx_wr = 0;
        }
        else if( rt_tick_get( ) - photo_tick > RT_TICK_PER_SECOND * 3 ) ///�ж��Ƿ�ʱ������������Ҫʱ���Գ�
            //else if(( rt_tick_get( ) - photo_tick > RT_TICK_PER_SECOND * 3 )||((power_cam_on == 0)&&(cam_photo_size == 0))) ///�ж��Ƿ�ʱ������������Ҫʱ���Գ�
        {
            Current_Cam_Para.Retry++;
            if( Current_Cam_Para.Retry >= 3 )
            {
                Current_Cam_Para.State = CAM_FALSE;
                sprintf(temp_buf, "����ʧ��,CH=%d", chn_id);
                debug_write(temp_buf);
                Current_Cam_Para.Retry = 0;
                goto CAM_PROC_FALSE;
            }
            else
            {
                Current_Cam_Para.State = CAM_START;
            }
        }
        if(Current_Cam_Para.State != CAM_OK)			///2013-11-18���ӵ����ݣ�û�е����ճɹ������λ����������ͼƬ����
            break;
    case CAM_OK:
        media_state = MEDIA_NONE;
        //rt_kprintf( "\n%d>���ճɹ�!", rt_tick_get( ) );
        getpicpara( );
        sprintf(temp_buf, "���ճɹ�,CH=%d", chn_id);
        debug_write(temp_buf);
        //Current_Cam_Para.Para.start_tick = rt_tick_get( );
        if(( cam_channal_used == 0) && (Current_Cam_Para.Para.Channel_ID == 0xFF))
        {
            cam_channal_check |= BIT(chn_id);
            if(cam_channal_used_max < chn_id)
                cam_channal_used_max = chn_id;
        }

        if( Current_Cam_Para.Para.cb_response_cam_ok != RT_NULL )   ///���õ�����Ƭ���ճɹ��ص�����
        {
            Current_Cam_Para.Para.cb_response_cam_ok( &Current_Cam_Para.Para, pack_head.id );
        }
        else  ///Ĭ�ϵĻص�����
        {
            Cam_response_ok( &Current_Cam_Para.Para, pack_head.id );
        }
        ////
        if( Current_Cam_Para.Para.Channel_ID == 0xFF )
        {
            if( cam_channal_used == 0)
            {
                if( chn_id == 4 )                                       /*��·���*/
                {
                    chn_id = 0;
                    Current_Cam_Para.Para.PhotoNum++;		/*����һ��ͼƬ����*/
                }
                chn_id++;
            }
            else
            {
                if( chn_id == cam_channal_used_max )                                       /*��·���*/
                {
                    chn_id = 0;
                    Current_Cam_Para.Para.PhotoNum++;		/*����һ��ͼƬ����*/
                }
                chn_id++;
            }
        }
        else
        {
            Current_Cam_Para.Para.PhotoNum++;			/*����һ��ͼƬ*/
        }
        if(Current_Cam_Para.Para.PhotoNum >= Current_Cam_Para.Para.PhotoTotal)
        {
            Current_Cam_Para.State = CAM_END;		///�����������
            goto CAM_PROC_END;
        }
        else
        {
            Current_Cam_Para.State = CAM_IDLE;		///������Ƭ�������
        }
        break;
        ////
    case CAM_FALSE:                                                 /*���ܳɹ�ʧ�ܶ�Ҫ����Ƿ����꣬����ʧ��Ҫ��Ҫ�ص�����ϵ�*/
CAM_PROC_FALSE:
        if(media_state != MEDIA_RECORD)
            media_state = MEDIA_NONE;

        if( Current_Cam_Para.Para.Channel_ID == 0xFF )              /*��·����*/
        {
            if( cam_channal_used == 0)
            {
                if( chn_id == 4 )                                       /*��·���*/
                {
                    chn_id = 0;
                    Current_Cam_Para.Para.PhotoNum++;		/*����һ��ͼƬ����*/
                }
                chn_id++;
            }
            else
            {
                if( chn_id == cam_channal_used_max )                                       /*��·���*/
                {
                    chn_id = 0;
                    Current_Cam_Para.Para.PhotoNum++;		/*����һ��ͼƬ����*/
                }
                chn_id++;
            }
            if( Current_Cam_Para.Para.PhotoNum >= Current_Cam_Para.Para.PhotoTotal )
            {
                Current_Cam_Para.State = CAM_END;                       ///�����������
            }
            else
            {
                Current_Cam_Para.State = CAM_IDLE;                      ///������Ƭ�������
            }
        }
        else
        {
            Current_Cam_Para.State = CAM_END;
        }
        break;

    case CAM_END:
CAM_PROC_END:
        rt_kprintf( "\n%d>����%x����!", rt_tick_get( ), Current_Cam_Para.Para.Channel_ID );

        if(( cam_channal_used == 0) && (Current_Cam_Para.Para.Channel_ID == 0xFF))
        {
            cam_channal_used = cam_channal_check;
        }
        Current_Cam_Para.State = CAM_NONE;
        if( Current_Cam_Para.Para.cb_response_cam_end != RT_NULL )  ///���õ�����Ƭ���ճɹ��ص�����
        {
            Current_Cam_Para.Para.cb_response_cam_end( &Current_Cam_Para.Para );
        }
        else  ///Ĭ�ϵĻص�����
        {
            Cam_response_end( &Current_Cam_Para.Para );
        }
        break;
    default:
        Current_Cam_Para.State = CAM_NONE;
    }
    return 1;
}

/*********************************************************************************
  *��������:void Cam_takepic_ex(u16 id,u16 num,u16 space,u8 send,Style_Cam_Requset_Para trige)
  *��������:	������������
  *��	��:	id		:�����ID��ΧΪ1-15
   num		:��������
   space	:���ռ������λΪ��
   save	:�Ƿ񱣴�ͼƬ��FLASH��
   send	:������Ƿ��ϴ���1��ʾ�ϴ�
   trige	:���մ���Դ����Ϊ	Style_Cam_Requset_Para
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_takepic_ex( u16 id, u16 num, u16 space, u8 save, u8 send, CAM_TRIGGER trige )
{
    Style_Cam_Requset_Para tempPara;
    if( trige > Cam_TRIGGER_OTHER )
    {
        trige = Cam_TRIGGER_OTHER;
    }
    memset( &tempPara, 0, sizeof( tempPara ) );
    tempPara.Channel_ID		= id;
    tempPara.PhotoSpace		= space * RT_TICK_PER_SECOND;
    tempPara.PhotoTotal		= num;
    tempPara.SavePhoto		= save;
    tempPara.SendPhoto		= send;
    tempPara.TiggerStyle	= trige;
    take_pic_request( &tempPara );
}

FINSH_FUNCTION_EXPORT( Cam_takepic_ex, para_id_num_space_save_send_trige );


/*********************************************************************************
  *��������:void Cam_takepic(u16 id,u8 send,Style_Cam_Requset_Para trige)
  *��������:	������������
  *��	��:	id		:�����ID��ΧΪ1-15
   save	:�Ƿ񱣴�ͼƬ��FLASH��
   send	:������Ƿ��ϴ���1��ʾ�ϴ�
   trige	:���մ���Դ����Ϊ	Style_Cam_Requset_Para
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_takepic( u16 chn_id, u8 save, u8 send, CAM_TRIGGER trige )
{
    Cam_takepic_ex( chn_id, 1, 0, save, send, trige );
}
FINSH_FUNCTION_EXPORT( Cam_takepic, take_pic );


/*********************************************************************************
  *��������:void Cam_get_All_pic(void)
  *��������:	��ȡͼƬ���ݣ���ӡ�����Դ���
  *��	��:	none
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_get_All_pic( void )
{
    Cam_Flash_InitPara( 1 );
}

FINSH_FUNCTION_EXPORT( Cam_get_All_pic, Cam_get_All_pic );


/*********************************************************************************
  *��������:void Cam_get_state(void)
  *��������:��ȡ��ǰ���յ�״̬
  *��	��:	none
  *��	��:	none
  *�� �� ֵ:uint8_t,0.��ʾû������,1.��ʾ�������յȴ���,2��ʾ��������
  *��	��:������
  *��������:2014-02-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t Cam_get_state(void)
{
    if(Current_Cam_Para.State == CAM_NONE )
        return 0;
    else if(Current_Cam_Para.State == CAM_IDLE )
        return 1;
    else
        return 2;
}

/************************************** The End Of File **************************************/
