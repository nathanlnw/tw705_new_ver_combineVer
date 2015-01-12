#ifndef   _GB_VDR_
#define   _GB_VDR_




#define  DF_Sem_Take     rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY )
#define  DF_Sem_Release  rt_sem_release(&sem_dataflash)

//  一.   vdr  所用DF  区域使用规划
/*
 Start Address offset :   0x1D0000
 Area Size :
					   213	Sector		 = 1704 pages
						----------------------

			 扇区
			 1									   00-07H
			 90 								   08H
			 85 								   09H
			 7										10H
			 2										11H
			 2										12H
			 1										13H
			 1										14H
			 1										15H

	   ----------  只是在这里做了---  注释 ，具体操作在 Vdr.C

#define ADDR_DF_VDR_BASE 			0x1D0000				///行驶记录仪VDR数据存储区域开始位置
#define ADDR_DF_VDR_END				0x2AC000				///行驶记录仪VDR数据存储区域结束位置
#define ADDR_DF_VDR_SECT			220						///多给分配了一些空间，实际需要213

   note:   我所用的区域大小为213 sector  ， 剩下7 个sector 用于存储 记录相关参数

*/



#define ADDR_DF_VDR_15minDATA           ADDR_DF_VDR_BASE+(213<<12)    //   停车前15分钟的平均速度
#define ADDR_DF_VDR_15minAddr           ADDR_DF_VDR_BASE+(214<<12)    //   停车前15分钟的平均速度















//   存储区域最大条数
#define  VDR_08_MAXindex         2880
#define  VDR_09_MAXindex         360
#define  VDR_10_MAXindex         100
#define  VDR_11_MAXindex         100
#define  VDR_12_MAXindex         200
#define  VDR_13_MAXindex         100
#define  VDR_14_MAXindex         100
#define  VDR_15_MAXindex         10








typedef  struct
{
    u16  V_08H_Write;	  // 08 H  位置
    u16  V_09H_Write;	  // 09H  位置
    u16  V_10H_Write;	  // 10 H  位置
    u16  V_11H_Write;   // 11 H  位置
    u16  V_12H_Write;   // 12 H	位置
    u16  V_13H_Write;   // 13H  位置
    u16  V_14H_Write;   // 14 H	位置
    u16  V_15H_Write;	  // 15 H 位置

    u16  V_08H_full;	  // 08 H  位置
    u16  V_09H_full;	  // 09H  位置
    u16  V_10H_full;	  // 10 H  位置
    u16  V_11H_full;   // 11 H  位置
    u16  V_12H_full;   // 12 H	位置
    u16  V_13H_full;   // 13H  位置
    u16  V_14H_full;   // 14 H	位置
    u16  V_15H_full;	  // 15 H 位置

    u8   V_08H_get; //  串口开始 获取
    u8   V_09H_get;
    u8   V_10H_get;
    u8   V_11H_get;
    u8   V_12H_get;
    u8   V_13H_get;
    u8   V_14H_get;
    u8   V_15H_get;

} VDR_INDEX;

extern VDR_INDEX      Vdr_Wr_Rd_Offset;



typedef struct
{
    //-----------------------------------
    u8  Lati[4];  //    纬度
    u8  Longi[4]; //    经度

    //-----------------------------------
    u8  H_08[126]; //   08H buf
    u8  H_08_counter; // 08 计数器 RTC 不准
    u8  H_08_BAK[126]; //   08H buf
    u8  H_08_saveFlag; //   存储标志位
    u8  H_09[666];//   09H  buf
    u8  H_09_saveFlag; //
    u32 H_09_spd_accumlate;  // 速度和
    u8  H_09_seconds_counter;// 速度
    u8  H_09_min_couner; // 每分钟
    u8  H_10[234]; //   10H buf
    u8  H_10_saveFlag;
    u8  H_11[50];//   11H  buf
    u8	H_11_start;	//	超时开始状态 记录 未开始是0  开始了为1
    u8  H_11_lastSave; // 再超时正式结束前，是否存储过记录   存储过为1: 未存储过为 0
    u8  H_11_saveFlag;
    u8	H_12[25]; //	12H buf
    u8  H_12_saveFlag;
    u8  H_13[7];//  13H  buf
    u8  H_13_saveFlag;
    u8  H_14[7]; //   14H buf
    u8  H_14_saveFlag;
    u8  H_15[133];//	 15H  buf
    u8  H_15_Set_StartDateTime[6];  // 中心设置的采集时间
    u8  H_15_Set_EndDateTime[6];  //  中心设置的采集结束时间
    u8  H_15_saveFlag;
} VDR_DATA;

extern VDR_DATA   VdrData;  //  行驶记录仪的databuf



typedef struct
{
    u16  Running_state_enable;   //工作状态
} VDR_TRIG_STATUS;
extern  VDR_TRIG_STATUS  VDR_TrigStatus;

//==================================================================================================
// 第五部分 :   以下是行车记录仪相关协议 即 附录A
//==================================================================================================
extern void OutPrint_HEX(u8 *Descrip, u8 *instr, u16 inlen );

extern void  total_ergotic(void);
extern void  gb_vdr_erase(void);

extern u16   stuff_drvData(u8 type, u16 Start_recNum, u16 REC_nums, u8 *dest);


extern void  VDR_product_08H_09H(void);
extern void  VDR_product_11H_Start(void);
extern void  VDR_product_11H_End(u8 value);
extern void  VDR_product_12H(u8  value);
extern void  VDR_product_13H(u8  value);
extern void  VDR_product_14H(u8 cmdID);
extern void  VDR_product_15H(u8  value);
extern void  VDR_get_15H_StartEnd_Time(u8  *Start, u8 *End);


extern u16 gb_vdr_get_08h( u16 indexnum, u8 *p);
extern u16 gb_vdr_get_09h( u16 indexnum, u8 *p);
extern u16 gb_vdr_get_10h( u16 indexnum, u8 *p);
extern u16 gb_vdr_get_11h( u16 indexnum, u8 *p);
extern u16 gb_vdr_get_12h( u16 indexnum, u8 *p);
extern u16 gb_vdr_get_13h( u16 indexnum, u8 *p);
extern u16 GB_VDR_READ_14H( u16 indexnum, u8 *p);
extern u16 gb_vdr_get_15h( u16 indexnum, u8 *p);





extern u16  vdr_creat_08h( u16 indexnum, u8 *p, u16 inLen);
extern u16  vdr_creat_09h( u16 indexnum, u8 *p, u16 inLen) ;
extern u16  vdr_creat_10h( u16 indexnum, u8 *p, u16 inLen) ;
extern u16  vdr_creat_11h( u16 indexnum, u8 *p, u16 inLen) ;
extern u16  vdr_creat_12h( u16 indexnum, u8 *p, u16 inLen) ;
extern u16  vdr_creat_13h( u16 indexnum, u8 *p, u16 inLen) ;
extern u16  vdr_creat_14h( u16 indexnum, u8 *p, u16 inLen) ;
extern u16  vdr_creat_15h( u16 indexnum, u8 *p, u16 inLen) ;
extern u8   vdr_cmd_writeIndex_save(u8 CMDType, u32 value);
extern u32  vdr_cmd_writeIndex_read(u8 CMDType);


extern void gb_index_write(u8 cmd, u32 value);

#ifdef JT808_TEST_JTB
extern uint8_t gb_get_08h( uint8_t *pout, u16 packet_in );
extern uint8_t gb_get_09h( uint8_t *pout, u16 packet_in );
extern uint8_t gb_get_10h( uint8_t *pout );
extern uint8_t gb_get_11h( uint8_t *pout, u16 packet_in ) ;
extern uint8_t gb_get_12h( uint8_t *pout, u16 packet_in ) ;
extern uint8_t gb_get_13h( uint8_t *pout );
extern uint8_t gb_get_14h( uint8_t *pout );
extern uint8_t gb_get_15h( uint8_t *pout , u16 packet_in );
#endif

#endif
