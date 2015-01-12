#ifndef   _GB_VDR_
#define   _GB_VDR_




#define  DF_Sem_Take     rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY )
#define  DF_Sem_Release  rt_sem_release(&sem_dataflash)

//  һ.   vdr  ����DF  ����ʹ�ù滮
/*
 Start Address offset :   0x1D0000
 Area Size :
					   213	Sector		 = 1704 pages
						----------------------

			 ����
			 1									   00-07H
			 90 								   08H
			 85 								   09H
			 7										10H
			 2										11H
			 2										12H
			 1										13H
			 1										14H
			 1										15H

	   ----------  ֻ������������---  ע�� ����������� Vdr.C

#define ADDR_DF_VDR_BASE 			0x1D0000				///��ʻ��¼��VDR���ݴ洢����ʼλ��
#define ADDR_DF_VDR_END				0x2AC000				///��ʻ��¼��VDR���ݴ洢�������λ��
#define ADDR_DF_VDR_SECT			220						///���������һЩ�ռ䣬ʵ����Ҫ213

   note:   �����õ������СΪ213 sector  �� ʣ��7 ��sector ���ڴ洢 ��¼��ز���

*/



#define ADDR_DF_VDR_15minDATA           ADDR_DF_VDR_BASE+(213<<12)    //   ͣ��ǰ15���ӵ�ƽ���ٶ�
#define ADDR_DF_VDR_15minAddr           ADDR_DF_VDR_BASE+(214<<12)    //   ͣ��ǰ15���ӵ�ƽ���ٶ�















//   �洢�����������
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
    u16  V_08H_Write;	  // 08 H  λ��
    u16  V_09H_Write;	  // 09H  λ��
    u16  V_10H_Write;	  // 10 H  λ��
    u16  V_11H_Write;   // 11 H  λ��
    u16  V_12H_Write;   // 12 H	λ��
    u16  V_13H_Write;   // 13H  λ��
    u16  V_14H_Write;   // 14 H	λ��
    u16  V_15H_Write;	  // 15 H λ��

    u16  V_08H_full;	  // 08 H  λ��
    u16  V_09H_full;	  // 09H  λ��
    u16  V_10H_full;	  // 10 H  λ��
    u16  V_11H_full;   // 11 H  λ��
    u16  V_12H_full;   // 12 H	λ��
    u16  V_13H_full;   // 13H  λ��
    u16  V_14H_full;   // 14 H	λ��
    u16  V_15H_full;	  // 15 H λ��

    u8   V_08H_get; //  ���ڿ�ʼ ��ȡ
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
    u8  Lati[4];  //    γ��
    u8  Longi[4]; //    ����

    //-----------------------------------
    u8  H_08[126]; //   08H buf
    u8  H_08_counter; // 08 ������ RTC ��׼
    u8  H_08_BAK[126]; //   08H buf
    u8  H_08_saveFlag; //   �洢��־λ
    u8  H_09[666];//   09H  buf
    u8  H_09_saveFlag; //
    u32 H_09_spd_accumlate;  // �ٶȺ�
    u8  H_09_seconds_counter;// �ٶ�
    u8  H_09_min_couner; // ÿ����
    u8  H_10[234]; //   10H buf
    u8  H_10_saveFlag;
    u8  H_11[50];//   11H  buf
    u8	H_11_start;	//	��ʱ��ʼ״̬ ��¼ δ��ʼ��0  ��ʼ��Ϊ1
    u8  H_11_lastSave; // �ٳ�ʱ��ʽ����ǰ���Ƿ�洢����¼   �洢��Ϊ1: δ�洢��Ϊ 0
    u8  H_11_saveFlag;
    u8	H_12[25]; //	12H buf
    u8  H_12_saveFlag;
    u8  H_13[7];//  13H  buf
    u8  H_13_saveFlag;
    u8  H_14[7]; //   14H buf
    u8  H_14_saveFlag;
    u8  H_15[133];//	 15H  buf
    u8  H_15_Set_StartDateTime[6];  // �������õĲɼ�ʱ��
    u8  H_15_Set_EndDateTime[6];  //  �������õĲɼ�����ʱ��
    u8  H_15_saveFlag;
} VDR_DATA;

extern VDR_DATA   VdrData;  //  ��ʻ��¼�ǵ�databuf



typedef struct
{
    u16  Running_state_enable;   //����״̬
} VDR_TRIG_STATUS;
extern  VDR_TRIG_STATUS  VDR_TrigStatus;

//==================================================================================================
// ���岿�� :   �������г���¼�����Э�� �� ��¼A
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
