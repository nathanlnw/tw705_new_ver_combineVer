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

#define    GB_VDR_MAX_1_PKG_SIZE        700     //  VDR 单包最大字节数


//  一.  停车前15分钟，每分钟平均速度
#ifdef AVRG15MIN
//   停车前15  分钟，每分钟的平均速度
AVRG15_SPD  Avrg_15minSpd;
#endif

// 二.  发送记录仪数据相关

VDR_JT808_SEND		VDR_send;                    // VDR


// 三.  记录仪数据执行相关

#define   OUT_PUT        SERIAL_OUTput(type)
#define   USB_OUT        USB_OUTput(CMD,add_fcs_byte)



//    GB  数据导出
#define GB_USB_OUT_idle           0
#define GB_USB_OUT_start          1
#define GB_USB_OUT_running        2
#define GB_USB_OUT_end            3

#define GB_OUT_TYPE_Serial         1
#define GB_OUT_TYPE_USB            2


struct rt_semaphore GB_RX_sem;  //  gb  接收AA 75     放到startup .c

GB_STRKT  GB19056;
static FIL 	file;

rt_device_t   Udisk_dev = RT_NULL;

//----------  事故疑点数据
DOUBT_TYPE  Sensor_buf[200];// 20s 状态记录
u8      save_sensorCounter = 0, sensor_writeOverFlag = 0;;
u16     Speed_cacu_BAK = 0; //   临时存储备份传感器速度
u8      Speed_cacu_Trigger_Flag = 0; // 事故疑点1 触发 状态


//  使能播报状态
u8    Warn_Play_controlBit = 0x16;
/*
                                      使能播报状态控制BIT
                                    BIT   0:    驾驶员未登录状态提示(建议默认取消)
                                    BIT   1:    超时预警
                                    BIT   2:    超时报警
                                    BIT   3:    速度异常报警(建议默认取消)
                                    BIT   4:    超速报警
                                    BIT   5:
                             */
//  三.1    数据导出相关变量


int  usb_fd = 0;
static  u8 usb_outbuf[1500];
static u16 usb_outbuf_Wr = 0;

static u8  Sps_larger_5_counter = 0;
static u8  gb_100ms_time_counter = 0; //
u8 gb_BCD_datetime[6];


//   四.    GB_IC  卡相关

u8   gb_IC_loginState = 0; //  IC  插入且正确识别:   1    否则   0

// -----IC  卡信息相关---
DRV_INFO    Read_ICinfo_Reg;   // 临时读取的IC 卡信息
DRIVE_STRUCT     Drivers_struct[MAX_DriverIC_num]; // 预留5 个驾驶员的插卡对比

//  五. 扩展功能  变量
u8 Limit_max_SateFlag = 0; //   限制 最大速度
/* 定时器的控制块 */
u8  gb_time_counter = 0;
u16  jia_speed = 0;





//   Head  define
void  GB_OUT_data(u8  ID, u8 type, u16 IndexNum, u16 total_pkg_num);
void  GB_ExceedSpd_warn(void);
void  GB_IC_Exceed_Drive_Check(void);    //  超时驾驶检测
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
               发送GB19056  相关数据
*/
static Send_gb_vdr_variable_init(void)
{
    VDR_send.Float_ID = 0;	 //  命令流水号
    //-------  记录仪列表重传---
    VDR_send.RSD_State = 0; 	//	重传状态   0 : 重传没有启用   1 :  重传开始    2  : 表示顺序传完但是还没收到中心的重传命令
    VDR_send.RSD_Reader = 0;	//	重传计数器当前数值
    VDR_send.RSD_total = 0; 	//	重传选项数目

    VDR_send.Total_pkt_num = 0; // 分包总包数
    VDR_send.Current_pkt_num = 0;
    VDR_send.Read_indexNum = 0; // 发送读取时候使用
    VDR_send.CMD = 0;	 //  数据采集
}

static u16 Send_gb_vdr_08to15_stuff( JT808_TX_NODEDATA *nodedata)
{
    JT808_TX_NODEDATA		 *iterdata	= nodedata;
    //VDR_JT808_SEND	* p_user= (VDR_JT808_SEND*)nodedata->user_para;
    uint8_t				    *pdata;
    uint16_t  i = 0, wrlen = 0;
    uint16_t    SregLen = 0, Swr = 0;
    uint16_t	body_len;   /*消息体长度*/
    uint16_t    stuff_len = 0;


    // step 0 :  init
    pdata = nodedata->tag_data;  // 填充数据的起始指针

    //	step1 :  消息填充
    pdata[0]	= 0x07; 					//	ID
    pdata[1]	= 0x00;

    //	消息体长度最后填写

    DF_Sem_Take;
    // step2:	更新当前包数和发送ID 流水号

    switch(VDR_send.CMD) 							  //  消息流水号号码
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

    //step3:   填充 VDR 内容
    wrlen = 16;					/*跳过消息头*/
    //--------------------------------------------------------------------------------------------
    switch(VDR_send.CMD)
    {
    case 0x08:															  //  08   采集指定的行驶速度记录
        if( iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // 命令字
        }

        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // 起始头
        pdata[wrlen++]   = 0x7A;
        pdata[wrlen++]   = 0x08;					  //命令字

        //  长度
#ifdef JT808_TEST_JTB
        SregLen							  = 504;//504;//630;					  // 信息长度		630
#else
        if(Vdr_Wr_Rd_Offset.V_08H_Write > 5)
            SregLen = 630;					 // 信息长度		630
        else
            SregLen = Vdr_Wr_Rd_Offset.V_08H_Write * 126;
#endif

        pdata[wrlen++]   = SregLen >> 8;			  // Hi
        pdata[wrlen++]   = SregLen;				  // Lo

        pdata[wrlen++] = 0x00;					  // 保留字
        //  信息内容

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

    case	 0x09:															  // 09   指定的位置信息记录
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // 命令字
        }
        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // 起始头
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x09;					  //命令字

        //  长度
#ifdef JT808_TEST_JTB
        SregLen							  = 666;//666 ; 				 // 信息长度
#else
        if(Vdr_Wr_Rd_Offset.V_09H_Write > 0)
            SregLen								= 666;	//666 * 2;					// 信息长度
        else
            SregLen = 0;
#endif
        pdata[wrlen++]   = SregLen >> 8;			  // Hi 	 666=0x29A
        pdata[wrlen++]   = SregLen;				  // Lo

        //pdata[wrlen++]=0;	// Hi	   666=0x29A
        //pdata[wrlen++]=0;	 // Lo

        pdata[wrlen++] = 0x00;					  // 保留字
        //  信息内容
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

    case	 0x10:															  // 10-13	   10	事故疑点采集记录
        //事故疑点数据
        if( iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // 命令字
        }
        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // 起始头
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x10;					  //命令字

#ifdef JT808_TEST_JTB
        SregLen							  = 234;				//0 	   // 信息长度
#else
        if(Vdr_Wr_Rd_Offset.V_10H_Write > 0)
            SregLen					= 234 ;//*100;				  //0		 // 信息长度
        else
            SregLen = 0;
#endif
        pdata[wrlen++]   = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]   = (u8)SregLen;			  // Lo

        pdata[wrlen++] = 0x00;					  // 保留字

        //------- 信息内容 ------
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
    case	0x11:															  // 11 采集指定的的超时驾驶记录
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // 命令字
        }

        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // 起始头
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x11;					  //命令字

#ifdef JT808_TEST_JTB
        SregLen							  = 50; 				// 信息长度
#else
        if(Vdr_Wr_Rd_Offset.V_11H_Write == 0)
            break;

        if(Vdr_Wr_Rd_Offset.V_11H_Write > 10)
            SregLen = 500;					 // 信息长度	   50*10
        else
            SregLen = Vdr_Wr_Rd_Offset.V_11H_Write * 50;
#endif

        pdata[wrlen++]   = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]   = (u8)SregLen;			  // Lo    65x7

        pdata[wrlen++] = 0x00;					  // 保留字


        /*
        每条 50 bytes	，100 条	获取的是每10 条打一包  500 packet	 Totalnum=10
        */
#ifdef JT808_TEST_JTB
        gb_get_11h( pdata + wrlen, VDR_send.Current_pkt_num);							 //50  packetsize	   num=100
        wrlen += 50;
#else


#endif

        break;





        //----------------------------------------------------------------------------------------------------------

    case	0x12:															  // 12 采集指定驾驶人身份记录	---Devide
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;		  // 命令字

        }
        Swr = wrlen;
        pdata[wrlen++]   = 0x55;					  // 起始头
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x12;					  //命令字

#ifdef JT808_TEST_JTB
        SregLen							  = 500;				 // 信息长度
#else
        if(Vdr_Wr_Rd_Offset.V_12H_Write > 10)
            SregLen = 250;					  // 信息长度		50*10
        else
            SregLen = Vdr_Wr_Rd_Offset.V_12H_Write * 25;
#endif
        pdata[wrlen++]   = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]   = (u8)SregLen;			  // Lo    65x7

        pdata[wrlen++] = 0x00;					  // 保留字
        //------- 信息内容 ------


        /*
        驾驶员身份登录记录  每条25 bytes		200条	   20条一包 500 packetsize	totalnum=10
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


    case	0x13:	   //  外部供电记录      7bytes
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]	 = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]	 = (u8)VDR_send.Float_ID;
            pdata[wrlen++]	 = VDR_send.CMD;		  // 命令字

        }
        Swr = wrlen;
        pdata[wrlen++]	 = 0x55;					  // 起始头
        pdata[wrlen++]	 = 0x7A;

        pdata[wrlen++] = 0x13;					  //命令字

        SregLen = Vdr_Wr_Rd_Offset.V_13H_Write * 7;
        pdata[wrlen++]	 = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]	 = (u8)SregLen; 		  // Lo    65x7

        pdata[wrlen++] = 0x00;					  // 保留字
        //------- 信息内容 ------


        /*
        外部供电记录      7bytes		100条
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
    case	0x14:	   //记录仪参数修改记录      7bytes
        if(iterdata->packet_no == 1 )
        {
            pdata[wrlen++]	 = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]	 = (u8)VDR_send.Float_ID;
            pdata[wrlen++]	 = VDR_send.CMD;		  // 命令字

        }
        Swr = wrlen;
        pdata[wrlen++]	 = 0x55;					  // 起始头
        pdata[wrlen++]	 = 0x7A;

        pdata[wrlen++] = 0x14;					  //命令字

        SregLen = Vdr_Wr_Rd_Offset.V_14H_Write * 7;
        pdata[wrlen++]	 = (u8)( SregLen >> 8 );	  // Hi
        pdata[wrlen++]	 = (u8)SregLen; 		  // Lo    65x7

        pdata[wrlen++] = 0x00;					  // 保留字
        //------- 信息内容 ------


        /*
        外部供电记录      7bytes		100条
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

    case	   0x15:													  // 15 采集指定的速度状态日志	   --------Divde
        if( iterdata->packet_no == 1 )
        {
            pdata[wrlen++]   = (u8)( VDR_send.Float_ID >> 8 );
            pdata[wrlen++]   = (u8)VDR_send.Float_ID;
            pdata[wrlen++]   = VDR_send.CMD;	 // 命令字
        }
        Swr = wrlen;
        pdata[wrlen++]   = 0x55;				  // 起始头
        pdata[wrlen++]   = 0x7A;

        pdata[wrlen++] = 0x15;				  //命令字

#ifdef JT808_TEST_JTB

        SregLen							  = 133 ;			  // 信息长度
#else

        if(Vdr_Wr_Rd_Offset.V_15H_Write > 2)
            SregLen = 266; 					 // 信息长度	  50*10
        else
            SregLen = Vdr_Wr_Rd_Offset.V_15H_Write * 133;
#endif

        pdata[wrlen++]   = (u8)( SregLen >> 8 ); // Hi
        pdata[wrlen++]   = (u8)SregLen;		  // Lo    65x7

        pdata[wrlen++] = 0x00;				  // 保留字


        /*
        每条 133 个字节   10 条	1个完整包	  5*133=665 	   totalnum=2
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
    pdata[wrlen++] = fcs_caulate(pdata + Swr, wrlen - Swr); // 填充记录仪校验数值
    //------------------------------------------------
    iterdata->msg_len	= wrlen;	   ///发送数据的长度加上消息头的长度

    //	消息体长度最后填写
    body_len = wrlen - 16;
    pdata[2]	= 0x20 | ( body_len >> 8 ); /*消息体长度*/
    pdata[3]	= body_len & 0xff;
    //-------------------------------------------------------------------------------------------
    /*调整数据 设置消息头*/
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

    // 判断是否有列表重传
    if((VDR_send.RSD_total) && (VDR_send.RSD_State == 1)) // 判断是否有列表
    {
        if(VDR_send.RSD_Reader < VDR_send.RSD_total)
        {
            //顺序读取列表重传项里边的序号
            iterdata->packet_no = VDR_send.ReSdList[VDR_send.RSD_Reader];
            Send_gb_vdr_08to15_stuff(nodedata);
            nodedata->state		= IDLE;
            nodedata->retry		= 0;
            nodedata->max_retry = 1;
            nodedata->timeout	=  RT_TICK_PER_SECOND; // timeout 500  ms
            //  setp1:   发送一包累计一个计数
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
            VDR_send.RSD_State = 2; // 列表重传已经发送完毕等待应答
    }
    else
    {
        if(VDR_send.Current_pkt_num < VDR_send.Total_pkt_num)
        {
            iterdata->packet_no = VDR_send.Current_pkt_num + 1; //  当前包序号 ，从1	开始
            Send_gb_vdr_08to15_stuff(nodedata);
            nodedata->state		= IDLE;
            nodedata->retry		= 0;
            nodedata->max_retry = 1;
            nodedata->timeout	=  RT_TICK_PER_SECOND; // timeout 500  ms
            //  setp1:   发送一包累计一个计数
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
    case 0x0700:                                            /*超时以后，直接上报数据*/
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
        ret = Send_gb_vdr_tx_data( nodedata);
        if(( ret == 0xFFFF ) || (ret == 0 ))
        {
            Send_gb_vdr_variable_init();
            rt_kprintf( "\n全部上报完成\n" );               ///等待应答0x8800
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
    uint16_t				ack_seq;                        /*应答序号*/
    u16						fram_num;

    // step 0:    get  msg_ID
    temp_msg_id = buf_to_data( pmsg, 2 );
    fram_num	= buf_to_data( pmsg + 10, 2 );

    // step 1:      通用应答
    if( 0x8001 == temp_msg_id )                             ///通用应答,有可能应答延时
    {
#if 0
        ack_seq = ( pmsg[12] << 8 ) | pmsg[13];             /*判断应答流水号*/
        if( ack_seq != nodedata->head_sn )                  /*流水号对应*/
        {
            nodedata->timeout	= 30 * RT_TICK_PER_SECOND;  /*等30秒*/
            nodedata->state		= WAIT_ACK;
            return WAIT_ACK;
        }
#endif
        //ret=Send_gb_vdr_tx_data(iterdata);

        return nodedata->state;
    }
    else
        //  step 2:     分包重传列表应答
        if( 0x8003 == temp_msg_id ) 					///专用应答
        {
            debug_write("收到补传应答8003");

            VDR_send.Float_ID = buf_to_data( pmsg + 12, 2 );	/*原始消息流水号*/


            if( pmsg[14] == 0 ) /*重传包总数*/
            {
                //  recover  state
                rt_kprintf("\n  0x%02X记录仪数据上报完成!", VDR_send.CMD);
                Send_gb_vdr_variable_init();
                nodedata->state = ACK_OK;
                jt808_tx_0x0001( fram_num, 0x8003, 0 );
                return ACK_OK;
            }
            else
            {
                // step 2.1:      列表重传
                // p_user = (VDR_JT808_SEND*)( iterdata->user_para );
                memset(VDR_send.ReSdList, 0, sizeof(VDR_send.ReSdList));
                // step 2.2:      列表重传相关状态初始化
                VDR_send.RSD_total = pmsg[14]; // 存储列表重传总包数
                VDR_send.RSD_Reader = 0; //
                VDR_send.RSD_State = 1; //   开始列表重传


                //
#if  1
                rt_kprintf( "\n 记录仪列表重传项数目=%d \n ", pmsg[14] );   /*重新获取发送数据*/
                for( i = 0; i < pmsg[14]; i++ )   // 存储列表重传项
                {
                    VDR_send.ReSdList[i] = buf_to_data( pmsg + i * 2 + 15, 2 );
                    rt_kprintf( "%d ", VDR_send.ReSdList[i]);
                }
                rt_kprintf( "\n ");

                ret = Send_gb_vdr_tx_data(nodedata);
                if(( ret == 0xFFFF ) || (ret == 0 ))  												/*bitter 没有找到图片id*/
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
    //    1. 创建节点，初始化节点，启动数据分包上传VDR      数据
#if 1
    u32						TempAddress;
    uint16_t				ret;
    JT808_TX_NODEDATA		 *pnodedata;

    // setp 1:   begin
    pnodedata = node_begin( linkno, MULTI_CMD, 0x0700, 0x8001, 800); // GB_VDR_MAX_1_PKG_SIZE);
    if( pnodedata == RT_NULL )
    {
        rt_kprintf( "\n分配记录仪资源失败" );
        return RT_ENOMEM;
    }
    // step 2:  stuff  init info
    //pnodedata->size			= TempPackageHead.Len - 64 + 36; // 该字段再VDR  传输中没有什么意义
    pnodedata->linkno = linkno;
    pnodedata->packet_num	= VDR_send.Total_pkt_num; // 总包数
    pnodedata->packet_no	= 0;
    pnodedata->user_para	= RT_NULL;

    ret = Send_gb_vdr_tx_data( pnodedata);
    if(ret && (ret != 0xFFFF))
    {
        pnodedata->retry		= 0;
        pnodedata->max_retry = 1;
        pnodedata->timeout	=  RT_TICK_PER_SECOND;    //  800ms  发送一包
        node_end( MULTI_CMD_NEXT, pnodedata, Send_gb_vdr_jt808_timeout, Send_gb_vdr_jt808_0x0700_response, RT_NULL );
    }
    else
    {
        rt_free( pnodedata );
        rt_kprintf( "\n没有找到记录仪数据cmd=0x%02X:%s", VDR_send.CMD, __func__ );
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

    //   step2:	判断 记录仪命令字，执行返回信息的填充
    switch( cmd )
    {
    case 0x00: /*采集记录仪执行标准版本*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;

        buf[Swr++]  = 0x55;			   // 起始头
        buf[Swr++]  = 0x7A;
        buf[Swr++]  = 0x00;			   //命令字
        buf[Swr++]  = 0x00;			   // Hi
        buf[Swr++]  = 2; 			   // Lo

        buf[Swr++]  = 0x00;			   // 保留字
        buf[Swr++]  = 0x12;			   //  12  年标准
        buf[Swr++]  = 0x00;
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );

        break;
    case 0x01: //  01  当前驾驶员代码及对应的机动车驾驶证号
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;

        buf[Swr++]  = 0x55;						   // 起始头
        buf[Swr++]  = 0x7A;
        buf[Swr++]  = 0x01;						   //命令字

        buf[Swr++]  = 0x00;						   // Hi
        buf[Swr++]  = 18;						   // Lo

        buf[Swr++] = 0x00;							// 保留字

        memcpy( buf + Swr, "410727198503294436", 18 );  //信息内容
        Swr				   += 18;

        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;
    case 0x02: /*行车记录仪时间*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;

        buf[Swr++]  = 0x55;						   // 起始头
        buf[Swr++]  = 0x7A;
        buf[Swr++]  = 0x02;						   //命令字

        buf[Swr++]  = 0x00;						   // Hi
        buf[Swr++]  = 6; 						   // Lo

        buf[Swr++] = 0x00;							// 保留字
        memcpy(buf + Swr, gps_baseinfo.datetime, 6);
        Swr += 6;
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;
    case 0x03:		// 03 采集360h内里程
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;


        buf[Swr++]  = 0x55;						   // 起始头
        buf[Swr++]  = 0x7A;
        buf[Swr++]  = 0x03;						   //命令字

        buf[Swr++]  = 0x00;						   // Hi
        buf[Swr++]  = 20;						   // Lo

        buf[Swr++] = 0x00;							// 保留字


        mytime_to_bcd( tmpbuf, mytime_now ); 		   /*实时时间*/
        memcpy( buf + Swr, tmpbuf, 6 );
        Swr += 6;
        mytime_to_bcd( tmpbuf, jt808_param.id_0xF030 ); /*首次安装时间*/
        memcpy( buf + 15, tmpbuf, 6);
        Swr += 6;
        tmpbuf[0]   = 0x00;
        tmpbuf[1]   = 0x00;
        tmpbuf[2]   = 0x00;
        tmpbuf[3]   = 0x00;
        memcpy( buf + Swr, tmpbuf, 4);
        Swr += 4;
        i		   = jt808_data.id_0xFA01 / 100;		 /*总里程 0.1KM BCD码 00-99999999*/
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
    case 0x04: /*特征系数*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;


        buf[Swr++]  = 0x55;							   // 起始头
        buf[Swr++]  = 0x7A;

        buf[Swr++] = 0x04;								//命令字

        buf[Swr++]  = 0x00;							   // Hi
        buf[Swr++]  = 8; 							   // Lo

        buf[Swr++] = 0x00;								// 保留字

        //  信息内容
        mytime_to_bcd( tmpbuf, mytime_now ); 		   /*实时时间*/
        memcpy( buf + Swr, tmpbuf, 6 );
        Swr += 6;

        buf[Swr++]  = (u8)( (jt808_param.id_0xF032 & 0x0000FFFF) >> 8 );
        buf[Swr++]  = (u8)( (jt808_param.id_0xF032 & 0x0000FFFF));
        //------------------------------------------
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;
    case 0x05: /*车辆信息  41byte*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;


        buf[Swr++]  = 0x55;							   // 起始头
        buf[Swr++]  = 0x7A;

        buf[Swr++] = 0x05;								//命令字

        buf[Swr++]  = 0x00;			   // Hi
        buf[Swr++]  = (u8)41;					  // Lo 	 65x7

        buf[Swr++] = 0x00;								// 保留字
        //-----------   信息内容  --------------
        memcpy( buf + Swr, "LGBK22E70AY100102", 17 );	//信息内容
        Swr += 17;
        memcpy( buf + Swr, jt808_param.id_0x0083, 12);			 // 车牌号
        Swr += 12;
        memcpy( buf + Swr, "小型车", 6 );
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
    case 0x06:							   /*状态信号配置信息 87byte*/
        buf[0]  = seq >> 8;
        buf[1]  = seq & 0xff;
        buf[2]  = cmd;
        fcs	   = 0;
        Swr = 3;


        //  06 信号配置信息
        buf[Swr++]  = 0x55;				   // 起始头
        buf[Swr++]  = 0x7A;

        buf[Swr++] = 0x06;					//命令字

        // 信息长度
        buf[Swr++]  = 0x00; // Hi
        buf[Swr++]  = 87;		  // Lo

        buf[Swr++] = 0x00;					// 保留字

        mytime_to_bcd( tmpbuf, mytime_now ); 		   /*实时时间*/
        memcpy( buf + Swr, tmpbuf, 6 );
        Swr += 6;
        //-------  状态字个数----------------------
        buf[Swr++] = 0;// 8//修改为0
        //---------- 状态字内容-------------------
        /*
        -------------------------------------------------------------
        F4  行车记录仪 TW705	管脚定义
        -------------------------------------------------------------
        遵循	GB10956 (2012)	Page26	表A.12	规定
        -------------------------------------------------------------
        | Bit  |	  Note		 |	必备|	MCUpin	|	PCB pin  |	 Colour | ADC
        ------------------------------------------------------------
        D7	   刹车 		  * 		   PE11 			9				 棕
        D6	   左转灯	  * 			PE10			10				 红
        D5	   右转灯	  * 			PC2 			 8				  白
        D4	   远光灯	  * 			PC0 			 4				  黑
        D3	   近光灯	  * 			PC1 			 5				  黄
        D2	   雾灯 		 add		  PC3			   7				绿		*
        D1	   车门 		 add		  PA1			   6				灰		*
        D0	   预留
        */
        memcpy( buf + Swr, "预留 	 ", 10 );		// D0
        Swr += 10;
        memcpy( buf + Swr, "预留 	 ", 10 );		// D1
        Swr += 10;
        memcpy( buf + Swr, "预留 	 ", 10 );		// D2
        Swr += 10;
        memcpy( buf + Swr, "近光灯	 ", 10 );		// D3
        Swr += 10;
        memcpy( buf + Swr, "远光灯	 ", 10 );		// D4
        Swr += 10;
        memcpy( buf + Swr, "右转灯	 ", 10 );		// D5
        Swr += 10;
        memcpy( buf + Swr, "左转灯	 ", 10 );		// D6
        Swr += 10;
        memcpy( buf + Swr, "刹车 	 ", 10 );		// D7
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
        buf[Swr++]	= 0x55; 						// 起始头
        buf[Swr++]	= 0x7A;

        buf[Swr++] = 0x07;							//命令字

        // 信息长度
        buf[Swr++]	= 0x00; 		// Hi
        buf[Swr++]	= 30;				// Lo

        buf[Swr++] = 0x00;							// 保留字
        //------- 信息内容 ------
        memcpy( buf + Swr, "C000863", 7 );			// 3C 认证代码
        Swr += 7;
        memcpy( buf + Swr, "TW705", 5 ); // 产品型号 16
        Swr 					+= 5;

        for(i = 0; i < 11; i++)
            buf[Swr++]	= 0x00;


        buf[Swr++]	= 0x13; 						// 生产日期
        buf[Swr++]	= 0x03;
        buf[Swr++]	= 0x01;
        buf[Swr++]	= 0x00; 						// 生产流水号
        buf[Swr++]	= 0x00;
        buf[Swr++]	= 0x00;
        buf[Swr++]	= 0x35;

        //------------------------------------------
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack( 0x0700, buf, Swr );
        break;

    case	0x08:  // 采集行驶速度记录
        VDR_send.Total_pkt_num   = 720;
        VDR_send.Current_pkt_num		  = 0;
        VDR_send.Float_ID = seq; //  返回ID

        break;

    case	0x09: // 位置信息记录
        VDR_send.Total_pkt_num   = 360;  //333----720    666--360
        VDR_send.Current_pkt_num		  = 0; //0
        VDR_send.Float_ID = seq; //  返回ID

        break;
    case	0x10:  //  事故疑点记录
        VDR_send.Total_pkt_num	= 100;
        VDR_send.Current_pkt_num	= 0;
        VDR_send.Float_ID = seq; //  返回ID

        break;
    case	0x11: //  超时驾驶记录
        VDR_send.Total_pkt_num	= 100;
        VDR_send.Current_pkt_num	= 0;
        VDR_send.Float_ID = seq; //  返回ID

        break;
    case	0x12: //驾驶员身份记录
        VDR_send.Total_pkt_num	= 10;
        VDR_send.Current_pkt_num	= 0;
        VDR_send.Float_ID = seq; //  返回ID

        break;
    case	0x13: //记录仪外部供电记录
        break;
    case	0x14: // 记录仪参数修改记录
        break;
    case	0x15: // 速度状态日志

        VDR_send.Total_pkt_num	= 10;
        VDR_send.Current_pkt_num	= 0;
        VDR_send.Float_ID = seq; //  返回ID
        break;
        // sub  step 1 :  和车辆设置有关
    case 0x82:		   /*设置车辆信息*/
        break;
    case 0x83:		   /*设置初次安装日期*/
        break;
    case 0x84:		   /*设置状态量配置*/
        break;

    case 0xC2:		   /*设置记录仪时间*/
        break;
    case 0xC3:		   /*设置脉冲系数*/
        break;
    case 0xC4:		   /*设置初始里程*/
        break;
    }
}



/*行车记录仪数据采集命令*/
void Send_gb_vdr_Rx_8700_ack( uint8_t linkno, uint8_t *pmsg )
{
    // note:     处理中心下发的 VDR 相关操作信息
    uint8_t		 *psrc;
    uint8_t		buf[128];
    uint8_t		tmpbuf[16];

    uint16_t	seq = JT808HEAD_SEQ( pmsg );
    uint16_t	len = JT808HEAD_LEN( pmsg );
    uint8_t		cmd = *(pmsg + 12 ); /*跳过前面12字节的头*/
    uint16_t	SregLen;
    uint32_t	i;
    //uint8_t		fcs;
    uint16_t    Swr = 0;

#if  1
    Send_gb_vdr_variable_init();
    //step0 :  get  system ID
    VDR_send.CMD = cmd;
    rt_kprintf("\r\n   Recorder=0x%02X \r\n", VDR_send.CMD);
    //step1 :  0x00~0x0700   不需要分包，只回复单包，立即回复即可
    if(cmd <= 0x07)
        ;  //not add  curretnly
    //step2:   判断 记录仪命令字，执行返回信息的填充
    switch( cmd )
    {
    case 0x00: /*采集记录仪执行标准版本*/
    case 0x01: //  01  当前驾驶员代码及对应的机动车驾驶证号
    case 0x02: /*行车记录仪时间*/
    case 0x03:  // 03 采集360h内里程
    case 0x04: /*特征系数*/
    case 0x05: /*车辆信息  41byte*/
    case 0x06:                              /*状态信号配置信息 87byte*/
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

    case  0x08:	// 采集行驶速度记录
        if(jt808_param.id_0xF041 == 0)
        {
            // 抽检时为 0
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // 起始头
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //命令字
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // 保留字
            // 没有信息填充
            buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
            jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );

        }
        else
        {
            //  填充完整车台
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

    case  0x09: // 位置信息记录
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // 起始头
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //命令字
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // 保留字
            // 没有信息填充
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
    case  0x10:  //  事故疑点记录
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // 起始头
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //命令字
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // 保留字
            // 没有信息填充
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
    case  0x11: //  超时驾驶记录
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // 起始头
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //命令字
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // 保留字
            // 没有信息填充
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
    case  0x12: //驾驶员身份记录
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // 起始头
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //命令字
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // 保留字
            // 没有信息填充
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
    case  0x13:   //  外部供电记录
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // 起始头
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //命令字
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // 保留字
            // 没有信息填充
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
                psrc[Swr++]	= cmd;       // 命令字
                psrc[Swr++]	= 0x55;                 // 起始头
                psrc[Swr++]	= 0x7A;

                psrc[Swr++] = 0x13;                   //命令字

                SregLen								= 700;                  // 信息长度
                psrc[Swr++]	= (u8)( SregLen >> 8 ); // Hi
                psrc[Swr++]	= (u8)SregLen;          // Lo    65x7

                psrc[Swr++] = 0x00;                   // 保留字
                //------- 信息内容 ------
                /*
                外部供电记录   7 个字节  100 条       一个完整包
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

            buf[Swr++]	= 0x55;             // 起始头
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //命令字
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // 保留字
            // 没有信息填充
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
                psrc[Swr++]	= cmd;       // 命令字
                // 14 记录仪参数修改记录
                psrc[Swr++]	= 0x55;                 // 起始头
                psrc[Swr++]	= 0x7A;

                psrc[Swr++] = 0x14;                   //命令字

                SregLen								= 700;                  // 信息长度
                psrc[Swr++]	= (u8)( SregLen >> 8 ); // Hi
                psrc[Swr++]	= (u8)SregLen;          // Lo    65x7

                psrc[Swr++] = 0x00;                   // 保留字
                //------- 信息内容 ------

                /*
                每条 7 个字节   100 条    1个完整包
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
    case  0x15: // 速度状态日志
        if(jt808_param.id_0xF041 == 0)
        {
            buf[0]	= seq >> 8;
            buf[1]	= seq & 0xff;
            buf[2]	= cmd;
            Swr = 3;

            buf[Swr++]	= 0x55;             // 起始头
            buf[Swr++]	= 0x7A;
            buf[Swr++]	= cmd;             //命令字
            buf[Swr++]	= 0x00;             // Hi
            buf[Swr++]	= 0x00;                // Lo

            buf[Swr++]	= 0x00;             // 保留字
            // 没有信息填充
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
    default:    //    采集错误
        buf[0]	= seq >> 8;
        buf[1]	= seq & 0xff;
        buf[2]	= cmd;
        Swr = 3;



        buf[Swr++]	= 0x55; 			// 起始头
        buf[Swr++]	= 0x7A;
        buf[Swr++]	= 0xFA;			   //命令字      0xFA   采集错误
        buf[Swr++]	= 0x00; 			// Hi
        buf[Swr++]	= 0x00; 			   // Lo

        buf[Swr++]	= 0x00; 			// 保留字
        // 没有信息填充
        buf[Swr++] = fcs_caulate(buf + 3, Swr - 3);
        jt808_tx_ack_ex(linkno, 0x0700, buf, Swr );
        break;
    }
#endif

}

void Send_gb_vdr_Rx_8701_ack( uint8_t linkno, uint8_t *pmsg )
{
    // note:     处理中心下发的 VDR 相关操作信息
    uint8_t		 *psrc;
    uint8_t		buf[128];
    uint8_t		tmpbuf[16];

    uint16_t	seq = JT808HEAD_SEQ( pmsg );
    uint16_t	len = JT808HEAD_LEN( pmsg );
    uint8_t		cmd = *(pmsg + 12 ); /*跳过前面12字节的头*/
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

        // sub	step 1 :  和车辆设置有关
    case 0x82:			/*设置车辆信息*/

        memset(jt808_param.id_0xF005, 0, sizeof(jt808_param.id_0xF005));
        memset(jt808_param.id_0x0083, 0, sizeof(jt808_param.id_0x0083));
        memset(jt808_param.id_0xF00A, 0, sizeof(jt808_param.id_0xF00A));

        //-----------------------------------------------------------------------
        memcpy(jt808_param.id_0xF005, pmsg + 19, 17);
        memcpy(jt808_param.id_0x0083, pmsg + 36, 12);
        memcpy(jt808_param.id_0xF00A, pmsg + 48, 12);
        //  存储
        param_save(1);

    case 0xC2: //设置记录仪时钟
        // 没啥用，给个回复就行，俺有GPS校准就够了
        //  年月日 时分秒 BCD
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


    case 0xC3: //车辆速度脉冲系数（特征系数）
        // 前6 个是当前时间
        year = (pmsg[19] >> 4) * 10 + (pmsg[19] & 0x0F);
        month = (pmsg[1 + 19] >> 4) * 10 + (pmsg[1 + 19] & 0x0F);
        day = (pmsg[2 + 19] >> 4) * 10 + (pmsg[2 + 19] & 0x0F);
        hour = (pmsg[3 + 19] >> 4) * 10 + (pmsg[3 + 19] & 0x0F);
        min = (pmsg[4 + 19] >> 4) * 10 + (pmsg[4 + 19] & 0x0F);
        sec = (pmsg[5 + 19] >> 4) * 10 + (pmsg[5 + 19] & 0x0F);

        //	update	RTC
        date_set(year, month, day);
        time_set(hour, min, sec);

        jt808_param.id_0xF032 = (pmsg[6 + 19] << 8) + (u32)pmsg[7 + 19]; // 特征系数  速度脉冲系数

        //  存储
        param_save( 1 );

        break;
    case 0x83: //  记录仪初次安装时间

        memcpy(temp_buffer, pmsg + 19, 6);
        jt808_param.id_0xF030 = mytime_from_bcd(temp_buffer);
        //  存储
        param_save( 1 );

        break;
    case 0x84: // 设置信号量配置信息
        break;

    case 0xC4: //	设置初始里程

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
        // instr	 + 12	   设置初始里程的位置	BCD

        reg_dis = (pmsg[18 + 19 - 6] >> 4) * 10000000 + (pmsg[18 + 19 - 6] & 0x0F) * 1000000 + (pmsg[19 + 19 - 6] >> 4) * 100000 + (pmsg[19 + 19 - 6] & 0x0F) * 10000 \
                  +(pmsg[20 + 19 - 6] >> 4) * 1000 + (pmsg[20 + 19 - 6] & 0x0F) * 100 + (pmsg[21 + 19 - 6] >> 4) * 10 + (pmsg[21 + 19 - 6] & 0x0F);


        jt808_data.id_0xFA01 = reg_dis * 100;
        //  存储
        param_save( 1 );
        data_save();
        break;

    default:
        Error = 2; // 设置错误
        break;
    }

    if(Error != 2)
        VDR_product_14H(cmd);
    else
        cmd = 0xFB; // 设置错误


    // -----统一回复中心
    buf[0]	= seq >> 8;
    buf[1]	= seq & 0xff;
    buf[2]	= cmd;
    Swr = 3;

    buf[Swr++]	= 0x55; 			// 起始头
    buf[Swr++]	= 0x7A;
    buf[Swr++]	= cmd;			   //命令字
    buf[Swr++]	= 0x00; 			// Hi
    buf[Swr++]	= 0x00; 			   // Lo

    buf[Swr++]	= 0x00; 			// 保留字
    // 没有信息填充
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
    u32	  SaveContent_add = 0;	 // 存储page 页
    u8	  reg[9];
    u8	  Flag_used = 0x01;


    //  2 .	Excute
    sst25_read(Add_offset, (unsigned char *)head, 448);

    /*

      通过查询Block 第1个Page的前448字节是否为0xFF 判断，计算出要写入内容的偏移地址，当448都标识使用完后，擦除该Block。
      然后从头开始。

           存储内容相关起始位置从512个字节开始，
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

    sst25_write_through(Add_offset + offset, &Flag_used, 1); //  更新状态位
    sst25_write_through(SaveContent_add, reg, 8);  //  更新记录内容

    return true;
    //				   The	End
}


u8  DF_Read_RecordAdd(u32 Wr_Address, u32 Rd_Address, u32 addr)
{
    u8		head[448];
    u16	  offset = 0;
    u32	 Add_offset = addr;	//	page offset
    u32	 Reg_wrAdd = 0, Reg_rdAdd = 0;
    u32	  SaveContent_add = 0;	 // 存储page 页

    //Wr_Address, Rd_Address  , 没有什么用只是表示输入，便于写观察而已
    //  1.	Classify

    //  2 .	Excute
    sst25_read(Add_offset, (unsigned char *)head, 448); //	读出信息
    /*

     通过查询Block 第1个Page的前448字节是否为0xFF 判断，计算出要写入内容的偏移地址，当448都标识使用完后，擦除该Block。
     然后从头开始。

    	  存储内容相关起始位置从512个字节开始，
    				Savepage=512+Add_offset*8

    */

    for(offset = 0; offset < 448; offset++)
    {
        if(0xFF == head[offset])
            break;
    }

    offset--;	// 第一个不会为0xFF

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
    u32  addr = ADDR_DF_VDR_15minDATA; // 起始地址
    //  每分钟记录 	7 byte	   6(datatime)+1 spd		15 分钟是15*7= 105 个字节 ,这里占用128 个字节每条记录
    // 1 个扇区  4096 bytes 能存储 32	 条记录
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
    u32  addr = ADDR_DF_VDR_15minDATA; // 起始地址

    if(Avrg_15minSpd.write == 0)
    {
        // rt_kprintf("\r\n 无15分钟最新记录1\r\n");
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
        // rt_kprintf("\r\n 无15分钟最新记录: 校验错误\r\n");
        return 0;
    }
    else
        memcpy(dest, regist_str, 105);

    //OutPrint_HEX("读取15分钟平均速度记录",regist_str,105);
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
    //  OutPrint_HEX("存储15分钟平均速度记录",regist_save,105);
    return true;
}

#endif

/*
       GB19056     触发执行函数 如下
*/



//    获取传感器GB19056  格式传感器状态
u8  GB_Get_SensorStatus(void)
{
    // 查询传感器状态
    u8  Sensorstatus = 0;

    /*
     -------------------------------------------------------------
    		  F4  行车记录仪 TW705	 管脚定义
     -------------------------------------------------------------
     遵循  GB10956 (2012)  Page26  表A.12  规定
    -------------------------------------------------------------
    | Bit  |	  Note		 |	必备|	MCUpin	|	PCB pin  |	 Colour | ADC
    ------------------------------------------------------------
    	D7		刹车		   *			PE11			 9				  黄
    	D6		左转灯	   *			 PE10						  10			  红
    	D5		右转灯	   *			 PC2			  8 			   白
    	D4		远光灯	   *			 PC0			  4 			   黑白
    	D3		近光灯	   *			 PA6			  5 			   粉
    	D2		雾灯			  add		   PA7				7							 绿 	 *
    	D1		车门		   add		   PA1				6							 灰 	 *
    	D0		预留
      */


    //  1.   D7	  刹车			 *			  PE11			   J1 pin9				  黄
    if(car_state_ex & 0x10) 	//PE11
    {
        // 	  接高	触发
        Sensorstatus |= 0x80;
        // bit4 ---->刹车
    }
    else
    {
        //	 常态
        Sensorstatus &= ~0x80;
        //bit4 ---->刹车
    }
    // 2.  D6 	 左转灯 	*			  PE10			  J1 pin10				红
    if(car_state_ex & 0x08) //  PE10
    {
        //		 接高  触发
        Sensorstatus |= 0x40;
        //bit3---->  左转灯
    }
    else
    {
        //	常态
        Sensorstatus &= ~0x40;
        //bit3---->  左转灯
    }
    //  3.  D5 	 右转灯 	*			  PC2			  J1  pin8				  白
    if(car_state_ex & 0x04) //PC2
    {
        // 	   接高  触发
        Sensorstatus |= 0x20;
        // bit2----> 右转灯
    }
    else
    {
        //   常态
        Sensorstatus &= ~0x20;
        //bit2----> 右转灯
    }

    // 4.	D4		远光灯	   *			 PC0			  J1 pin4				 黑白
    if(car_state_ex & 0x02) // PC0
    {
        //	   接高  触发
        Sensorstatus |= 0x10;
        //bit 1	----->	远光灯

    }
    else
    {
        //   常态
        Sensorstatus &= ~0x10;
        //bit 1  ----->  远光灯

    }
    //5.	 D3 	 近光灯 	*			  PC1			   J1 pin5				  粉
    if(car_state_ex & 0x01) // PC1
    {
        //		 接高  触发
        Sensorstatus |= 0x08;
        //bit 0  ----->  近光灯
    }
    else
    {
        //	常态
        Sensorstatus &= ~0x08;
        //bit 0  ----->  近光灯

    }
    //	6.	  D2	  雾灯			add 		 PA7			  7 			   绿	   *
    if(car_state_ex & 0x40) //PA7
    {
        //		 接高  触发
        Sensorstatus |= 0x04;
        //	bit6 ----> 雾灯
    }
    else
    {
        //	常态
        Sensorstatus &= ~0x04;
        // bit6 ----> 雾灯
    }
    // 7.	 D1 	 车门		   add			PA1 			 6				  灰	  *
    if(jt808_status & BIT_STATUS_DOOR1) // PA1
    {
        //	   接高  触发
        Sensorstatus |= 0x02;
    }
    else
    {
        //   常态
        Sensorstatus &= ~0x02;
    }

    //	 8.  Reserved

    return Sensorstatus;
}

void  GB_IO_statusCheck(void)
{
    u16  Speed_working = 0;

    //  0 .  根据校验状态选择速度
    if(0 == jt808_param.id_0xF043)  //0  :脉冲    1  : GPS
        Speed_working = car_data.get_speed; //脉冲速度
    else
        Speed_working = gps_speed;
    //=============================================================================
    GB19056.Vehicle_sensor = GB_Get_SensorStatus();
    //------------ 0.2s    速度状态 -----------------------
    Sensor_buf[save_sensorCounter].DOUBTspeed = Speed_working / 10; //   速度  单位是km/h 所以除以10
    Sensor_buf[save_sensorCounter++].DOUBTstatus = GB19056.Vehicle_sensor; //   状态
    if(save_sensorCounter > 200)
    {
        save_sensorCounter = 0;
        sensor_writeOverFlag = 1;
    }
    //============================================================================

    //  事故疑点触发判断

    //  停车触发 事故疑点1
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
    // 2.	GB19056 相关
    //	事故疑点2	  : 外部断电触发事故疑点存储
    moni_drv(0x10, 7);
    //save 断电记录
    VDR_product_13H(0x02);
    //-------------------------------------------------------
}

void GB_Vehicle_RUNjudge(void)
{
    if(current_speed_10x > 50) //   速度大于 5  km/h
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
        //  判断 记录仪超速报警
        GB_ExceedSpd_warn();

        //  产生08  09   数据
        VDR_product_08H_09H();

        //  超时驾驶判断  产生 11 数据
        GB_IC_Exceed_Drive_Check();

        // 判断速度异常
        if((GB19056.speedlog_Day_save_times == 0) && (jt808_status & BIT_STATUS_FIXED))	// 每天只存储一条
            GB_SpeedSatus_Judge();

        gb_100ms_time_counter = 0;
    }

    //  0.2 s   isp
#if  0
    if(gb_100ms_time_counter >> 1)
    {
        GB_IO_statusCheck();  //  0.2 获取外部传感器状态
    }
#endif

    // 0.8 s  isp
    if(gb_100ms_time_counter % 5 == 0)
    {
        mytime_to_bcd(gb_BCD_datetime, mytime_now );            /*实时时间*/
    }

}

void  GB_200ms_Hard_timer(void)
{
    GB_IO_statusCheck();  //  0.2 获取外部传感器状态
}


//-------------模拟测试记录仪数据----------------------------------------
void  moni_drv(u8 CMD, u16 delta)
{

    u16 j = 0, cur = 0, i = 0, icounter = 0;

    //  行驶记录相关数据产生 触发,  且定位的情况下
    switch(CMD)
    {
#if 1
    case  0x08:

        //-------------------  08H	 --------------------------------------------------

        //  1.  年月日 时分秒	 秒是0
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
        VdrData.H_08_saveFlag = 1; //   保存	当前分钟的数据记录
        OutPrint_HEX("08 ", VdrData.H_08_BAK, 126);
        break;
    case 0x09:
        //------------------------09H ---------  min=0;	sec=0	----------------------------
        // 起始时间
        VdrData.H_09[0] = ((YEAR(mytime_now) / 10) << 4) + (YEAR(mytime_now) % 10);
        VdrData.H_09[1] = ((MONTH(mytime_now) / 10) << 4) + (MONTH(mytime_now) % 10);
        VdrData.H_09[2] = ((DAY(mytime_now) / 10) << 4) + (DAY(mytime_now) % 10);
        VdrData.H_09[3] = ((HOUR(mytime_now) / 10) << 4) + (HOUR(mytime_now) % 10);
        VdrData.H_09[4] = 0;
        VdrData.H_09[5] = 0;

        //
        for(i = 0; i < 60; i++)
        {
            //  4 .	填写常规位置	1-59 min
            {
                //   表 A.20		  (当前分钟的位置  +	当前分钟的平均速度)
                memcpy( VdrData.H_09 + 6 + i * 11, VdrData.Longi, 4); // 经度
                memcpy( VdrData.H_09 + 6 + i * 11 + 4, VdrData.Lati, 4); //纬度
                VdrData.H_09[6 + i * 11 + 8] = (gps_alti >> 8);
                VdrData.H_09[6 + i * 11 + 9] = gps_alti;
                //  当前分钟的平均速度	 从AvrgSpd_MintProcess()  引用的变量
                VdrData.H_09[6 + i * 11 + 10] = current_speed_10x;
            }
        }


        // 1.  判断是否是需要存储
        VdrData.H_09_saveFlag = 1; //   保存  当前分钟的数据记录

        OutPrint_HEX("09 ", VdrData.H_09, 666);
        break;
#endif
    case 0x10:
        //-------------------- 10H    事故疑点数据  ---------------------------------
        if(delta != 10)
        {
            //  1.  行驶结束时间
            mytime_to_bcd(VdrData.H_10, mytime_now);
        }
        //   2.   机动车驾驶证号码
        memcpy(VdrData.H_10 + 6, jt808_param.id_0xF009, 18);
        //   3.  速度和状态信息
        //-----------------------  Status Register   --------------------------------
        cur = save_sensorCounter; //20s的事故疑点
        //------------------------------------------
        if(cur > 0)	 // 从当前往前读取
            cur--;
        else
            cur = 200;
        //-----------------------------------------
        icounter = 100 + delta;
        for(j = 0; j < icounter; j++)
        {
            if(j >= delta)
            {
                VdrData.H_10[24 + (j - delta) * 2] = Sensor_buf[cur].DOUBTspeed;	//速度
                //    rt_kprintf("%d-",Sensor_buf[cur].DOUBTspeed);
                VdrData.H_10[24 + (j - delta) * 2 + 1] = Sensor_buf[cur].DOUBTstatus; //状态
            }

            if(cur > 0)	 // 从当前往前读取
                cur--;
            else
                cur = 200;
        }
        // 	4.	位置信息
        memcpy( VdrData.H_10 + 224, VdrData.Longi, 4); // 经度
        memcpy( VdrData.H_10 + 224 + 4, VdrData.Lati, 4); //纬度
        VdrData.H_10[224 + 8] = (gps_alti >> 8);
        VdrData.H_10[224 + 9] = gps_alti;

        //--------------------------------------------------------------------------
        VdrData.H_10_saveFlag = 1;
        // if(GB19056.workstate==0)
        // OutPrint_HEX("10 ",VdrData.H_10,234);
        break;

#if 1
    case 0x11:
        //  1.   机动车驾驶证号

        if( VdrData.H_12[24] == 0x01)	 //已登录
            memcpy(VdrData.H_11, Read_ICinfo_Reg.DriverCard_ID, 18);
        else
            memcpy(VdrData.H_11, jt808_param.id_0xF009, 18);
        //   2.   起始时间
        mytime_to_bcd(VdrData.H_11 + 18, mytime_now);
        //   3.  起始位置
        memcpy( VdrData.H_11 + 30, VdrData.Longi, 4); // 经度
        memcpy( VdrData.H_11 + 30 + 4, VdrData.Lati, 4); //纬度
        VdrData.H_11[30 + 8] = (gps_alti >> 8);
        VdrData.H_11[30 + 9] = gps_alti;
        //   2.   结束时间
        mytime_to_bcd((VdrData.H_11 + 24), mytime_now + 100);
        //   3.  起始位置
        memcpy( VdrData.H_11 + 40, VdrData.Longi, 4); // 经度
        memcpy( VdrData.H_11 + 40 + 4, VdrData.Lati, 4); //纬度
        VdrData.H_11[40 + 8] = (gps_alti >> 8);
        VdrData.H_11[40 + 9] = gps_alti;
        //    4.   save
        VdrData.H_11_saveFlag = 1;
        OutPrint_HEX("11 ", VdrData.H_11, 50);
        break;
    case 0x12:    // 插卡拔卡时间
        //   1. 时间发生的时间
        mytime_to_bcd(VdrData.H_12, mytime_now);

        //  2.   机动车驾驶证号
        memcpy(VdrData.H_12 + 6, jt808_param.id_0xF009, 18);
        //  3.  结果
        VdrData.H_12[24] = ((rt_tick_get() % 2) + 1);
        // 4.  save
        VdrData.H_12_saveFlag = 1;
        OutPrint_HEX("12 ", VdrData.H_12, 25);
        break;
    case 0x13:   //  充放电
        //   1. 时间发生的时间
        mytime_to_bcd(VdrData.H_13, mytime_now);
        //  2.  value
        VdrData.H_13[6] = ((rt_tick_get() % 2) + 1);
        //  3. save
        VdrData.H_13_saveFlag = 1;
        OutPrint_HEX("13 ", VdrData.H_13, 7);
        break;
    case 0x14:              // 记录仪参数修改
        //   1. 时间发生的时间
        mytime_to_bcd(VdrData.H_14, mytime_now);
        //  2.  value
        VdrData.H_14[6] = ((rt_tick_get() % 10));
        //  3. save
        VdrData.H_14_saveFlag = 1;
        OutPrint_HEX("14 ", VdrData.H_14, 7);
        break;
    case 0x15:            //  速度状态日志
        //   1. 时间发生的时间
        VdrData.H_15[0] = (rt_tick_get() % 2) + 1; // 状态
        // 起始时间
        mytime_to_bcd(VdrData.H_15 + 1, mytime_now);
        //  结束时间
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
        dest[return_len++]	 = 0x12;		   //  12  年标准
        dest[return_len++]	 = 0x00;
        break;
    case 0x01:
        memcpy( dest + return_len, jt808_param.id_0xF009, 18 ); //信息内容
        return_len				   += 18;
        break;
    case 0x02: //  02  采集记录仪的实时时钟
        /*实时时间*/
        memcpy(dest + return_len, gb_BCD_datetime, 6 );
        return_len += 6;
        break;

    case 0x03:	   // 03 采集360h内里程
        memcpy(dest + return_len, gb_BCD_datetime, 6 );
        return_len += 6;

        mytime_to_bcd( tmpbuf, jt808_param.id_0xF030 ); /*首次安装时间*/
        memcpy( dest + return_len, tmpbuf, 6);
        return_len += 6;
        //  初始里程
        tmpbuf[0]	= 0x00;
        tmpbuf[1]	= 0x00;
        tmpbuf[2]	= 0x00;
        tmpbuf[3]	= 0x00;
        memcpy(  dest + return_len, tmpbuf, 4);
        return_len += 4;
        i			= jt808_data.id_0xFA01 / 100;     /*总里程 0.1KM BCD码 00-99999999*/
        tmpbuf[0]	= ( ( ( i / 10000000 ) % 10 ) << 4 ) | ( ( i / 1000000 ) % 10 );
        tmpbuf[1]	= ( ( ( i / 100000 ) % 10 ) << 4 ) | ( ( i / 10000 ) % 10 );
        tmpbuf[2]	= ( ( ( i / 1000 ) % 10 ) << 4 ) | ( ( i / 100 ) % 10 );
        tmpbuf[3]	= ( ( ( i / 10 ) % 10 ) << 4 ) | ( i % 10 );
        memcpy( dest + return_len, tmpbuf, 4);
        return_len += 4;
        break;

    case 0x04:																   // 04  采集记录仪脉冲系数
        /*实时时间*/
        memcpy( dest + return_len, gb_BCD_datetime, 6 );
        return_len += 6;
        dest[return_len++] = (u8)( (jt808_param.id_0xF032 & 0x0000FFFF) >> 8 );
        dest[return_len++] = (u8)( (jt808_param.id_0xF032 & 0x0000FFFF));
        break;
    case 0x05:																   //05 	 车辆信息采集
        //-----------   信息内容  --------------
        memcpy( dest + return_len, jt808_param.id_0xF005, 17 ); 	  //信息内容
        return_len += 17;
        memcpy( dest + return_len, jt808_param.id_0x0083, 12 );			 // 车牌号
        return_len				   += 12;
        //车辆类型
        memcpy( dest + return_len, jt808_param.id_0xF00A, 12 );
        return_len				   += 12;
        break;
    case   0x06: 												   // 06-09
        memcpy( dest + return_len, gb_BCD_datetime, 6 );
        return_len += 6;
        //-------  状态字个数----------------------
        dest[return_len++] = 0;// 8//修改为0
        //---------- 状态字内容-------------------

        /*
          -------------------------------------------------------------
        		 F4  行车记录仪 TW705	管脚定义
          -------------------------------------------------------------
          遵循	GB10956 (2012)	Page26	表A.12	规定
          -------------------------------------------------------------
        | Bit  |	  Note		 |	必备|	MCUpin	|	PCB pin  |	 Colour | ADC
          ------------------------------------------------------------
           D7	   刹车 		  * 		   PE11 			9				 棕
           D6	   左转灯	  * 			PE10			10				 红
           D5	   右转灯	  * 			PC2 			 8				  白
           D4	   远光灯	  * 			PC0 			 4				  黑
           D3	   近光灯	  * 			PC1 			 5				  黄
           D2	   雾灯 		 add		  PC3			   7				绿		*
           D1	   车门 		 add		  PA1			   6				灰		*
           D0	   预留
        */
        memcpy( dest + return_len, "预留	", 10 );	   // D0
        return_len += 10;
        memcpy( dest + return_len, "预留	", 10 );	   // D1
        return_len += 10;
        memcpy( dest + return_len, "预留	", 10 );	   // D2
        return_len += 10;
        memcpy( dest + return_len, "近光灯	  ", 10 );	   // D3
        return_len += 10;
        memcpy( dest + return_len, "远光灯	  ", 10 );	   // D4
        return_len += 10;
        memcpy( dest + return_len, "右转灯	  ", 10 );	   // D5
        return_len += 10;
        memcpy( dest + return_len, "左转灯	  ", 10 );	   // D6
        return_len += 10;
        memcpy( dest + return_len, "刹车	", 10 );	   // D7
        return_len += 10;
        break;

    case 0x07:															   //07  记录仪唯一编号
        //------- 信息内容 ------
        memcpy( dest + return_len, "C000863", 7 );		   // 3C 认证代码
        return_len += 7;
        memcpy( dest + return_len, "TW705", 5 ); // 产品型号
        return_len				   += 5;
        for(i = 0; i < 11; i++)
            dest[return_len++]	= 0x00;

        dest[return_len++]	 = 0x14;					   // 生产日期
        dest[return_len++]	 = 0x03;
        dest[return_len++]	 = 0x01;
        dest[return_len++]	 = jt808_param.id_0xF002[3];					   // 生产流水号
        dest[return_len++]	 = jt808_param.id_0xF002[4];
        dest[return_len++]	 = jt808_param.id_0xF002[5];
        dest[return_len++]	 = jt808_param.id_0xF002[6];
        //  31--35  备用
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
    u16   get_indexnum = 0; // 获取校验
    //u16  Sd_DataBloknum=0; //   发送数据块  数目 <GB19056.Max_dataBlocks
    u8   Not_null_flag = 0; // NO  data ACK null once
    u8   return_value = 0;
    u32  Rd_Time = 0;

    u16  return_len = 0;


    return_len = 0;
    switch(ID)
    {
    case 0x00:
        dest[return_len++]	 = 0x55;		   // 起始头
        dest[return_len++]	 = 0x7A;
        dest[return_len++]	 = 0x00;		   //命令字
        SregLen							   = 0x02;			   // 信息长度
        dest[return_len++]	 = 0x00;		   // Hi
        dest[return_len++]	 = 2;				   // Lo

        dest[return_len++] = 0x00;						   // 保留字
        break;
    case 0x01:
        return_len = 0;
        dest[return_len++]	 = 0x55;					   // 起始头
        dest[return_len++]	 = 0x7A;
        dest[return_len++]	 = 0x01;					   //命令字

        dest[return_len++]	 = 0x00;					   // Hi
        dest[return_len++]	 = 18;						   // Lo

        dest[return_len++] = 0x00;						   // 保留字
        break;
    case 0x02: //  02  采集记录仪的实时时钟
        return_len = 0;

        dest[return_len++]	 = 0x55;					   // 起始头
        dest[return_len++]	 = 0x7A;
        dest[return_len++]	 = 0x02;					   //命令字

        dest[return_len++]	 = 0x00;					   // Hi
        dest[return_len++]	 = 6;							   // Lo

        dest[return_len++] = 0x00;						   // 保留字
        break;

    case 0x03:	   // 03 采集360h内里程
        return_len = 0;												 // 03 采集360h内里程

        dest[return_len++] = 0x55;						 // 起始头
        dest[return_len++] = 0x7A;
        dest[return_len++] = 0x03;						 //命令字

        dest[return_len++] = 0x00;						 // Hi
        dest[return_len++] = 20;							 // Lo

        dest[return_len++] = 0x00;						   // 保留字
        break;

    case 0x04:																   // 04  采集记录仪脉冲系数
        return_len = 0;
        dest[return_len++]	 = 0x55;						   // 起始头
        dest[return_len++]	 = 0x7A;

        dest[return_len++] = 0x04;							   //命令字

        dest[return_len++]	 = 0x00;						   // Hi
        dest[return_len++]	 = 8;								   // Lo

        dest[return_len++] = 0x00;							   // 保留字
        break;
    case 0x05:																   //05 	 车辆信息采集
        return_len = 0;
        dest[return_len++]	 = 0x55;						   // 起始头
        dest[return_len++]	 = 0x7A;

        dest[return_len++] = 0x05;							   //命令字

        SregLen							   = 41;							   // 信息长度
        dest[return_len++]	 = (u8)( SregLen >> 8 );		   // Hi
        dest[return_len++]	 = (u8)SregLen; 				   // Lo	 65x7

        dest[return_len++] = 0x00;							   // 保留字
        //-----------   信息内容  --------------
        break;
    case   0x06: 												   // 06-09
        return_len = 0;
        //  06 信号配置信息
        dest[return_len++]	 = 0x55;			   // 起始头
        dest[return_len++]	 = 0x7A;

        dest[return_len++] = 0x06;				   //命令字

        SregLen							   = 87;				   // 信息长度
        dest[return_len++]	 = (u8)( SregLen >> 8 ); // Hi
        dest[return_len++]	 = (u8)SregLen; 	   // Lo
        dest[return_len++] = 0x00;				   // 保留字
        break;

    case 0x07:															   //07  记录仪唯一编号
        return_len = 0;
        dest[return_len++]	 = 0x55;					   // 起始头
        dest[return_len++]	 = 0x7A;

        dest[return_len++] = 0x07;						   //命令字

        SregLen							   = 35;						   //206;		// 信息长度
        dest[return_len++]	 = (u8)( SregLen >> 8 );	   // Hi
        dest[return_len++]	 = (u8)SregLen; 			   // Lo

        dest[return_len++] = 0x00;						   // 保留字
        break;
    }

    return_len += GB_00_07_info_only(dest + return_len, ID);
    return return_len;

}


void  GB_null_content(u8 ID, u8 type)
{
    usb_outbuf_Wr = 0;
    usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// 起始头
    usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
    usb_outbuf[usb_outbuf_Wr++]   = ID;					//命令字
    usb_outbuf[usb_outbuf_Wr++]   = 0;			// Hi
    usb_outbuf[usb_outbuf_Wr++]   = 0;				// Lo

    usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// 保留字
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

void GB_Drv_init(void)     // 放到 startup .c
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

    //   安全警示 相关初始化
    GB_SPK_DriveExceed_clear();
    GB_SPK_SpeedStatus_Abnormal_clear();
    GB_SPK_Speed_Warn_clear();
    GB_SPK_UnloginWarn_clear();
    GB_SPK_preTired_clear();

    //  读取DF  相关参数
    total_ergotic();
}

void GB_ExceedSpd_warn(void)
{
    //   校准情况下   根据传感器速度    判断记录仪超速
    if ((jt808_param.id_0xF032 & 0x00000000) == 0x00) // 最高bit 位   第32 位  1 : 表示为校准  0:   已经校准
    {
        //  GB   超速只判断传感器速度
        if(car_data.get_speed > (jt808_param.id_0x0055 * 10) )
        {
            if(GB19056.SPK_Speed_Warn.Warn_state_Enable == 0)
                GB19056.SPK_Speed_Warn.Warn_state_Enable = 1;
        }
        else
            GB19056.SPK_Speed_Warn.Warn_state_Enable = 0; // clear
    }
}

//  国标安全提示 语音                                    Note:  放在 1s 定时器里运行
void  GB_Warn_Running(void)
{
    //   5. 驾驶员未登录且有速度
    if((GB19056.SPK_UnloginWarn.Warn_state_Enable) && (current_speed_10x > 50) && (Sps_larger_5_counter > 10))
    {
        //    在前3  个 优先级 空闲的情况下开始
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
                    tts( "驾驶员未登录请插卡登录");
                //rt_kprintf("\r\n-----驾驶员未登录请插卡登录\r\n");
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
    //   4.  疲劳驾驶与提醒功能
    if(GB19056.SPK_PreTired.Warn_state_Enable)
    {

        //	 在前3	个 优先级 空闲的情况下开始
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
                    tts( "您即将超时驾驶,请注意休息" );
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
    //   3.     超时驾驶
    if(GB19056.SPK_DriveExceed_Warn.Warn_state_Enable)
    {
        //   在前2个  优先级空闲的情况下
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
                    tts( "您已超时驾驶，请注意休息" );
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
    //  2.   速度异常
    if(GB19056.SPK_SpeedStatus_Abnormal.Warn_state_Enable)
    {
        //   在前1个优先级 空闲情况下进行
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
                    tts( "速度状态异常，请安全驾驶" );

                if(GB19056.SPK_SpeedStatus_Abnormal.group_playTimes >= 4)
                {
                    GB19056.SPK_SpeedStatus_Abnormal.group_playTimes = 0;
                    GB19056.SPK_SpeedStatus_Abnormal.Warn_state_Enable = 0; // clear     速度异常一天一次
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
    //   1.超速报警
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
                tts( "您已超速请安全驾驶" );
            // rt_kprintf("\r\n-----您已超速请安全驾驶\r\n");
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


//    事故疑点3 触发判决: 传感器有速度，但是位置10s  不变化
void GB_doubt_Data3_Trigger(u32  lati_old, u32 longi_old, u32  lati_new, u32 longi_new)
{
    //  放到   process_rmc 下
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

//    速度状态日志判断
void  GB_SpeedSatus_Judge(void)
{
    u16  deltaSpd = 0;
    u8  spd_reg = 0;

    // 只要GPS 速度(参考速度大于40 就 判断)
    if(gps_speed > 400)
    {
        //0 . 参考速度大于40 开始计数
        GB19056.gps_largeThan40_counter++;
        //1.  计算速度差值
        deltaSpd = abs(gps_speed - car_data.get_speed) * 100; // 速度差值扩大100倍
        // 2. 大于11%  计数器
        if((deltaSpd / gps_speed) >= 11)
        {
            GB19056.delataSpd_largerThan11_counter++;
            //  如果速度偏差一直小于11% ,那么首次大于11%
            if(GB19056.delataSpd_largerThan11_counter == 0)
            {
                GB19056.gps_largeThan40_counter = 0; // clear
                GB19056.speed_larger40km_counter = 0; // clear
            }
        }
        else
        {
            //  如果速度偏差一直大于11% ,那么首次小于11%
            if(GB19056.delataSpd_largerThan11_counter)
            {
                GB19056.gps_largeThan40_counter = 0; // clear
                GB19056.speed_larger40km_counter = 0; // clear
            }
            GB19056.delataSpd_largerThan11_counter = 0;
        }
        // 开始计数了再判断
        if(GB19056.gps_largeThan40_counter)
        {
            if(GB19056.gps_largeThan40_counter == 2) // 从0秒开始
                GB19056.start_record_speedlog_flag = 1;

            if( (GB19056.start_record_speedlog_flag) && (VdrData.H_15_saveFlag == 0))
            {
                if((GB19056.speed_larger40km_counter == 0) && (jt808_status & BIT_STATUS_FIXED )) //
                {
                    //   1. 时间发生的时间
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

        //    End Judge     5 分钟以后
        if(GB19056.gps_largeThan40_counter >= 299)
        {
            // 异常和计数数值相等
            if(GB19056.delataSpd_largerThan11_counter == GB19056.gps_largeThan40_counter)
            {
                VdrData.H_15[0] = 0x02;	//异常
                GB19056.SPK_SpeedStatus_Abnormal.Warn_state_Enable = 1;
            }
            else
                VdrData.H_15[0] = 0x01;

            //   1. 结束的时间
            memcpy(VdrData.H_15 + 7, gb_BCD_datetime, 6);

            //  使能存储
            VdrData.H_15_saveFlag = 1;

            GB19056.speedlog_Day_save_times++;
            if(GB19056.workstate == 0)
                rt_kprintf("\r\n  速度状态日志触发!:%d  total:%d  abnormal:%d\r\n", VdrData.H_15[0], GB19056.gps_largeThan40_counter, GB19056.delataSpd_largerThan11_counter);
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
    else if(Read_Time <= GB19056.Query_Start_Time)	 // 当前时间小于小的
        return 2;  //直接break
    else
        return  1; //当前时间大于大的，继续找，直到小于等于小的
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
    case  0x08:     //  采集记录仪指定的行驶速度记录
    case  0x09:    //  采集指定的指定位置信息记录
    case  0x10:   // 采集事故疑点记录
    case  0x11:    //  采集指定的超时驾驶记录
    case  0x12:    //  采集指定的驾驶人身份记录
    case  0x13:    // 采集指定的外部供电记录
    case  0x14:    //  采集指定的参数修改记录
    case  0x15:    // 采集指定的速度状态日志
        GB19056.Query_Start_Time = Time_sec_u32(instr + 6, 6);
        GB19056.Query_End_Time = Time_sec_u32(instr + 12, 6);
        GB19056.Max_dataBlocks = (instr[18] << 8) + instr[19];



    case  0x00:   //采集记录仪执行版本
    case  0x01:   //  采集驾驶人信息
    case  0x02:   // 采集记录仪时间
    case  0x03:   //  采集累计行驶里程
    case  0x04:   // 采集记录仪脉冲系数
    case  0x05:   // 采集车辆信息
    case  0x06:    // 采集记录仪信号配置信息
    case  0x07:    //  采集记录仪唯一性编号
    case  0xFE:    // ALL
    case  0xFB:	   //  Recommond
        DF_Sem_Take;
        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        DF_Sem_Release;
        break;
        //  设置记录仪相关
    case 0x82: //	  中心设置车牌号
        memset(jt808_param.id_0xF005, 0, sizeof(jt808_param.id_0xF005));
        memset(jt808_param.id_0x0083, 0, sizeof(jt808_param.id_0x0083));
        memset(jt808_param.id_0xF00A, 0, sizeof(jt808_param.id_0xF00A));

        //-----------------------------------------------------------------------
        memcpy(jt808_param.id_0xF005, instr + 6, 17);
        memcpy(jt808_param.id_0x0083, instr + 23, 12);
        memcpy(jt808_param.id_0xF00A, instr + 25, 12);
        //  存储
        param_save( 1 );

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;

    case 0xC2: //设置记录仪时钟
        // 没啥用，给个回复就行，俺有GPS校准就够了
        //  年月日 时分秒 BCD
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


    case 0xC3: //车辆速度脉冲系数（特征系数）
        // 前6 个是当前时间
        year = (instr[6] >> 4) * 10 + (instr[6] & 0x0F);
        month = (instr[7] >> 4) * 10 + (instr[7] & 0x0F);
        day = (instr[8] >> 4) * 10 + (instr[8] & 0x0F);
        hour = (instr[9] >> 4) * 10 + (instr[9] & 0x0F);
        min = (instr[10] >> 4) * 10 + (instr[10] & 0x0F);
        sec = (instr[11] >> 4) * 10 + (instr[11] & 0x0F);

        //  update  RTC
        date_set(year, month, day);
        time_set(hour, min, sec);

        jt808_param.id_0xF032 = (instr[12] << 8) + (u32)instr[13]; // 特征系数  速度脉冲系数

        //  存储
        param_save( 1 );

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;
    case 0x83: //  记录仪初次安装时间

        memcpy(temp_buffer, instr + 6, 6);
        jt808_param.id_0xF030 = mytime_from_bcd(temp_buffer);
        //  存储
        param_save( 1 );

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;
    case 0x84: // 设置信号量配置信息
        //memcpy(Setting08,instr+6,80);
        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;

    case 0xC4: //   设置初始里程

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
        // instr     + 12      设置初始里程的位置   BCD

        reg_dis = (instr[18] >> 4) * 10000000 + (instr[18] & 0x0F) * 1000000 + (instr[19] >> 4) * 100000 + (instr[19] & 0x0F) * 10000 \
                  +(instr[20] >> 4) * 1000 + (instr[20] & 0x0F) * 100 + (instr[21] >> 4) * 10 + (instr[21] & 0x0F);


        jt808_data.id_0xFA01 = reg_dis * 100;
        //  存储
        param_save( 1 );
        data_save();

        GB_OUT_data(instr[2], GB_OUT_TYPE_Serial, 1, 1);
        VDR_product_14H(instr[2]);
        break;
    case 0xE0:   //pMenuItem=&Menu_8_statechecking;  //  正式出货没有检定状态
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

    case  0xFF:    //还原显示状态
        GB19056.workstate = 0;
        printf_on = 1;	// 使能答应
        rt_kprintf(" GB_Data_disable\r\n");
        break;
    default:
        break;




    }
}

void  gb_usb_out(u8 ID)
{
    /*
               导出信息到USB device
      */
    if(USB_Disk_RunStatus() == USB_FIND)
    {
        if(GB19056.usb_exacute_output == 2)
        {

            if(GB19056.workstate == 0)
                rt_kprintf("\r\n GB USB 数据导出中 \r\n");
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
                    rt_kprintf("\r\n 导出ID 有误!  idle:  FF    0x00~0x15     0xFE :out put all        0xFB : output  recommond info   \r\n");
                return;


            }
            GB19056.usb_write_step = GB_USB_OUT_start;
            GB19056.usb_exacute_output = 1;
            GB19056.usb_out_selectID = ID;


            if(GB19056.workstate == 0)
                rt_kprintf("\r\n USB 数据导出\r\n");
        }
    }
    else
    {
        if(GB19056.workstate == 0)
            rt_kprintf("\r\n 没有检测到 USB device \r\n");
    }

}
FINSH_FUNCTION_EXPORT(gb_usb_out, gb_usb_out(ID) idle:  FF    0x00~0x15     0xFE : out put all        0xFB : output  recommond info   );
/*
    ( 命令: CMD,
      描述: u8*discrip_str,
      当前存储记录数目:u16 Ownindex_num,
      单挑记录的大小u16 indexSize)
*/
void GB_USBfile_stuff(u8 CMD, u8 *discrip_str, u16 Ownindex_num, u16 indexSize)
{
    u8  discr_str_local[18];
    u16  i = 0, pkg_counter = 0, get_indexnum = 0;
    u16  pakge_num = 0;
    u16 distr_len = strlen(discrip_str);
    u32  content_len = 0; // 填充信息长度
    u32  regdis 			= 0, reg2 = 0;
    u8  pos_V = 0;
    u8   add_fcs_byte = 0;

    memset(discr_str_local, 0, sizeof(discr_str_local));
    memcpy(discr_str_local, discrip_str, distr_len);
    usb_outbuf_Wr = 0; // star from here
    if(CMD == 0x00)
    {
        // 第一包 ，数据块个数
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
                pos_V++;   // 位置无效就累加
        }
        if(pos_V != 60)
        {
            content_len = indexSize * (Ownindex_num + 1); // 09 有一个是当前的
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

    case 0x08:	    //  写完数据头
        USB_OUT;
        // 写纯内容
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  每条记录写一次
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_08H_Write >= (pkg_counter + 1)) //没存满
                get_indexnum = Vdr_Wr_Rd_Offset.V_08H_Write - 1 - pkg_counter;
            else
                get_indexnum = VDR_08_MAXindex + Vdr_Wr_Rd_Offset.V_08H_Write - 1 - pkg_counter;

            gb_vdr_get_08h(get_indexnum, usb_outbuf + usb_outbuf_Wr);
            usb_outbuf_Wr += indexSize;
            USB_OUT;
        }
        break;
    case 0x09:
        //  写完数据头
        USB_OUT;
        // 写纯内容
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  每条记录写一次
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_09H_Write >= pkg_counter) //没存满
                get_indexnum = Vdr_Wr_Rd_Offset.V_09H_Write - pkg_counter;
            else
                get_indexnum = VDR_09_MAXindex + Vdr_Wr_Rd_Offset.V_09H_Write - pkg_counter;
            //------------------------------------------------------------------------------
            if(pkg_counter == 0)
            {
                // 当前小时的数据
                memcpy(VdrData.H_09, gb_BCD_datetime, 6);
                VdrData.H_09[4] = 0;
                VdrData.H_09[5] = 0;

                pos_V = 0;
                for(i = 0; i < 60; i++)
                {
                    if((VdrData.H_09[6 + i * 11] == 0x7F) && (VdrData.H_09[6 + i * 11 + 4] == 0x7F))
                        pos_V++;   // 位置无效就累加
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
        // 写纯内容
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  每条记录写一次
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_10H_Write >= (pkg_counter + 1)) //没存满
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
        // 写纯内容
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  每条记录写一次
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_11H_Write >= (pkg_counter + 1)) //没存满
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
        // 写纯内容
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  每条记录写一次
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_12H_Write >= (pkg_counter + 1)) //没存满
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
        // 写纯内容
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  每条记录写一次
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_13H_Write >= (pkg_counter + 1)) //没存满
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
        // 写纯内容
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  每条记录写一次
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_14H_Write >= (pkg_counter + 1)) //没存满
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
        // 写纯内容
        if(pakge_num == 0)
        {
            add_fcs_byte = 1;
            USB_OUT;
            break;
        }
        for(pkg_counter = 0; pkg_counter < pakge_num; pkg_counter++)
        {
            //  每条记录写一次
            usb_outbuf_Wr = 0;
            if(Vdr_Wr_Rd_Offset.V_15H_Write >= (pkg_counter + 1)) //没存满
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
    u16   get_indexnum = 0; // 获取校验
    //u16  Sd_DataBloknum=0; //   发送数据块  数目 <GB19056.Max_dataBlocks
    u8   Not_null_flag = 0; // NO  data ACK null once
    u8   return_value = 0;
    u32  Rd_Time = 0;

    switch(ID)
    {
    case 0x00:  //  执行标准版本年号
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "执行标准版本年号", 1, 2);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;
    case 0x01:

        //---------------- 上传数类型  -------------------------------------
        //	01	当前驾驶员代码及对应的机动车驾驶证号
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "当前驾驶人信息", 1, 18);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;
    case 0x02: //  02  采集记录仪的实时时钟
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "实时时间", 1, 6);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;

    case 0x03:		// 03 采集360h内里程
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "累计行驶里程", 1, 20);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;												 // 03 采集360h内里程
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;

    case 0x04:																	// 04  采集记录仪脉冲系数
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "脉冲系数", 1, 8);
            return;
        }
        //------------------------------------------------------------

        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;
    case 0x05:																	//05	  车辆信息采集
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "车辆信息", 1, 41);
            return;
        }
        //------------------------------------------------------------

        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);

        break;
    case   0x06:													// 06-09
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "状态信号配置信息", 1, 87);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;

    case 0x07:																//07  记录仪唯一编号
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "记录仪唯一性编号", 1, 35);
            return;
        }
        //------------------------------------------------------------
        usb_outbuf_Wr = 0;
        usb_outbuf_Wr += GB_557A_protocol_00_07_stuff(usb_outbuf, ID);
        break;
    case 0x08:															//	08	 采集指定的行驶速度记录
        //  获取当前总包数
        if(Vdr_Wr_Rd_Offset.V_08H_full)
        {
            GB19056.Total_dataBlocks = VDR_08_MAXindex;
        }
        else
            GB19056.Total_dataBlocks = Vdr_Wr_Rd_Offset.V_08H_Write;
        //   USB
        if(type == GB_OUT_TYPE_USB)
        {
            GB_USBfile_stuff(ID, "行驶速度记录", GB19056.Total_dataBlocks, 126);
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
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// 起始头
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x08;					//命令字


            SregLen = GB19056.Max_dataBlocks * 126; //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// 保留字
            //	信息内容
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < GB19056.Total_dataBlocks)
            {

                //---------------------------------------------------------------------
                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {
                    if(Vdr_Wr_Rd_Offset.V_08H_Write >= (subi + 1)) //没存满
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
                if(continue_counter != GB19056.Max_dataBlocks) // 如果信息不为空，则发送
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 126; // 把没有用的信息去掉
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


    case   0x09:															// 09	指定的位置信息记录
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
            GB_USBfile_stuff(ID, "位置信息记录", GB19056.Total_dataBlocks, 666);
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
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// 起始头
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x09;					//命令字


            SregLen = 666;  //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// 保留字
            //	信息内容
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < (GB19056.Total_dataBlocks + 1))
            {

                for(subi = GB19056.Send_add; subi < (GB19056.Total_dataBlocks + 1); subi++)
                {
                    if(subi == 0)
                    {
                        // 当前小时的数据
                        memcpy(VdrData.H_09, gb_BCD_datetime, 6);
                        VdrData.H_09[4] = 0;
                        VdrData.H_09[5] = 0;

                        memcpy(usb_outbuf + usb_outbuf_Wr, VdrData.H_09, 666);
                    }
                    else
                    {
                        if(Vdr_Wr_Rd_Offset.V_09H_Write >= subi) //没存满
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
                if(continue_counter == 0) // 如果信息不为空，则发送
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 666; // 把没有用的信息去掉
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
    case   0x10:															// 10-13	 10   事故疑点采集记录
        //事故疑点数据   ------    事故疑点状态是倒序排列的
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
            GB_USBfile_stuff(ID, "事故疑点记录", GB19056.Total_dataBlocks, 234);
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
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// 起始头
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x10;					//命令字


            SregLen = GB19056.Max_dataBlocks * 234; //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// 保留字
            //	信息内容
            Not_null_flag = 0;
            continue_counter = 0;


            if(GB19056.Send_add < (GB19056.Total_dataBlocks))
            {

                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_10H_Write >= (subi + 1)) //没存满
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
                if(continue_counter != GB19056.Max_dataBlocks) // 如果信息不为空，则发送
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 234; // 把没有用的信息去掉
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


    case  0x11: 															// 11 采集指定的的超时驾驶记录
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
            GB_USBfile_stuff(ID, "超时驾驶记录", GB19056.Total_dataBlocks, 50);
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
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// 起始头
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x11;					//命令字

            if(VdrData.H_11_lastSave)
                SregLen = GB19056.Max_dataBlocks * 50;
            else
                SregLen = 50;  //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// 保留字
            //	信息内容
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
                        if(Vdr_Wr_Rd_Offset.V_11H_Write >= (subi + 1)) //没存满
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
                if(continue_counter != GB19056.Max_dataBlocks) // 如果信息不为空，则发送
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 50; // 把没有用的信息去掉
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
    case  0x12: 															// 12 采集指定驾驶人身份记录  ---Devide
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
            GB_USBfile_stuff(ID, "驾驶人身份记录", GB19056.Total_dataBlocks, 25);
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
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					 // 起始头
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x12;					 //命令字

            SregLen = GB19056.Max_dataBlocks * 25;	//

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);		 // Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				 // Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00;					 // 保留字
            //  信息内容
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < (GB19056.Total_dataBlocks))
            {

                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_12H_Write >= (subi + 1)) //没存满
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
                if(continue_counter != GB19056.Max_dataBlocks) // 如果信息不为空，则发送
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 25; // 把没有用的信息去掉
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


    case  0x13: 													// 13 采集记录仪外部供电记录
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
            GB_USBfile_stuff(ID, "外部供电记录", GB19056.Total_dataBlocks, 7);
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
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// 起始头
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x13;					//命令字


            SregLen = GB19056.Max_dataBlocks * 7; //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// 保留字
            //	信息内容
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < (GB19056.Total_dataBlocks))
            {

                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_13H_Write >= (subi + 1)) //没存满
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
                if(continue_counter != GB19056.Max_dataBlocks) // 如果信息不为空，则发送
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 7; // 把没有用的信息去掉
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
    case   0x14:   //   参数修改记录
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
            GB_USBfile_stuff(ID, "参数修改记录", GB19056.Total_dataBlocks, 7);
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
            usb_outbuf[usb_outbuf_Wr++]   = 0x55;					// 起始头
            usb_outbuf[usb_outbuf_Wr++]   = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]   = 0x14;					//命令字


            SregLen = GB19056.Max_dataBlocks * 7; //

            usb_outbuf[usb_outbuf_Wr++]   = (SregLen >> 8);			// Hi
            usb_outbuf[usb_outbuf_Wr++]   = SregLen;				// Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00; 					// 保留字
            //	信息内容
            Not_null_flag = 0;
            continue_counter = 0;


            if(GB19056.Send_add < GB19056.Total_dataBlocks)
            {
                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_14H_Write >= (subi + 1)) //没存满
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
                if(continue_counter != GB19056.Max_dataBlocks) // 如果信息不为空，则发送
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 7; // 把没有用的信息去掉
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
    case	 0x15:														// 15 采集指定的速度状态日志	 --------Divde
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
            GB_USBfile_stuff(ID, "速度状态日志", GB19056.Total_dataBlocks, 133);
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
            usb_outbuf[usb_outbuf_Wr++]	 = 0x55;				   // 起始头
            usb_outbuf[usb_outbuf_Wr++]	 = 0x7A;
            usb_outbuf[usb_outbuf_Wr++]	 = 0x15;				   //命令字


            SregLen = 133;	 //

            usb_outbuf[usb_outbuf_Wr++]	 = (SregLen >> 8);		   // Hi
            usb_outbuf[usb_outbuf_Wr++]	 = SregLen; 			   // Lo

            usb_outbuf[usb_outbuf_Wr++] = 0x00;					   // 保留字
            //  信息内容
            Not_null_flag = 0;
            continue_counter = 0;

            if(GB19056.Send_add < (GB19056.Total_dataBlocks))
            {

                for(subi = GB19056.Send_add; subi < GB19056.Total_dataBlocks; subi++)
                {

                    if(Vdr_Wr_Rd_Offset.V_15H_Write >= (subi + 1)) //没存满
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
                if(continue_counter == 0)	// 如果信息不为空，则发送
                {
                    // ---caculate  infolen
                    SregLen = continue_counter * 133; // 把没有用的信息去掉
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
    case 0xE1:  // 里程误差测量
        usb_outbuf_Wr = 0;
        usb_outbuf[usb_outbuf_Wr++]   = 0x55;				// 起始头
        usb_outbuf[usb_outbuf_Wr++]   = 0x7A;

        usb_outbuf[usb_outbuf_Wr++] = 0xE1; 				//命令字

        usb_outbuf[usb_outbuf_Wr++] = 0x00;              // 长度
        usb_outbuf[usb_outbuf_Wr++] = 44;


        usb_outbuf[usb_outbuf_Wr++] = 0x00;					  // 保留字
        //------- 信息内容 ------
        // 0.    35  个字节唯一编号
        usb_outbuf_Wr += GB_00_07_info_only(usb_outbuf + usb_outbuf_Wr, 0x07);
        //  1.  脉冲系数
        usb_outbuf[usb_outbuf_Wr++]   = (jt808_param.id_0xF032 >> 8);
        usb_outbuf[usb_outbuf_Wr++]   = (u8)jt808_param.id_0xF032;
        //   2.  速度
        usb_outbuf[usb_outbuf_Wr++]   = (car_data.get_speed >> 8);
        usb_outbuf[usb_outbuf_Wr++]   = (u8)car_data.get_speed;
        //   3. 累计里程
        usb_outbuf[usb_outbuf_Wr++]   = (jt808_data.id_0xFA01 >> 24);
        usb_outbuf[usb_outbuf_Wr++]   = (jt808_data.id_0xFA01 >> 16);
        usb_outbuf[usb_outbuf_Wr++]   = (jt808_data.id_0xFA01 >> 8);
        usb_outbuf[usb_outbuf_Wr++]   = jt808_data.id_0xFA01;
        //    4.  状态第 1个 字节
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


/*记录仪数据交互状态*/

void Virtual_thread_of_GB19056( void)
{
    //   Application .c

    u8 str_12len[15];
    /*定义一个函数指针，用作结果处理	*/

    rt_err_t res;


    // part1:  serial
    if (rt_sem_take(&GB_RX_sem, 20) == RT_EOK)
    {
        // 0  debug out
        //OutPrint_HEX("记录接收",GB19056.rx_buf,GB19056.rx_wr);
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
                //  创建文件
                memset(GB19056.Usbfilename, 0, sizeof(GB19056.Usbfilename));
                sprintf((char *)GB19056.Usbfilename, "1:D%c%c%c%c%c%c_%c%c%c%c_%s.VDR", (YEAR(mytime_now) / 10 + 0x30), (YEAR(mytime_now) % 10 + 0x30), (MONTH(mytime_now) / 10 + 0x30), (MONTH(mytime_now) % 10 + 0x30), \
                        (DAY(mytime_now) / 10 + 0x30), (DAY(mytime_now) % 10 + 0x30), (HOUR(mytime_now) / 10 + 0x30), (HOUR(mytime_now) % 10 + 0x30), (MINUTE(mytime_now) / 10 + 0x30), (MINUTE(mytime_now) % 10 + 0x30), jt808_param.id_0x0083);

                res = f_open(&file, GB19056.Usbfilename, FA_READ | FA_WRITE | FA_OPEN_ALWAYS );


                if(GB19056.workstate == 0)
                    rt_kprintf(" \r\n udiskfile: %s  create res=%d	 \r\n", GB19056.Usbfilename, res);
                if(res == 0)
                {

                    if(GB19056.workstate == 0)
                        rt_kprintf("\r\n			  创建Drv名称: %s \r\n ", GB19056.Usbfilename);

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
        //------  显示屏输出 update  ------------------------------
#if  1
        debug_write("USB所有数据导出完成!");
        usb_export_status = 66;		///0,表示没有检测到U盘，无法导出；1表示正常导出数据，66表示导出成功

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
        printf_on = 1; // 使能答应
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

//   执行存储记录仪相关数据任务
void  GB19056_VDR_Save_Process(void)
{
    //  放在    rt_thread_entry_gps

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
        Vdr_Wr_Rd_Offset.V_08H_Write++; //  写完之后累加，就不用遍历了
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
        Vdr_Wr_Rd_Offset.V_09H_Write++; // //  写完之后累加，就不用遍历了
        vdr_cmd_writeIndex_save(0x09, Vdr_Wr_Rd_Offset.V_09H_Write);
        DF_Sem_Release;
        memset(VdrData.H_09 + 6, 0x0, 660);	 // 默认是 0xFF
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
        Vdr_Wr_Rd_Offset.V_10H_Write++; // //  写完之后累加，就不用遍历了
        vdr_cmd_writeIndex_save(0x10, Vdr_Wr_Rd_Offset.V_10H_Write);
        DF_Sem_Release;
        VdrData.H_10_saveFlag = 0;
        return;
    }
    //  4.  VDR  11H  Data  Save
    if(VdrData.H_11_saveFlag)  //  1 : 存储递增    2: 存储不递增
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
            Vdr_Wr_Rd_Offset.V_11H_Write++; // //  写完之后累加，就不用遍历了

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
        Vdr_Wr_Rd_Offset.V_12H_Write++; // //  写完之后累加，就不用遍历了
        vdr_cmd_writeIndex_save(0x12, Vdr_Wr_Rd_Offset.V_12H_Write);
        DF_Sem_Release;
        VdrData.H_12_saveFlag = 0;

        // rt_kprintf("\r\n 写IC 卡  记录  \r\n");
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
        Vdr_Wr_Rd_Offset.V_13H_Write++; // //  写完之后累加，就不用遍历了
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
        Vdr_Wr_Rd_Offset.V_14H_Write++; // //  写完之后累加，就不用遍历了
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
        Vdr_Wr_Rd_Offset.V_15H_Write++; // //  写完之后累加，就不用遍历了
        vdr_cmd_writeIndex_save(0x15, Vdr_Wr_Rd_Offset.V_15H_Write);
        DF_Sem_Release;
        VdrData.H_15_saveFlag = 0;
    }
    //-----------------  超速报警 ----------------------
    /*	if(speed_Exd.excd_status==2)
    	{
    	   Spd_Exp_Wr();
    	   return;
    	}
    */
    //	 定时存储里程--xiaobai  已经有里程统计了
    /*
    if((Vehicle_RunStatus==0x01)&&(DistanceWT_Flag==1))
    {   //  如果车辆在行驶过程中，每255 秒存储一次里程数据
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



//    四   GB   IC   相关
void  GB_IC_info_default(void)
{
    memcpy(Read_ICinfo_Reg.DriverCard_ID, "000000000000000000", 18);
    memset(Read_ICinfo_Reg.Effective_Date, 0, sizeof(Read_ICinfo_Reg.Effective_Date));
    Read_ICinfo_Reg.Effective_Date[0] = 0x20;
    Read_ICinfo_Reg.Effective_Date[1] = 0x08;
    Read_ICinfo_Reg.Effective_Date[2] = 0x08;
    memcpy(Read_ICinfo_Reg.Drv_CareerID, "000000000000000000", 18);
}
//   判断多个驾驶员，插卡后信息的更新
void  GB_IC_Different_DriverIC_InfoUpdate(void)
{
    u8  i = 0, compare = 0; // compare 是否有匹配的
    s8  cmp_res = 0; // 匹配为0
    u8  selected_num = 0; // 表示第几个被选择了

    //  0 .    先遍历是否和既有的匹配
    for(i = 0; i < MAX_DriverIC_num; i++)
    {
        if(Drivers_struct[i].Working_state)
        {
            // check compare
            cmp_res = memcmp(Drivers_struct[i].Driver_BASEinfo.DriverCard_ID, Read_ICinfo_Reg.DriverCard_ID, 18);
            if(cmp_res == 0) // 匹配
            {

                Drivers_struct[i].Working_state = 2;
                selected_num = MAX_DriverIC_num + 2; //   表完全匹配
                compare = 1; //enable
                if(GB19056.workstate == 0)
                    rt_kprintf("\r\n  有完全匹配的 i=%d\r\n", i);
            }
            else
            {
                if(Drivers_struct[i].Working_state == 2)
                {
                    Drivers_struct[i].Working_state = 1;
                    selected_num = i; //  记录以前为2 的 下标
                    if(GB19056.workstate == 0)
                        rt_kprintf("\r\n  上次插过卡 2bian1  i=%d\r\n", i);
                }
            }
        }
    }

    if(compare)         //  找到匹配的了，且更新状态了
        return;
    //--------------------------------------------------------------
    // 1. 遍历驾驶员状态，找出当前要用的位置
    for(i = 0; i < MAX_DriverIC_num; i++)
    {
        // 如果有当前的，那么把它当前状态转换成激活状态

        //    状态为2 的下一个即为 当前位置
        if((Drivers_struct[i].Working_state == 1) && (selected_num == 0x0F))
        {
            selected_num = i;
            break;
        }
        if(Drivers_struct[i].Working_state == 1)
        {
            if(selected_num == i) // 判断当前记录是否是上次为2 的记录
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

    if(i >= MAX_DriverIC_num) //   全满了，且最后一个是2，那么第一个就是当前新的
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
        rt_kprintf("\r\n  更新新内容 i=%d  ID=%s\r\n", selected_num, Drivers_struct[selected_num].Driver_BASEinfo.DriverCard_ID);


        rt_kprintf("\r\n  1--更新新内容 i=%d  ID=%s\r\n", 0, Drivers_struct[0].Driver_BASEinfo.DriverCard_ID);
        rt_kprintf("\r\n  2--更新新内容 i=%d  ID=%s\r\n", 1, Drivers_struct[1].Driver_BASEinfo.DriverCard_ID);
        rt_kprintf("\r\n  3--更新新内容 i=%d  ID=%s\r\n", 2, Drivers_struct[2].Driver_BASEinfo.DriverCard_ID);
    }
}



//  判断 不同驾驶员 ,连续驾驶的开始时间
void  GB_IC_Different_DriverIC_Start_Process(void)
{
    u8  i = 0;
    // 判断各个驾驶员的连续驾驶的开始时间
    if((current_speed > 60) && (Sps_larger_5_counter > 10))
    {
        for(i = 0; i < MAX_DriverIC_num; i++)
        {
            if((Drivers_struct[i].Working_state == 2) && (Drivers_struct[i].Running_counter == 0))
            {
                memcpy(Drivers_struct[i].Start_Datetime, gb_BCD_datetime, 6);
                //   3.  起始位置
                memcpy(Drivers_struct[i].Longi, VdrData.Longi, 4); // 经度
                memcpy(Drivers_struct[i].Lati, VdrData.Lati, 4); //纬度
                Drivers_struct[i].Hight = BYTESWAP2( gps_baseinfo.altitude);
                Drivers_struct[i].H_11_start = 1; //  start
                Drivers_struct[i].H_11_lastSave_state = 0;
            }
        }
    }

}

//  判断不同驾驶员，连续驾驶的结束时间 // 速度为0  时候执行
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

            //  1.   机动车驾驶证号
            memcpy(VdrData.H_11, Drivers_struct[i].Driver_BASEinfo.DriverCard_ID, 18);
            if(GB19056.workstate == 0)
                rt_kprintf("\r\n    drivernum=%d drivercode-=%s\r\n", i, Drivers_struct[i].Driver_BASEinfo.DriverCard_ID);
            //   2.   起始时间
            memcpy(VdrData.H_11 + 18, Drivers_struct[i].Start_Datetime, 6); // 起始时间
            memcpy(Drivers_struct[i].End_Datetime, gb_BCD_datetime, 6);
            memcpy(VdrData.H_11 + 24, Drivers_struct[i].End_Datetime, 6); // 结束时间
            //   3.  起始位置
            memcpy( VdrData.H_11 + 30, Drivers_struct[i].Longi, 4); // 经度
            memcpy( VdrData.H_11 + 30 + 4, Drivers_struct[i].Lati, 4); //纬度
            VdrData.H_11[30 + 8] = (Drivers_struct[i].Hight >> 8);
            VdrData.H_11[30 + 9] = Drivers_struct[i].Hight;
            //VdrData.H_11_start=1; //  start
            //VdrData.H_11_lastSave=0;

            //   4.   结束位置
            memcpy( VdrData.H_11 + 40, VdrData.Longi, 4); // 经度
            memcpy( VdrData.H_11 + 40 + 4, VdrData.Lati, 4); //纬度
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

//  判断不同驾驶员 疲劳状态
void  GB_IC_Different_DriverIC_Checking(void)
{
    u8 i = 0;

    if((current_speed > 60 ) && (Sps_larger_5_counter > 10))	// current_speed 单位为0.1 km/h  速度大于6 km/h  认为是行驶
    {
        // 行驶状态
        for(i = 0; i < MAX_DriverIC_num; i++)
        {
            if( Drivers_struct[i].Working_state == 2)
            {
                Drivers_struct[i].Running_counter++;

                //       提前30 min  语音提示
                if(jt808_param.id_0x0057 > 1800)
                {
                    if(Drivers_struct[i].Running_counter == ( jt808_param.id_0x0057 - 1800 ) ) //提前5分钟蜂鸣器提示注意疲劳驾驶 14100
                    {
                        //   疲劳报警预提示 开始
                        if(GB19056.SPK_PreTired.Warn_state_Enable == 0)
                            GB19056.SPK_PreTired.Warn_state_Enable = 1;
                    }
                }
                //--   判断疲劳驾驶--------------------------------------------------------------------
                if(  Drivers_struct[i].Running_counter == (jt808_param.id_0x0057) )//14400
                {
                    if(GB19056.workstate == 0)
                        rt_kprintf( "\r\n   疲劳驾驶触发了!  on\r\n" );
                    //  TTS_play( "您已经疲劳驾驶，请注意休息" );


                    //	超时驾驶 警示触发了
                    if(GB19056.SPK_DriveExceed_Warn.Warn_state_Enable == 0)
                        GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 1;
                    //	疲劳报警预提示 结束
                    GB19056.SPK_PreTired.Warn_state_Enable = 0;



                    //---- 触发即时上报数据	  ，记录仪产生疲劳只记录不上报-------
                    //-------------------------------------
                }
                //----------------------------------------------------------------------------------------
                Drivers_struct[i].Stopping_counter = 0;
            }
            else if(Drivers_struct[i].Working_state == 1) //  状态为1  也算休息
            {
                Drivers_struct[i].Stopping_counter++; // 状态为1 的车算休息

                if(Drivers_struct[i].Stopping_counter >= jt808_param.id_0x0059 )	//1200	 // ACC 关20分钟视为休息
                {
                    if(Drivers_struct[i].Stopping_counter == jt808_param.id_0x0059 )
                    {
                        Drivers_struct[i].Stopping_counter = 0;
                        Drivers_struct[i].Running_counter = 0;

                        // 	只有触发过疲劳驾驶的且开始时间赋过数值才会存储判断
                        if((Drivers_struct[i].Running_counter >= jt808_param.id_0x0057) && \
                                (Drivers_struct[i].H_11_start == 2));
                        {
                            Drivers_struct[i].Working_state = 0; //
                            //---- 触发即时上报数据    ，记录仪产生疲劳只记录不上报-------

                            //	只有不为当前才清除，当前为2 不清除
                            if(Drivers_struct[i].Working_state == 1)
                                memset((u8 *)&Drivers_struct[i], 0, sizeof(DRIVE_STRUCT)); // clear


                            //  超时驾驶结束
                            GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 0;
                            if(GB19056.workstate == 0)
                                rt_kprintf( "\r\n	i=%d 您的疲劳驾驶报警已经解除-on 1 \r\n", i);
                        }
                    }
                }
            }

        }

    }
    else
    {
        // 停止状态
        for(i = 0; i < MAX_DriverIC_num; i++)
        {
            if( Drivers_struct[i].Working_state)
            {
                if(Drivers_struct[i].Running_counter)
                {
                    //-- ACC 没有休息前还算AccON 的状态  ---------------
                    Drivers_struct[i].Running_counter++;

                    //---------------------------------------------------------------
                    //       提前30 min  语音提示
                    if(jt808_param.id_0x0057 > 1800)
                    {
                        if(Drivers_struct[i].Running_counter == ( jt808_param.id_0x0057 - 1800 ) ) //提前5分钟蜂鸣器提示注意疲劳驾驶 14100
                        {
                            //   疲劳报警预提示 开始
                            if(GB19056.SPK_PreTired.Warn_state_Enable == 0)
                                GB19056.SPK_PreTired.Warn_state_Enable = 1;
                        }
                    }


                    //      ACC off    but     Acc on  conintue  run
                    if( Drivers_struct[i].Running_counter >= ( jt808_param.id_0x0057 ) )       //14400 // 连续驾驶超过4小时算疲劳驾驶
                    {
                        if(Drivers_struct[i].Running_counter == ( jt808_param.id_0x0057  ) )  //14400
                        {
                            if(GB19056.workstate == 0)
                                rt_kprintf( "\r\n	 速度小，但未满足休息门限时间 疲劳驾驶触发了! \r\n");
                            //TTS_play( "您已经疲劳驾驶，请注意休息" );

                            //	 超时驾驶 触发了
                            if(GB19056.SPK_DriveExceed_Warn.Warn_state_Enable == 0)
                                GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 1;
                            //   疲劳报警预提示 结束
                            GB19056.SPK_PreTired.Warn_state_Enable = 0;

                            //---- 触发即时上报数据    ，记录仪产生疲劳只记录不上报-------
                        }
                    }


                    //     ACC  off   counter
                    Drivers_struct[i].Stopping_counter++;
                    if(Drivers_struct[i].Stopping_counter >= jt808_param.id_0x0059 ) //1200	// ACC 关20分钟视为休息
                    {
                        if(Drivers_struct[i].Stopping_counter == jt808_param.id_0x0059 )
                        {
                            Drivers_struct[i].Stopping_counter = 0;
                            Drivers_struct[i].Running_counter = 0;

                            //     只有触发过疲劳驾驶的且开始时间赋过数值才会存储判断
                            if((Drivers_struct[i].Running_counter >= jt808_param.id_0x0057) && \
                                    (Drivers_struct[i].H_11_start == 2));
                            {
                                Drivers_struct[i].Working_state = 0; //

                                //---- 触发即时上报数据    ，记录仪产生疲劳只记录不上报-------
                                //-------------------------------------
                                //    超时结束  存储，索引累加
                                //  1.   机动车驾驶证号
                                memcpy(VdrData.H_11, Drivers_struct[i].Driver_BASEinfo.DriverCard_ID, 18);
                                //   2.   起始时间
                                memcpy(VdrData.H_11 + 18, Drivers_struct[i].Start_Datetime, 6); // 起始时间
                                memcpy(VdrData.H_11 + 24, Drivers_struct[i].End_Datetime, 6); // 结束时间
                                //   3.  起始位置
                                memcpy( VdrData.H_11 + 30, Drivers_struct[i].Longi, 4); // 经度
                                memcpy( VdrData.H_11 + 30 + 4, Drivers_struct[i].Lati, 4); //纬度
                                VdrData.H_11[30 + 8] = (Drivers_struct[i].Hight >> 8);
                                VdrData.H_11[30 + 9] = Drivers_struct[i].Hight;

                                //   4.   结束位置
                                memcpy( VdrData.H_11 + 40, VdrData.Longi, 4); // 经度
                                memcpy( VdrData.H_11 + 40 + 4, VdrData.Lati, 4); //纬度
                                VdrData.H_11[40 + 8] = (BYTESWAP2( gps_baseinfo.altitude) >> 8);
                                VdrData.H_11[40 + 9] = BYTESWAP2( gps_baseinfo.altitude);

                                if((Drivers_struct[i].End_Datetime[0] == 0x00) && (Drivers_struct[i].End_Datetime[1] == 0x00) && (Drivers_struct[i].End_Datetime[2] == 0x00))
                                    ; // 年月日 为0  过滤
                                else
                                    VDR_product_11H_End(1);

                                //  只有不为当前才清除，当前为2 不清除
                                if(Drivers_struct[i].Working_state == 1)
                                    memset((u8 *)&Drivers_struct[i], 0, sizeof(DRIVE_STRUCT)); // clear
                                else



                                    //  超时驾驶结束
                                    GB19056.SPK_DriveExceed_Warn.Warn_state_Enable = 0;
                                if(GB19056.workstate == 0)
                                    rt_kprintf( "\r\n	您的疲劳驾驶报警已经解除 \r\n");
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
    //  放到  CheckICCard   中                       12H

    u8 result0 = 0, result1 = 1, result2 = 2, result3 = 4, result4 = 5; //i=0;
    u8 regNum = 0, readbuf[32];
    u8  red_byte[128];
    u8  fcs = 0, i = 0;


    if((current_speed < 40) && (Sps_larger_5_counter == 0))		// 在行驶结束状态下才开始 判断插拔卡
    {

        memset(Read_ICinfo_Reg.DriverCard_ID, 0, sizeof(Read_ICinfo_Reg.DriverCard_ID));
        memcpy((unsigned char *)Read_ICinfo_Reg.DriverCard_ID, jt808_param.id_0xF009, 18);	//读驾驶证号码
        memset(Read_ICinfo_Reg.Effective_Date, 0, sizeof(Read_ICinfo_Reg.Effective_Date));
        memcpy((unsigned char *)Read_ICinfo_Reg.Effective_Date, ic_card_para.IC_Card_valitidy + 1, 3);	//有效日期
        memset(Read_ICinfo_Reg.Drv_CareerID, 0, sizeof(Read_ICinfo_Reg.Drv_CareerID));
        memcpy((unsigned char *)Read_ICinfo_Reg.Drv_CareerID, jt808_param.id_0xF00B, 18);	//从业资格证

        //-----------  停车插卡 且识别 -------
        GB_IC_Different_DriverIC_InfoUpdate();  // 更新IC 驾驶员信息
        gb_IC_loginState = 1;

        DF_Sem_Take;
        if(Vdr_Wr_Rd_Offset.V_12H_Write)
            regNum = Vdr_Wr_Rd_Offset.V_12H_Write - 1;
        else
            regNum = Vdr_Wr_Rd_Offset.V_12H_Write;
        gb_vdr_get_12h(regNum, readbuf);

        DF_Sem_Release;
        if(readbuf[24] == 0x02) //  之前有过登陆
        {
            VDR_product_12H(0x01);	//	登录
            tts("驾驶员登录"); // 第一次上电播读，但要记录

        }
        GB19056.SPK_UnloginWarn.Warn_state_Enable = 0; // clear
    }
}

void  GB_IC_Check_OUT(void)
{
    //  放到  CheckICCard   中           12H

    u8 regNum = 0, readbuf[32];

    if((current_speed < 40) && (gb_IC_loginState == 1)) 	 // 在行驶结束状态下才开始 判断插拔卡
    {
        DF_Sem_Take;
        if(Vdr_Wr_Rd_Offset.V_12H_Write)
            regNum = Vdr_Wr_Rd_Offset.V_12H_Write - 1;
        else
            regNum = Vdr_Wr_Rd_Offset.V_12H_Write;
        gb_vdr_get_12h(regNum, readbuf);
        DF_Sem_Release;
        if(readbuf[24] == 0x01) //  之前有过登陆
        {
            VDR_product_12H(0x02);  //  登出
            tts("驾驶员退出");

            gb_IC_loginState = 0;
        }

    }
}

void GB_IC_Exceed_Drive_Check( void )    //  超时驾驶检测
{
    //    1.   判断是否ACC  开
    if(!(jt808_status & BIT_STATUS_ACC))    //  acc  关情况下
        return;

    //	2.	判断行驶状态
    if((gps_speed >= 60) || (car_data.get_speed >= 60))	// 记录仪认证时候是这样的 1km/h
    {
        //	11 H   连续驾驶开始时间   和起始位置	 从60s 开始算驾驶起始
        if(VDR_TrigStatus.Running_state_enable == 0)
        {
            VdrData.H_08_counter = SEC(mytime_now); // time_now.sec; // start
        }
        VDR_TrigStatus.Running_state_enable = 1; //  处于行驶状态
    }
    if((gps_speed < 60) && (car_data.get_speed < 60))
    {
        //		  11 H	   相关    超过4 小时 且   速度从大降到 0  ，才触发超时记录
        if(VDR_TrigStatus.Running_state_enable == 1)
        {
            GB_IC_Different_DriverIC_End_Process();
        }
        VDR_TrigStatus.Running_state_enable = 0; //  处于停止状态
    }


    //   2.   驾驶超时判断
    // 2.1    H_11   Start    acc on 为 0 时候开始超时记录
    GB_IC_Different_DriverIC_Start_Process();
    // 2.2    acc on  counter     -------------疲劳驾驶相关 -----------------------
    GB_IC_Different_DriverIC_Checking();
    //--------------------------------------------------------------------------------
    //------------------------------------------------
}

void gb_100ms_timer(void)
{
    //   放到      cb_tmr_50ms     处
    GB_100ms_Timer_ISP();

    gb_time_counter++;
    if((gb_time_counter % 2) == 0)
    {
        GB_200ms_Hard_timer();
        gb_time_counter = 0;
    }

}


