#include <finsh.h>
#include <rtdevice.h>

#include "include.h"
#include "jt808.h"
#include "msglist.h"
#include "jt808_sprintf.h"
#include "sst25.h"
#include "usbh_usr.h"
#include "rtc.h"
#include "jt808_gps.h"
#include "jt808_util.h"
#include "jt808_param.h"
#include "GB_vdr.h"



#include "common.h"
#include "m66.h"
#include "jt808_GB19056.h"
#include "Jt808_config.h"

#include "usart.h"
#include "board.h"
#include <serial.h>
#include "Vuart.h"
#include "include.h"
#include "jt808_vehicle.h"
#include "menu_include.h"
#include "SLE4442.h"

#include "Usbh_conf.h"

#include "ffconf.h"
#include "ff.h"
#include "SPI_SD_driver.h"
#include <finsh.h>
#include "diskio.h"


//#include <dfs.h>
//#include <dfs_posix.h>



//#include <stm32f2xx_usart.h>



#define   true     1
#define   false    0

#define    GB_VDR_MAX_1_PKG_SIZE        700     //  VDR ��������ֽ���


//  һ.  ͣ��ǰ15���ӣ�ÿ����ƽ���ٶ�
#ifdef AVRG15MIN
//   ͣ��ǰ15  ���ӣ�ÿ���ӵ�ƽ���ٶ�
AVRG15_SPD  Avrg_15minSpd;
#endif

// ��.  ���ͼ�¼���������

VDR_JT808_SEND		VDR_send;                    // VDR


// ��.  ��¼������ִ�����

#define   OUT_PUT        SERIAL_OUTput(type)
#define   USB_OUT        USB_OUTput(CMD,add_fcs_byte)



//    GB  ���ݵ���
#define GB_USB_OUT_idle           0
#define GB_USB_OUT_start          1
#define GB_USB_OUT_running        2
#define GB_USB_OUT_end            3

#define GB_OUT_TYPE_Serial         1
#define GB_OUT_TYPE_USB            2


struct rt_semaphore GB_RX_sem;  //  gb  ����AA 75     �ŵ�startup .c

GB_STRKT  GB19056;
static FIL 	file;

rt_device_t   Udisk_dev = RT_NULL;

//----------  �¹��ɵ�����
DOUBT_TYPE  Sensor_buf[200];// 20s ״̬��¼
u8      save_sensorCounter = 0, sensor_writeOverFlag = 0;;
u16     Speed_cacu_BAK = 0; //   ��ʱ�洢���ݴ������ٶ�
u8      Speed_cacu_Trigger_Flag = 0; // �¹��ɵ�1 ���� ״̬


//  ʹ�ܲ���״̬
u8    Warn_Play_controlBit = 0x16;
/*
                                      ʹ�ܲ���״̬����BIT
                                    BIT   0:    ��ʻԱδ��¼״̬��ʾ(����Ĭ��ȡ��)
                                    BIT   1:    ��ʱԤ��
                                    BIT   2:    ��ʱ����
                                    BIT   3:    �ٶ��쳣����(����Ĭ��ȡ��)
                                    BIT   4:    ���ٱ���
                                    BIT   5:
                             */
//  ��.1    ���ݵ�����ر���


int  usb_fd = 0;
static  u8 usb_outbuf[1500];
static u16 usb_outbuf_Wr = 0;

static u8  Sps_larger_5_counter = 0;
static u8  gb_100ms_time_counter = 0; //
u8 gb_BCD_datetime[6];


//   ��.    GB_IC  �����

u8   gb_IC_loginState = 0; //  IC  ��������ȷʶ��:   1    ����   0

// -----IC  ����Ϣ���---
DRV_INFO    Read_ICinfo_Reg;   // ��ʱ��ȡ��IC ����Ϣ
DRIVE_STRUCT     Drivers_struct[MAX_DriverIC_num]; // Ԥ��5 ����ʻԱ�Ĳ忨�Ա�

//  ��. ��չ����  ����
u8 Limit_max_SateFlag = 0; //   ���� ����ٶ�
/* ��ʱ���Ŀ��ƿ� */
u8  gb_time_counter = 0;
u16  jia_speed = 0;





//   Head  define
void  GB_OUT_data(u8  ID, u8 type, u16 IndexNum, u16 total_pkg_num);
void  GB_ExceedSpd_warn(void);
void  GB_IC_Exceed_Drive_Check(void);    //  ��ʱ��ʻ���
void  GB_SpeedSatus_Judge(void);
u16   GB_557A_protocol_00_07_stuff(u8 *dest, u8 ID);



//--------------------------------------------------------
static uint8_t fcs_caulate(uint8_t *dst, uint16_t len)
{
    uint8_t  fcs = 0;
    uint16_t i = 0;

    for(i = 0; i < len; i++)
    {
        fcs ^= dst[i];
    }
    return  fcs;
}
/*
               ����GB19056  �������
*/
static Send_gb_vdr_variable_init(void)
{
    VDR_send.Float_ID = 0;	 //  ������ˮ��
    //-------  ��¼���б��ش�---
    VDR_send.RSD_State = 0; 	//	�ش�״̬   0 : �ش�û������   1 :  �ش���ʼ    2  : ��ʾ˳���굫�ǻ�û�յ����ĵ��ش�����
    VDR_send.RSD_Reader = 0;	//	�ش���������ǰ��ֵ
    VDR_send.RSD_total = 0; 	//	�ش�ѡ����Ŀ

    VDR_send.Total_pkt_num = 0; // �ְ��ܰ���
    VDR_send.Current_pkt_num = 0;
    VDR_send.Read_indexNum = 0; // ���Ͷ�ȡʱ��ʹ��
    VDR_send.CMD = 0;	 //  ���ݲɼ�
}

static u16 Send_gb_vdr_08to15_stuff( JT808_TX_NODEDATA *nodedata)
{
    JT808_TX_NODEDATA		 *iterdata	= nodedata;
    //VDR_JT808_SEND	* p_user= (VDR_JT808_SEND*)nodedata->user_para;
    uint8_t				    *pdata;
    uint16_t  i = 0, wrlen = 0;
    uint16_t    SregLen = 0, Swr = 0;
    uint16_t	body_len;   /*��Ϣ�峤��*/
    uint16_t    stuff_len = 0;


    // step 0 :  init
    pdata = nodedata->tag_data;  // ������ݵ���ʼָ��

    //	step1 :  ��Ϣ���
    pdata[0]	= 0x07; 					//	ID
    pdata[1]	= 0x00;

    //	��Ϣ�峤�������д

    DF_Sem_Take;
    // step2:	���µ�ǰ�����ͷ���ID ��ˮ��

    switch(VDR_send.CMD) 							  //  ��Ϣ��ˮ�ź���
    {




    case 0x08:
        iterdata->head_sn = 0x8000 + i;
        break;
    case 0x09:
        iterdata->head_sn = 0x9000 + i;
        break;
    case 0x10:
        iterdata->head_sn = 0xA000 + i;
        break;
    case 0x11:
        iterdata->head_sn = 0xB000 + i;
        break;
    case 0x12:
        iterdata->head_sn = 0xC000 + i;
        break;
    case  0x15:
        iterdata->head_sn = 0xD000 + i;
        break;
    }
    pdata[10]			= ( iterdata->head_sn >> 8 );  //
    pdata[11]			= ( iterdata->head_sn & 0xFF );

    //step3:   ��� VDR ����
    wrlen = 16;					/*������Ϣͷ*/
    //--------------------------------------------------------------------------------------------
    switch(VDR_send.CMD)
    {
    case 0x08:															  //  08   �ɼ�ָ������ʻ�ٶȼ�¼
        if( iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // ������
        }

        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // ��ʼͷ
        pdata[wrlen++]   = 0x7A;
        pdata[wrlen++]   = 0x08;					  //������

        //  ����
#ifdef JT808_TEST_JTB
        SregLen							  = 504;//504;//630;					  // ��Ϣ����		630
#else
        if(Vdr_Wr_Rd_Offset.V_08H_Write > 5)
            SregLen = 630;					 // ��Ϣ����		630
        else
            SregLen = Vdr_Wr_Rd_Offset.V_08H_Write * 126;
#endif

        pdata[wrlen++]   = SregLen >> 8;			  // Hi
        pdata[wrlen++]   = SregLen;				  // Lo

        pdata[wrlen++] = 0x00;					  // ������
        //  ��Ϣ����

#ifdef JT808_TEST_JTB
        if(jt808_param.id_0xF041 == 1)
        {
            gb_get_08h( pdata + wrlen, VDR_send.Current_pkt_num);			  //126*5=630		  num=576  packet
            wrlen += 504; //630;
        }
#else

        if(Vdr_Wr_Rd_Offset.V_08H_Write == 0)
        {
            wrlen += stuff_len;
            break;
        }

        if(Vdr_Wr_Rd_Offset.V_08H_Write > 5)
        {
            stuff_len = stuff_drvData(0x08, Vdr_Wr_Rd_Offset.V_08H_Write - VDR_send.Read_indexNum, 5, pdata + wrlen);
            VDR_send.Read_indexNum += 5;
        }
        else
        {
            stuff_len = stuff_drvData(0x08, Vdr_Wr_Rd_Offset.V_08H_Write, Vdr_Wr_Rd_Offset.V_08H_Write, pdata + wrlen);
            VDR_send.Read_indexNum += Vdr_Wr_Rd_Offset.V_08H_Write;
        }

        wrlen += stuff_len;
#endif
        break;






        //----------------------------------------------------------------------------------------------------------

    case	 0x09:															  // 09   ָ����λ����Ϣ��¼
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // ������
        }
        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // ��ʼͷ
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x09;					  //������

        //  ����
#ifdef JT808_TEST_JTB
        SregLen							  = 666;//666 ; 				 // ��Ϣ����
#else
        if(Vdr_Wr_Rd_Offset.V_09H_Write > 0)
            SregLen								= 666;	//666 * 2;					// ��Ϣ����
        else
            SregLen = 0;
#endif
        pdata[wrlen++]   = SregLen >> 8;			  // Hi 	 666=0x29A
        pdata[wrlen++]   = SregLen;				  // Lo

        //pdata[wrlen++]=0;	// Hi	   666=0x29A
        //pdata[wrlen++]=0;	 // Lo

        pdata[wrlen++] = 0x00;					  // ������
        //  ��Ϣ����
#ifdef JT808_TEST_JTB
        gb_get_09h( pdata + wrlen, VDR_send.Current_pkt_num);
        wrlen += 666;
#else
        if(Vdr_Wr_Rd_Offset.V_09H_Write > 0)
        {
            stuff_len = stuff_drvData(0x09, Vdr_Wr_Rd_Offset.V_09H_Write - VDR_send.Read_indexNum, 1, pdata + wrlen);
            wrlen += stuff_len;
            VDR_send.Read_indexNum++;
        }

#endif
        break;




        //----------------------------------------------------------------------------------------------------------

    case	 0x10:															  // 10-13	   10	�¹��ɵ�ɼ���¼
        //�¹��ɵ�����
        if( iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // ������
        }
        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // ��ʼͷ
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x10;					  //������

#ifdef JT808_TEST_JTB
        SregLen							  = 234;				//0 	   // ��Ϣ����
#else
        if(Vdr_Wr_Rd_Offset.V_10H_Write > 0)
            SregLen					= 234 ;//*100;				  //0		 // ��Ϣ����
        else
            SregLen = 0;
#endif
        pdata[wrlen++]   = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]   = (u8)SregLen;			  // Lo

        pdata[wrlen++] = 0x00;					  // ������

        //------- ��Ϣ���� ------
#ifdef JT808_TEST_JTB
        gb_get_10h( pdata + wrlen );						  //234  packetsize 	 num=100
        wrlen += 234;
#else
        if(Vdr_Wr_Rd_Offset.V_10H_Write > 0)
        {
            stuff_len = stuff_drvData(0x10, Vdr_Wr_Rd_Offset.V_10H_Write - VDR_send.Read_indexNum, 1, pdata + wrlen);
            wrlen += stuff_len;
            VDR_send.Read_indexNum++;
        }
#endif

        break;






        //----------------------------------------------------------------------------------------------------------
    case	0x11:															  // 11 �ɼ�ָ���ĵĳ�ʱ��ʻ��¼
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // ������
        }

        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // ��ʼͷ
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x11;					  //������

#ifdef JT808_TEST_JTB
        SregLen							  = 50; 				// ��Ϣ����
#else
        if(Vdr_Wr_Rd_Offset.V_11H_Write == 0)
            break;

        if(Vdr_Wr_Rd_Offset.V_11H_Write > 10)
            SregLen = 500;					 // ��Ϣ����	   50*10
        else
            SregLen = Vdr_Wr_Rd_Offset.V_11H_Write * 50;
#endif

        pdata[wrlen++]   = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]   = (u8)SregLen;			  // Lo    65x7

        pdata[wrlen++] = 0x00;					  // ������


        /*
        ÿ�� 50 bytes	��100 ��	��ȡ����ÿ10 ����һ��  500 packet	 Totalnum=10
        */
#ifdef JT808_TEST_JTB
        gb_get_11h( pdata + wrlen, VDR_send.Current_pkt_num);							 //50  packetsize	   num=100
        wrlen += 50;
#else


#endif

        break;





        //----------------------------------------------------------------------------------------------------------

    case	0x12:															  // 12 �ɼ�ָ����ʻ����ݼ�¼	---Devide
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // ������

        }
        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // ��ʼͷ
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x12;					  //������

#ifdef JT808_TEST_JTB
        SregLen							  = 500;				 // ��Ϣ����
#else
        if(Vdr_Wr_Rd_Offset.V_12H_Write > 10)
            SregLen = 250;					  // ��Ϣ����		50*10
        else
            SregLen = Vdr_Wr_Rd_Offset.V_12H_Write * 25;
#endif
        pdata[wrlen++]   = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]   = (u8)SregLen;			  // Lo    65x7

        pdata[wrlen++] = 0x00;					  // ������
        //------- ��Ϣ���� ------


        /*
        ��ʻԱ��ݵ�¼��¼  ÿ��25 bytes		200��	   20��һ�� 500 packetsize	totalnum=10
        */
#ifdef JT808_TEST_JTB
        gb_get_12h( pdata + wrlen, VDR_send.Current_pkt_num);
        wrlen += 500;
#else
        if(Vdr_Wr_Rd_Offset.V_12H_Write == 0)
        {
            wrlen += stuff_len;
            break;
        }

        if(Vdr_Wr_Rd_Offset.V_12H_Write > 10)
        {
            stuff_len = stuff_drvData(0x12, Vdr_Wr_Rd_Offset.V_12H_Write - VDR_send.Read_indexNum, 10, pdata + wrlen);
            VDR_send.Read_indexNum += 10;
        }
        else
        {
            stuff_len = stuff_drvData(0x12, Vdr_Wr_Rd_Offset.V_12H_Write, Vdr_Wr_Rd_Offset.V_12H_Write, pdata + wrlen);
            VDR_send.Read_indexNum += Vdr_Wr_Rd_Offset.V_12H_Write;
        }

        wrlen += stuff_len;
#endif
        break;
        //----------------------------------------------------------------------------------------------------
#ifndef JT808_TEST_JTB


    case	0x13:	   //  �ⲿ�����¼      7bytes
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]	 = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]	 = (u8)VDR_send.Float_ID;
            pdata[wrlen++]	 = VDR_send.CMD;		  // ������

        }
        Swr = wrlen;
        pdata[wrlen++]	 = 0x55;					  // ��ʼͷ
        pdata[wrlen++]	 = 0x7A;

        pdata[wrlen++] = 0x13;					  //������

        SregLen = Vdr_Wr_Rd_Offset.V_13H_Write * 7;
        pdata[wrlen++]	 = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]	 = (u8)SregLen; 		  // Lo    65x7

        pdata[wrlen++] = 0x00;					  // ������
        //------- ��Ϣ���� ------


        /*
        �ⲿ�����¼      7bytes		100��
        */
        if(Vdr_Wr_Rd_Offset.V_13H_Write == 0)
        {
            wrlen += stuff_len;
            break;
        }
        stuff_len = stuff_drvData(0x13, Vdr_Wr_Rd_Offset.V_13H_Write, Vdr_Wr_Rd_Offset.V_13H_Write, pdata + wrlen);
        wrlen += stuff_len;
        break;


        //----------------------------------------------------------------------------------------------------------
    case	0x14:	   //��¼�ǲ����޸ļ�¼      7bytes
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]	 = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]	 = (u8)VDR_send.Float_ID;
            pdata[wrlen++]	 = VDR_send.CMD;		  // ������

        }
        Swr = wrlen;
        pdata[wrlen++]	 = 0x55;					  // ��ʼͷ
        pdata[wrlen++]	 = 0x7A;

        pdata[wrlen++] = 0x14;					  //������

        SregLen = Vdr_Wr_Rd_Offset.V_14H_Write * 7;
        pdata[wrlen++]	 = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]	 = (u8)SregLen; 		  // Lo    65x7

        pdata[wrlen++] = 0x00;					  // ������
        //------- ��Ϣ���� ------


        /*
        �ⲿ�����¼      7bytes		100��
        */
        if(Vdr_Wr_Rd_Offset.V_14H_Write == 0)
        {
            wrlen += stuff_len;
            break;
        }
        stuff_len = stuff_drvData(0x14, Vdr_Wr_Rd_Offset.V_14H_Write, Vdr_Wr_Rd_Offset.V_14H_Write, pdata + wrlen);
        wrlen += stuff_len;
        break;
#endif




        //----------------------------------------------------------------------------------------------------------

    case	   0x15:													  // 15 �ɼ�ָ�����ٶ�״̬��־	   --------Divde
        if( iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;	 // ������
        }
        Swr = wrlen;
        pdata[wrlen++]   = 0x55;				  // ��ʼͷ
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x15;				  //������

#ifdef JT808_TEST_JTB

        SregLen							  = 133 ;			  // ��Ϣ����
#else

        if(Vdr_Wr_Rd_Offset.V_15H_Write > 2)
            SregLen = 266; 					 // ��Ϣ����	  50*10
        else
            SregLen = Vdr_Wr_Rd_Offset.V_15H_Write * 133;
#endif

        pdata[wrlen++]   = (u8)( SregLen >> 8 ); // Hi
        pdata[wrlen++]   = (u8)SregLen;		  // Lo    65x7

        pdata[wrlen++] = 0x00;				  // ������


        /*
        ÿ�� 133 ���ֽ�   10 ��	1��������	  5*133=665 	   totalnum=2
        */
#ifdef JT808_TEST_JTB
        gb_get_15h( pdata + wrlen, VDR_send.Current_pkt_num);
        wrlen += 133;
#else

        if(Vdr_Wr_Rd_Offset.V_15H_Write == 0)
        {
            wrlen += stuff_len;
            break;
        }

        if(Vdr_Wr_Rd_Offset.V_15H_Write > 2)
        {
            stuff_len = stuff_drvData(0x15, Vdr_Wr_Rd_Offset.V_15H_Write - VDR_send.Read_indexNum, 2, pdata + wrlen);
            VDR_send.Read_indexNum += 2;
        }
        else
        {
            stuff_len = stuff_drvData(0x15, Vdr_Wr_Rd_Offset.V_15H_Write, Vdr_Wr_Rd_Offset.V_15H_Write, pdata + wrlen);
            VDR_send.Read_indexNum += Vdr_Wr_Rd_Offset.V_15H_Write;

        }
        wrlen += stuff_len;
#endif
        break;

    }

    DF_Sem_Release;
    //-----------------------------------------------
    pdata[wrlen++] = fcs_caulate(pdata + Swr, wrlen - Swr); // ����¼��У����ֵ
    //------------------------------------------------
    iterdata->msg_len	= wrlen;	   ///�������ݵĳ��ȼ�����Ϣͷ�ĳ���

    //	��Ϣ�峤�������д
    body_len = wrlen - 16;
    pdata[2]	= 0x20 | ( body_len >> 8 ); /*��Ϣ�峤��*/
    pdata[3]	= body_len & 0xff;
    //-------------------------------------------------------------------------------------------
    /*�������� ������Ϣͷ*/
    pdata[12]			= ( iterdata->packet_num >> 8 );
    pdata[13]			= ( iterdata->packet_num & 0xFF );
    pdata[14]			= ( iterdata->packet_no >> 8 );
    pdata[15]			= ( iterdata->packet_no & 0xFF );

    return  true;
}


static u16 Send_gb_vdr_tx_data( JT808_TX_NODEDATA *nodedata)
{
    JT808_TX_NODEDATA		 *iterdata	= nodedata;
    //VDR_JT808_SEND	* p_user= (VDR_JT808_SEND*)nodedata->user_para;
    uint32_t				tempu32data;
    u16						ret = 0;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

    // �ж��Ƿ����б��ش�
    if((VDR_send.RSD_total) && (VDR_send.RSD_State == 1)) // �ж��Ƿ����б�
    {
        if(VDR_send.RSD_Reader < VDR_send.RSD_total)
        {
            //˳���ȡ�б��ش�����ߵ����
            iterdata->packet_no = VDR_send.ReSdList[VDR_send.RSD_Reader];
            Send_gb_vdr_08to15_stuff(nodedata);
            nodedata->state		= IDLE;
            nodedata->retry		= 0;
            nodedata->max_retry = 1;
            nodedata->timeout	=  RT_TICK_PER_SECOND; // timeout 500  ms
            //  setp1:   ����һ���ۼ�һ������
            VDR_send.RSD_Reader++;
            if(VDR_send.RSD_Reader == VDR_send.RSD_total)
            {
                nodedata->timeout	=  RT_TICK_PER_SECOND * 10; // timeout 500  ms
                nodedata->max_retry = 3;
            }
            rt_kprintf( "\n			 	Resend list cmd=0x0%02X  pkgnum=%d   current=%d  total=%d \n", VDR_send.CMD, VDR_send.ReSdList[VDR_send.RSD_Reader - 1], VDR_send.RSD_Reader, VDR_send.RSD_total );
            ret = iterdata->packet_no;
            goto GB_FUNC_RET;
        }
        else
            VDR_send.RSD_State = 2; // �б��ش��Ѿ�������ϵȴ�Ӧ��
    }
    else
    {
        if(VDR_send.Current_pkt_num < VDR_send.Total_pkt_num)
        {
            iterdata->packet_no = VDR_send.Current_pkt_num + 1; //  ��ǰ����� ����1	��ʼ
            Send_gb_vdr_08to15_stuff(nodedata);
            nodedata->state		= IDLE;
            nodedata->retry		= 0;
            nodedata->max_retry = 1;
            nodedata->timeout	=  RT_TICK_PER_SECOND; // timeout 500  ms
            //  setp1:   ����һ���ۼ�һ������
            VDR_send.Current_pkt_num++;
            if(VDR_send.Current_pkt_num == VDR_send.Total_pkt_num)
            {
                nodedata->timeout	=  RT_TICK_PER_SECOND * 10; // timeout 500  ms
                nodedata->max_retry = 3;
            }
            rt_kprintf( "\n					cmd=0x0%02X  current=%d  total=%d \n", VDR_send.CMD, iterdata->packet_no, iterdata->packet_num );
            ret = iterdata->packet_no;
            goto GB_FUNC_RET;
        }
    }
    ret = 0;
GB_FUNC_RET:
    rt_sem_release( &sem_dataflash );
    return ret;
}


static JT808_MSG_STATE Send_gb_vdr_jt808_timeout( JT808_TX_NODEDATA *nodedata )
{
#if 1
    u16					cmd_id;
    //VDR_JT808_SEND  * p_para;
    uint16_t			ret;

    cmd_id	= nodedata->head_id;
    //p_para	= (VDR_JT808_SEND*)( nodedata->user_para );
    switch( cmd_id )
    {
    case 0x0700:                                            /*��ʱ�Ժ�ֱ���ϱ�����*/
        /*
        if( nodedata->packet_no == nodedata->packet_num )   ///���ϱ����ˣ�����ʱ
        {
        rt_kprintf( "\nȫ���ϱ����\n" );               ///�ȴ�Ӧ��0x8800
        return nodedata->state = ACK_OK;
        }
        ret = Cam_add_tx_pic_getdata( nodedata );
        if( ret == 0xFFFF )                                 ///bitter û���ҵ�ͼƬid
        {
        rt_kprintf( "\nȫ���ϱ����\n" );               ///�ȴ�Ӧ��0x8800
        return nodedata->state = ACK_OK;
        }
        */
        ret = Send_gb_vdr_tx_data( nodedata);
        if(( ret == 0xFFFF ) || (ret == 0 ))
        {
            Send_gb_vdr_variable_init();
            rt_kprintf( "\nȫ���ϱ����\n" );               ///�ȴ�Ӧ��0x8800
            nodedata->state = ACK_OK;
            return  ACK_OK;
        }
        break;
    default:
        break;
    }
    return IDLE;
#endif
}

static JT808_MSG_STATE Send_gb_vdr_jt808_0x0700_response( JT808_TX_NODEDATA *nodedata, uint8_t *pmsg )
{
#if  1
    JT808_TX_NODEDATA		 *iterdata = nodedata;
    //VDR_JT808_SEND 	* p_user;
    uint16_t				temp_msg_id;
    //	uint16_t				temp_msg_len;
    uint16_t				i;                              //, pack_num;
    uint32_t				tempu32data;
    uint16_t				ret;
    uint16_t				ack_seq;                        /*Ӧ�����*/
    u16						fram_num;

    // step 0:    get  msg_ID
    temp_msg_id = buf_to_data( pmsg, 2 );
    fram_num	= buf_to_data( pmsg + 10, 2 );

    // step 1:      ͨ��Ӧ��
    if( 0x8001 == temp_msg_id )                             ///ͨ��Ӧ��,�п���Ӧ����ʱ
    {
#if 0
        ack_seq = ( pmsg[12] << 8 ) | pmsg[13];             /*�ж�Ӧ����ˮ��*/
        if( ack_seq != nodedata->head_sn )                  /*��ˮ�Ŷ�Ӧ*/
        {
            nodedata->timeout	= 30 * RT_TICK_PER_SECOND;  /*��30��*/
            nodedata->state		= WAIT_ACK;
            return WAIT_ACK;
        }
#endif
        //ret=Send_gb_vdr_tx_data(iterdata);

        return nodedata->state;
    }
    else
        //  step 2:     �ְ��ش��б�Ӧ��
        if( 0x8003 == temp_msg_id ) 					///ר��Ӧ��
        {
            debug_write("�յ�����Ӧ��8003");

            VDR_send.Float_ID = buf_to_data( pmsg + 12, 2 );	/*ԭʼ��Ϣ��ˮ��*/


            if( pmsg[14] == 0 ) /*�ش�������*/
            {
                //  recover  state
                rt_kprintf("\n  0x%02X��¼�������ϱ����!", VDR_send.CMD);
                Send_gb_vdr_variable_init();
                nodedata->state = ACK_OK;
                jt808_tx_0x0001( fram_num, 0x8003, 0 );
                return ACK_OK;
            }
            else
            {
                // step 2.1:      �б��ش�
                // p_user = (VDR_JT808_SEND*)( iterdata->user_para );
                memset(VDR_send.ReSdList, 0, sizeof(VDR_send.ReSdList));
                // step 2.2:      �б��ش����״̬��ʼ��
                VDR_send.RSD_total = pmsg[14]; // �洢�б��ش��ܰ���
                VDR_send.RSD_Reader = 0; //
                VDR_send.RSD_State = 1; //   ��ʼ�б��ش�


                //
#if  1
                rt_kprintf( "\n ��¼���б��ش�����Ŀ=%d \n ", pmsg[14] );   /*���»�ȡ��������*/
                for( i = 0; i < pmsg[14]; i++ )   // �洢�б��ش���
                {
                    VDR_send.ReSdList[i] = buf_to_data( pmsg + i * 2 + 15, 2 );
                    rt_kprintf( "%d ", VDR_send.ReSdList[i]);
                }
                rt_kprintf( "\n ");

                ret = Send_gb_vdr_tx_data(nodedata);
                if(( ret == 0xFFFF ) || (ret == 0 ))  												/*bitter û���ҵ�ͼƬid*/
                {
                    //rt_free( p_para );
                    //p_para = RT_NULL;
                    nodedata->state = ACK_OK;
                    return ACK_OK;
                }
#endif
            }
            nodedata->state = IDLE;
            return IDLE;
        }

    return nodedata->state;
#endif
}

rt_err_t Send_gb_vdr_jt808_0x0700_creat(uint8_t linkno)
{
    //    1. �����ڵ㣬��ʼ���ڵ㣬�������ݷְ��ϴ�VDR      ����
#if 1
    u32						TempAddress;
    uint16_t				ret;
    JT808_TX_NODEDATA		 *pnodedata;

    // setp 1:   begin
    pnodedata = node_begin( linkno, MULTI_CMD, 0x0700, 0x8001, 800); // GB_VDR_MAX_1_PKG_SIZE);
    if( pnodedata == RT_NULL )
    {
        rt_kprintf( "\n�����¼����Դʧ��" );
        return RT_ENOMEM;
    }
    // step 2:  stuff  init info
    //pnodedata->size			= TempPackageHead.Len - 64 + 36; // ���ֶ���VDR  ������û��ʲô����
    pnodedata->linkno = linkno;
    pnodedata->packet_num	= VDR_send.Total_pkt_num; // �ܰ���
    pnodedata->packet_no	= 0;
    pnodedata->user_para	= RT_NULL;

    ret = Send_gb_vdr_tx_data( pnodedata);
    if(ret && (ret != 0xFFFF))
    {
        pnodedata->retry		= 0;
        pnodedata->max_retry = 1;
        pnodedata->timeout	=  RT_TICK_PER_SECOND;    //  800ms  ����һ��
        node_end( MULTI_CMD_NEXT, pnodedata, Send_gb_vdr_jt808_timeout, Send_gb_vdr_jt808_0x0700_response, RT_NULL );
    }
    else
    {
        rt_free( pnodedata );
        rt_kprintf( "\nû���ҵ���¼������cmd=0x%02X:%s", VDR_send.CMD, __func__ );
    }
    return RT_EOK;
#endif
}

static u16 Send_gb_vdr_stuffinfo(uint16_t seq, uint8_t cmd)
{

    uint8_t	  buf[150];
    uint8_t	  tmpbuf[32];

    MYTIME	  start, end;
    uint32_t	  i;
    uint8_t	  fcs;
    uint8_t	  Swr = 0;

    //   step2:	�ж� ��¼�������֣�ִ�з�����Ϣ�����
    switch( cmd )
    {
    case 0x00: /*�ɼ���¼��ִ�б�׼�汾*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;

        buf[Swr++]  = 0x55;			   // ��ʼͷ
        buf[Swr++]  = 0x7A;
        buf[Swr++]  = 0x00;			   //������
        buf[Swr++]  = 0x00;			   // Hi
        buf[Swr++]  = 2; 			   // Lo

        buf[Swr++]  = 0x00;			   // ������
        buf[Swr++]  = 0x12;			   //  12  ���׼
        buf[Swr++]  = 0x00;
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );

        break;
    case 0x01: //  01  ��ǰ��ʻԱ���뼰��Ӧ�Ļ�������ʻ֤��
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;

        buf[Swr++]  = 0x55;						   // ��ʼͷ
        buf[Swr++]  = 0x7A;
        buf[Swr++]  = 0x01;						   //������

        buf[Swr++]  = 0x00;						   // Hi
        buf[Swr++]  = 18;						   // Lo

        buf[Swr++] = 0x00;							// ������

        memcpy( buf + Swr, "410727198503294436", 18 );  //��Ϣ����
        Swr				   += 18;

        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;
    case 0x02: /*�г���¼��ʱ��*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;

        buf[Swr++]  = 0x55;						   // ��ʼͷ
        buf[Swr++]  = 0x7A;
        buf[Swr++]  = 0x02;						   //������

        buf[Swr++]  = 0x00;						   // Hi
        buf[Swr++]  = 6; 						   // Lo

        buf[Swr++] = 0x00;							// ������
        memcpy(buf + Swr, gps_baseinfo.datetime, 6);
        Swr += 6;
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;
    case 0x03:		// 03 �ɼ�360h�����
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;


        buf[Swr++]  = 0x55;						   // ��ʼͷ
        buf[Swr++]  = 0x7A;
        buf[Swr++]  = 0x03;						   //������

        buf[Swr++]  = 0x00;						   // Hi
        buf[Swr++]  = 20;						   // Lo

        buf[Swr++] = 0x00;							// ������


        mytime_to_bcd( tmpbuf, mytime_now ); 		   /*ʵʱʱ��*/
        memcpy( buf + Swr, tmpbuf, 6 );
        Swr += 6;
        mytime_to_bcd( tmpbuf, jt808_param.id_0xF030 ); /*�״ΰ�װʱ��*/
        memcpy( buf + 15, tmpbuf, 6);
        Swr += 6;
        tmpbuf[0]   = 0x00;
        tmpbuf[1]   = 0x00;
        tmpbuf[2]   = 0x00;
        tmpbuf[3]   = 0x00;
        memcpy( buf + Swr, tmpbuf, 4);
        Swr += 4;
        i		   = jt808_data.id_0xFA01 / 100;		 /*����� 0.1KM BCD�� 00-99999999*/
        tmpbuf[0]   = ( ( ( i / 10000000 ) % 10 ) << 4 ) | ( ( i / 1000000 ) % 10 );
        tmpbuf[1]   = ( ( ( i / 100000 ) % 10 ) << 4 ) | ( ( i / 10000 ) % 10 );
        tmpbuf[2]   = ( ( ( i / 1000 ) % 10 ) << 4 ) | ( ( i / 100 ) % 10 );
        tmpbuf[3]   = ( ( ( i / 10 ) % 10 ) << 4 ) | ( i % 10 );
        memcpy( buf + Swr, tmpbuf, 4);
        Swr += 4;
        //------------------------------------------
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;
    case 0x04: /*����ϵ��*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;


        buf[Swr++]  = 0x55;							   // ��ʼͷ
        buf[Swr++]  = 0x7A;

        buf[Swr++] = 0x04;								//������

        buf[Swr++]  = 0x00;							   // Hi
        buf[Swr++]  = 8; 							   // Lo

        buf[Swr++] = 0x00;								// ������

        //  ��Ϣ����
        mytime_to_bcd( tmpbuf, mytime_now ); 		   /*ʵʱʱ��*/
        memcpy( buf + Swr, tmpbuf, 6 );
        Swr += 6;

        buf[Swr++]  = (u8)( (jt808_param.id_0xF032 & 0x0000FFFF) >> 8 );
        buf[Swr++]  = (u8)( (jt808_param.id_0xF032 & 0x0000FFFF));
        //------------------------------------------
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;
    case 0x05: /*������Ϣ  41byte*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;


        buf[Swr++]  = 0x55;							   // ��ʼͷ
        buf[Swr++]  = 0x7A;

        buf[Swr++] = 0x05;								//������

        buf[Swr++]  = 0x00;			   // Hi
        buf[Swr++]  = (u8)41;					  // Lo 	 65x7

        buf[Swr++] = 0x00;								// ������
        //-----------   ��Ϣ����  --------------
        memcpy( buf + Swr, "LGBK22E70AY100102", 17 );	//��Ϣ����
        Swr += 17;
        memcpy( buf + Swr, jt808_param.id_0x0083, 12);			 // ���ƺ�
        Swr += 12;
        memcpy( buf + Swr, "С�ͳ�", 6 );
        Swr += 6;
        buf[Swr++]  = 0x00;
        buf[Swr++]  = 0x00;
        buf[Swr++]  = 0x00;
        buf[Swr++]  = 0x00;
        buf[Swr++]  = 0x00;
        buf[Swr++]  = 0x00;
        buf[Swr++]  = 0x00;
        //------------------------------------------
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;
    case 0x06:							   /*״̬�ź�������Ϣ 87byte*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;


        //  06 �ź�������Ϣ
        buf[Swr++]  = 0x55;				   // ��ʼͷ
        buf[Swr++]  = 0x7A;

        buf[Swr++] = 0x06;					//������

        // ��Ϣ����
        buf[Swr++]  = 0x00; // Hi
        buf[Swr++]  = 87;		  // Lo

        buf[Swr++] = 0x00;					// ������

        mytime_to_bcd( tmpbuf, mytime_now ); 		   /*ʵʱʱ��*/
        memcpy( buf + Swr, tmpbuf, 6 );
        Swr += 6;
        //-------  ״̬�ָ���----------------------
        buf[Swr++] = 0;// 8//�޸�Ϊ0
        //---------- ״̬������-------------------
        /*
        -------------------------------------------------------------
        F4  �г���¼�� TW705	�ܽŶ���
        -------------------------------------------------------------
        ��ѭ	GB10956 (2012)	Page26	��A.12	�涨
        -------------------------------------------------------------
        | Bit  |	  Note		 |	�ر�|	MCUpin	|	PCB pin  |	 Colour | ADC
        ------------------------------------------------------------
        D7	   ɲ�� 		  * 		   PE11 			9				 ��
        D6	   ��ת��	  * 			PE10			10				 ��
        D5	   ��ת��	  * 			PC2 			 8				  ��
        D4	   Զ���	  * 			PC0 			 4				  ��
        D3	   �����	  * 			PC1 			 5				  ��
        D2	   ��� 		 add		  PC3			   7				��		*
        D1	   ���� 		 add		  PA1			   6				��		*
        D0	   Ԥ��
        */
        memcpy( buf + Swr, "Ԥ�� 	 ", 10 );		// D0
        Swr += 10;
        memcpy( buf + Swr, "Ԥ�� 	 ", 10 );		// D1
        Swr += 10;
        memcpy( buf + Swr, "Ԥ�� 	 ", 10 );		// D2
        Swr += 10;
        memcpy( buf + Swr, "�����	 ", 10 );		// D3
        Swr += 10;
        memcpy( buf + Swr, "Զ���	 ", 10 );		// D4
        Swr += 10;
        memcpy( buf + Swr, "��ת��	 ", 10 );		// D5
        Swr += 10;
        memcpy( buf + Swr, "��ת��	 ", 10 );		// D6
        Swr += 10;
        memcpy( buf + Swr, "ɲ�� 	 ", 10 );		// D7
        Swr += 10;

        //------------------------------------------
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;
    case 7:
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	 = 0;
        Swr = 3;
        //------------------------------------------
        buf[Swr++]	= 0x55; 						// ��ʼͷ
        buf[Swr++]	= 0x7A;

        buf[Swr++] = 0x07;							//������

        // ��Ϣ����
        buf[Swr++]	= 0x00; 		// Hi
        buf[Swr++]	= 30;				// Lo

        buf[Swr++] = 0x00;							// ������
        //------- ��Ϣ���� ------
        memcpy( buf + Swr, "C000863", 7 );			// 3C ��֤����
        Swr += 7;
        memcpy( buf + Swr, "TW705", 5 ); // ��Ʒ�ͺ� 16
        Swr 					+= 5;

        for(i = 0; i < 11; i++)
            buf[Swr++]	= 0x00;


        buf[Swr++]	= 0x13; 						// ��������
        buf[Swr++]	= 0x03;
        buf[Swr++]	= 0x01;
        buf[Swr++]	= 0x00; 						// ������ˮ��
        buf[Swr++]	= 0x00;
        buf[Swr++]	= 0x00;
        buf[Swr++]	= 0x35;

        //------------------------------------------
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;

    case	0x08:  // �ɼ���ʻ�ٶȼ�¼
        VDR_send.Total_pkt_num   = 720;
        VDR_send.Current_pkt_num		  = 0;
        VDR_send.Float_ID = seq; //  ����ID

        break;

    case	0x09: // λ����Ϣ��¼
        VDR_send.Total_pkt_num   = 360;  //333----720    666--360
        VDR_send.Current_pkt_num		  = 0; //0
        VDR_send.Float_ID = seq; //  ����ID

        break;
    case	0x10:  //  �¹��ɵ��¼
        VDR_send.Total_pkt_num	= 100;
        VDR_send.Current_pkt_num	= 0;
        VDR_send.Float_ID = seq; //  ����ID

        break;
    case	0x11: //  ��ʱ��ʻ��¼
        VDR_send.Total_pkt_num	= 100;
        VDR_send.Current_pkt_num	= 0;
        VDR_send.Float_ID = seq; //  ����ID

        break;
    case	0x12: //��ʻԱ��ݼ�¼
        VDR_send.Total_pkt_num	= 10;
        VDR_send.Current_pkt_num	= 0;
        VDR_send.Float_ID = seq; //  ����ID

        break;
    case	0x13: //��¼���ⲿ�����¼
        break;
    case	0x14: // ��¼�ǲ����޸ļ�¼
        break;
    case	0x15: // �ٶ�״̬��־

        VDR_send.Total_pkt_num	= 10;
        VDR_send.Current_pkt_num	= 0;
        VDR_send.Float_ID = seq; //  ����ID
        break;
        // sub  step 1 :  �ͳ��������й�
    case 0x82:		   /*���ó�����Ϣ*/
        break;
    case 0x83:		   /*���ó��ΰ�װ����*/
        break;
    case 0x84:		   /*����״̬������*/
        break;

    case 0xC2:		   /*���ü�¼��ʱ��*/
        break;
    case 0xC3:		   /*��������ϵ��*/
        break;
    case 0xC4:		   /*���ó�ʼ���*/
        break;
    }
}



/*�г���¼�����ݲɼ�����*/
void Send_gb_vdr_Rx_8700_ack( uint8_t linkno, uint8_t *pmsg )
{
    // note:     ���������·��� VDR ��ز�����Ϣ
    uint8_t		 *psrc;
    uint8_t		buf[128];
    uint8_t		tmpbuf[16];

    uint16_t	seq = JT808HEAD_SEQ( pmsg );
    uint16_t	len = JT808HEAD_LEN( pmsg );
    uint8_t		cmd = *(pmsg + 12 ); /*����ǰ��12�ֽڵ�ͷ*/
    uint16_t	SregLen;
    uint32_t	i;
    //uint8_t		fcs;
    uint16_t    Swr = 0;

#if  1
    Send_gb_vdr_variable_init();
    //step0 :  get  system ID
    VDR_send.CMD = cmd;
    rt_kprintf("\r\n   Recorder=0x%02X \r\n", VDR_send.CMD);
    //step1 :  0x00~0x0700   ����Ҫ�ְ���ֻ�ظ������������ظ�����
    if(cmd <= 0x07)
        ;  //not add  curretnly
    //step2:   �ж� ��¼�������֣�ִ�з�����Ϣ�����
    switch( cmd )
    {
    case 0x00: /*�ɼ���¼��ִ�б�׼�汾*/
    case 0x01: //  01  ��ǰ��ʻԱ���뼰��Ӧ�Ļ�������ʻ֤��
    case 0x02: /*�г���¼��ʱ��*/
    case 0x03:  // 03 �ɼ�360h�����
    case 0x04: /*����ϵ��*/
    case 0x05: /*������Ϣ  41byte*/
    case 0x06:                              /*״̬�ź�������Ϣ 87byte*/
    case 0x07:
        buf[0]	= seq >> 8;
        buf[1]	= seq & 0xff;
        buf[2]	= cmd;
        Swr = 3;

        // stuff  info
        Swr += GB_557A_protocol_00_07_stuff(buf + 3, cmd);
        //  stuff   fcs
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );
        break;

    case  0x08:	// �ɼ���ʻ�ٶȼ�¼
        if(jt808_param.id_0xF041 == 0)
        {
            // ���ʱΪ 0
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // ��ʼͷ
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //������
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // ������
            // û����Ϣ���
            buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
            jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );

        }
        else
        {
            //  ���������̨
            VDR_send.Current_pkt_num		   = 0;


#ifdef JT808_TEST_JTB
            VDR_send.Total_pkt_num   = 720;
#else
            //    get  total paket
            if(Vdr_Wr_Rd_Offset.V_08H_Write <= 5)
                VDR_send.Total_pkt_num = 1;
            else if(Vdr_Wr_Rd_Offset.V_08H_Write > 5)
            {
                if(Vdr_Wr_Rd_Offset.V_08H_Write % 5)
                    VDR_send.Total_pkt_num = Vdr_Wr_Rd_Offset.V_08H_Write / 5 + 1;
                else
                    VDR_send.Total_pkt_num = Vdr_Wr_Rd_Offset.V_08H_Write / 5;
            }
#endif
            Send_gb_vdr_jt808_0x0700_creat(linkno);
        }
        break;

    case  0x09: // λ����Ϣ��¼
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // ��ʼͷ
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //������
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // ������
            // û����Ϣ���
            buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
            jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );

        }
        else
        {
#ifdef JT808_TEST_JTB
            VDR_send.Total_pkt_num = 360;
#else
            if(Vdr_Wr_Rd_Offset.V_09H_Write <= 0)
                VDR_send.Total_pkt_num = 1;
            else
                VDR_send.Total_pkt_num   = Vdr_Wr_Rd_Offset.V_09H_Write; // 666 bytes  1 packet     360 total
#endif
            VDR_send.Current_pkt_num		   = 0; //0
            Send_gb_vdr_jt808_0x0700_creat(linkno);
        }
        break;
    case  0x10:  //  �¹��ɵ��¼
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // ��ʼͷ
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //������
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // ������
            // û����Ϣ���
            buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
            jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );

        }
        else
        {
#ifdef JT808_TEST_JTB
            VDR_send.Total_pkt_num = 100;
#else
            if(Vdr_Wr_Rd_Offset.V_10H_Write <= 0)
                VDR_send.Total_pkt_num = 1;
            else
                VDR_send.Total_pkt_num    = Vdr_Wr_Rd_Offset.V_10H_Write;  //100;     234  bytes 1  index
#endif
            VDR_send.Current_pkt_num  = 0;
            Send_gb_vdr_jt808_0x0700_creat(linkno);
        }
        break;
    case  0x11: //  ��ʱ��ʻ��¼
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // ��ʼͷ
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //������
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // ������
            // û����Ϣ���
            buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
            jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );

        }
        else
        {
#ifdef JT808_TEST_JTB
            VDR_send.Total_pkt_num = 100;
#else
            // get   total
            if(Vdr_Wr_Rd_Offset.V_11H_Write <= 10)
                VDR_send.Total_pkt_num = 1;
            else if(Vdr_Wr_Rd_Offset.V_11H_Write > 10)
            {
                if(Vdr_Wr_Rd_Offset.V_11H_Write % 10)
                    VDR_send.Total_pkt_num = Vdr_Wr_Rd_Offset.V_11H_Write / 10 + 1;
                else
                    VDR_send.Total_pkt_num = Vdr_Wr_Rd_Offset.V_11H_Write / 10;
            }
#endif
            VDR_send.Current_pkt_num  = 0;
            Send_gb_vdr_jt808_0x0700_creat(linkno);
        }
        break;
    case  0x12: //��ʻԱ��ݼ�¼
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // ��ʼͷ
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //������
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // ������
            // û����Ϣ���
            buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
            jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );

        }
        else
        {
#ifdef JT808_TEST_JTB
            VDR_send.Total_pkt_num = 10;
#else
            if(Vdr_Wr_Rd_Offset.V_12H_Write <= 10)
                VDR_send.Total_pkt_num = 1;
            else if(Vdr_Wr_Rd_Offset.V_12H_Write > 10)
            {
                if(Vdr_Wr_Rd_Offset.V_12H_Write % 10)
                    VDR_send.Total_pkt_num = Vdr_Wr_Rd_Offset.V_12H_Write / 10 + 1;
                else
                    VDR_send.Total_pkt_num = Vdr_Wr_Rd_Offset.V_12H_Write / 10;
            }
#endif
            VDR_send.Current_pkt_num  = 0;
            Send_gb_vdr_jt808_0x0700_creat(linkno);
        }
        break;
    case  0x13:   //  �ⲿ�����¼
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // ��ʼͷ
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //������
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // ������
            // û����Ϣ���
            buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
            jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );

        }
        else if(jt808_param.id_0xF041 == 1)
        {
#ifdef JT808_TEST_JTB
            psrc = rt_malloc(750);
            if(psrc != RT_NULL)
            {
                //--------------------------------
                psrc[Swr++]	= (u8)( seq >> 8 );
                psrc[Swr++]	= (u8)seq;
                psrc[Swr++]	= cmd;       // ������
                psrc[Swr++]	= 0x55;                 // ��ʼͷ
                psrc[Swr++]	= 0x7A;

                psrc[Swr++] = 0x13;                   //������

                SregLen								= 700;                  // ��Ϣ����
                psrc[Swr++]	= (u8)( SregLen >> 8 ); // Hi
                psrc[Swr++]	= (u8)SregLen;          // Lo    65x7

                psrc[Swr++] = 0x00;                   // ������
                //------- ��Ϣ���� ------
                /*
                �ⲿ�����¼   7 ���ֽ�  100 ��       һ��������
                */
                gb_get_13h( psrc + Swr );

                Swr += 700;
                //------------------------------------------
                psrc[Swr++] = fcs_caulate(psrc + 3, Swr - 3);
                jt808_tx_ack_ex(linkno, 0x0700, psrc, Swr );
                rt_free(psrc);
                psrc = RT_NULL;
            }
#endif
        }
        else if(jt808_param.id_0xF041 == 2)
        {
            VDR_send.Total_pkt_num	 = 1;
            VDR_send.Current_pkt_num  = 0;
            Send_gb_vdr_jt808_0x0700_creat(linkno);
        }
        break;
    case  0x14:
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // ��ʼͷ
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //������
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // ������
            // û����Ϣ���
            buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
            jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );

        }
        else if(jt808_param.id_0xF041 == 1)
        {
#ifdef JT808_TEST_JTB
            psrc = rt_malloc(750);
            if(psrc != RT_NULL)
            {
                psrc[Swr++]	= (u8)(seq >> 8 );
                psrc[Swr++]	= (u8)seq;
                psrc[Swr++]	= cmd;       // ������
                // 14 ��¼�ǲ����޸ļ�¼
                psrc[Swr++]	= 0x55;                 // ��ʼͷ
                psrc[Swr++]	= 0x7A;

                psrc[Swr++] = 0x14;                   //������

                SregLen								= 700;                  // ��Ϣ����
                psrc[Swr++]	= (u8)( SregLen >> 8 ); // Hi
                psrc[Swr++]	= (u8)SregLen;          // Lo    65x7

                psrc[Swr++] = 0x00;                   // ������
                //------- ��Ϣ���� ------

                /*
                ÿ�� 7 ���ֽ�   100 ��    1��������
                */
                gb_get_14h( psrc + Swr );
                Swr += 700;
                //------------------------------------------
                psrc[Swr++] = fcs_caulate(psrc + 3, Swr - 3);
                jt808_tx_ack_ex(linkno, 0x0700, psrc, Swr );
                rt_free(psrc);
                psrc = RT_NULL;
            }
#endif
        }
        else if(jt808_param.id_0xF041 == 2)
        {
            VDR_send.Total_pkt_num	 = 1;
            VDR_send.Current_pkt_num  = 0;
            Send_gb_vdr_jt808_0x0700_creat(linkno);
        }
        break;
    case  0x15: // �ٶ�״̬��־
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // ��ʼͷ
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //������
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // ������
            // û����Ϣ���
            buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
            jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );

        }
        else
        {

            VDR_send.Current_pkt_num  = 0;
#ifdef JT808_TEST_JTB
            VDR_send.Total_pkt_num	 = 10;
#else
            if(Vdr_Wr_Rd_Offset.V_15H_Write <= 2)
                VDR_send.Total_pkt_num = 1;
            else if(Vdr_Wr_Rd_Offset.V_15H_Write > 2)
            {
                if(Vdr_Wr_Rd_Offset.V_15H_Write % 2)
                    VDR_send.Total_pkt_num = Vdr_Wr_Rd_Offset.V_15H_Write / 2 + 1;
                else
                    VDR_send.Total_pkt_num = Vdr_Wr_Rd_Offset.V_15H_Write / 2;
            }
#endif
            Send_gb_vdr_jt808_0x0700_creat(linkno);
        }
        break;
    default:    //    �ɼ�����
        buf[0]	= seq >> 8;
        buf[1]	= seq & 0xff;
        buf[2]	= cmd;
        Swr = 3;



        buf[Swr++]	= 0x55; 			// ��ʼͷ
        buf[Swr++]	= 0x7A;
        buf[Swr++]	= 0xFA;			   //������      0xFA   �ɼ�����
        buf[Swr++]	= 0x00; 			// Hi
        buf[Swr++]	= 0x00; 			   // Lo

        buf[Swr++]	= 0x00; 			// ������
        // û����Ϣ���
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );
        break;
    }
#endif

}

void Send_gb_vdr_Rx_8701_ack( uint8_t linkno, uint8_t *pmsg )
{
    // note:     ���������·��� VDR ��ز�����Ϣ
    uint8_t		 *psrc;
    uint8_t		buf[128];
    uint8_t		tmpbuf[16];

    uint16_t	seq = JT808HEAD_SEQ( pmsg );
    uint16_t	len = JT808HEAD_LEN( pmsg );
    uint8_t		cmd = *(pmsg + 12 ); /*����ǰ��12�ֽڵ�ͷ*/
    uint16_t	SregLen;
    uint32_t	i;
    //uint8_t		fcs;
    uint16_t    Swr = 0;
    uint8_t    Error = 0;

    u8	year, month, day, hour, min, sec;
    u32  reg_dis = 0;
    u8	temp_buffer[6];

    Send_gb_vdr_variable_init();
    //step0 :  get  system ID
    VDR_send.CMD = cmd;
    rt_kprintf("\r\n  set  Recorder=0x%02X \r\n", VDR_send.CMD);

    switch( cmd )
    {

        // sub	step 1 :  �ͳ��������й�
    case 0x82:			/*���ó�����Ϣ*/

        memset(jt808_param.id_0xF005, 0, sizeof(jt808_param.id_0xF005));
        memset(jt808_param.id_0x0083, 0, sizeof(jt808_param.id_0x0083));
        memset(jt808_param.id_0xF00A, 0, sizeof(jt808_param.id_0xF00A));

        //-----------------------------------------------------------------------
        memcpy(jt808_param.id_0xF005, pmsg + 19, 17);
        memcpy(jt808_param.id_0x0083, pmsg + 36, 12);
        memcpy(jt808_param.id_0xF00A, pmsg + 48, 12);
        //  �洢
        param_save(1);

    case 0xC2: //���ü�¼��ʱ��
        // ûɶ�ã������ظ����У�����GPSУ׼�͹���
        //  ������ ʱ���� BCD
        year = (pmsg[19] >> 4) * 10 + (pmsg[19] & 0x0F);
        month = (pmsg[1 + 19] >> 4) * 10 + (pmsg[1 + 19] & 0x0F);
        day = (pmsg[2 + 19] >> 4) * 10 + (pmsg[2 + 19] & 0x0F);
        hour = (pmsg[3 + 19] >> 4) * 10 + (pmsg[3 + 19] & 0x0F);
        min = (pmsg[4 + 19] >> 4) * 10 + (pmsg[4 + 19] & 0x0F);
        sec = (pmsg[5 + 19] >> 4) * 10 + (pmsg[5 + 19] & 0x0F);
        //  update  RTC
        date_set(year, month, day);
        time_set(hour, min, sec);
        break;


    case 0xC3: //�����ٶ�����ϵ��������ϵ����
        // ǰ6 ���ǵ�ǰʱ��
        year = (pmsg[19] >> 4) * 10 + (pmsg[19] & 0x0F);
        month = (pmsg[1 + 19] >> 4) * 10 + (pmsg[1 + 19] & 0x0F);
        day = (pmsg[2 + 19] >> 4) * 10 + (pmsg[2 + 19] & 0x0F);
        hour = (pmsg[3 + 19] >> 4) * 10 + (pmsg[3 + 19] & 0x0F);
        min = (pmsg[4 + 19] >> 4) * 10 + (pmsg[4 + 19] & 0x0F);
        sec = (pmsg[5 + 19] >> 4) * 10 + (pmsg[5 + 19] & 0x0F);

        //	update	RTC
        date_set(year, month, day);
        time_set(hour, min, sec);

        jt808_param.id_0xF032 = (pmsg[6 + 19] << 8) + (u32)pmsg[7 + 19]; // ����ϵ��  �ٶ�����ϵ��

        //  �洢
        param_save( 1 );

        break;
    case 0x83: //  ��¼�ǳ��ΰ�װʱ��

        memcpy(temp_buffer, pmsg + 19, 6);
        jt808_param.id_0xF030 = mytime_from_bcd(temp_buffer);
        //  �洢
        param_save( 1 );

        break;
    case 0x84: // �����ź���������Ϣ
        break;

    case 0xC4: //	���ó�ʼ���

        year = (pmsg[19] >> 4) * 10 + (pmsg[19] & 0x0F);
        month = (pmsg[1 + 19] >> 4) * 10 + (pmsg[1 + 19] & 0x0F);
        day = (pmsg[2 + 19] >> 4) * 10 + (pmsg[2 + 19] & 0x0F);
        hour = (pmsg[3 + 19] >> 4) * 10 + (pmsg[3 + 19] & 0x0F);
        min = (pmsg[4 + 19] >> 4) * 10 + (pmsg[4 + 19] & 0x0F);
        sec = (pmsg[5 + 19] >> 4) * 10 + (pmsg[5 + 19] & 0x0F);
        //  update  RTC
        date_set(year, month, day);
        time_set(hour, min, sec);

        memcpy(temp_buffer, pmsg + 19, 6);
        jt808_param.id_0xF030 = mytime_from_bcd(temp_buffer);
        //Api_Config_Recwrite_Large(jt808,0,(u8*)&JT808Conf_struct,sizeof(JT808Conf_struct));
        // instr	 + 12	   ���ó�ʼ��̵�λ��	BCD

        reg_dis = (pmsg[18 + 19 - 6] >> 4) * 10000000 + (pmsg[18 + 19 - 6] & 0x0F) * 1000000 + (pmsg[19 + 19 - 6] >> 4) * 100000 + (pmsg[19 + 19 - 6] & 0x0F) * 10000 \
                  +(pmsg[20 + 19 - 6] >> 4) * 1000 + (pmsg[20 + 19 - 6] & 0x0F) * 100 + (pmsg[21 + 19 - 6] >> 4) * 10 + (pmsg[21 + 19 - 6] & 0x0F);


        jt808_data.id_0xFA01 = reg_dis * 100;
        //  �洢
        param_save( 1 );
        data_save();
        break;

    default:
        Error = 2; // ���ô���
        break;
    }

    if(Error != 2)
        VDR_product_14H(cmd);
    else
        cmd = 0xFB; // ���ô���


    // -----ͳһ�ظ�����
    buf[0]	= seq >> 8;
    buf[1]	= seq & 0xff;
    buf[2]	= cmd;
    Swr = 3;

    buf[Swr++]	= 0x55; 			// ��ʼͷ
    buf[Swr++]	= 0x7A;
    buf[Swr++]	= cmd;			   //������
    buf[Swr++]	= 0x00; 			// Hi
    buf[Swr++]	= 0x00; 			   // Lo

    buf[Swr++]	= 0x00; 			// ������
    // û����Ϣ���
    buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
    jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );


}
//void rx_rec( uint8_t linkno,u8 *instr)
void rx_rec(u8 *instr)
{
    u8 in[200];
    u8 len = strlen(instr);

    Ascii_To_Hex(in, instr + 2, len - 2);

    Send_gb_vdr_Rx_8700_ack(mast_socket->linkno, in);


}
FINSH_FUNCTION_EXPORT( rx_rec,  rx_rec );


u8  DF_Write_RecordAdd(u32 Wr_Address, u32 Rd_Address, u32 addr)
{
    u8	  head[448];
    u16	  offset = 0;
    u32	  Add_offset = addr; //  page offset
    u32	  SaveContent_add = 0;	 // �洢page ҳ
    u8	  reg[9];
    u8	  Flag_used = 0x01;


    //  2 .	Excute
    sst25_read(Add_offset, (unsigned char *)head, 448);

    /*

      ͨ����ѯBlock ��1��Page��ǰ448�ֽ��Ƿ�Ϊ0xFF �жϣ������Ҫд�����ݵ�ƫ�Ƶ�ַ����448����ʶʹ����󣬲�����Block��
      Ȼ���ͷ��ʼ��

           �洢���������ʼλ�ô�512���ֽڿ�ʼ��
                     Savepage=512+Add_offset*8

     */
    for(offset = 0; offset < 448; offset++)
    {
        if(0xFF == head[offset])
            break;
    }

    if(offset == 448)
    {
        sst25_erase_4k(Add_offset); // Erase block
        offset = 0;
    }
    SaveContent_add = Add_offset + 512 + offset * 8;


    memcpy(reg, &Wr_Address, 4);
    memcpy(reg + 4, &Rd_Address, 4);

    sst25_write_through(Add_offset + offset, &Flag_used, 1); //  ����״̬λ
    sst25_write_through(SaveContent_add, reg, 8);  //  ���¼�¼����

    return true;
    //				   The	End
}


u8  DF_Read_RecordAdd(u32 Wr_Address, u32 Rd_Address, u32 addr)
{
    u8		head[448];
    u16	  offset = 0;
    u32	 Add_offset = addr;	//	page offset
    u32	 Reg_wrAdd = 0, Reg_rdAdd = 0;
    u32	  SaveContent_add = 0;	 // �洢page ҳ

    //Wr_Address, Rd_Address  , û��ʲô��ֻ�Ǳ�ʾ���룬����д�۲����
    //  1.	Classify

    //  2 .	Excute
    sst25_read(Add_offset, (unsigned char *)head, 448); //	������Ϣ
    /*

     ͨ����ѯBlock ��1��Page��ǰ448�ֽ��Ƿ�Ϊ0xFF �жϣ������Ҫд�����ݵ�ƫ�Ƶ�ַ����448����ʶʹ����󣬲�����Block��
     Ȼ���ͷ��ʼ��

    	  �洢���������ʼλ�ô�512���ֽڿ�ʼ��
    				Savepage=512+Add_offset*8

    */

    for(offset = 0; offset < 448; offset++)
    {
        if(0xFF == head[offset])
            break;
    }

    offset--;	// ��һ������Ϊ0xFF

    SaveContent_add = Add_offset + 512 + offset * 8;

    sst25_read(SaveContent_add, (u8 *)&Reg_wrAdd, 4);
    sst25_read(SaveContent_add + 4, (u8 *)&Reg_rdAdd, 4);


    // rt_kprintf("\r\n  RecordAddress  READ-1   write=%d , read=%d \r\n",Reg_wrAdd,Reg_rdAdd);
    //  3. Get reasult
    Avrg_15minSpd.write = Reg_wrAdd;
    Avrg_15minSpd.read = Reg_rdAdd;

    return true;

}

#ifdef AVRG15MIN
u8  Api_avrg15minSpd_Content_write(u8 *buffer, u16 wr_len)
{
    u8  fcs_in = 0, i = 0;
    u8  regist_str[128];
    u32  addr = ADDR_DF_VDR_15minDATA; // ��ʼ��ַ
    //  ÿ���Ӽ�¼ 	7 byte	   6(datatime)+1 spd		15 ������15*7= 105 ���ֽ� ,����ռ��128 ���ֽ�ÿ����¼
    // 1 ������  4096 bytes �ܴ洢 32	 ����¼
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    // 1. caculate   FCS
    memset(regist_str, 0, sizeof(regist_str));
    memcpy(regist_str, buffer, wr_len);
    fcs_in = 0;
    for(i = 0; i < 105; i++)
    {
        fcs_in ^= regist_str[i];
    }
    regist_str[i] = fcs_in;
    //  2.  Check  wehter  need  to earase  sector
    if(Avrg_15minSpd.write >= 32)
        Avrg_15minSpd.write = 0;
    if(Avrg_15minSpd.write == 0)
        sst25_erase_4k(addr);
    //  3. save
    // save content
    sst25_write_through( addr + Avrg_15minSpd.write * 128, regist_str, 128);
    Avrg_15minSpd.write++;
    // save wr pos
    DF_Write_RecordAdd(Avrg_15minSpd.write, Avrg_15minSpd.read, ADDR_DF_VDR_15minAddr);
    rt_sem_release( &sem_dataflash );;
    return true;
}


u8  Api_avrg15minSpd_Content_read(u8 *dest)
{
    u8  fcs_in = 0, i = 0;
    u8  regist_str[128];
    u32  addr = ADDR_DF_VDR_15minDATA; // ��ʼ��ַ

    if(Avrg_15minSpd.write == 0)
    {
        // rt_kprintf("\r\n ��15�������¼�¼1\r\n");
        return 0;

    }

    sst25_read(addr + 128 * (Avrg_15minSpd.write - 1), regist_str, 128);
    fcs_in = 0;
    for(i = 0; i < 105; i++)
    {
        fcs_in ^= regist_str[i];
    }
    if(regist_str[i] != fcs_in)
    {
        // rt_kprintf("\r\n ��15�������¼�¼: У�����\r\n");
        return 0;
    }
    else
        memcpy(dest, regist_str, 105);

    //OutPrint_HEX("��ȡ15����ƽ���ٶȼ�¼",regist_str,105);
    return 105;
}
u8  Avrg15_min_generate(u8 spd)
{
    if(Avrg_15minSpd.Ram_counter >= 15)
    {
        Avrg_15minSpd.Ram_counter = 0;
        Avrg_15minSpd.savefull_state = 1;
    }
    Avrg_15minSpd.content[Avrg_15minSpd.Ram_counter * 7 + 0] = YEAR( mytime_now );
    Avrg_15minSpd.content[Avrg_15minSpd.Ram_counter * 7 + 1] = YEAR( mytime_now );
    Avrg_15minSpd.content[Avrg_15minSpd.Ram_counter * 7 + 2] = DAY( mytime_now );
    Avrg_15minSpd.content[Avrg_15minSpd.Ram_counter * 7 + 3] = HOUR( mytime_now );
    Avrg_15minSpd.content[Avrg_15minSpd.Ram_counter * 7 + 4] = MINUTE( mytime_now );
    Avrg_15minSpd.content[Avrg_15minSpd.Ram_counter * 7 + 5] = 0;
    Avrg_15minSpd.content[Avrg_15minSpd.Ram_counter * 7 + 6] = spd;
    Avrg_15minSpd.Ram_counter++;
    return true;
}

u8 Avrg15_min_save(void)
{
    u8  regist_save[128];
    u8  i = 0;
    s8 reg_wr = Avrg_15minSpd.Ram_counter;


    if(reg_wr == 0)
        reg_wr = 14;
    else
        reg_wr--;

    memset(regist_save, 0, sizeof(regist_save));

    for(i = 0; i < 15; i++)
    {
        memcpy(regist_save + (14 - i) * 7, Avrg_15minSpd.content + reg_wr * 7, 7);
        if(reg_wr == 0)
            reg_wr = 14;
        else
            reg_wr--;
    }

    Api_avrg15minSpd_Content_write(regist_save, 105);
    //  OutPrint_HEX("�洢15����ƽ���ٶȼ�¼",regist_save,105);
    return true;
}

#endif

/*
       GB19056     ����ִ�к��� ����
*/



//    ��ȡ������GB19056  ��ʽ������״̬
u8  GB_Get_SensorStatus(void)
{
    // ��ѯ������״̬
    u8  Sensorstatus = 0;

    /*
     -------------------------------------------------------------
    		  F4  �г���¼�� TW705	 �ܽŶ���
     -------------------------------------------------------------
     ��ѭ  GB10956 (2012)  Page26  ��A.12  �涨
    -------------------------------------------------------------
    | Bit  |	  Note		 |	�ر�|	MCUpin	|	PCB pin  |	 Colour | ADC
    ------------------------------------------------------------
    	D7		ɲ��		   *			PE11			 9				  ��
    	D6		��ת��	   *			 PE10						  10			  ��
    	D5		��ת��	   *			 PC2			  8 			   ��
    	D4		Զ���	   *			 PC0			  4 			   �ڰ�
    	D3		�����	   *			 PA6			  5 			   ��
    	D2		���			  add		   PA7				7							 �� 	 *
    	D1		����		   add		   PA1				6							 �� 	 *
    	D0		Ԥ��
      */


    //  1.   D7	  ɲ��			 *			  PE11			   J1 pin9				  ��
    if(car_state_ex & 0x10) 	//PE11
    {
        // 	  �Ӹ�	����
        Sensorstatus |= 0x80;
        // bit4 ---->ɲ��
    }
    else
    {
        //	 ��̬
        Sensorstatus &= ~0x80;
        //bit4 ---->ɲ��
    }
    // 2.  D6 	 ��ת�� 	*			  PE10			  J1 pin10				��
    if(car_state_ex & 0x08) //  PE10
    {
        //		 �Ӹ�  ����
        Sensorstatus |= 0x40;
        //bit3---->  ��ת��
    }
    else
    {
        //	��̬
        Sensorstatus &= ~0x40;
        //bit3---->  ��ת��
    }
    //  3.  D5 	 ��ת�� 	*			  PC2			  J1  pin8				  ��
    if(car_state_ex & 0x04) //PC2
    {
        // 	   �Ӹ�  ����
        Sensorstatus |= 0x20;
        // bit2----> ��ת��
    }
    else
    {
        //   ��̬
        Sensorstatus &= ~0x20;
        //bit2----> ��ת��
    }

    // 4.	D4		Զ���	   *			 PC0			  J1 pin4				 �ڰ�
    if(car_state_ex & 0x02) // PC0
    {
        //	   �Ӹ�  ����
        Sensorstatus |= 0x10;
        //bit 1	----->	Զ���

    }
    else
    {
        //   ��̬
        Sensorstatus &= ~0x10;
        //bit 1  ----->  Զ���

    }
    //5.	 D3 	 ����� 	*			  PC1			   J1 pin5				  ��
    if(car_state_ex & 0x01) // PC1
    {
        //		 �Ӹ�  ����
        Sensorstatus |= 0x08;
        //bit 0  ----->  �����
    }
    else
    {
        //	��̬
        Sensorstatus &= ~0x08;
        //bit 0  ----->  �����

    }
    //	6.	  D2	  ���			add 		 PA7			  7 			   ��	   *
    if(car_state_ex & 0x40) //PA7
    {
        //		 �Ӹ�  ����
        Sensorstatus |= 0x04;
        //	bit6 ----> ���
    }
    else
    {
        //	��̬
        Sensorstatus &= ~0x04;
        // bit6 ----> ���
    }
    // 7.	 D1 	 ����		   add			PA1 			 6				  ��	  *
    if(jt808_status & BIT_STATUS_DOOR1) // PA1
    {
        //	   �Ӹ�  ����
        Sensorstatus |= 0x02;
    }
    else
    {
        //   ��̬
        Sensorstatus &= ~0x02;
    }

    //	 8.  Reserved

    return Sensorstatus;
}

void  GB_IO_statusCheck(void)
{
    u16  Speed_working = 0;

    //  0 .  ����У��״̬ѡ���ٶ�
    if(0 == jt808_param.id_0xF043)  //0  :����    1  : GPS
        Speed_working = car_data.get_speed; //�����ٶ�
    else
        Speed_working = gps_speed;
    //=============================================================================
    GB19056.Vehicle_sensor = GB_Get_SensorStatus();
    //------------ 0.2s    �ٶ�״̬ -----------------------
    Sensor_buf[save_sensorCounter].DOUBTspeed = Speed_working / 10; //   �ٶ�  ��λ��km/h ���Գ���10
    Sensor_buf[save_sensorCounter++].DOUBTstatus = GB19056.Vehicle_sensor; //   ״̬
    if(save_sensorCounter > 200)
    {
        save_sensorCounter = 0;
        sensor_writeOverFlag = 1;
    }
    //============================================================================

    //  �¹��ɵ㴥���ж�

    //  ͣ������ �¹��ɵ�1
    if((current_speed < 70) && (Sps_larger_5_counter > 10) && (Speed_cacu_BAK > current_speed) && (Speed_cacu_Trigger_Flag == 0))
    {
        moni_drv(0x10, 0);
        Speed_cacu_Trigger_Flag = 1;
    }
    else if((current_speed > 60) && (Speed_cacu_BAK > 60) && (Sps_larger_5_counter > 10))
        Speed_cacu_Trigger_Flag = 0; // clear

    Speed_cacu_BAK = current_speed;
    //--------------------------------------------------


}

void GB_DOUBT2_Generate(void)
{
    // 2.	GB19056 ���
    //	�¹��ɵ�2	  : �ⲿ�ϵ紥���¹��ɵ�洢
    moni_drv(0x10, 7);
    //save �ϵ��¼
    VDR_product_13H(0x02);
    //-------------------------------------------------------
}

void GB_Vehicle_RUNjudge(void)
{
    if(current_speed_10x > 50) //   �ٶȴ��� 5  km/h
    {
        Sps_larger_5_counter++;
    }
    else
        Sps_larger_5_counter = 0;
}

void GB_100ms_Timer_ISP(void)
{
    gb_100ms_time_counter++;
    if(gb_100ms_time_counter >= 10)
    {
        //   1  second  isp
        GB_Vehicle_RUNjudge();
        //  �ж� ��¼�ǳ��ٱ���
        GB_ExceedSpd_warn();

        //  ����08  09   ����
        VDR_product_08H_09H();

        //  ��ʱ��ʻ�ж�  ���� 11 ����
        GB_IC_Exceed_Drive_Check();

        // �ж��ٶ��쳣
        if((GB19056.speedlog_Day_save_times == 0) && (jt808_status & BIT_STATUS_FIXED))	// ÿ��ֻ�洢һ��
            GB_SpeedSatus_Judge();

        gb_100ms_time_counter = 0;
    }

    //  0.2 s   isp
#if  0
    if(gb_100ms_time_counter >> 1)
    {
        GB_IO_statusCheck();  //  0.2 ��ȡ�ⲿ������״̬
    }
#endif

    // 0.8 s  isp
    if(gb_100ms_time_counter % 5 == 0)
    {
        mytime_to_bcd(gb_BCD_datetime, mytime_now );            /*ʵʱʱ��*/
    }

}

void  GB_200ms_Hard_timer(void)
{
    GB_IO_statusCheck();  //  0.2 ��ȡ�ⲿ������״̬
}


//-------------ģ����Լ�¼������----------------------------------------
void  moni_drv(u8 CMD, u16 delta)
{

    u16 j = 0, cur = 0, i = 0, icounter = 0;

    //  ��ʻ��¼������ݲ��� ����,  �Ҷ�λ�������
    switch(CMD)
    {
#if 1
    case  0x08:

        //-------------------  08H	 --------------------------------------------------

        //  1.  ������ ʱ����	 ����0
        VdrData.H_08[0] = ((YEAR(mytime_now) / 10) << 4) + (YEAR(mytime_now) % 10);
        VdrData.H_08[1] = ((MONTH(mytime_now) / 10) << 4) + (MONTH(mytime_now) % 10);
        VdrData.H_08[2] = ((DAY(mytime_now) / 10) << 4) + (DAY(mytime_now) % 10);
        VdrData.H_08[3] = ((HOUR(mytime_now) / 10) << 4) + (HOUR(mytime_now) % 10);
        VdrData.H_08[4] = ((MINUTE(mytime_now) / 10) << 4) + (MINUTE(mytime_now) % 10);
        VdrData.H_08[5] = 0;


        for(i = 0; i < 60; i++)
        {
            // 3.	 save	  Minute		 1-59	s
            VdrData.H_08[6 + 2 * i] = 20; //km/h
            VdrData.H_08[7 + 2 * i] = 23;
        }

        // last
        memcpy(VdrData.H_08_BAK, VdrData.H_08, 126);
        VdrData.H_08_saveFlag = 1; //   ����	��ǰ���ӵ����ݼ�¼
        OutPrint_HEX("08 ", VdrData.H_08_BAK, 126);
        break;
    case 0x09:
        //------------------------09H ---------  min=0;	sec=0	----------------------------
        // ��ʼʱ��
        VdrData.H_09[0] = ((YEAR(mytime_now) / 10) << 4) + (YEAR(mytime_now) % 10);
        VdrData.H_09[1] = ((MONTH(mytime_now) / 10) << 4) + (MONTH(mytime_now) % 10);
        VdrData.H_09[2] = ((DAY(mytime_now) / 10) << 4) + (DAY(mytime_now) % 10);
        VdrData.H_09[3] = ((HOUR(mytime_now) / 10) << 4) + (HOUR(mytime_now) % 10);
        VdrData.H_09[4] = 0;
        VdrData.H_09[5] = 0;

        //
        for(i = 0; i < 60; i++)
        {
            //  4 .	��д����λ��	1-59 min
            {
                //   �� A.20		  (��ǰ���ӵ�λ��  +	��ǰ���ӵ�ƽ���ٶ�)
                memcpy( VdrData.H_09 + 6 + i * 11, VdrData.Longi, 4); // ����
                memcpy( VdrData.H_09 + 6 + i * 11 + 4, VdrData.Lati, 4); //γ��
                VdrData.H_09[6 + i * 11 + 8] = (gps_alti >> 8);
                VdrData.H_09[6 + i * 11 + 9] = gps_alti;
                //  ��ǰ���ӵ�ƽ���ٶ�	 ��AvrgSpd_MintProcess()  ���õı���
                VdrData.H_09[6 + i * 11 + 10] = current_speed_10x;
            }
        }


        // 1.  �ж��Ƿ�����Ҫ�洢
        VdrData.H_09_saveFlag = 1; //   ����  ��ǰ���ӵ����ݼ�¼

        OutPrint_HEX("09 ", VdrData.H_09, 666);
        break;
#endif
    case 0x10:
        //-------------------- 10H    �¹��ɵ�����  ---------------------------------
        if(delta != 10)
        {
            //  1.  ��ʻ����ʱ��
            mytime_to_bcd(VdrData.H_10, mytime_now);
        }
        //   2.   ��������ʻ֤����
        memcpy(VdrData.H_10 + 6, jt808_param.id_0xF009, 18);
        //   3.  �ٶȺ�״̬��Ϣ
        //-----------------------  Status Register   --------------------------------
        cur = save_sensorCounter; //20s���¹��ɵ�
        //------------------------------------------
        if(cur > 0)	 // �ӵ�ǰ��ǰ��ȡ
            cur--;
        else
            cur = 200;
        //-----------------------------------------
        icounter = 100 + delta;
        for(j = 0; j < icounter; j++)
        {
            if(j >= delta)
            {
                VdrData.H_10[24 + (j - delta) * 2] = Sensor_buf[cur].DOUBTspeed;	//�ٶ�
                //    rt_kprintf("%d-",Sensor_buf[cur].DOUBTspeed);
                VdrData.H_10[24 + (j - delta) * 2 + 1] = Sensor_buf[cur].DOUBTstatus; //״̬
            }

            if(cur > 0)	 // �ӵ�ǰ��ǰ��ȡ
                cur--;
            else
                cur = 200;
        }
        // 	4.	λ����Ϣ
        memcpy( VdrData.H_10 + 224, VdrData.Longi, 4); // ����
        memcpy( VdrData.H_10 + 224 + 4, VdrData.Lati, 4); //γ��
        VdrData.H_10[224 + 8] = (gps_alti >> 8);
        VdrData.H_10[224 + 9] = gps_alti;

        //--------------------------------------------------------------------------
        VdrData.H_10_saveFlag = 1;
        // if(GB19056.workstate==0)
        // OutPrint_HEX("10 ",VdrData.H_10,234);
        break;

#if 1
    case 0x11:
        //  1.   ��������ʻ֤��

        if( VdrData.H_12[24] == 0x01)	 //�ѵ�¼
            memcpy(VdrData.H_11, Read_ICinfo_Reg.DriverCard_ID, 18);
        else
            memcpy(VdrData.H_11, jt808_param.id_0xF009, 18);
        //   2.   ��ʼʱ��
        mytime_to_bcd(VdrData.H_11 + 18, mytime_now);
        //   3.  ��ʼλ��
        memcpy( VdrData.H_11 + 30, VdrData.Longi, 4); // ����
        memcpy( VdrData.H_11 + 30 + 4, VdrData.Lati, 4); //γ��
        VdrData.H_11[30 + 8] = (gps_alti >> 8);
        VdrData.H_11[30 + 9] = gps_alti;
        //   2.   ����ʱ��
        mytime_to_bcd((VdrData.H_11 + 24), mytime_now + 100);
        //   3.  ��ʼλ��
        memcpy( VdrData.H_11 + 40, VdrData.Longi, 4); // ����
        memcpy( VdrData.H_11 + 40 + 4, VdrData.Lati, 4); //γ��
        VdrData.H_11[40 + 8] = (gps_alti >> 8);
        VdrData.H_11[40 + 9] = gps_alti;
        //    4.   save
        VdrData.H_11_saveFlag = 1;
        OutPrint_HEX("11 ", VdrData.H_11, 50);
        break;
    case 0x12:    // �忨�ο�ʱ��
        //   1. ʱ�䷢����ʱ��
        mytime_to_bcd(VdrData.H_12, mytime_now);

        //  2.   ��������ʻ֤��
        memcpy(VdrData.H_12 + 6, jt808_param.id_0xF009, 18);
        //  3.  ���
        VdrData.H_12[24] = ((rt_tick_get() % 2) + 1);
        // 4.  save
        VdrData.H_12_saveFlag = 1;
        OutPrint_HEX("12 ", VdrData.H_12, 25);
        break;
    case 0x13:   //  ��ŵ�
        //   1. ʱ�䷢����ʱ��
        mytime_to_bcd(VdrData.H_13, mytime_now);
        //  2.  value
        VdrData.H_13[6] = ((rt_tick_get() % 2) + 1);
        //  3. save
        VdrData.H_13_saveFlag = 1;
        OutPrint_HEX("13 ", VdrData.H_13, 7);
        break;
    case 0x14:              // ��¼�ǲ����޸�
        //   1. ʱ�䷢����ʱ��
        mytime_to_bcd(VdrData.H_14, mytime_now);
        //  2.  value
        VdrData.H_14[6] = ((rt_tick_get() % 10));
        //  3. save
        VdrData.H_14_saveFlag = 1;
        OutPrint_HEX("14 ", VdrData.H_14, 7);
        break;
    case 0x15:            //  �ٶ�״̬��־
        //   1. ʱ�䷢����ʱ��
        VdrData.H_15[0] = (rt_tick_get() % 2) + 1; // ״̬
        // ��ʼʱ��
        mytime_to_bcd(VdrData.H_15 + 1, mytime_now);
        //  ����ʱ��
        mytime_to_bcd(VdrData.H_15 + 7, mytime_now);
        //  2.  value
        for(i = 0; i < 60; i++)
        {
            VdrData.H_15[13 + 2 * i] = current_speed_10x / 10;
            VdrData.H_15[14 + 2 * i] = 40;
        }
        //  3. save
        VdrData.H_15_saveFlag = 1;
        OutPrint_HEX("15 ", VdrData.H_15, 133);
        break;
#endif
    default:
        //rt_kprintf("\r\n  CMD =%d   not  support ",CMD);
        break;


    }
}
FINSH_FUNCTION_EXPORT(moni_drv, moni_drv(u8 CMD, u16 delta));


u8 GB_fcs(u8 *instr, u16 infolen, u8 fcs_value)
{
    u16 i = 0;
    u8  FCS = fcs_value;

    for(i = 0; i < infolen; i++)
        FCS ^= instr[i];
    return  FCS;
}

void  SERIAL_OUTput(u8 type)
{
    if(type == GB_OUT_TYPE_Serial)
    {
        usb_outbuf[usb_outbuf_Wr++] = GB_fcs(usb_outbuf, usb_outbuf_Wr, 0);
        dev_vuart.flag &= ~RT_DEVICE_FLAG_STREAM;
        rt_device_write( &dev_vuart,  0,  usb_outbuf, usb_outbuf_Wr);
        dev_vuart.flag |= RT_DEVICE_FLAG_STREAM;
    }
}

void USB_OUTput(u8  CMD, u8 fcsAdd)
{
    u32 len = 0;
    FRESULT res;

    GB19056.usb_xor_fcs = GB_fcs(usb_outbuf, usb_outbuf_Wr, GB19056.usb_xor_fcs);
    if(fcsAdd == 1)
        usb_outbuf[usb_outbuf_Wr++] = GB19056.usb_xor_fcs;
    f_lseek( &file, file.fsize);
    res = f_write(&file, usb_outbuf, usb_outbuf_Wr, &len);
    if(res)
        rt_kprintf("res=%d ", res);

}

u16  GB_00_07_info_only(u8 *dest, u8 ID)
{
    u32  i = 0;
    u16  return_len = 0;
    u8   tmpbuf[6];


    return_len = 0;
    switch(ID)
    {
    case 0x00:
        dest[return_len++]	 = 0x12;		   //  12  ���׼
        dest[return_len++]	 = 0x00;
        break;
    case 0x01:
        memcpy( dest + return_len, jt808_param.id_0xF009, 18 ); //��Ϣ����
        return_len				   += 18;
        break;
    case 0x02: //  02  �ɼ���¼�ǵ�ʵʱʱ��
        /*ʵʱʱ��*/
        memcpy(dest + return_len, gb_BCD_datetime, 6 );
        return_len += 6;
        break;

    case 0x03:	   // 03 �ɼ�360h�����
        memcpy(dest + return_len, gb_BCD_datetime, 6 );
        return_len += 6;

        mytime_to_bcd( tmpbuf, jt808_param.id_0xF030 ); /*�״ΰ�װʱ��*/
        memcpy( dest + return_len, tmpbuf, 6);
        return_len += 6;
        //  ��ʼ���
        tmpbuf[0]	= 0x00;
        tmpbuf[1]	= 0x00;
        tmpbuf[2]	= 0x00;
        tmpbuf[3]	= 0x00;
        memcpy(  dest + return_len, tmpbuf, 4);
        return_len += 4;
        i			= jt808_data.id_0xFA01 / 100;     /*����� 0.1KM BCD�� 00-99999999*/
        tmpbuf[0]	= ( ( ( i / 10000000 ) % 10 ) << 4 ) | ( ( i / 1000000 ) % 10 );
        tmpbuf[1]	= ( ( ( i / 100000 ) % 10 ) << 4 ) | ( ( i / 10000 ) % 10 );
        tmpbuf[2]	= ( ( ( i / 1000 ) % 10 ) << 4 ) | ( ( i / 100 ) % 10 );
        tmpbuf[3]	= ( ( ( i / 10 ) % 10 ) << 4 ) | ( i % 10 );
        memcpy( dest + return_len, tmpbuf, 4);
        return_len += 4;
        break;

    case 0x04:																   // 04  �ɼ���¼������ϵ��
        /*ʵʱʱ��*/
        memcpy( dest + return_len, gb_BCD_datetime, 6 );
        return_len += 6;
        dest[return_len++] = (u8)( (jt808_param.id_0xF032 & 0x0000FFFF) >> 8 );
        dest[return_len++] = (u8)( (jt808_param.id_0xF032 & 0x0000FFFF));
        break;
    case 0x05:																   //05 	 ������Ϣ�ɼ�
        //-----------   ��Ϣ����  --------------
        memcpy( dest + return_len, jt808_param.id_0xF005, 17 ); 	  //��Ϣ����
        return_len += 17;
        memcpy( dest + return_len, jt808_param.id_0x0083, 12 );			 // ���ƺ�
        return_len				   += 12;
        //��������
        memcpy( dest + return_len, jt808_param.id_0xF00A, 12 );
        return_len				   += 12;
        break;
    case   0x06: 												   // 06-09
        memcpy( dest + return_len, gb_BCD_datetime, 6 );
        return_len += 6;
        //-------  ״̬�ָ���----------------------
        dest[return_len++] = 0;// 8//�޸�Ϊ0
        //---------- ״̬������-------------------

        /*
          -------------------------------------------------------------
        		 F4  �г���¼�� TW705	�ܽŶ���
          -------------------------------------------------------------
          ��ѭ	GB10956 (2012)	Page26	��A.12	�涨
          -------------------------------------------------------------
        | Bit  |	  Note		 |	�ر�|	MCUpin	|	PCB pin  |	 Colour | ADC
          ------------------------------------------------------------
           D7	   ɲ�� 		  * 		   PE11 			9				 ��
           D6	   ��ת��	  * 			PE10			10				 ��
           D5	   ��ת��	  * 			PC2 			 8				  ��
           D4	   Զ���	  * 			PC0 			 4				  ��
           D3	   �����	  * 			PC1 			 5				  ��
           D2	   ��� 		 add		  PC3			   7				��		*
           D1	   ���� 		 add		  PA1			   6				��		*
           D0	   Ԥ��
        */
        memcpy( dest + return_len, "Ԥ��	", 10 );	   // D0
        return_len += 10;
        memcpy( dest + return_len, "Ԥ��	", 10 );	   // D1
        return_len += 10;
        memcpy( dest + return_len, "Ԥ��	", 10 );	   // D2
        return_len += 10;
        memcpy( dest + return_len, "�����	  ", 10 );	   // D3
        return_len += 10;
        memcpy( dest + return_len, "Զ���	  ", 10 );	   // D4
        return_len += 10;
        memcpy( dest + return_len, "��ת��	  ", 10 );	   // D5
        return_len += 10;
        memcpy( dest + return_len, "��ת��	  ", 10 );	   // D6
        return_len += 10;
        memcpy( dest + return_len, "ɲ��	", 10 );	   // D7
        return_len += 10;
        break;

    case 0x07:															   //07  ��¼��Ψһ���
        //------- ��Ϣ���� ------
        memcpy( dest + return_len, "C000863", 7 );		   // 3C ��֤����
        return_len += 7;
        memcpy( dest + return_len, "TW705", 5 ); // ��Ʒ�ͺ�
        return_len				   += 5;
        for(i = 0; i < 11; i++)
            dest[return_len++]	= 0x00;

        dest[return_len++]	 = 0x14;					   // ��������
        dest[return_len++]	 = 0x03;
        dest[return_len++]	 = 0x01;
        dest[return_len++]	 = jt808_param.id_0xF002[3];					   // ������ˮ��
        dest[return_len++]	 = jt808_param.id_0xF002[4];
        dest[return_len++]	 = jt808_param.id_0xF002[5];
        dest[return_len++]	 = jt808_param.id_0xF002[6];
        //  31--35  ����
        dest[return_len++]	 = 0x00;
        dest[return_len++]	 = 0x00;
        dest[return_len++]	 = 0x00;
        dest[return_len++]	 = 0x00;
        dest[return_len++]	 = 0x00;
        break;
    }
    return return_len;
}


u16  GB_557A_protocol_00_07_stuff(u8 *dest, u8 ID)
{
    u32	regdis			   = 0, reg2 = 0;
    u16	SregLen = 0;
    u16  i = 0, subi = 0, continue_counter = 0;
    u16   get_indexnum = 0; // ��ȡУ��
    //u16  Sd_DataBloknum=0; //   �������ݿ�  ��Ŀ <GB19056.Max_dataBlocks
    u8   Not_null_flag = 0; // NO  data ACK null once
    u8   return_value = 0;
    u32  Rd_Time = 0;

    u16  return_len = 0;


    return_len = 0;
    switch(ID)
    {
    case 0x00:
        dest[return_len++]	 = 0x55;		   // ��ʼͷ
        dest[return_len++]	 = 0x7A;
        dest[return_len++]	 = 0x00;		   //������
        SregLen							   = 0x02;			   // ��Ϣ����
        dest[return_len++]	 = 0x00;		   // Hi
        dest[return_len++]	 = 2;				   // Lo

        dest[return_len++] = 0x00;						   // ������
        break;
    case 0x01:
        return_len = 0;
        dest[return_len++]	 = 0x55;					   // ��ʼͷ
        dest[return_len++]	 = 0x7A;
        dest[return_len++]	 = 0x01;					   //������

        dest[return_len++]	 = 0x00;					   // Hi
        dest[return_len++]	 = 18;						   // Lo

        dest[return_len++] = 0x00;						   // ������
        break;
    case 0x02: //  02  �ɼ���¼�ǵ�ʵʱʱ��
        return_len = 0;

        dest[return_len++]	 = 0x55;					   // ��ʼͷ
        dest[return_len++]	 = 0x7A;
        dest[return_len++]	 = 0x02;					   //������

        dest[return_len++]	 = 0x00;					   // Hi
        dest[return_len++]	 = 6;							   // Lo

        dest[return_len++] = 0x00;						   // ������
        break;

    case 0x03:	   // 03 �ɼ�360h�����
        return_len = 0;												 // 03 �ɼ�360h�����

        dest[return_len++] = 0x55;						 // ��ʼͷ
        dest[return_len++] = 0x7A;
        dest[return_len++] = 0x03;						 //������

        dest[return_len++] = 0x00;						 // Hi
        dest[return_len++] = 20;							 // Lo

        dest[return_len++] = 0x00;						   // ������
        break;

    case 0x04:																   // 04  �ɼ���¼������ϵ��
        return_len = 0;
        dest[return_len++]	 = 0x55;						   // ��ʼͷ
        dest[return_len++]	 = 0x7A;

        dest[return_len++] = 0x04;							   //������

        dest[return_len++]	 = 0x00;						   // Hi
        dest[return_len++]	 = 8;								   // Lo

        dest[return_len++] = 0x00;							   // ������
        break;
    case 0x05:																   //05 	 ������Ϣ�ɼ�
        return_len = 0;
        dest[return_len++]	 = 0x55;						   // ��ʼͷ
        dest[return_len++]	 = 0x7A;

        dest[return_len++] = 0x05;							   //������

        SregLen							   = 41;							   // ��Ϣ����
        dest[return_len++]	 = (u8)( SregLen >> 8 );		   // Hi
        dest[return_len++]	 = (u8)SregLen; 				   // Lo	 65x7

        dest[return_len++] = 0x00;							   // ������
        //-----------   ��Ϣ����  --------------
        break;
    case   0x06: 												   // 06-09
        return_len = 0;
        //  06 �ź�������Ϣ
        dest[return_len++]	 = 0x55;			   // ��ʼͷ
        dest[return_len++]	 = 0x7A;

        dest[return_len++] = 0x06;				   //������

        SregLen							   = 87;				   // ��Ϣ����
        dest[return_len++]	 = (u8)( SregLen >> 8 ); // Hi
        dest[return_len++]	 = (u8)SregLen; 	   // Lo
        dest[return_len++] = 0x00;				   // ������
        break;

    case 0x07:															   //07  ��¼��Ψһ���
        return_len = 0;
        dest[return_len++]	 = 0x55;					   // ��ʼͷ
        dest[return_len++]	 = 0x7A;

        dest[return_len++] = 0x07;						   //������

        SregLen							   = 35;						   //206;		// ��Ϣ����
        dest[return_len++]	 = (u8)( SregLen >> 8 );	   // Hi
        dest[return_len++]	 = (u8)SregLen; 			   // Lo

        dest[return_len++] = 0x00;						   // ������
        break;
    }

    return_len += GB_00_07_info_only(dest + return_len, ID);
    return return_len;

}


void  GB_null_content(u8 ID, u8 type)
{
    usb_outbuf_Wr = 0;
    usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// ��ʼͷ
    usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
    usb_outbuf[usb_outbuf_Wr++]   = ID;					//������
    usb_outbuf[usb_outbuf_Wr++]   = 0;			// Hi
    usb_outbuf[usb_outbuf_Wr++]   = 0;				// Lo

    usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// ������
    OUT_PUT;
}

void GB_SPK_DriveExceed_clear(void)
{
    GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 0;
    GB19056.SPK_DriveExceed_Warn.group_playTimes = 0;
    GB19056.SPK_DriveExceed_Warn.FiveMin_sec_counter = 0;
}
void GB_SPK_SpeedStatus_Abnormal_clear(void)
{
    GB19056.SPK_SpeedStatus_Abnormal.Warn_state_Enable = 0;
    GB19056.SPK_SpeedStatus_Abnormal.group_playTimes = 0;
    GB19056.SPK_SpeedStatus_Abnormal.FiveMin_sec_counter = 0;
}

void GB_SPK_Speed_Warn_clear(void)
{
    GB19056.SPK_Speed_Warn.Warn_state_Enable = 0;
    GB19056.SPK_Speed_Warn.group_playTimes = 0;
    GB19056.SPK_Speed_Warn.FiveMin_sec_counter = 0;
}

void GB_SPK_UnloginWarn_clear(void)
{
    GB19056.SPK_UnloginWarn.Warn_state_Enable = 0;
    GB19056.SPK_UnloginWarn.group_playTimes = 0;
    GB19056.SPK_UnloginWarn.FiveMin_sec_counter = 0;
}

void  GB_SPK_preTired_clear(void)
{
    GB19056.SPK_PreTired.Warn_state_Enable = 0;
    GB19056.SPK_PreTired.group_playTimes = 0;
    GB19056.SPK_PreTired.FiveMin_sec_counter = 0;
}

void GB_serial_get_clear(void)
{
    Vdr_Wr_Rd_Offset.V_08H_get = 0;
    Vdr_Wr_Rd_Offset.V_09H_get = 0;
    Vdr_Wr_Rd_Offset.V_10H_get = 0;
    Vdr_Wr_Rd_Offset.V_11H_get = 0;
    Vdr_Wr_Rd_Offset.V_12H_get = 0;
    Vdr_Wr_Rd_Offset.V_13H_get = 0;
    Vdr_Wr_Rd_Offset.V_14H_get = 0;
    Vdr_Wr_Rd_Offset.V_15H_get = 0;
}

void GB_Drv_init(void)     // �ŵ� startup .c
{
    u8 i = 0;


    GB19056.workstate = 0; // default   1
    GB19056.RX_CMD = 0;
    GB19056.TX_CMD = 0;
    GB19056.RX_FCS = 0;
    GB19056.TX_FCS = 0;
    GB19056.rx_wr = 0;
    GB19056.rx_flag = 0;
    GB19056.u1_rx_timer = 0;
    GB19056.usb_write_step = GB_USB_OUT_idle;
    GB19056.usb_xor_fcs = 0;
    GB19056.speed_larger40km_counter = 0;
    GB19056.speedlog_Day_save_times = 0;
    GB19056.delataSpd_largerThan11_counter = 0;
    GB19056.gps_largeThan40_counter = 0;
    GB19056.start_record_speedlog_flag = 0;

    GB19056.DoubtData3_counter = 0;
    GB19056.DoubtData3_triggerFlag = 0;
    GB19056.Checking_status = 0;

    //  init 09  data
    for(i = 0; i < 60; i++)
    {
        VdrData.H_09[6 + i * 11] = 0x7F;
        VdrData.H_09[7 + i * 11] = 0xFF;
        VdrData.H_09[8 + i * 11] = 0xFF;
        VdrData.H_09[9 + i * 11] = 0xFF;
        VdrData.H_09[10 + i * 11] = 0x7F;
        VdrData.H_09[11 + i * 11] = 0xFF;
        VdrData.H_09[12 + i * 11] = 0xFF;
        VdrData.H_09[13 + i * 11] = 0xFF;
        VdrData.H_09[14 + i * 11] = 0x7F;
        VdrData.H_09[15 + i * 11] = 0xFF;
        VdrData.H_09[16 + i * 11] = 0x00;
    }

    //  add  later
    if(printf_on == 0)
    {
        gb_work(1);
    }
    else
    {
        gb_work(0);
    }

    //   ��ȫ��ʾ ��س�ʼ��
    GB_SPK_DriveExceed_clear();
    GB_SPK_SpeedStatus_Abnormal_clear();
    GB_SPK_Speed_Warn_clear();
    GB_SPK_UnloginWarn_clear();
    GB_SPK_preTired_clear();

    //  ��ȡDF  ��ز���
    total_ergotic();
}

void GB_ExceedSpd_warn(void)
{
    //   У׼�����   ���ݴ������ٶ�    �жϼ�¼�ǳ���
    if ((jt808_param.id_0xF032 & 0x00000000) == 0x00) // ���bit λ   ��32 λ  1 : ��ʾΪУ׼  0:   �Ѿ�У׼
    {
        //  GB   ����ֻ�жϴ������ٶ�
        if(car_data.get_speed > (jt808_param.id_0x0055 * 10) )
        {
            if(GB19056.SPK_Speed_Warn.Warn_state_Enable == 0)
                GB19056.SPK_Speed_Warn.Warn_state_Enable = 1;
        }
        else
            GB19056.SPK_Speed_Warn.Warn_state_Enable = 0; // clear
    }
}

//  ���갲ȫ��ʾ ����                                    Note:  ���� 1s ��ʱ��������
void  GB_Warn_Running(void)
{
    //   5. ��ʻԱδ��¼�����ٶ�
    if((GB19056.SPK_UnloginWarn.Warn_state_Enable) && (current_speed_10x > 50) && (Sps_larger_5_counter > 10))
    {
        //    ��ǰ3  �� ���ȼ� ���е�����¿�ʼ
        if((GB19056.SPK_Speed_Warn.group_playTimes == 0) && (GB19056.SPK_SpeedStatus_Abnormal.group_playTimes == 0) && \
                (GB19056.SPK_DriveExceed_Warn.group_playTimes == 0) && (GB19056.SPK_PreTired.group_playTimes == 0))
        {
            //  5.0.	firs trigger
            if(GB19056.SPK_UnloginWarn.Warn_state_Enable == 1)
            {
                GB19056.SPK_UnloginWarn.Warn_state_Enable = 2;
                GB19056.SPK_UnloginWarn.group_playTimes = 1;
            }
            //5.1  play  operate
            if((GB19056.SPK_UnloginWarn.group_playTimes))
            {
                GB19056.SPK_UnloginWarn.group_playTimes++;
                if(Warn_Play_controlBit & 0x01)
                    tts( "��ʻԱδ��¼��忨��¼");
                //rt_kprintf("\r\n-----��ʻԱδ��¼��忨��¼\r\n");
                if(GB19056.SPK_UnloginWarn.group_playTimes >= 4)
                    GB19056.SPK_UnloginWarn.group_playTimes = 0;
                return;
            }
            // 5.2	timer
            GB19056.SPK_UnloginWarn.FiveMin_sec_counter++;
            if(GB19056.SPK_UnloginWarn.FiveMin_sec_counter >= 300)
            {
                GB19056.SPK_UnloginWarn.group_playTimes = 1;
                GB19056.SPK_UnloginWarn.FiveMin_sec_counter = 0;
            }

        }

    }
    else
        GB_SPK_UnloginWarn_clear();
    //   4.  ƣ�ͼ�ʻ�����ѹ���
    if(GB19056.SPK_PreTired.Warn_state_Enable)
    {

        //	 ��ǰ3	�� ���ȼ� ���е�����¿�ʼ
        if((GB19056.SPK_Speed_Warn.group_playTimes == 0) && (GB19056.SPK_SpeedStatus_Abnormal.group_playTimes == 0) && \
                (GB19056.SPK_DriveExceed_Warn.group_playTimes == 0))
        {
            //  4.0.  firs trigger
            if(GB19056.SPK_PreTired.Warn_state_Enable == 1)
            {
                GB19056.SPK_PreTired.Warn_state_Enable = 2;
                GB19056.SPK_PreTired.group_playTimes = 1;
            }
            //  4.1	play  operate
            if((GB19056.SPK_PreTired.group_playTimes))
            {
                GB19056.SPK_PreTired.group_playTimes++;
                if(Warn_Play_controlBit & 0x02)
                    tts( "��������ʱ��ʻ,��ע����Ϣ" );
                if(GB19056.SPK_PreTired.group_playTimes >= 4)
                    GB19056.SPK_PreTired.group_playTimes = 0;
                return;
            }
            // 4.2	 timer
            GB19056.SPK_PreTired.FiveMin_sec_counter++;
            if(GB19056.SPK_PreTired.FiveMin_sec_counter >= 300)
            {
                GB19056.SPK_PreTired.group_playTimes = 1;
                GB19056.SPK_PreTired.FiveMin_sec_counter = 0;
            }

        }


    }
    else
        GB_SPK_preTired_clear();
    //   3.     ��ʱ��ʻ
    if(GB19056.SPK_DriveExceed_Warn.Warn_state_Enable)
    {
        //   ��ǰ2��  ���ȼ����е������
        if((GB19056.SPK_Speed_Warn.group_playTimes == 0) && (GB19056.SPK_SpeedStatus_Abnormal.group_playTimes == 0))
        {

            //  3.0.  firs trigger
            if(GB19056.SPK_DriveExceed_Warn.Warn_state_Enable == 1)
            {
                GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 2;
                GB19056.SPK_DriveExceed_Warn.group_playTimes = 1;
            }
            //  3.1  play  operate
            if((GB19056.SPK_DriveExceed_Warn.group_playTimes))
            {
                GB19056.SPK_DriveExceed_Warn.group_playTimes++;
                if(Warn_Play_controlBit & 0x04)
                    tts( "���ѳ�ʱ��ʻ����ע����Ϣ" );
                if(GB19056.SPK_DriveExceed_Warn.group_playTimes >= 4)
                    GB19056.SPK_DriveExceed_Warn.group_playTimes = 0;
                return;
            }
            // 3.2    timer
            GB19056.SPK_DriveExceed_Warn.FiveMin_sec_counter++;
            if(GB19056.SPK_DriveExceed_Warn.FiveMin_sec_counter >= 300)
            {
                GB19056.SPK_DriveExceed_Warn.group_playTimes = 1;
                GB19056.SPK_DriveExceed_Warn.FiveMin_sec_counter = 0;
            }

        }

    }
    else
        GB_SPK_DriveExceed_clear();
    //  2.   �ٶ��쳣
    if(GB19056.SPK_SpeedStatus_Abnormal.Warn_state_Enable)
    {
        //   ��ǰ1�����ȼ� ��������½���
        if(GB19056.SPK_Speed_Warn.group_playTimes == 0)
        {
            //  2.0.  firs trigger
            if(GB19056.SPK_SpeedStatus_Abnormal.Warn_state_Enable == 1)
            {
                GB19056.SPK_SpeedStatus_Abnormal.Warn_state_Enable = 2;
                GB19056.SPK_SpeedStatus_Abnormal.group_playTimes = 1;
            }
            //  2.1  play  operate
            if((GB19056.SPK_SpeedStatus_Abnormal.group_playTimes))
            {
                GB19056.SPK_SpeedStatus_Abnormal.group_playTimes++;
                if(Warn_Play_controlBit & 0x08)
                    tts( "�ٶ�״̬�쳣���밲ȫ��ʻ" );

                if(GB19056.SPK_SpeedStatus_Abnormal.group_playTimes >= 4)
                {
                    GB19056.SPK_SpeedStatus_Abnormal.group_playTimes = 0;
                    GB19056.SPK_SpeedStatus_Abnormal.Warn_state_Enable = 0; // clear     �ٶ��쳣һ��һ��
                }
                return;
            }
            // 2.2    timer
            GB19056.SPK_SpeedStatus_Abnormal.FiveMin_sec_counter++;
            if(GB19056.SPK_SpeedStatus_Abnormal.FiveMin_sec_counter >= 300)
            {
                GB19056.SPK_SpeedStatus_Abnormal.group_playTimes = 1;
                GB19056.SPK_SpeedStatus_Abnormal.FiveMin_sec_counter = 0;
            }


        }
    }
    else
        GB_SPK_SpeedStatus_Abnormal_clear();
    //   1.���ٱ���
    if(GB19056.SPK_Speed_Warn.Warn_state_Enable)
    {
        //  1.0.  firs trigger
        if(GB19056.SPK_Speed_Warn.Warn_state_Enable == 1)
        {
            GB19056.SPK_Speed_Warn.Warn_state_Enable = 2;
            GB19056.SPK_Speed_Warn.group_playTimes = 1;
        }
        //  1.1  play  operate
        if((GB19056.SPK_Speed_Warn.group_playTimes))
        {
            GB19056.SPK_Speed_Warn.group_playTimes++;
            if(Warn_Play_controlBit & 0x10)
                tts( "���ѳ����밲ȫ��ʻ" );
            // rt_kprintf("\r\n-----���ѳ����밲ȫ��ʻ\r\n");
            if(GB19056.SPK_Speed_Warn.group_playTimes >= 4)
                GB19056.SPK_Speed_Warn.group_playTimes = 0;
            return;
        }
        // 1.2    timer
        GB19056.SPK_Speed_Warn.FiveMin_sec_counter++;
        if(GB19056.SPK_Speed_Warn.FiveMin_sec_counter >= 300)
        {
            GB19056.SPK_Speed_Warn.group_playTimes = 1;
            GB19056.SPK_Speed_Warn.FiveMin_sec_counter = 0;
        }

    }
    else
        GB_SPK_Speed_Warn_clear();



}


//    �¹��ɵ�3 �����о�: ���������ٶȣ�����λ��10s  ���仯
void GB_doubt_Data3_Trigger(u32  lati_old, u32 longi_old, u32  lati_new, u32 longi_new)
{
    //  �ŵ�   process_rmc ��
    u32   Delta_lati = 0, Delta_longi = 0;

    Delta_lati = abs(lati_old - lati_new);
    Delta_longi = abs(longi_old - longi_new);

    // rt_kprintf("\r\ndeltalati: %d    deltati_longi: %d\r\n",Delta_lati,Delta_longi);

    if((current_speed_10x > 50) && (Sps_larger_5_counter > 10) && (Delta_lati < 20) && (Delta_longi < 20) && (GB19056.DoubtData3_triggerFlag == 0))
    {
        GB19056.DoubtData3_counter++;
        if(GB19056.DoubtData3_counter == 21)
        {
            memcpy(VdrData.H_10, gb_BCD_datetime, 6 );// bcd

        }
        if(GB19056.DoubtData3_counter >= 30)
        {
            GB19056.DoubtData3_counter = 0;
            GB19056.DoubtData3_triggerFlag = 1; // enable
            moni_drv(0x10, 10);
        }
    }
    else if(!((current_speed_10x > 50) && (Delta_lati < 10) && (Delta_longi < 10)))
    {
        GB19056.DoubtData3_counter = 0;
        GB19056.DoubtData3_triggerFlag = 0; // clear
    }

}

//    �ٶ�״̬��־�ж�
void  GB_SpeedSatus_Judge(void)
{
    u16  deltaSpd = 0;
    u8  spd_reg = 0;

    // ֻҪGPS �ٶ�(�ο��ٶȴ���40 �� �ж�)
    if(gps_speed > 400)
    {
        //0 . �ο��ٶȴ���40 ��ʼ����
        GB19056.gps_largeThan40_counter++;
        //1.  �����ٶȲ�ֵ
        deltaSpd = abs(gps_speed - car_data.get_speed) * 100; // �ٶȲ�ֵ����100��
        // 2. ����11%  ������
        if((deltaSpd / gps_speed) >= 11)
        {
            GB19056.delataSpd_largerThan11_counter++;
            //  ����ٶ�ƫ��һֱС��11% ,��ô�״δ���11%
            if(GB19056.delataSpd_largerThan11_counter == 0)
            {
                GB19056.gps_largeThan40_counter = 0; // clear
                GB19056.speed_larger40km_counter = 0; // clear
            }
        }
        else
        {
            //  ����ٶ�ƫ��һֱ����11% ,��ô�״�С��11%
            if(GB19056.delataSpd_largerThan11_counter)
            {
                GB19056.gps_largeThan40_counter = 0; // clear
                GB19056.speed_larger40km_counter = 0; // clear
            }
            GB19056.delataSpd_largerThan11_counter = 0;
        }
        // ��ʼ���������ж�
        if(GB19056.gps_largeThan40_counter)
        {
            if(GB19056.gps_largeThan40_counter == 2) // ��0�뿪ʼ
                GB19056.start_record_speedlog_flag = 1;

            if( (GB19056.start_record_speedlog_flag) && (VdrData.H_15_saveFlag == 0))
            {
                if((GB19056.speed_larger40km_counter == 0) && (jt808_status & BIT_STATUS_FIXED )) //
                {
                    //   1. ʱ�䷢����ʱ��
                    memcpy(VdrData.H_15 + 1, gb_BCD_datetime, 6);

                }
                //2.  set  15  speed
                spd_reg = car_data.get_speed % 10;
                if(spd_reg >= 5)
                    spd_reg = car_data.get_speed / 10 + 1;
                else
                    spd_reg = car_data.get_speed / 10;
                VdrData.H_15[13 + 2 * GB19056.speed_larger40km_counter] = spd_reg;
                spd_reg = gps_speed % 10;
                if(spd_reg >= 5)
                    spd_reg = gps_speed / 10 + 1;
                else
                    spd_reg = gps_speed / 10;
                VdrData.H_15[14 + 2 * GB19056.speed_larger40km_counter] = spd_reg;

                GB19056.speed_larger40km_counter++;
                if(GB19056.speed_larger40km_counter >= 60)
                {
                    if(GB19056.speed_larger40km_counter == 60)
                    {
                        VdrData.H_15_saveFlag = 2;
                    }
                }
            }
        }

        //    End Judge     5 �����Ժ�
        if(GB19056.gps_largeThan40_counter >= 299)
        {
            // �쳣�ͼ�����ֵ���
            if(GB19056.delataSpd_largerThan11_counter == GB19056.gps_largeThan40_counter)
            {
                VdrData.H_15[0] = 0x02;	//�쳣
                GB19056.SPK_SpeedStatus_Abnormal.Warn_state_Enable = 1;
            }
            else
                VdrData.H_15[0] = 0x01;

            //   1. ������ʱ��
            memcpy(VdrData.H_15 + 7, gb_BCD_datetime, 6);

            //  ʹ�ܴ洢
            VdrData.H_15_saveFlag = 1;

            GB19056.speedlog_Day_save_times++;
            if(GB19056.workstate == 0)
                rt_kprintf("\r\n  �ٶ�״̬��־����!:%d  total:%d  abnormal:%d\r\n", VdrData.H_15[0], GB19056.gps_largeThan40_counter, GB19056.delataSpd_largerThan11_counter);
            GB19056.speed_larger40km_counter = 0;	 // clear
            GB19056.delataSpd_largerThan11_counter = 0; // clear


            //---------------------------------------
            GB19056.gps_largeThan40_counter = 0; // clear
        }
    }
}

u32 Time_sec_u32(u8 *instr, u8 len)
{
    u32  value = 0;

    if(len < 6)
        return  0;
    value = (BCD2HEX(instr[0]) * 356 + BCD2HEX(instr[1]) * 32 + BCD2HEX(instr[2])) * 86400 + \
            BCD2HEX(instr[3]) * 3600 + BCD2HEX(instr[4]) * 60 + BCD2HEX(instr[5]);

    return value;
}

u8  Timezone_judge(u32 Read_Time)
{
    if((Read_Time > GB19056.Query_Start_Time) && (Read_Time <= GB19056.Query_End_Time))
        return 0;
    else if(Read_Time <= GB19056.Query_Start_Time)	 // ��ǰʱ��С��С��
        return 2;  //ֱ��break
    else
        return  1; //��ǰʱ����ڴ�ģ������ң�ֱ��С�ڵ���С��
}


void  GB19056_Decode(u8 *instr, u8  len)
{
    u8  year, month, day, hour, min, sec;
    u32  reg_dis = 0;
    u8  temp_buffer[6];
    // AA 75   already   check     AA 75 08 00 01 00 BB
    //  cmd  ID
    switch(instr[2])
    {
    case  0x08:     //  �ɼ���¼��ָ������ʻ�ٶȼ�¼
    case  0x09:    //  �ɼ�ָ����ָ��λ����Ϣ��¼
    case  0x10:   // �ɼ��¹��ɵ��¼
    case  0x11:    //  �ɼ�ָ���ĳ�ʱ��ʻ��¼
    case  0x12:    //  �ɼ�ָ���ļ�ʻ����ݼ�¼
    case  0x13:    // �ɼ�ָ�����ⲿ�����¼
    case  0x14:    //  �ɼ�ָ���Ĳ����޸ļ�¼
    case  0x15:    // �ɼ�ָ�����ٶ�״̬��־
        GB19056.Query_Start_Time = Time_sec_u32(instr + 6, 6);
        GB19056.Query_End_Time = Time_sec_u32(instr + 12, 6);
        GB19056.Max_dataBlocks = (instr[18] << 8) + instr[19];



    case  0x00:   //�ɼ���¼��ִ�а汾
    case  0x01:   //  �ɼ���ʻ����Ϣ
    case  0x02:   // �ɼ���¼��ʱ��
    case  0x03:   //  �ɼ��ۼ���ʻ���
    case  0x04:   // �ɼ���¼������ϵ��
    case  0x05:   // �ɼ�������Ϣ
    case  0x06:    // �ɼ���¼���ź�������Ϣ
    case  0x07:    //  �ɼ���¼��Ψһ�Ա��
    case  0xFE:    // ALL
    case  0xFB:	   //  Recommond
        DF_Sem_Take;
        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        DF_Sem_Release;
        break;
        //  ���ü�¼�����
    case 0x82: //	  �������ó��ƺ�
        memset(jt808_param.id_0xF005, 0, sizeof(jt808_param.id_0xF005));
        memset(jt808_param.id_0x0083, 0, sizeof(jt808_param.id_0x0083));
        memset(jt808_param.id_0xF00A, 0, sizeof(jt808_param.id_0xF00A));

        //-----------------------------------------------------------------------
        memcpy(jt808_param.id_0xF005, instr + 6, 17);
        memcpy(jt808_param.id_0x0083, instr + 23, 12);
        memcpy(jt808_param.id_0xF00A, instr + 25, 12);
        //  �洢
        param_save( 1 );

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;

    case 0xC2: //���ü�¼��ʱ��
        // ûɶ�ã������ظ����У�����GPSУ׼�͹���
        //  ������ ʱ���� BCD
        year = (instr[6] >> 4) * 10 + (instr[6] & 0x0F);
        month = (instr[7] >> 4) * 10 + (instr[7] & 0x0F);
        day = (instr[8] >> 4) * 10 + (instr[8] & 0x0F);
        hour = (instr[9] >> 4) * 10 + (instr[9] & 0x0F);
        min = (instr[10] >> 4) * 10 + (instr[10] & 0x0F);
        sec = (instr[11] >> 4) * 10 + (instr[11] & 0x0F);
        //  update  RTC
        date_set(year, month, day);
        time_set(hour, min, sec);

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;


    case 0xC3: //�����ٶ�����ϵ��������ϵ����
        // ǰ6 ���ǵ�ǰʱ��
        year = (instr[6] >> 4) * 10 + (instr[6] & 0x0F);
        month = (instr[7] >> 4) * 10 + (instr[7] & 0x0F);
        day = (instr[8] >> 4) * 10 + (instr[8] & 0x0F);
        hour = (instr[9] >> 4) * 10 + (instr[9] & 0x0F);
        min = (instr[10] >> 4) * 10 + (instr[10] & 0x0F);
        sec = (instr[11] >> 4) * 10 + (instr[11] & 0x0F);

        //  update  RTC
        date_set(year, month, day);
        time_set(hour, min, sec);

        jt808_param.id_0xF032 = (instr[12] << 8) + (u32)instr[13]; // ����ϵ��  �ٶ�����ϵ��

        //  �洢
        param_save( 1 );

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;
    case 0x83: //  ��¼�ǳ��ΰ�װʱ��

        memcpy(temp_buffer, instr + 6, 6);
        jt808_param.id_0xF030 = mytime_from_bcd(temp_buffer);
        //  �洢
        param_save( 1 );

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;
    case 0x84: // �����ź���������Ϣ
        //memcpy(Setting08,instr+6,80);
        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;

    case 0xC4: //   ���ó�ʼ���

        year = (instr[6] >> 4) * 10 + (instr[6] & 0x0F);
        month = (instr[7] >> 4) * 10 + (instr[7] & 0x0F);
        day = (instr[8] >> 4) * 10 + (instr[8] & 0x0F);
        hour = (instr[9] >> 4) * 10 + (instr[9] & 0x0F);
        min = (instr[10] >> 4) * 10 + (instr[10] & 0x0F);
        sec = (instr[11] >> 4) * 10 + (instr[11] & 0x0F);
        //  update  RTC
        date_set(year, month, day);
        time_set(hour, min, sec);

        memcpy(temp_buffer, instr + 6, 6);
        jt808_param.id_0xF030 = mytime_from_bcd(temp_buffer);
        //Api_Config_Recwrite_Large(jt808,0,(u8*)&JT808Conf_struct,sizeof(JT808Conf_struct));
        // instr     + 12      ���ó�ʼ��̵�λ��   BCD

        reg_dis = (instr[18] >> 4) * 10000000 + (instr[18] & 0x0F) * 1000000 + (instr[19] >> 4) * 100000 + (instr[19] & 0x0F) * 10000 \
                  +(instr[20] >> 4) * 1000 + (instr[20] & 0x0F) * 100 + (instr[21] >> 4) * 10 + (instr[21] & 0x0F);


        jt808_data.id_0xFA01 = reg_dis * 100;
        //  �洢
        param_save( 1 );
        data_save();

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;
    case 0xE0:   //pMenuItem=&Menu_8_statechecking;  //  ��ʽ����û�м춨״̬
        //pMenuItem->show();
        // out_plus(0);
        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        break;

    case 0xE4:
        pMenuItem = &Menu_1_Idle;
        pMenuItem->show();
        // out_plus(0);
        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        break;

    case 0xE1:  //GB19056.Deltaplus_outEnble=3;
        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        break;
    case 0xE2: // output_spd(car_data.get_speed);
        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        break;
    case 0xE3: // output_rtcplus(1);
        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        break;

    case  0xFF:    //��ԭ��ʾ״̬
        GB19056.workstate = 0;
        printf_on = 1;	// ʹ�ܴ�Ӧ
        rt_kprintf(" GB_Data_disable\r\n");
        break;
    default:
        break;




    }
}

void  gb_usb_out(u8 ID)
{
    /*
               ������Ϣ��USB device
      */
    if(USB_Disk_RunStatus() == USB_FIND)
    {
        if(GB19056.usb_exacute_output == 2)
        {

            if(GB19056.workstate == 0)
                rt_kprintf("\r\n GB USB ���ݵ����� \r\n");
            return;
        }
        else
        {

            switch(ID)
            {
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            case 0x08:
            case 0x09:
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:

            case 0xFE:  // output all
            case 0xFB:  // recommond output
            case 0xFF:  //idle
                break;
            default:
                if(GB19056.workstate == 0)
                    rt_kprintf("\r\n ����ID ����!  idle:  FF    0x00~0x15     0xFE :out put all        0xFB : output  recommond info   \r\n");
                return;


            }
            GB19056.usb_write_step = GB_USB_OUT_start;
            GB19056.usb_exacute_output = 1;
            GB19056.usb_out_selectID = ID;


            if(GB19056.workstate == 0)
                rt_kprintf("\r\n USB ���ݵ���\r\n");
        }
    }
    else
    {
        if(GB19056.workstate == 0)
            rt_kprintf("\r\n û�м�⵽ USB device \r\n");
    }

}
FINSH_FUNCTION_EXPORT(gb_usb_out, gb_usb_out(ID) idle:  FF    0x00~0x15     0xFE : out put all        0xFB : output  recommond info   );
/*
    ( ����: CMD,
      ����: u8*discrip_str,
      ��ǰ�洢��¼��Ŀ:u16 Ownindex_num,
      ������¼�Ĵ�Сu16 indexSize)
*/
void GB_USBfile_stuff(u8 CMD, u8 *discrip_str, u16 Ownindex_num, u16 indexSize)
{
    u8  discr_str_local[18];
    u16  i = 0, pkg_counter = 0, get_indexnum = 0;
    u16  pakge_num = 0;
    u16 distr_len = strlen(discrip_str);
    u32  content_len = 0; // �����Ϣ����
    u32  regdis 			= 0, reg2 = 0;
    u8  pos_V = 0;
    u8   add_fcs_byte = 0;

    memset(discr_str_local, 0, sizeof(discr_str_local));
    memcpy(discr_str_local, discrip_str, distr_len);
    usb_outbuf_Wr = 0; // star from here
    if(CMD == 0x00)
    {
        // ��һ�� �����ݿ����
        usb_outbuf[usb_outbuf_Wr++] = 0x00;
        usb_outbuf[usb_outbuf_Wr++] = 0x10;
    }

    // cmd
    usb_outbuf[usb_outbuf_Wr++] = CMD;
    // discription
    memcpy(usb_outbuf + usb_outbuf_Wr, discr_str_local, 18);
    usb_outbuf_Wr += 18;
    //    length
    if(CMD != 0x09)
    {
        content_len = indexSize * Ownindex_num;
        pakge_num = Ownindex_num;
    }
    else
    {
        pos_V = 0;
        for(i = 0; i < 60; i++)
        {
            if((VdrData.H_09[6 + i * 11] == 0x7F) && (VdrData.H_09[6 + i * 11 + 4] == 0x7F))
                pos_V++;   // λ����Ч���ۼ�
        }
        if(pos_V != 60)
        {
            content_len = indexSize * (Ownindex_num + 1); // 09 ��һ���ǵ�ǰ��
            pakge_num = Ownindex_num + 1;
        }
        else
        {
            content_len = indexSize * (Ownindex_num);
            pakge_num = Ownindex_num + 1;
        }
    }

    usb_outbuf[usb_outbuf_Wr++]   = (u8)(content_len >> 24);
    usb_outbuf[usb_outbuf_Wr++]   = (u8)(content_len >> 16);
    usb_outbuf[usb_outbuf_Wr++]   = (u8)(content_len >> 8);
    usb_outbuf[usb_outbuf_Wr++]   = (u8)(content_len);


    // content  stuff
    switch(CMD)
    {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
        usb_outbuf_Wr += GB_00_07_info_only(usb_outbuf + usb_outbuf_Wr, CMD);
        USB_OUT;
        break;

    case 0x08:	    //  д������ͷ
        USB_OUT;
        // д������
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  ÿ����¼дһ��
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_08H_Write >= (pkg_counter + 1)) //û����
                get_indexnum = Vdr_Wr_Rd_Offset.V_08H_Write - 1 - pkg_counter;
            else
                get_indexnum = VDR_08_MAXindex + Vdr_Wr_Rd_Offset.V_08H_Write - 1 - pkg_counter;

            gb_vdr_get_08h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
            usb_outbuf_Wr += indexSize;
            USB_OUT;
        }
        break;
    case 0x09:
        //  д������ͷ
        USB_OUT;
        // д������
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  ÿ����¼дһ��
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_09H_Write >= pkg_counter) //û����
                get_indexnum = Vdr_Wr_Rd_Offset.V_09H_Write - pkg_counter;
            else
                get_indexnum = VDR_09_MAXindex + Vdr_Wr_Rd_Offset.V_09H_Write - pkg_counter;
            //------------------------------------------------------------------------------
            if(pkg_counter == 0)
            {
                // ��ǰСʱ������
                memcpy(VdrData.H_09, gb_BCD_datetime, 6);
                VdrData.H_09[4] = 0;
                VdrData.H_09[5] = 0;

                pos_V = 0;
                for(i = 0; i < 60; i++)
                {
                    if((VdrData.H_09[6 + i * 11] == 0x7F) && (VdrData.H_09[6 + i * 11 + 4] == 0x7F))
                        pos_V++;   // λ����Ч���ۼ�
                }
                if(pos_V != 60)
                {
                    memcpy(usb_outbuf + usb_outbuf_Wr, VdrData.H_09, 666);
                    usb_outbuf_Wr += indexSize;
                }
            }

            else
            {
                gb_vdr_get_09h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
                usb_outbuf_Wr += indexSize;
            }
            USB_OUT;
        }
        break;
    case 0x10:
        USB_OUT;
        // д������
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  ÿ����¼дһ��
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_10H_Write >= (pkg_counter + 1)) //û����
                get_indexnum = Vdr_Wr_Rd_Offset.V_10H_Write - 1 - pkg_counter;
            else
                get_indexnum = VDR_10_MAXindex + Vdr_Wr_Rd_Offset.V_10H_Write - 1 - pkg_counter;

            gb_vdr_get_10h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
            usb_outbuf_Wr += indexSize;
            USB_OUT;
        }
        break;
    case 0x11:
        USB_OUT;
        // д������
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  ÿ����¼дһ��
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_11H_Write >= (pkg_counter + 1)) //û����
                get_indexnum = Vdr_Wr_Rd_Offset.V_11H_Write - 1 - pkg_counter;
            else
                get_indexnum = VDR_11_MAXindex + Vdr_Wr_Rd_Offset.V_11H_Write - 1 - pkg_counter;

            gb_vdr_get_11h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
            usb_outbuf_Wr += indexSize;
            USB_OUT;
        }
        break;
    case 0x12:
        USB_OUT;
        // д������
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  ÿ����¼дһ��
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_12H_Write >= (pkg_counter + 1)) //û����
                get_indexnum = Vdr_Wr_Rd_Offset.V_12H_Write - 1 - pkg_counter;
            else
                get_indexnum = VDR_12_MAXindex + Vdr_Wr_Rd_Offset.V_12H_Write - 1 - pkg_counter;

            gb_vdr_get_12h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
            usb_outbuf_Wr += indexSize;
            USB_OUT;
        }
        break;
    case 0x13:
        USB_OUT;
        // д������
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  ÿ����¼дһ��
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_13H_Write >= (pkg_counter + 1)) //û����
                get_indexnum = Vdr_Wr_Rd_Offset.V_13H_Write - 1 - pkg_counter;
            else
                get_indexnum = VDR_13_MAXindex + Vdr_Wr_Rd_Offset.V_13H_Write - 1 - pkg_counter;

            gb_vdr_get_13h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
            usb_outbuf_Wr += indexSize;
            USB_OUT;
        }
        break;
    case 0x14:
        USB_OUT;
        // д������
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  ÿ����¼дһ��
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_14H_Write >= (pkg_counter + 1)) //û����
                get_indexnum = Vdr_Wr_Rd_Offset.V_14H_Write - 1 - pkg_counter;
            else
                get_indexnum = VDR_14_MAXindex + Vdr_Wr_Rd_Offset.V_14H_Write - 1 - pkg_counter;

            GB_VDR_READ_14H(get_indexnum, usb_outbuf + usb_outbuf_Wr);
            usb_outbuf_Wr += indexSize;
            USB_OUT;
        }
        break;
    case 0x15:
        USB_OUT;
        // д������
        if(pakge_num == 0)
        {
            add_fcs_byte = 1;
            USB_OUT;
            break;
        }
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  ÿ����¼дһ��
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_15H_Write >= (pkg_counter + 1)) //û����
                get_indexnum = Vdr_Wr_Rd_Offset.V_15H_Write - 1 - pkg_counter;
            else
                get_indexnum = VDR_15_MAXindex + Vdr_Wr_Rd_Offset.V_15H_Write - 1 - pkg_counter;

            gb_vdr_get_15h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
            usb_outbuf_Wr += indexSize;

            if((pkg_counter + 1) == pakge_num)
                add_fcs_byte = 1;
            USB_OUT;
        }
        break;

    }

}


void GB_OUT_data(u8  ID, u8 type, u16 IndexNum, u16 total_pkg_num)
{

    u32	regdis			   = 0, reg2 = 0;
    u16	SregLen = 0;
    u16  i = 0, subi = 0, continue_counter = 0;
    u16   get_indexnum = 0; // ��ȡУ��
    //u16  Sd_DataBloknum=0; //   �������ݿ�  ��Ŀ <GB19056.Max_dataBlocks
    u8   Not_null_flag = 0; // NO  data ACK null once
    u8   return_value = 0;
    u32  Rd_Time = 0;

    switch(ID)
    {
    case 0x00:  //  ִ�б�׼�汾���
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "ִ�б�׼�汾���", 1, 2);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;
    case 0x01:

        //---------------- �ϴ�������  -------------------------------------
        //	01	��ǰ��ʻԱ���뼰��Ӧ�Ļ�������ʻ֤��
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "��ǰ��ʻ����Ϣ", 1, 18);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;
    case 0x02: //  02  �ɼ���¼�ǵ�ʵʱʱ��
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "ʵʱʱ��", 1, 6);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;

    case 0x03:		// 03 �ɼ�360h�����
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "�ۼ���ʻ���", 1, 20);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;												 // 03 �ɼ�360h�����
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;

    case 0x04:																	// 04  �ɼ���¼������ϵ��
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "����ϵ��", 1, 8);
            return;
        }
        //------------------------------------------------------------

        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;
    case 0x05:																	//05	  ������Ϣ�ɼ�
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "������Ϣ", 1, 41);
            return;
        }
        //------------------------------------------------------------

        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);

        break;
    case   0x06:													// 06-09
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "״̬�ź�������Ϣ", 1, 87);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;

    case 0x07:																//07  ��¼��Ψһ���
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "��¼��Ψһ�Ա��", 1, 35);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;
    case 0x08:															//	08	 �ɼ�ָ������ʻ�ٶȼ�¼
        //  ��ȡ��ǰ�ܰ���
        if(Vdr_Wr_Rd_Offset.V_08H_full)
        {
            GB19056.Total_dataBlocks = VDR_08_MAXindex;
        }
        else
            GB19056.Total_dataBlocks = Vdr_Wr_Rd_Offset.V_08H_Write;
        //   USB
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "��ʻ�ٶȼ�¼", GB19056.Total_dataBlocks, 126);
            return;
        }
        // Serial
#if 1
        if(type == GB_OUT_TYPE_Serial)
        {
            //---------------------------------------------------------------------
            if(Vdr_Wr_Rd_Offset.V_08H_get == 0)
            {
                GB_serial_get_clear();
                Vdr_Wr_Rd_Offset.V_08H_get = 1;



                GB19056.Send_add = 0; // clear
            }
            //--------------------------------------------------------------------
            usb_outbuf_Wr = 0;
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// ��ʼͷ
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x08;					//������


            SregLen = GB19056.Max_dataBlocks * 126; //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// ������
            //	��Ϣ����
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < GB19056.Total_dataBlocks)
            {

                //---------------------------------------------------------------------
                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {
                    if(Vdr_Wr_Rd_Offset.V_08H_Write >= (subi + 1)) //û����
                        get_indexnum = Vdr_Wr_Rd_Offset.V_08H_Write - 1 - subi;
                    else
                        get_indexnum = VDR_08_MAXindex + Vdr_Wr_Rd_Offset.V_08H_Write - 1 - subi;

                    gb_vdr_get_08h(get_indexnum, usb_outbuf + usb_outbuf_Wr); //126*5=630		 num=576  packet

                    //-----time  filter ------------
                    Rd_Time = Time_sec_u32(usb_outbuf + usb_outbuf_Wr, 6);
                    return_value = Timezone_judge(Rd_Time);

                    if(return_value) // end
                    {
                        continue;
                    }
                    else
                        usb_outbuf_Wr += 126;
                    continue_counter++;
                    if(continue_counter == GB19056.Max_dataBlocks)
                        break;
                }
                //---------------------------------------------------
                if(continue_counter != GB19056.Max_dataBlocks) // �����Ϣ��Ϊ�գ�����
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 126; // ��û���õ���Ϣȥ��
                    usb_outbuf[3]   = (SregLen >> 8);			// Hi
                    usb_outbuf[4]   = SregLen;				// Lo
                }
                // -----update---
                GB19056.Send_add += continue_counter;
                OUT_PUT;

                if(GB19056.Send_add >= GB19056.Total_dataBlocks)
                    GB_serial_get_clear();// clear


            }
            else
                GB_null_content(0x08, type);




            //----------------------------------------------------------------------
        }
#endif

        break;


    case   0x09:															// 09	ָ����λ����Ϣ��¼
        //  Get total effective num
        if(Vdr_Wr_Rd_Offset.V_09H_full)
        {
            GB19056.Total_dataBlocks = VDR_09_MAXindex;
        }
        else
            GB19056.Total_dataBlocks = Vdr_Wr_Rd_Offset.V_09H_Write;

        //   USB
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "λ����Ϣ��¼", GB19056.Total_dataBlocks, 666);
            return;
        }
        //   Serial
#if 1
        if(type == GB_OUT_TYPE_Serial)
        {
            //---------------------------------------------------------------------
            if(Vdr_Wr_Rd_Offset.V_09H_get == 0)
            {
                GB_serial_get_clear();
                Vdr_Wr_Rd_Offset.V_09H_get = 1;

                GB19056.Send_add = 0; // clear
            }
            //--------------------------------------------------------------------
            usb_outbuf_Wr = 0;
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// ��ʼͷ
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x09;					//������


            SregLen = 666;  //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// ������
            //	��Ϣ����
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < (GB19056.Total_dataBlocks + 1))
            {

                for(subi = GB19056.Send_add; subi < (GB19056.Total_dataBlocks + 1); subi++)
                {
                    if(subi == 0)
                    {
                        // ��ǰСʱ������
                        memcpy(VdrData.H_09, gb_BCD_datetime, 6);
                        VdrData.H_09[4] = 0;
                        VdrData.H_09[5] = 0;

                        memcpy(usb_outbuf + usb_outbuf_Wr, VdrData.H_09, 666);
                    }
                    else
                    {
                        if(Vdr_Wr_Rd_Offset.V_09H_Write >= subi) //û����
                            get_indexnum = Vdr_Wr_Rd_Offset.V_09H_Write - subi;
                        else
                            get_indexnum = VDR_09_MAXindex + Vdr_Wr_Rd_Offset.V_09H_Write - subi;

                        gb_vdr_get_09h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
                    }
                    //-----time  filter ------------
                    Rd_Time = Time_sec_u32(usb_outbuf + usb_outbuf_Wr, 6);
                    return_value = Timezone_judge(Rd_Time);

                    if(return_value) // end
                    {
                        continue;
                    }
                    else
                        usb_outbuf_Wr += 666;
                    continue_counter++;
                    break;
                }
                //---------------------------------------------------
                if(continue_counter == 0) // �����Ϣ��Ϊ�գ�����
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 666; // ��û���õ���Ϣȥ��
                    usb_outbuf[3]   = (SregLen >> 8);			// Hi
                    usb_outbuf[4]   = SregLen;				// Lo
                }
                // -----update---
                GB19056.Send_add += continue_counter;
                OUT_PUT;

                if(GB19056.Send_add >= GB19056.Total_dataBlocks)
                    GB_serial_get_clear();// clear

            }
            else
                GB_null_content(0x09, type);




            //----------------------------------------------------------------------
        }
#endif
        break;
    case   0x10:															// 10-13	 10   �¹��ɵ�ɼ���¼
        //�¹��ɵ�����   ------    �¹��ɵ�״̬�ǵ������е�
        //  Get total effective num
        if(Vdr_Wr_Rd_Offset.V_10H_full)
        {
            GB19056.Total_dataBlocks = VDR_10_MAXindex;
        }
        else
            GB19056.Total_dataBlocks = Vdr_Wr_Rd_Offset.V_10H_Write;

        //   USB
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "�¹��ɵ��¼", GB19056.Total_dataBlocks, 234);
            return;
        }
        //  serial
#if 1
        if(type == GB_OUT_TYPE_Serial)
        {
            //---------------------------------------------------------------------
            if(Vdr_Wr_Rd_Offset.V_10H_get == 0)
            {
                GB_serial_get_clear();
                Vdr_Wr_Rd_Offset.V_10H_get = 1;

                GB19056.Send_add = 0; // clear
            }


            if(GB19056.Max_dataBlocks >= 5)
                GB19056.Max_dataBlocks = 4;
            //--------------------------------------------------------------------
            usb_outbuf_Wr = 0;
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// ��ʼͷ
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x10;					//������


            SregLen = GB19056.Max_dataBlocks * 234; //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// ������
            //	��Ϣ����
            Not_null_flag = 0;
            continue_counter = 0;


            if(GB19056.Send_add < (GB19056.Total_dataBlocks))
            {

                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_10H_Write >= (subi + 1)) //û����
                        get_indexnum = Vdr_Wr_Rd_Offset.V_10H_Write - 1 - subi;
                    else
                        get_indexnum = VDR_10_MAXindex + Vdr_Wr_Rd_Offset.V_10H_Write - 1 - subi;

                    gb_vdr_get_10h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
                    //-----time  filter ------------
                    Rd_Time = Time_sec_u32(usb_outbuf + usb_outbuf_Wr, 6);
                    return_value = Timezone_judge(Rd_Time);
                    // rt_kprintf("\r\n return_value=%d \r\n",return_value);
                    if(return_value) // end
                    {
                        continue;
                    }
                    else
                        usb_outbuf_Wr += 234;
                    continue_counter++;
                    if(continue_counter == GB19056.Max_dataBlocks)
                        break;
                }
                //---------------------------------------------------
                if(continue_counter != GB19056.Max_dataBlocks) // �����Ϣ��Ϊ�գ�����
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 234; // ��û���õ���Ϣȥ��
                    usb_outbuf[3]   = (SregLen >> 8);			// Hi
                    usb_outbuf[4]   = SregLen;				// Lo
                }
                // -----update---
                GB19056.Send_add += continue_counter;
                OUT_PUT;

                if(GB19056.Send_add >= GB19056.Total_dataBlocks)
                    GB_serial_get_clear();// clear

            }
            else
                GB_null_content(0x10, type);




            //----------------------------------------------------------------------
        }
#endif

        break;


    case  0x11: 															// 11 �ɼ�ָ���ĵĳ�ʱ��ʻ��¼
        //  Get total effective num
        if(Vdr_Wr_Rd_Offset.V_11H_full)
        {
            GB19056.Total_dataBlocks = VDR_11_MAXindex;
        }
        else
            GB19056.Total_dataBlocks = Vdr_Wr_Rd_Offset.V_11H_Write;

        //   USB
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "��ʱ��ʻ��¼", GB19056.Total_dataBlocks, 50);
            return;
        }
        //  serial
#if 1
        if(type == GB_OUT_TYPE_Serial)
        {
            if(VdrData.H_11_lastSave)
            {
                GB19056.Total_dataBlocks = 1 + Vdr_Wr_Rd_Offset.V_11H_Write;
                GB_serial_get_clear();
            }
            //---------------------------------------------------------------------
            if(Vdr_Wr_Rd_Offset.V_11H_get == 0)
            {
                GB_serial_get_clear();
                Vdr_Wr_Rd_Offset.V_11H_get = 1;

                GB19056.Send_add = 0; // clear
            }
            //--------------------------------------------------------------------
            usb_outbuf_Wr = 0;
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// ��ʼͷ
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x11;					//������

            if(VdrData.H_11_lastSave)
                SregLen = GB19056.Max_dataBlocks * 50;
            else
                SregLen = 50;  //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// ������
            //	��Ϣ����
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < GB19056.Total_dataBlocks)
            {

                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {
                    if(VdrData.H_11_lastSave)
                        get_indexnum = Vdr_Wr_Rd_Offset.V_11H_Write;
                    else
                    {
                        if(Vdr_Wr_Rd_Offset.V_11H_Write >= (subi + 1)) //û����
                            get_indexnum = Vdr_Wr_Rd_Offset.V_11H_Write - 1 - subi;
                        else
                            get_indexnum = VDR_11_MAXindex + Vdr_Wr_Rd_Offset.V_11H_Write - 1 - subi;
                    }

                    gb_vdr_get_11h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
                    //-----time  filter ------------
                    Rd_Time = Time_sec_u32(usb_outbuf + usb_outbuf_Wr + 18, 6);
                    return_value = Timezone_judge(Rd_Time);

                    if(return_value) // end
                    {
                        continue;
                    }
                    else
                        usb_outbuf_Wr += 50;
                    continue_counter++;
                    if(VdrData.H_11_lastSave)
                        break;
                    else if(continue_counter == GB19056.Max_dataBlocks)
                        break;

                }
                //---------------------------------------------------
                if(continue_counter != GB19056.Max_dataBlocks) // �����Ϣ��Ϊ�գ�����
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 50; // ��û���õ���Ϣȥ��
                    usb_outbuf[3]   = (SregLen >> 8);			// Hi
                    usb_outbuf[4]   = SregLen;				// Lo
                }
                // -----update---
                if(VdrData.H_11_lastSave == 0)
                    GB19056.Send_add += continue_counter;
                OUT_PUT;

                if(GB19056.Send_add >= GB19056.Total_dataBlocks)
                    GB_serial_get_clear();// clear

            }
            else
                GB_null_content(0x11, type);




            //----------------------------------------------------------------------
        }
#endif

        break;
    case  0x12: 															// 12 �ɼ�ָ����ʻ����ݼ�¼  ---Devide
        //	Get total effective num
        if(Vdr_Wr_Rd_Offset.V_12H_full)
        {
            GB19056.Total_dataBlocks = VDR_12_MAXindex;
        }
        else
            GB19056.Total_dataBlocks = Vdr_Wr_Rd_Offset.V_12H_Write;

        //   USB
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "��ʻ����ݼ�¼", GB19056.Total_dataBlocks, 25);
            return;
        }
        //  serial

#if 1
        if(type == GB_OUT_TYPE_Serial)
        {
            //---------------------------------------------------------------------
            if(Vdr_Wr_Rd_Offset.V_12H_get == 0)
            {
                GB_serial_get_clear();
                Vdr_Wr_Rd_Offset.V_12H_get = 1;

                GB19056.Send_add = 0; // clear
            }
            //--------------------------------------------------------------------
            usb_outbuf_Wr = 0;
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					 // ��ʼͷ
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x12;					 //������

            SregLen = GB19056.Max_dataBlocks * 25;	//

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);		 // Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				 // Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00;					 // ������
            //  ��Ϣ����
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < (GB19056.Total_dataBlocks))
            {

                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_12H_Write >= (subi + 1)) //û����
                        get_indexnum = Vdr_Wr_Rd_Offset.V_12H_Write - 1 - subi;
                    else
                        get_indexnum = VDR_12_MAXindex + Vdr_Wr_Rd_Offset.V_12H_Write - 1 - subi;

                    gb_vdr_get_12h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
                    //-----time  filter ------------
                    Rd_Time = Time_sec_u32(usb_outbuf + usb_outbuf_Wr, 6);
                    return_value = Timezone_judge(Rd_Time);

                    if(return_value) // end
                    {
                        continue;
                    }
                    else
                        usb_outbuf_Wr += 25;
                    continue_counter++;
                    if(continue_counter == GB19056.Max_dataBlocks)
                        break;
                }
                //---------------------------------------------------
                if(continue_counter != GB19056.Max_dataBlocks) // �����Ϣ��Ϊ�գ�����
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 25; // ��û���õ���Ϣȥ��
                    usb_outbuf[3]	= (SregLen >> 8);			// Hi
                    usb_outbuf[4]	= SregLen;				// Lo
                }
                // -----update---
                GB19056.Send_add += continue_counter;
                OUT_PUT;


                if(GB19056.Send_add >= GB19056.Total_dataBlocks)
                    GB_serial_get_clear();// clear
            }
            else
                GB_null_content(0x12, type);




            //----------------------------------------------------------------------
        }
#endif

        break;


    case  0x13: 													// 13 �ɼ���¼���ⲿ�����¼
        //	Get total effective num
        if(Vdr_Wr_Rd_Offset.V_13H_full)
        {
            GB19056.Total_dataBlocks = VDR_13_MAXindex;
        }
        else
            GB19056.Total_dataBlocks = Vdr_Wr_Rd_Offset.V_13H_Write;

        //   USB
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "�ⲿ�����¼", GB19056.Total_dataBlocks, 7);
            return;
        }
        //  serial
        if(type == GB_OUT_TYPE_Serial)
        {
            //---------------------------------------------------------------------
            if(Vdr_Wr_Rd_Offset.V_13H_get == 0)
            {
                GB_serial_get_clear();
                Vdr_Wr_Rd_Offset.V_13H_get = 1;



                GB19056.Send_add = 0; // clear
            }
            //--------------------------------------------------------------------
            usb_outbuf_Wr = 0;
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// ��ʼͷ
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x13;					//������


            SregLen = GB19056.Max_dataBlocks * 7; //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// ������
            //	��Ϣ����
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < (GB19056.Total_dataBlocks))
            {

                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_13H_Write >= (subi + 1)) //û����
                        get_indexnum = Vdr_Wr_Rd_Offset.V_13H_Write - 1 - subi;
                    else
                        get_indexnum = VDR_13_MAXindex + Vdr_Wr_Rd_Offset.V_13H_Write - 1 - subi;


                    gb_vdr_get_13h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
                    //-----time  filter ------------
                    Rd_Time = Time_sec_u32(usb_outbuf + usb_outbuf_Wr, 6);
                    return_value = Timezone_judge(Rd_Time);

                    if(return_value) // end
                    {
                        continue;
                    }
                    else
                        usb_outbuf_Wr += 7;
                    continue_counter++;
                    if(continue_counter == GB19056.Max_dataBlocks)
                        break;
                }
                //---------------------------------------------------
                if(continue_counter != GB19056.Max_dataBlocks) // �����Ϣ��Ϊ�գ�����
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 7; // ��û���õ���Ϣȥ��
                    usb_outbuf[3]	= (SregLen >> 8);			// Hi
                    usb_outbuf[4]	= SregLen;				// Lo
                }
                // -----update---
                GB19056.Send_add += continue_counter;
                OUT_PUT;

                if(GB19056.Send_add >= GB19056.Total_dataBlocks)
                    GB_serial_get_clear();// clear

            }
            else
                GB_null_content(0x13, type);




            //----------------------------------------------------------------------
        }
        break;
    case   0x14:   //   �����޸ļ�¼
        //	Get total effective num
        if(Vdr_Wr_Rd_Offset.V_14H_full)
        {
            GB19056.Total_dataBlocks = VDR_14_MAXindex;
        }
        else
            GB19056.Total_dataBlocks = Vdr_Wr_Rd_Offset.V_14H_Write;
        //   USB
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "�����޸ļ�¼", GB19056.Total_dataBlocks, 7);
            return;
        }
        //  serial
        if(type == GB_OUT_TYPE_Serial)
        {
            //---------------------------------------------------------------------
            if(Vdr_Wr_Rd_Offset.V_14H_get == 0)
            {
                GB_serial_get_clear();
                Vdr_Wr_Rd_Offset.V_14H_get = 1;

                GB19056.Send_add = 0; // clear
            }
            //--------------------------------------------------------------------
            usb_outbuf_Wr = 0;
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// ��ʼͷ
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x14;					//������


            SregLen = GB19056.Max_dataBlocks * 7; //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// ������
            //	��Ϣ����
            Not_null_flag = 0;
            continue_counter = 0;


            if(GB19056.Send_add < GB19056.Total_dataBlocks)
            {
                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_14H_Write >= (subi + 1)) //û����
                        get_indexnum = Vdr_Wr_Rd_Offset.V_14H_Write - 1 - subi;
                    else
                        get_indexnum = VDR_14_MAXindex + Vdr_Wr_Rd_Offset.V_14H_Write - 1 - subi;

                    GB_VDR_READ_14H(get_indexnum, usb_outbuf + usb_outbuf_Wr);
                    //-----time  filter ------------
                    Rd_Time = Time_sec_u32(usb_outbuf + usb_outbuf_Wr, 6);
                    return_value = Timezone_judge(Rd_Time);

                    if(return_value) // end
                    {
                        continue;
                    }
                    else
                        usb_outbuf_Wr += 7;
                    continue_counter++;

                    if(continue_counter == GB19056.Max_dataBlocks)
                        break;
                }

                //---------------------------------------------------
                if(continue_counter != GB19056.Max_dataBlocks) // �����Ϣ��Ϊ�գ�����
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 7; // ��û���õ���Ϣȥ��
                    usb_outbuf[3]	= (SregLen >> 8);			// Hi
                    usb_outbuf[4]	= SregLen;				// Lo
                }
                // -----update---
                GB19056.Send_add += continue_counter;


                OUT_PUT;
                if(GB19056.Send_add >= GB19056.Total_dataBlocks)
                    GB_serial_get_clear();// clear

            }
            else
                GB_null_content(0x14, type);
            //----------------------------------------------------------------------
        }
        break;
    case	 0x15:														// 15 �ɼ�ָ�����ٶ�״̬��־	 --------Divde
        //	Get total effective num
        if(Vdr_Wr_Rd_Offset.V_15H_full)
        {
            GB19056.Total_dataBlocks = VDR_15_MAXindex;
        }
        else
            GB19056.Total_dataBlocks = Vdr_Wr_Rd_Offset.V_15H_Write;
        //   USB
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "�ٶ�״̬��־", GB19056.Total_dataBlocks, 133);
            return;
        }
        //  serial
#if 1
        if(type == GB_OUT_TYPE_Serial)
        {
            //---------------------------------------------------------------------
            if(Vdr_Wr_Rd_Offset.V_15H_get == 0)
            {
                GB_serial_get_clear();
                Vdr_Wr_Rd_Offset.V_15H_get = 1;

                GB19056.Send_add = 0; // clear
            }
            //--------------------------------------------------------------------
            usb_outbuf_Wr = 0;
            usb_outbuf[usb_outbuf_Wr++]	 = 0x55;				   // ��ʼͷ
            usb_outbuf[usb_outbuf_Wr++]	 = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]	 = 0x15;				   //������


            SregLen = 133;	 //

            usb_outbuf[usb_outbuf_Wr++]	 = (SregLen >> 8);		   // Hi
            usb_outbuf[usb_outbuf_Wr++]	 = SregLen; 			   // Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00;					   // ������
            //  ��Ϣ����
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < (GB19056.Total_dataBlocks))
            {

                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_15H_Write >= (subi + 1)) //û����
                        get_indexnum = Vdr_Wr_Rd_Offset.V_15H_Write - 1 - subi;
                    else
                        get_indexnum = VDR_15_MAXindex + Vdr_Wr_Rd_Offset.V_15H_Write - 1 - subi;

                    gb_vdr_get_15h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
                    //-----time  filter ------------
                    Rd_Time = Time_sec_u32(usb_outbuf + usb_outbuf_Wr + 1, 6);
                    return_value = Timezone_judge(Rd_Time);

                    if(return_value) // end
                    {
                        continue;
                    }
                    else
                        usb_outbuf_Wr += 133;
                    continue_counter++;
                    break;
                }
                //---------------------------------------------------
                if(continue_counter == 0)	// �����Ϣ��Ϊ�գ�����
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 133; // ��û���õ���Ϣȥ��
                    usb_outbuf[3]   = (SregLen >> 8);		   // Hi
                    usb_outbuf[4]   = SregLen;			   // Lo
                }
                // -----update---
                GB19056.Send_add += continue_counter;
                OUT_PUT;

                if(GB19056.Send_add >= GB19056.Total_dataBlocks)
                    GB_serial_get_clear();// clear

            }
            else
                GB_null_content(0x15, type);

            //----------------------------------------------------------------------
        }
#endif

        break;
    case 0xFE:  // output all
        for(i = 0; i < 8; i++)
            GB_OUT_data(GB_SAMPLE_00H + i, type, 1, 1);

        GB_OUT_data(GB_SAMPLE_08H, type, 1, 720);
        GB_OUT_data(GB_SAMPLE_09H, type, 1, 360);
        GB_OUT_data(GB_SAMPLE_10H, type, 1, 100);
        GB_OUT_data(GB_SAMPLE_11H, type, 1, 100);
        GB_OUT_data(GB_SAMPLE_12H, type, 1, 10);
        GB_OUT_data(GB_SAMPLE_13H, type, 1, 1);
        GB_OUT_data(GB_SAMPLE_14H, type, 1, 1);
        GB_OUT_data(GB_SAMPLE_15H, type, 1, 10);
        break;
    case 0xFB:  // recommond output
        for(i = 0; i < 4; i++)
            GB_OUT_data(GB_SAMPLE_00H + i, type, 1, 1);
        GB_OUT_data(GB_SAMPLE_10H, type, 1, 100);
    case 0xFF:  //idle
        break;
    case 0x82:
    case 0x83:
    case 0x84:
    case 0xC2:
    case 0xC3:
    case 0xC4:
    case 0xE0:
    case 0xE2:
    case 0xE3:
    case 0xE4:
        GB_null_content(ID, type);
        break;
    case 0xE1:  // ���������
        usb_outbuf_Wr = 0;
        usb_outbuf[usb_outbuf_Wr++]   = 0x55;				// ��ʼͷ
        usb_outbuf[usb_outbuf_Wr++]   = 0x7A;

        usb_outbuf[usb_outbuf_Wr++] = 0xE1; 				//������

        usb_outbuf[usb_outbuf_Wr++] = 0x00;              // ����
        usb_outbuf[usb_outbuf_Wr++] = 44;


        usb_outbuf[usb_outbuf_Wr++] = 0x00;					  // ������
        //------- ��Ϣ���� ------
        // 0.    35  ���ֽ�Ψһ���
        usb_outbuf_Wr += GB_00_07_info_only(usb_outbuf + usb_outbuf_Wr, 0x07);
        //  1.  ����ϵ��
        usb_outbuf[usb_outbuf_Wr++]   = (jt808_param.id_0xF032 >> 8);
        usb_outbuf[usb_outbuf_Wr++]   = (u8)jt808_param.id_0xF032;
        //   2.  �ٶ�
        usb_outbuf[usb_outbuf_Wr++]   = (car_data.get_speed >> 8);
        usb_outbuf[usb_outbuf_Wr++]   = (u8)car_data.get_speed;
        //   3. �ۼ����
        usb_outbuf[usb_outbuf_Wr++]   = (jt808_data.id_0xFA01 >> 24);
        usb_outbuf[usb_outbuf_Wr++]   = (jt808_data.id_0xFA01 >> 16);
        usb_outbuf[usb_outbuf_Wr++]   = (jt808_data.id_0xFA01 >> 8);
        usb_outbuf[usb_outbuf_Wr++]   = jt808_data.id_0xFA01;
        //    4.  ״̬�� 1�� �ֽ�
        usb_outbuf[usb_outbuf_Wr++]   = GB_Get_SensorStatus();
        OUT_PUT;
        break;
    default:
        break;

    }

    if(ID <= 0x07)
    {
        OUT_PUT;
    }

}

void GB_out_E1_ACK(void)
{
    GB_OUT_data(0xE1, GB_OUT_TYPE_Serial, 1, 1);
}


/*��¼�����ݽ���״̬*/

void Virtual_thread_of_GB19056( void)
{
    //   Application .c

    u8 str_12len[15];
    /*����һ������ָ�룬�����������	*/

    rt_err_t res;


    // part1:  serial
    if (rt_sem_take(&GB_RX_sem, 20) == RT_EOK)
    {
        // 0  debug out
        //OutPrint_HEX("��¼����",GB19056.rx_buf,GB19056.rx_wr);
        //rt_device_write( &dev_vuart,  0,GB19056.rx_buf,GB19056.rx_wr);
        if(strncmp(GB19056.rx_buf, "deviceid", 8) == 0) // deviceid("012345678901")
        {
            GB19056.workstate = 0;
            rt_thread_delay(15);
            // for(i=0;i<24;i++)
            //	 rt_kprintf("%c",GB19056.rx_buf[i]);
            //rt_kprintf("%c",0x0a);
            memset(str_12len, 0, sizeof(str_12len));
            memcpy(str_12len, GB19056.rx_buf + 10, 12);
            deviceid(str_12len);
            // tail
            rt_hw_console_output("0,0x00000000\r\n");
            rt_hw_console_output("aa mode \r\n");
            rt_thread_delay(10);
            GB19056.workstate = 1;
        }                // 1  process  rx
        else
            GB19056_Decode(GB19056.rx_buf, GB19056.rx_infoLen + 7);


        GB19056.rx_wr = 0;
        GB19056.rx_flag = 0;
        GB19056.rx_infoLen = 0;
        GB19056.u1_rx_timer = 0;
    }
    // part2:  USB
    if(GB19056.usb_write_step == GB_USB_OUT_start)
    {
        //------ U disk
        Udisk_dev = rt_device_find("udisk");
        if (Udisk_dev != RT_NULL)
        {
            res = rt_device_open(Udisk_dev, RT_DEVICE_OFLAG_RDWR);
            if(res == RT_EOK)
            {
                GB19056.usb_xor_fcs = 0; // clear fcs
                //  �����ļ�
                memset(GB19056.Usbfilename, 0, sizeof(GB19056.Usbfilename));
                sprintf((char *)GB19056.Usbfilename, "1:D%c%c%c%c%c%c_%c%c%c%c_%s.VDR", (YEAR(mytime_now) / 10 + 0x30), (YEAR(mytime_now) % 10 + 0x30), (MONTH(mytime_now) / 10 + 0x30), (MONTH(mytime_now) % 10 + 0x30), \
                        (DAY(mytime_now) / 10 + 0x30), (DAY(mytime_now) % 10 + 0x30), (HOUR(mytime_now) / 10 + 0x30), (HOUR(mytime_now) % 10 + 0x30), (MINUTE(mytime_now) / 10 + 0x30), (MINUTE(mytime_now) % 10 + 0x30), jt808_param.id_0x0083);

                res = f_open(&file, GB19056.Usbfilename, FA_READ | FA_WRITE | FA_OPEN_ALWAYS );


                if(GB19056.workstate == 0)
                    rt_kprintf(" \r\n udiskfile: %s  create res=%d	 \r\n", GB19056.Usbfilename, res);
                if(res == 0)
                {

                    if(GB19056.workstate == 0)
                        rt_kprintf("\r\n			  ����Drv����: %s \r\n ", GB19056.Usbfilename);

                    GB19056.usb_write_step = GB_USB_OUT_running; // goto  next step
                }
                else
                {
                    if(GB19056.workstate == 0)
                        rt_kprintf(" \r\n udiskfile create Fail   \r\n", GB19056.Usbfilename, res);
                    GB19056.usb_write_step = GB_USB_OUT_idle;
                }
            }
        }
    }
    if(GB19056.usb_write_step == GB_USB_OUT_running)
    {

        GB_OUT_data(GB19056.usb_out_selectID, GB_OUT_TYPE_USB, 1, 1);
        GB19056.usb_write_step = GB_USB_OUT_end;
    }
    if(GB19056.usb_write_step == GB_USB_OUT_end)
    {

        f_close(&file);
        GB19056.usb_write_step = GB_USB_OUT_idle;

        if(GB19056.workstate == 0)
            rt_kprintf(" \r\n usboutput   over! \r\n");
        //------  ��ʾ����� update  ------------------------------
#if  1
        debug_write("USB�������ݵ������!");
        usb_export_status = 66;		///0,��ʾû�м�⵽U�̣��޷�������1��ʾ�����������ݣ�66��ʾ�����ɹ�

#endif
        //--------------------------------------
    }

    //-------------------------------------------

}

void  gb_work(u8 value)
{
    if(value)
    {
        rt_kprintf(" GB_Data_enable\r\n");
        printf_on = 0;
        GB19056.workstate = 1;
    }
    else
    {
        GB19056.workstate = 0;
        printf_on = 1; // ʹ�ܴ�Ӧ
        rt_kprintf(" GB_Data_disable\r\n");

    }

}
FINSH_FUNCTION_EXPORT(gb_work, gb_work(1 : 0));

void jia_spd(u16 spd)
{
    jia_speed = spd;
    rt_kprintf("\r\n-----jia_speed:%d \r\n",  jia_speed);
}
FINSH_FUNCTION_EXPORT(jia_spd, jia_spd(1 : 0));

//   ִ�д洢��¼�������������
void  GB19056_VDR_Save_Process(void)
{
    //  ����    rt_thread_entry_gps

#ifdef JT808_TEST_JTB
    return;
#else


    // 1. VDR  08H  Data  Save
    if(VdrData.H_08_saveFlag == 1)
    {
        //OutPrint_HEX("08H save",VdrData.H_08_BAK,126);
        if( GB19056.workstate == 0)
            rt_kprintf("\r\n 08H save  %d \r\n", Vdr_Wr_Rd_Offset.V_08H_Write);

        /* rt_kprintf("   save time:20%02d-%02d-%02d %02d:%02d:%02d\n",
        	  YEAR( mytime_now ),
        	  MONTH( mytime_now ),
        	  DAY( mytime_now ),
        	  HOUR( mytime_now ),
        	  MINUTE( mytime_now ),
        	  SEC( mytime_now )
        	  );
        	  */
        DF_Sem_Take;
        vdr_creat_08h(Vdr_Wr_Rd_Offset.V_08H_Write, VdrData.H_08_BAK, 126);
        Vdr_Wr_Rd_Offset.V_08H_Write++; //  д��֮���ۼӣ��Ͳ��ñ�����
        vdr_cmd_writeIndex_save(0x08, Vdr_Wr_Rd_Offset.V_08H_Write);
        DF_Sem_Release;
        VdrData.H_08_saveFlag = 0;
        return;
    }
    // 2.  VDR  09H  Data  Save
    if(VdrData.H_09_saveFlag == 1)
    {
        // OutPrint_HEX("09H save",VdrData.H_09,666);
        if( GB19056.workstate == 0)
            rt_kprintf("\r\n 09H save  %d\r\n", Vdr_Wr_Rd_Offset.V_09H_Write);
        DF_Sem_Take;
        vdr_creat_09h(Vdr_Wr_Rd_Offset.V_09H_Write, VdrData.H_09, 666);
        Vdr_Wr_Rd_Offset.V_09H_Write++; // //  д��֮���ۼӣ��Ͳ��ñ�����
        vdr_cmd_writeIndex_save(0x09, Vdr_Wr_Rd_Offset.V_09H_Write);
        DF_Sem_Release;
        memset(VdrData.H_09 + 6, 0x0, 660);	 // Ĭ���� 0xFF
        VdrData.H_09_saveFlag = 0;
        return;
    }
    //  3. VDR  10H  Data  Save
    if(VdrData.H_10_saveFlag == 1)
    {
        // OutPrint_HEX("10H save",VdrData.H_10,234);
        if( GB19056.workstate == 0)
            rt_kprintf("\r\n 10H save  %d\r\n", Vdr_Wr_Rd_Offset.V_10H_Write);
        DF_Sem_Take;
        vdr_creat_10h(Vdr_Wr_Rd_Offset.V_10H_Write, VdrData.H_10, 234);
        Vdr_Wr_Rd_Offset.V_10H_Write++; // //  д��֮���ۼӣ��Ͳ��ñ�����
        vdr_cmd_writeIndex_save(0x10, Vdr_Wr_Rd_Offset.V_10H_Write);
        DF_Sem_Release;
        VdrData.H_10_saveFlag = 0;
        return;
    }
    //  4.  VDR  11H  Data  Save
    if(VdrData.H_11_saveFlag)  //  1 : �洢����    2: �洢������
    {
        // OutPrint_HEX("11H save",VdrData.H_11,50);
        if( GB19056.workstate == 0)
        {
            rt_kprintf("\r\n 11H save   %d \r\n", Vdr_Wr_Rd_Offset.V_11H_Write);
            // OutPrint_HEX("11H save",VdrData.H_11,50);
        }
        DF_Sem_Take;
        vdr_creat_11h(Vdr_Wr_Rd_Offset.V_11H_Write, VdrData.H_11, 50);
        if(VdrData.H_11_saveFlag == 1)
            Vdr_Wr_Rd_Offset.V_11H_Write++; // //  д��֮���ۼӣ��Ͳ��ñ�����

        vdr_cmd_writeIndex_save(0x11, Vdr_Wr_Rd_Offset.V_11H_Write);
        DF_Sem_Release;
        VdrData.H_11_saveFlag = 0;
        return;
    }
    //  5.  VDR  12H  Data  Save
    if(VdrData.H_12_saveFlag == 1)
    {
        //OutPrint_HEX("12H save",VdrData.H_12,25);
        if( GB19056.workstate == 0)
            rt_kprintf("\r\n 12H save  %d \r\n", Vdr_Wr_Rd_Offset.V_12H_Write);
        DF_Sem_Take;
        vdr_creat_12h(Vdr_Wr_Rd_Offset.V_12H_Write, VdrData.H_12, 25);
        Vdr_Wr_Rd_Offset.V_12H_Write++; // //  д��֮���ۼӣ��Ͳ��ñ�����
        vdr_cmd_writeIndex_save(0x12, Vdr_Wr_Rd_Offset.V_12H_Write);
        DF_Sem_Release;
        VdrData.H_12_saveFlag = 0;

        // rt_kprintf("\r\n дIC ��  ��¼  \r\n");
        return;
    }
    //  6.  VDR  13H  Data  Save
    if(VdrData.H_13_saveFlag == 1)
    {
        // OutPrint_HEX("13H save",VdrData.H_13,7);
        if( GB19056.workstate == 0)
            rt_kprintf("\r\n 13H save %d \r\n", Vdr_Wr_Rd_Offset.V_13H_Write);
        DF_Sem_Take;
        vdr_creat_13h(Vdr_Wr_Rd_Offset.V_13H_Write, VdrData.H_13, 7);
        Vdr_Wr_Rd_Offset.V_13H_Write++; // //  д��֮���ۼӣ��Ͳ��ñ�����
        vdr_cmd_writeIndex_save(0x13, Vdr_Wr_Rd_Offset.V_13H_Write);
        DF_Sem_Release;
        VdrData.H_13_saveFlag = 0;
        return;
    }
    //  7.  VDR  14H  Data  Save
    if(VdrData.H_14_saveFlag == 1)
    {
        //OutPrint_HEX("14H save",VdrData.H_14,7);
        if( GB19056.workstate == 0)
            rt_kprintf("\r\n 14H save  %d\r\n", Vdr_Wr_Rd_Offset.V_14H_Write);
        DF_Sem_Take;
        vdr_creat_14h(Vdr_Wr_Rd_Offset.V_14H_Write, VdrData.H_14, 7);
        Vdr_Wr_Rd_Offset.V_14H_Write++; // //  д��֮���ۼӣ��Ͳ��ñ�����
        vdr_cmd_writeIndex_save(0x14, Vdr_Wr_Rd_Offset.V_14H_Write);
        DF_Sem_Release;
        VdrData.H_14_saveFlag = 0;
        return;
    }
    // 8.  VDR  15H  Data save
    if(VdrData.H_15_saveFlag == 1)
    {
        //OutPrint_HEX("15H save",VdrData.H_15,133;
        if( GB19056.workstate == 0)
            rt_kprintf("\r\n 15H save  %d\r\n", Vdr_Wr_Rd_Offset.V_15H_Write);
        DF_Sem_Take;
        vdr_creat_15h(Vdr_Wr_Rd_Offset.V_15H_Write, VdrData.H_15, 133);
        Vdr_Wr_Rd_Offset.V_15H_Write++; // //  д��֮���ۼӣ��Ͳ��ñ�����
        vdr_cmd_writeIndex_save(0x15, Vdr_Wr_Rd_Offset.V_15H_Write);
        DF_Sem_Release;
        VdrData.H_15_saveFlag = 0;
    }
    //-----------------  ���ٱ��� ----------------------
    /*	if(speed_Exd.excd_status==2)
    	{
    	   Spd_Exp_Wr();
    	   return;
    	}
    */
    //	 ��ʱ�洢���--xiaobai  �Ѿ������ͳ����
    /*
    if((Vehicle_RunStatus==0x01)&&(DistanceWT_Flag==1))
    {   //  �����������ʻ�����У�ÿ255 ��洢һ���������
          //rt_kprintf("\r\n distance --------\r\n");
    	  DistanceWT_Flag=0;// clear
    	  DF_Write_RecordAdd(Distance_m_u32,DayStartDistance_32,TYPE_DayDistancAdd);
    	  JT808Conf_struct.DayStartDistance_32=DayStartDistance_32;
    	  JT808Conf_struct.Distance_m_u32=Distance_m_u32;
    	  return;
    }
    */
    //--------------------------------------------------------------
    return;
#endif
}



//    ��   GB   IC   ���
void  GB_IC_info_default(void)
{
    memcpy(Read_ICinfo_Reg.DriverCard_ID, "000000000000000000", 18);
    memset(Read_ICinfo_Reg.Effective_Date, 0, sizeof(Read_ICinfo_Reg.Effective_Date));
    Read_ICinfo_Reg.Effective_Date[0] = 0x20;
    Read_ICinfo_Reg.Effective_Date[1] = 0x08;
    Read_ICinfo_Reg.Effective_Date[2] = 0x08;
    memcpy(Read_ICinfo_Reg.Drv_CareerID, "000000000000000000", 18);
}
//   �ж϶����ʻԱ���忨����Ϣ�ĸ���
void  GB_IC_Different_DriverIC_InfoUpdate(void)
{
    u8  i = 0, compare = 0; // compare �Ƿ���ƥ���
    s8  cmp_res = 0; // ƥ��Ϊ0
    u8  selected_num = 0; // ��ʾ�ڼ�����ѡ����

    //  0 .    �ȱ����Ƿ�ͼ��е�ƥ��
    for(i = 0; i < MAX_DriverIC_num; i++)
    {
        if(Drivers_struct[i].Working_state)
        {
            // check compare
            cmp_res = memcmp(Drivers_struct[i].Driver_BASEinfo.DriverCard_ID, Read_ICinfo_Reg.DriverCard_ID, 18);
            if(cmp_res == 0) // ƥ��
            {

                Drivers_struct[i].Working_state = 2;
                selected_num = MAX_DriverIC_num + 2; //   ����ȫƥ��
                compare = 1; //enable
                if(GB19056.workstate == 0)
                    rt_kprintf("\r\n  ����ȫƥ��� i=%d\r\n", i);
            }
            else
            {
                if(Drivers_struct[i].Working_state == 2)
                {
                    Drivers_struct[i].Working_state = 1;
                    selected_num = i; //  ��¼��ǰΪ2 �� �±�
                    if(GB19056.workstate == 0)
                        rt_kprintf("\r\n  �ϴβ���� 2bian1  i=%d\r\n", i);
                }
            }
        }
    }

    if(compare)         //  �ҵ�ƥ����ˣ��Ҹ���״̬��
        return;
    //--------------------------------------------------------------
    // 1. ������ʻԱ״̬���ҳ���ǰҪ�õ�λ��
    for(i = 0; i < MAX_DriverIC_num; i++)
    {
        // ����е�ǰ�ģ���ô������ǰ״̬ת���ɼ���״̬

        //    ״̬Ϊ2 ����һ����Ϊ ��ǰλ��
        if((Drivers_struct[i].Working_state == 1) && (selected_num == 0x0F))
        {
            selected_num = i;
            break;
        }
        if(Drivers_struct[i].Working_state == 1)
        {
            if(selected_num == i) // �жϵ�ǰ��¼�Ƿ����ϴ�Ϊ2 �ļ�¼
            {
                Drivers_struct[i].Working_state = 1;
                selected_num = 0x0F;
            }
        }
        if(Drivers_struct[i].Working_state == 0)
        {
            selected_num = i;
            break;
        }

    }

    if(i >= MAX_DriverIC_num) //   ȫ���ˣ������һ����2����ô��һ�����ǵ�ǰ�µ�
    {
        selected_num = 0;
    }

    // 2. update new  info
    memcpy((u8 *)(&Drivers_struct[selected_num].Driver_BASEinfo), (u8 *)(&Read_ICinfo_Reg), sizeof(Read_ICinfo_Reg));
    Drivers_struct[selected_num].Working_state = 2; //
    Drivers_struct[selected_num].Running_counter = 0;
    Drivers_struct[selected_num].Stopping_counter = 0;

    if(GB19056.workstate == 0)
    {
        rt_kprintf("\r\n  ���������� i=%d  ID=%s\r\n", selected_num, Drivers_struct[selected_num].Driver_BASEinfo.DriverCard_ID);


        rt_kprintf("\r\n  1--���������� i=%d  ID=%s\r\n", 0, Drivers_struct[0].Driver_BASEinfo.DriverCard_ID);
        rt_kprintf("\r\n  2--���������� i=%d  ID=%s\r\n", 1, Drivers_struct[1].Driver_BASEinfo.DriverCard_ID);
        rt_kprintf("\r\n  3--���������� i=%d  ID=%s\r\n", 2, Drivers_struct[2].Driver_BASEinfo.DriverCard_ID);
    }
}



//  �ж� ��ͬ��ʻԱ ,������ʻ�Ŀ�ʼʱ��
void  GB_IC_Different_DriverIC_Start_Process(void)
{
    u8  i = 0;
    // �жϸ�����ʻԱ��������ʻ�Ŀ�ʼʱ��
    if((current_speed > 60) && (Sps_larger_5_counter > 10))
    {
        for(i = 0; i < MAX_DriverIC_num; i++)
        {
            if((Drivers_struct[i].Working_state == 2) && (Drivers_struct[i].Running_counter == 0))
            {
                memcpy(Drivers_struct[i].Start_Datetime, gb_BCD_datetime, 6);
                //   3.  ��ʼλ��
                memcpy(Drivers_struct[i].Longi, VdrData.Longi, 4); // ����
                memcpy(Drivers_struct[i].Lati, VdrData.Lati, 4); //γ��
                Drivers_struct[i].Hight = BYTESWAP2( gps_baseinfo.altitude);
                Drivers_struct[i].H_11_start = 1; //  start
                Drivers_struct[i].H_11_lastSave_state = 0;
            }
        }
    }

}

//  �жϲ�ͬ��ʻԱ��������ʻ�Ľ���ʱ�� // �ٶ�Ϊ0  ʱ��ִ��
void  GB_IC_Different_DriverIC_End_Process(void)
{
    u8 i = 0;
    u8  value = 0;

    for(i = 0; i < MAX_DriverIC_num; i++)
    {
        if((Drivers_struct[i].Working_state) && (Drivers_struct[i].Running_counter >= jt808_param.id_0x0057))
        {
            if((jt808_param.id_0x0057 <= 0) && (Drivers_struct[i].H_11_start == 0))
                break;

            //  1.   ��������ʻ֤��
            memcpy(VdrData.H_11, Drivers_struct[i].Driver_BASEinfo.DriverCard_ID, 18);
            if(GB19056.workstate == 0)
                rt_kprintf("\r\n    drivernum=%d drivercode-=%s\r\n", i, Drivers_struct[i].Driver_BASEinfo.DriverCard_ID);
            //   2.   ��ʼʱ��
            memcpy(VdrData.H_11 + 18, Drivers_struct[i].Start_Datetime, 6); // ��ʼʱ��
            memcpy(Drivers_struct[i].End_Datetime, gb_BCD_datetime, 6);
            memcpy(VdrData.H_11 + 24, Drivers_struct[i].End_Datetime, 6); // ����ʱ��
            //   3.  ��ʼλ��
            memcpy( VdrData.H_11 + 30, Drivers_struct[i].Longi, 4); // ����
            memcpy( VdrData.H_11 + 30 + 4, Drivers_struct[i].Lati, 4); //γ��
            VdrData.H_11[30 + 8] = (Drivers_struct[i].Hight >> 8);
            VdrData.H_11[30 + 9] = Drivers_struct[i].Hight;
            //VdrData.H_11_start=1; //  start
            //VdrData.H_11_lastSave=0;

            //   4.   ����λ��
            memcpy( VdrData.H_11 + 40, VdrData.Longi, 4); // ����
            memcpy( VdrData.H_11 + 40 + 4, VdrData.Lati, 4); //γ��
            VdrData.H_11[40 + 8] = (BYTESWAP2( gps_baseinfo.altitude) >> 8);
            VdrData.H_11[40 + 9] = BYTESWAP2( gps_baseinfo.altitude);

            value = 2;

            VDR_product_11H_End(2);

            Drivers_struct[i].H_11_start = 2; // end over

            if(value == 2)
            {
                Drivers_struct[i].H_11_lastSave_state = 1;
            }
        }

    }
}

//  �жϲ�ͬ��ʻԱ ƣ��״̬
void  GB_IC_Different_DriverIC_Checking(void)
{
    u8 i = 0;

    if((current_speed > 60 ) && (Sps_larger_5_counter > 10))	// current_speed ��λΪ0.1 km/h  �ٶȴ���6 km/h  ��Ϊ����ʻ
    {
        // ��ʻ״̬
        for(i = 0; i < MAX_DriverIC_num; i++)
        {
            if( Drivers_struct[i].Working_state == 2)
            {
                Drivers_struct[i].Running_counter++;

                //       ��ǰ30 min  ������ʾ
                if(jt808_param.id_0x0057 > 1800)
                {
                    if(Drivers_struct[i].Running_counter == ( jt808_param.id_0x0057 - 1800 ) ) //��ǰ5���ӷ�������ʾע��ƣ�ͼ�ʻ 14100
                    {
                        //   ƣ�ͱ���Ԥ��ʾ ��ʼ
                        if(GB19056.SPK_PreTired.Warn_state_Enable == 0)
                            GB19056.SPK_PreTired.Warn_state_Enable = 1;
                    }
                }
                //--   �ж�ƣ�ͼ�ʻ--------------------------------------------------------------------
                if(  Drivers_struct[i].Running_counter == (jt808_param.id_0x0057) )//14400
                {
                    if(GB19056.workstate == 0)
                        rt_kprintf( "\r\n   ƣ�ͼ�ʻ������!  on\r\n" );
                    //  TTS_play( "���Ѿ�ƣ�ͼ�ʻ����ע����Ϣ" );


                    //	��ʱ��ʻ ��ʾ������
                    if(GB19056.SPK_DriveExceed_Warn.Warn_state_Enable == 0)
                        GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 1;
                    //	ƣ�ͱ���Ԥ��ʾ ����
                    GB19056.SPK_PreTired.Warn_state_Enable = 0;



                    //---- ������ʱ�ϱ�����	  ����¼�ǲ���ƣ��ֻ��¼���ϱ�-------
                    //-------------------------------------
                }
                //----------------------------------------------------------------------------------------
                Drivers_struct[i].Stopping_counter = 0;
            }
            else if(Drivers_struct[i].Working_state == 1) //  ״̬Ϊ1  Ҳ����Ϣ
            {
                Drivers_struct[i].Stopping_counter++; // ״̬Ϊ1 �ĳ�����Ϣ

                if(Drivers_struct[i].Stopping_counter >= jt808_param.id_0x0059 )	//1200	 // ACC ��20������Ϊ��Ϣ
                {
                    if(Drivers_struct[i].Stopping_counter == jt808_param.id_0x0059 )
                    {
                        Drivers_struct[i].Stopping_counter = 0;
                        Drivers_struct[i].Running_counter = 0;

                        // 	ֻ�д�����ƣ�ͼ�ʻ���ҿ�ʼʱ�丳����ֵ�Ż�洢�ж�
                        if((Drivers_struct[i].Running_counter >= jt808_param.id_0x0057) && \
                                (Drivers_struct[i].H_11_start == 2));
                        {
                            Drivers_struct[i].Working_state = 0; //
                            //---- ������ʱ�ϱ�����    ����¼�ǲ���ƣ��ֻ��¼���ϱ�-------

                            //	ֻ�в�Ϊ��ǰ���������ǰΪ2 �����
                            if(Drivers_struct[i].Working_state == 1)
                                memset((u8 *)&Drivers_struct[i], 0, sizeof(DRIVE_STRUCT)); // clear


                            //  ��ʱ��ʻ����
                            GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 0;
                            if(GB19056.workstate == 0)
                                rt_kprintf( "\r\n	i=%d ����ƣ�ͼ�ʻ�����Ѿ����-on 1 \r\n", i);
                        }
                    }
                }
            }

        }

    }
    else
    {
        // ֹͣ״̬
        for(i = 0; i < MAX_DriverIC_num; i++)
        {
            if( Drivers_struct[i].Working_state)
            {
                if(Drivers_struct[i].Running_counter)
                {
                    //-- ACC û����Ϣǰ����AccON ��״̬  ---------------
                    Drivers_struct[i].Running_counter++;

                    //---------------------------------------------------------------
                    //       ��ǰ30 min  ������ʾ
                    if(jt808_param.id_0x0057 > 1800)
                    {
                        if(Drivers_struct[i].Running_counter == ( jt808_param.id_0x0057 - 1800 ) ) //��ǰ5���ӷ�������ʾע��ƣ�ͼ�ʻ 14100
                        {
                            //   ƣ�ͱ���Ԥ��ʾ ��ʼ
                            if(GB19056.SPK_PreTired.Warn_state_Enable == 0)
                                GB19056.SPK_PreTired.Warn_state_Enable = 1;
                        }
                    }


                    //      ACC off    but     Acc on  conintue  run
                    if( Drivers_struct[i].Running_counter >= ( jt808_param.id_0x0057 ) )       //14400 // ������ʻ����4Сʱ��ƣ�ͼ�ʻ
                    {
                        if(Drivers_struct[i].Running_counter == ( jt808_param.id_0x0057  ) )  //14400
                        {
                            if(GB19056.workstate == 0)
                                rt_kprintf( "\r\n	 �ٶ�С����δ������Ϣ����ʱ�� ƣ�ͼ�ʻ������! \r\n");
                            //TTS_play( "���Ѿ�ƣ�ͼ�ʻ����ע����Ϣ" );

                            //	 ��ʱ��ʻ ������
                            if(GB19056.SPK_DriveExceed_Warn.Warn_state_Enable == 0)
                                GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 1;
                            //   ƣ�ͱ���Ԥ��ʾ ����
                            GB19056.SPK_PreTired.Warn_state_Enable = 0;

                            //---- ������ʱ�ϱ�����    ����¼�ǲ���ƣ��ֻ��¼���ϱ�-------
                        }
                    }


                    //     ACC  off   counter
                    Drivers_struct[i].Stopping_counter++;
                    if(Drivers_struct[i].Stopping_counter >= jt808_param.id_0x0059 ) //1200	// ACC ��20������Ϊ��Ϣ
                    {
                        if(Drivers_struct[i].Stopping_counter == jt808_param.id_0x0059 )
                        {
                            Drivers_struct[i].Stopping_counter = 0;
                            Drivers_struct[i].Running_counter = 0;

                            //     ֻ�д�����ƣ�ͼ�ʻ���ҿ�ʼʱ�丳����ֵ�Ż�洢�ж�
                            if((Drivers_struct[i].Running_counter >= jt808_param.id_0x0057) && \
                                    (Drivers_struct[i].H_11_start == 2));
                            {
                                Drivers_struct[i].Working_state = 0; //

                                //---- ������ʱ�ϱ�����    ����¼�ǲ���ƣ��ֻ��¼���ϱ�-------
                                //-------------------------------------
                                //    ��ʱ����  �洢�������ۼ�
                                //  1.   ��������ʻ֤��
                                memcpy(VdrData.H_11, Drivers_struct[i].Driver_BASEinfo.DriverCard_ID, 18);
                                //   2.   ��ʼʱ��
                                memcpy(VdrData.H_11 + 18, Drivers_struct[i].Start_Datetime, 6); // ��ʼʱ��
                                memcpy(VdrData.H_11 + 24, Drivers_struct[i].End_Datetime, 6); // ����ʱ��
                                //   3.  ��ʼλ��
                                memcpy( VdrData.H_11 + 30, Drivers_struct[i].Longi, 4); // ����
                                memcpy( VdrData.H_11 + 30 + 4, Drivers_struct[i].Lati, 4); //γ��
                                VdrData.H_11[30 + 8] = (Drivers_struct[i].Hight >> 8);
                                VdrData.H_11[30 + 9] = Drivers_struct[i].Hight;

                                //   4.   ����λ��
                                memcpy( VdrData.H_11 + 40, VdrData.Longi, 4); // ����
                                memcpy( VdrData.H_11 + 40 + 4, VdrData.Lati, 4); //γ��
                                VdrData.H_11[40 + 8] = (BYTESWAP2( gps_baseinfo.altitude) >> 8);
                                VdrData.H_11[40 + 9] = BYTESWAP2( gps_baseinfo.altitude);

                                if((Drivers_struct[i].End_Datetime[0] == 0x00) && (Drivers_struct[i].End_Datetime[1] == 0x00) && (Drivers_struct[i].End_Datetime[2] == 0x00))
                                    ; // ������ Ϊ0  ����
                                else
                                    VDR_product_11H_End(1);

                                //  ֻ�в�Ϊ��ǰ���������ǰΪ2 �����
                                if(Drivers_struct[i].Working_state == 1)
                                    memset((u8 *)&Drivers_struct[i], 0, sizeof(DRIVE_STRUCT)); // clear
                                else



                                    //  ��ʱ��ʻ����
                                    GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 0;
                                if(GB19056.workstate == 0)
                                    rt_kprintf( "\r\n	����ƣ�ͼ�ʻ�����Ѿ���� \r\n");
                            }

                        }
                    }
                    //-------------------------------------------------------------------------------------
                }
            }
        }
    }
}


void GB_IC_Check_IN(void)
{
    //  �ŵ�  CheckICCard   ��                       12H

    u8 result0 = 0, result1 = 1, result2 = 2, result3 = 4, result4 = 5; //i=0;
    u8 regNum = 0, readbuf[32];
    u8  red_byte[128];
    u8  fcs = 0, i = 0;


    if((current_speed < 40) && (Sps_larger_5_counter == 0))		// ����ʻ����״̬�²ſ�ʼ �жϲ�ο�
    {

        memset(Read_ICinfo_Reg.DriverCard_ID, 0, sizeof(Read_ICinfo_Reg.DriverCard_ID));
        memcpy((unsigned char *)Read_ICinfo_Reg.DriverCard_ID, jt808_param.id_0xF009, 18);	//����ʻ֤����
        memset(Read_ICinfo_Reg.Effective_Date, 0, sizeof(Read_ICinfo_Reg.Effective_Date));
        memcpy((unsigned char *)Read_ICinfo_Reg.Effective_Date, ic_card_para.IC_Card_valitidy + 1, 3);	//��Ч����
        memset(Read_ICinfo_Reg.Drv_CareerID, 0, sizeof(Read_ICinfo_Reg.Drv_CareerID));
        memcpy((unsigned char *)Read_ICinfo_Reg.Drv_CareerID, jt808_param.id_0xF00B, 18);	//��ҵ�ʸ�֤

        //-----------  ͣ���忨 ��ʶ�� -------
        GB_IC_Different_DriverIC_InfoUpdate();  // ����IC ��ʻԱ��Ϣ
        gb_IC_loginState = 1;

        DF_Sem_Take;
        if(Vdr_Wr_Rd_Offset.V_12H_Write)
            regNum = Vdr_Wr_Rd_Offset.V_12H_Write - 1;
        else
            regNum = Vdr_Wr_Rd_Offset.V_12H_Write;
        gb_vdr_get_12h(regNum, readbuf);

        DF_Sem_Release;
        if(readbuf[24] == 0x02) //  ֮ǰ�й���½
        {
            VDR_product_12H(0x01);	//	��¼
            tts("��ʻԱ��¼"); // ��һ���ϵ粥������Ҫ��¼

        }
        GB19056.SPK_UnloginWarn.Warn_state_Enable = 0; // clear
    }
}

void  GB_IC_Check_OUT(void)
{
    //  �ŵ�  CheckICCard   ��           12H

    u8 regNum = 0, readbuf[32];

    if((current_speed < 40) && (gb_IC_loginState == 1)) 	 // ����ʻ����״̬�²ſ�ʼ �жϲ�ο�
    {
        DF_Sem_Take;
        if(Vdr_Wr_Rd_Offset.V_12H_Write)
            regNum = Vdr_Wr_Rd_Offset.V_12H_Write - 1;
        else
            regNum = Vdr_Wr_Rd_Offset.V_12H_Write;
        gb_vdr_get_12h(regNum, readbuf);
        DF_Sem_Release;
        if(readbuf[24] == 0x01) //  ֮ǰ�й���½
        {
            VDR_product_12H(0x02);  //  �ǳ�
            tts("��ʻԱ�˳�");

            gb_IC_loginState = 0;
        }

    }
}

void GB_IC_Exceed_Drive_Check( void )    //  ��ʱ��ʻ���
{
    //    1.   �ж��Ƿ�ACC  ��
    if(!(jt808_status & BIT_STATUS_ACC))    //  acc  �������
        return;

    //	2.	�ж���ʻ״̬
    if((gps_speed >= 60) || (car_data.get_speed >= 60))	// ��¼����֤ʱ���������� 1km/h
    {
        //	11 H   ������ʻ��ʼʱ��   ����ʼλ��	 ��60s ��ʼ���ʻ��ʼ
        if(VDR_TrigStatus.Running_state_enable == 0)
        {
            VdrData.H_08_counter = SEC(mytime_now); // time_now.sec; // start
        }
        VDR_TrigStatus.Running_state_enable = 1; //  ������ʻ״̬
    }
    if((gps_speed < 60) && (car_data.get_speed < 60))
    {
        //		  11 H	   ���    ����4 Сʱ ��   �ٶȴӴ󽵵� 0  ���Ŵ�����ʱ��¼
        if(VDR_TrigStatus.Running_state_enable == 1)
        {
            GB_IC_Different_DriverIC_End_Process();
        }
        VDR_TrigStatus.Running_state_enable = 0; //  ����ֹͣ״̬
    }


    //   2.   ��ʻ��ʱ�ж�
    // 2.1    H_11   Start    acc on Ϊ 0 ʱ��ʼ��ʱ��¼
    GB_IC_Different_DriverIC_Start_Process();
    // 2.2    acc on  counter     -------------ƣ�ͼ�ʻ��� -----------------------
    GB_IC_Different_DriverIC_Checking();
    //--------------------------------------------------------------------------------
    //------------------------------------------------
}

void gb_100ms_timer(void)
{
    //   �ŵ�      cb_tmr_50ms     ��
    GB_100ms_Timer_ISP();

    gb_time_counter++;
    if((gb_time_counter % 2) == 0)
    {
        GB_200ms_Hard_timer();
        gb_time_counter = 0;
    }

}


