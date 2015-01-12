/*
vdr Vichle Driver Record ������ʻ��¼
*/

#include <rtthread.h>
#include <finsh.h>
#include "include.h"

#include <rtdevice.h>
#include <dfs_posix.h>

#include <time.h>

#include "sst25.h"
#include "jt808_gps.h"
#include "jt808_util.h"
#include "jt808_param.h"
#include "Jt808_config.h"
#include "Jt808_GB19056.h"
#include "GB_vdr.h"


#define false	0
#define true	1


#define  STRT_YEAR   14
#define  STRT_MON     2
#define  STRT_DAY    27

// һ.
#define  NATHAN_DRV       //   ��������صļ�¼


//  =================  �г���¼�� ��� ============================
/*
    StartPage :    6320          Start Address offset :   0x316000

    Area Size :
                          213  Sector       = 1704 pages
                           ----------------------

				����
				1                                     00-07H
				90                                    08H
				85                                    09H
				7                                      10H
				2                                      11H
				2                                      12H
				1                                      13H
				1                                      14H
				1                                      15H

          ----------  ֻ������������---  ע�� ����������� Vdr.C
*/

#define       SECTORSIZE                  4096
#define       VDR_START_ADDRESS           ADDR_DF_VDR_BASE
#define       MarkByte                    0x0F          // ��ʾд��������Ϣ

//-------------------------- Sectors-----------  total   195 +8 Sectors --------------
#define  VDR_07_SIZE          1
#define  VDR_08_SIZE         90    //   128 rec_size               1sector=32 recrods    2880/32=90
#define  VDR_09_SIZE         72    //  666=>>768 rec_size      1sector=5 recrods     360/5=72
#define  VDR_10_SIZE         7     //   234=>> 256 rec_size    1sector=16              100/16 =7
#define  VDR_11_SIZE         2     //   50 =>> 64                  1 sector=64             100/64 =2 
#define  VDR_12_SIZE         2     //    25 =>> 32                 1 sector=128            200/128=2
#define  VDR_13_SIZE         1 // 1     //     7==>>  8                 1 sector= 512             100/512=1
#define  VDR_14_SIZE         1 // 1     //     7==>>  8                 1 sector= 512             100/512=1
#define  VDR_15_SIZE         1 //  1     //  133==>256                  1 sector =16               10/16 =1  


//----------------------absolutely  address   --------------
#define VDR_00_to_07H    VDR_START_ADDRESS

#define VDR_08H_START	 VDR_START_ADDRESS+VDR_07_SIZE*SECTORSIZE
#define VDR_08H_END		 VDR_START_ADDRESS+VDR_07_SIZE*SECTORSIZE-1

#define VDR_09H_START	 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE)*SECTORSIZE
#define VDR_09H_END		 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE)*SECTORSIZE-1


#define VDR_10H_START	 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE)*SECTORSIZE
#define VDR_10H_END		 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE)*SECTORSIZE-1

#define VDR_11H_START	 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE)*SECTORSIZE
#define VDR_11H_END		 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE)*SECTORSIZE-1

#define VDR_12H_START	 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE+VDR_11_SIZE)*SECTORSIZE
#define VDR_12H_END		 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE+VDR_11_SIZE)*SECTORSIZE-1

#define VDR_13H_START	 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE+VDR_11_SIZE+VDR_12_SIZE)*SECTORSIZE
#define VDR_13H_END		 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE+VDR_11_SIZE+VDR_12_SIZE)*SECTORSIZE-1

#define VDR_14H_START	 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE+VDR_11_SIZE+VDR_12_SIZE+VDR_13_SIZE)*SECTORSIZE
#define VDR_14H_END		 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE+VDR_11_SIZE+VDR_12_SIZE+VDR_13_SIZE)*SECTORSIZE-1

#define VDR_15H_START	 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE+VDR_11_SIZE+VDR_12_SIZE+VDR_13_SIZE+VDR_14_SIZE)*SECTORSIZE
#define VDR_15H_END		 VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE+VDR_11_SIZE+VDR_12_SIZE+VDR_13_SIZE+VDR_14_SIZE)*SECTORSIZE-1

//  ----08~ 15   ÿ������洢���˵�״̬��¼  ǰ8 ���ֽ�  need15  ��sector
#define VDR_WRITEINDEX  VDR_START_ADDRESS+(VDR_07_SIZE+VDR_08_SIZE+VDR_09_SIZE+VDR_10_SIZE+VDR_11_SIZE+VDR_12_SIZE+VDR_13_SIZE+VDR_14_SIZE+VDR_15_SIZE+1)*SECTORSIZE




#ifdef JT808_TEST_JTB

/*
   ʵ��ÿ����¼128�ֽڣ����ڶ�λ��Ҫ��һ���Ĵ���û��ʹ��delta����

   ��¼��ʽ

   000--003  yy-mm-dd hh:mm   ���԰�ʱ��ת��UTC��ʽ,���ڱȽϲ���
      �ֿ��Խ�ʡһ���ֽڣ���Ч��ʱ���ʽFFFFFFFFFF
   004--056  ���ٶȼ�¼ �ٶ�Ҫ��+/- 1kmh
          ԭ��60*8=480bit ������7bit������Ҫ 60*7=420bit=53byte
          7bit�� 0b1111111 ��ʾ�ٶ���Ч(gpsδ��λ)
   057--116  ״̬��Ϣ
   117--126  ��λ����λ��,�μ��г���¼�� GBT19065 �׸���Чλ��

 */
#define GuoJian_VDR_08H_09H_START	ADDR_DF_VDR_BASE      //0x300000
#define GuoJian_VDR_08H_09H_END		ADDR_DF_VDR_BASE+0x2FFFF

#define GuoJian_VDR_10H_START	ADDR_DF_VDR_BASE+0x30000    /*256*100= 0x6400*/
#define GuoJian_VDR_10H_END		ADDR_DF_VDR_BASE+0x363FF

#define GuoJian_VDR_11H_START	ADDR_DF_VDR_BASE+0x38000
#define GuoJian_VDR_11H_END		ADDR_DF_VDR_BASE+0x398FF    /*64*100=0x1900*/

#define GuoJian_VDR_12H_START	ADDR_DF_VDR_BASE+0x3A000
#define GuoJian_VDR_12H_END		ADDR_DF_VDR_BASE+0x3B8FF    /*32*200=*/

#define GuoJian_VDR_13H_START	ADDR_DF_VDR_BASE+0x3C000
#define GuoJian_VDR_13H_END		ADDR_DF_VDR_BASE+0x3C31F    /*128*100*/

#define GuoJian_VDR_14H_START	ADDR_DF_VDR_BASE+0x3D000
#define GuoJian_VDR_14H_END		ADDR_DF_VDR_BASE+0x3D31F

#define GuoJian_VDR_15H_START	ADDR_DF_VDR_BASE+0x3E000
#define GuoJian_VDR_15H_END		ADDR_DF_VDR_BASE+0x3E98F
#endif


/*
   4MB serial flash 0x400000
 */

/*ת��hex��bcd�ı���*/
#define HEX_TO_BCD( A ) ( ( ( ( A ) / 10 ) << 4 ) | ( ( A ) % 10 ) )



/*����Сʱ��ʱ���,��Ҫ��Ϊ�˱Ƚϴ�Сʹ��
   byte0 year
   byte1 month
   byte2 day
   byte3 hour
 */
typedef unsigned int YMDH_TIME;

typedef struct
{
    uint8_t cmd;

    uint32_t	ymdh_start;
    uint8_t		minute_start;

    uint32_t	ymdh_end;
    uint8_t		minute_end;

    uint32_t	ymdh_curr;
    uint8_t		minute_curr;
    uint32_t	addr;

    uint16_t	blocks;         /*����ÿ���ϴ����ٸ����ݿ�*/
    uint16_t	blocks_remain;  /*��ǰ��֯�ϴ����ǻ���Ҫ�ĵ�blocks*/
} VDR_CMD;

VDR_CMD		vdr_cmd;

uint8_t		vdr_tx_info[1024];
uint16_t	vtr_tx_len = 0;


/*����д���ļ�����Ϣ
   0...3  д��SerialFlash�ĵ�ַ
   4...31 �ļ���
 */

//=====================================================
VDR_INDEX         Vdr_Wr_Rd_Offset;      //  ��¼��д���� λ��
VDR_DATA          VdrData;               //  ��ʻ��¼�ǵ�databuf
VDR_TRIG_STATUS   VDR_TrigStatus;





/*
    ��ӡ��� HEX ��Ϣ��Descrip : ������Ϣ ��instr :��ӡ��Ϣ�� inlen: ��ӡ����
*/
void OutPrint_HEX(u8 *Descrip, u8 *instr, u16 inlen )
{
    u32  i = 0;
    rt_kprintf("\r\n %s:", Descrip);
    for( i = 0; i < inlen; i++)
        rt_kprintf("%02X ", instr[i]);
    rt_kprintf("\r\n");
}

//======================================================
/*
      ������ȡ��ǰ��ʻ��¼�Ĳ���λ��

      type :    0  �����������      08  09  10  11 12 13  14
      u8    :     0   :��������

*/
u8  Vdr_PowerOn_getWriteIndex(u8 type)
{

    u16    i;
    u8	  c;

    //	 1.  get  address


    switch(type)
    {
    case 0x08:
        // 1.1  �ȶ�ȡ��һ���ַ����ж�
        //rt_kprintf("\r\n 08H write=0x%X  \r\n",VDR_08H_START);
        for(i = 0; i < VDR_08_MAXindex; i++)
        {
            //c=SST25V_ByteRead(VDR_08H_START+i*128);
            sst25_read(VDR_08H_START + i * 128, &c, 1); // Read  1  Byte
            if(c == 0xFF)
            {
                break;
            }
        }
        // 1.2. û���ҵ�2880 ��ô ������һ��  ��0 ��ʼ
        if(i == VDR_08_MAXindex)
        {
            Vdr_Wr_Rd_Offset.V_08H_Write = VDR_08_MAXindex;
            Vdr_Wr_Rd_Offset.V_08H_full = 1; // enable
        }
        if(GB19056.workstate == 0)
            rt_kprintf("\r\n 08H write=%d   total=%d\r\n", Vdr_Wr_Rd_Offset.V_08H_Write, VDR_08_MAXindex);
        break;
    case 0x09:
        //  2.1
        // rt_kprintf("\r\n 09H write=0x%X  \r\n",VDR_09H_START);
        for(i = 0; i < VDR_09_MAXindex; i++)
        {
            // c=SST25V_ByteRead(VDR_09H_START+i*768);
            sst25_read(VDR_09H_START + i * 768, &c, 1); // Read  1  Byte
            if(c == 0xFF)
            {
                break;
            }
        }
        // 2. û���ҵ�360 ��ô ������һ��  ��0 ��ʼ
        if(i == VDR_09_MAXindex)
        {
            Vdr_Wr_Rd_Offset.V_09H_Write = VDR_09_MAXindex;
            Vdr_Wr_Rd_Offset.V_09H_full = 1;
        }
        if(GB19056.workstate == 0)
            rt_kprintf("\r\n 09H write=%d  total=%d\r\n", Vdr_Wr_Rd_Offset.V_09H_Write, VDR_09_MAXindex);
        break;

    case 0x10:
        //  rt_kprintf("\r\n 10H write=0x%X  \r\n",VDR_10H_START);
        for(i = 0; i < VDR_10_MAXindex; i++)
        {
            //c=SST25V_ByteRead(VDR_10H_START+i*256);
            sst25_read(VDR_10H_START + i * 256, &c, 1); // Read  1  Byte
            if(c == 0xFF)
            {
                break;
            }
        }
        // 2. û���ҵ�100 ��ô ������һ��  ��0 ��ʼ
        if(i == VDR_10_MAXindex)
        {
            Vdr_Wr_Rd_Offset.V_10H_Write = VDR_10_MAXindex;
            Vdr_Wr_Rd_Offset.V_10H_full = 1;
        }
        if(GB19056.workstate == 0)
            rt_kprintf("\r\n 10H write=%d   total=%d\r\n", Vdr_Wr_Rd_Offset.V_10H_Write, VDR_10_MAXindex);
        break;

    case 0x11:
        for(i = 0; i < VDR_11_MAXindex; i++)
        {
            //c=SST25V_ByteRead(VDR_11H_START+i*64);
            sst25_read(VDR_11H_START + i * 64, &c, 1); // Read  1  Byte
            if(c == 0xFF)
            {
                break;
            }
        }
        // 2. û���ҵ�100 ��ô ������һ��  ��0 ��ʼ
        if(i == VDR_11_MAXindex)
        {
            Vdr_Wr_Rd_Offset.V_11H_Write = VDR_11_MAXindex;
            Vdr_Wr_Rd_Offset.V_11H_full = 1;
        }
        if(GB19056.workstate == 0)
            rt_kprintf("\r\n 11H write=%d   total=%d\r\n", Vdr_Wr_Rd_Offset.V_11H_Write, VDR_11_MAXindex);
        break;

    case 0x12:
        for(i = 0; i < VDR_12_MAXindex; i++)
        {
            //c=SST25V_ByteRead(VDR_12H_START+i*32);
            sst25_read(VDR_12H_START + i * 32, &c, 1); // Read  1  Byte
            if(c == 0xFF)
            {
                break;
            }
        }
        // 2. û���ҵ�200 ��ô ������һ��  ��0 ��ʼ
        if(i == VDR_12_MAXindex)
        {
            Vdr_Wr_Rd_Offset.V_12H_Write = VDR_12_MAXindex;
            Vdr_Wr_Rd_Offset.V_12H_full = 1;
        }
        if(GB19056.workstate == 0)
            rt_kprintf("\r\n 12H write=%d   total=%d\r\n", Vdr_Wr_Rd_Offset.V_12H_Write, VDR_12_MAXindex);
        break;

    case 0x13:
        for(i = 0; i < VDR_13_MAXindex; i++)
        {
            //c=SST25V_ByteRead(VDR_13H_START+i*8);
            sst25_read(VDR_13H_START + i * 8, &c, 1); // Read  1  Byte
            if(c == 0xFF)
            {
                break;
            }
        }
        // 2. û���ҵ�100 ��ô ������һ��  ��0 ��ʼ
        if(i == VDR_13_MAXindex)
        {
            Vdr_Wr_Rd_Offset.V_13H_Write = VDR_13_MAXindex;
            Vdr_Wr_Rd_Offset.V_13H_full = 1;
        }

        if(GB19056.workstate == 0)
            rt_kprintf("\r\n 13H write=%d   total=%d\r\n", Vdr_Wr_Rd_Offset.V_13H_Write, VDR_13_MAXindex);
        break;

    case 0x14:
        for(i = 0; i < VDR_14_MAXindex; i++)
        {
            //c=SST25V_ByteRead(VDR_14H_START+i*8);
            sst25_read(VDR_14H_START + i * 8, &c, 1); // Read  1  Byte
            if(c == 0xFF)
            {
                break;
            }
        }
        // 2. û���ҵ�100 ��ô ������һ��  ��0 ��ʼ
        if(i == VDR_14_MAXindex)
        {
            Vdr_Wr_Rd_Offset.V_14H_Write = VDR_14_MAXindex;
            Vdr_Wr_Rd_Offset.V_14H_full = 1;
        }
        if(GB19056.workstate == 0)
            rt_kprintf("\r\n 14H write=%d   total=%d\r\n", Vdr_Wr_Rd_Offset.V_14H_Write, VDR_14_MAXindex);
        break;

    case 0x15:
        for(i = 0; i < VDR_15_MAXindex; i++)
        {
            //c=SST25V_ByteRead(VDR_15H_START+i*256);
            sst25_read(VDR_15H_START + i * 256, &c, 1); // Read  1  Byte
            if(c == 0xFF)
            {
                break;
            }
        }
        // 2. û���ҵ�10 ��ô ������һ��  ��0 ��ʼ
        if(i == VDR_15_MAXindex)
        {
            Vdr_Wr_Rd_Offset.V_15H_Write = VDR_15_MAXindex;
            Vdr_Wr_Rd_Offset.V_15H_full = 1;
        }
        if(GB19056.workstate == 0)
            rt_kprintf("\r\n 15H write=%d   total=%d\r\n\r\nAvrg_15minSpd.write=%d, Avrg_15minSpd.read=%d  \r\n", Vdr_Wr_Rd_Offset.V_15H_Write, VDR_15_MAXindex, Avrg_15minSpd.write, Avrg_15minSpd.read);
        break;

    }
    return true;
}


//    ���������г���¼д�����ʼ��ַ   from  08H
void total_ergotic(void)
{
    DF_Sem_Take;
    Vdr_Wr_Rd_Offset.V_08H_Write = vdr_cmd_writeIndex_read(0x08);
    Vdr_Wr_Rd_Offset.V_09H_Write = vdr_cmd_writeIndex_read(0x09);
    Vdr_Wr_Rd_Offset.V_10H_Write = vdr_cmd_writeIndex_read(0x10);
    Vdr_Wr_Rd_Offset.V_11H_Write = vdr_cmd_writeIndex_read(0x11);
    Vdr_Wr_Rd_Offset.V_12H_Write = vdr_cmd_writeIndex_read(0x12);
    Vdr_Wr_Rd_Offset.V_13H_Write = vdr_cmd_writeIndex_read(0x13);
    Vdr_Wr_Rd_Offset.V_14H_Write = vdr_cmd_writeIndex_read(0x14);
    Vdr_Wr_Rd_Offset.V_15H_Write = vdr_cmd_writeIndex_read(0x15);

    //  read   15 miniute  index
    DF_Read_RecordAdd(Avrg_15minSpd.write, Avrg_15minSpd.read, ADDR_DF_VDR_15minAddr);


    Vdr_PowerOn_getWriteIndex(0x08);
    Vdr_PowerOn_getWriteIndex(0x09);
    Vdr_PowerOn_getWriteIndex(0x10);
    Vdr_PowerOn_getWriteIndex(0x11);
    Vdr_PowerOn_getWriteIndex(0x12);
    Vdr_PowerOn_getWriteIndex(0x13);
    Vdr_PowerOn_getWriteIndex(0x14);
    Vdr_PowerOn_getWriteIndex(0x15);

    DF_Sem_Release;

}
FINSH_FUNCTION_EXPORT( total_ergotic, total_ergotic );

void gb_index_write(u8 cmd, u32 value)
{
    vdr_cmd_writeIndex_save(cmd, value);

    total_ergotic();

}
//FINSH_FUNCTION_EXPORT( gb_index_write, gb_index_write(cmd,value));

//   ֻ���ڳ�ʼ�����õ�
void gb_vdr_erase(void)      // �ŵ�  factory_ex    ���
{
    u16  i = 0;
    u32  addr = 0;
    // start address      0x1C0000  (64K ��ʼ��ַ)     end address   0x2AC000	   SIZE: 0xEC000   (12  ��64K ������ʣ��44 ��4K   ��236 ��4K)

    //  1. erase  to  839          840-790=50
    {
        //   ����ʼ��ַ��ʼ  12 ��  64KB ����
        rt_kprintf("\r\n   <------------  vdr  area  erase  0 ------------------>\r\n");
        for(i = 0; i < ADDR_DF_VDR_SECT; i++)
        {
            sst25_erase_4k(VDR_START_ADDRESS + addr + i * 0x1000); // 32k  <=> 0x8000   8sector
        }
        rt_kprintf("\r\n   <------------  vdr  area  erase  1 ------------------>\r\n");

        Vdr_Wr_Rd_Offset.V_08H_Write = 0;
        vdr_cmd_writeIndex_save(0x08, Vdr_Wr_Rd_Offset.V_08H_Write);
        Vdr_Wr_Rd_Offset.V_09H_Write = 0;
        vdr_cmd_writeIndex_save(0x09, Vdr_Wr_Rd_Offset.V_09H_Write);
        Vdr_Wr_Rd_Offset.V_10H_Write = 0;
        vdr_cmd_writeIndex_save(0x10, Vdr_Wr_Rd_Offset.V_10H_Write);
        Vdr_Wr_Rd_Offset.V_11H_Write = 0;
        vdr_cmd_writeIndex_save(0x11, Vdr_Wr_Rd_Offset.V_11H_Write);
        Vdr_Wr_Rd_Offset.V_12H_Write = 0;
        vdr_cmd_writeIndex_save(0x12, Vdr_Wr_Rd_Offset.V_12H_Write);
        Vdr_Wr_Rd_Offset.V_13H_Write = 0;
        vdr_cmd_writeIndex_save(0x13, Vdr_Wr_Rd_Offset.V_13H_Write);
        Vdr_Wr_Rd_Offset.V_14H_Write = 0;
        vdr_cmd_writeIndex_save(0x14, Vdr_Wr_Rd_Offset.V_14H_Write);
        Vdr_Wr_Rd_Offset.V_15H_Write = 0;
        vdr_cmd_writeIndex_save(0x15, Vdr_Wr_Rd_Offset.V_15H_Write);

        //  write  15 minute  index
        Avrg_15minSpd.write = 0;
        Avrg_15minSpd.read = 0;
        DF_Write_RecordAdd(Avrg_15minSpd.write, Avrg_15minSpd.read, ADDR_DF_VDR_15minAddr);
    }
    rt_kprintf("\r\n   <------------  vdr  area  erase   over  ------------------>\r\n");
}
FINSH_FUNCTION_EXPORT( gb_vdr_erase, gb_vdr_erase );

//    ������ʻ��¼������

void  VDR_product_08H_09H(void)
{

    // u16 j=0,cur=0;
    u8 spd_reg = 0, i = 0, spd_usefull_counter = 0, pos_V = 0, avrg_spd = 0;
    u8 reg8[6];

    //mytime_now=Get_RTC(); 	//	RTC  ���
    //  ��ʻ��¼������ݲ��� ����,  �Ҷ�λ�������
    // if(VDR_TrigStatus.Running_state_enable==1)
    {
        //	  ��������ʻ״̬��

        //-------------------  08H	 --------------------------------------------------
        // 1.	save	 Minute 		1-59   s
        spd_reg = current_speed_10x % 10;
        if(spd_reg >= 5)
            spd_reg = current_speed_10x / 10 + 1;
        else
            spd_reg = current_speed_10x / 10;
        VdrData.H_08[6 + 2 * VdrData.H_08_counter] = spd_reg; //km/h
        VdrData.H_08[7 + 2 * VdrData.H_08_counter] = GB19056.Vehicle_sensor;
        if(VdrData.H_08_counter == 56)
            mytime_to_bcd(VdrData.H_08, mytime_now );

        if(VdrData.H_08_counter == 59)
        {
            // ��Ϊ0
            // 0.  �ж��Ƿ��ǵ�һ���ϵ�
            //  1.  ������ ʱ����	 ����0

            reg8[0] = ((YEAR( mytime_now ) / 10) << 4) + (YEAR( mytime_now ) % 10);
            reg8[1] = ((MONTH( mytime_now ) / 10) << 4) + (MONTH( mytime_now ) % 10);
            reg8[2] = ((DAY( mytime_now ) / 10) << 4) + (DAY( mytime_now ) % 10);
            reg8[3] = ((HOUR( mytime_now ) / 10) << 4) + (HOUR( mytime_now ) % 10);
            reg8[4] = ((MINUTE( mytime_now ) / 10) << 4) + (MINUTE( mytime_now ) % 10);
            if((reg8[0] == VdrData.H_08[0]) && (reg8[1] == VdrData.H_08[1]) && (reg8[2] == VdrData.H_08[2]) && (reg8[3] == VdrData.H_08[3]))
                ;
            else
            {

                VdrData.H_08[0] = ((YEAR( mytime_now ) / 10) << 4) + (YEAR( mytime_now ) % 10);
                VdrData.H_08[1] = ((MONTH( mytime_now ) / 10) << 4) + (MONTH( mytime_now ) % 10);
                VdrData.H_08[2] = ((DAY( mytime_now ) / 10) << 4) + (DAY( mytime_now ) % 10);
                VdrData.H_08[3] = ((HOUR( mytime_now ) / 10) << 4) + (HOUR( mytime_now ) % 10);
                VdrData.H_08[4] = ((MINUTE( mytime_now ) / 10) << 4) + (MINUTE( mytime_now ) % 10);

            }


            VdrData.H_08[5] = 0;


            memcpy(VdrData.H_08_BAK, VdrData.H_08, 126);

            //--- Check  save  or not
            spd_usefull_counter = 0;
            for(i = 0; i < 60; i++)
            {
                if(VdrData.H_08[6 + 2 * i])
                    spd_usefull_counter++;
            }
            if(spd_usefull_counter)		// ��һ���ٶȲ�Ϊ0  ����洢
                VdrData.H_08_saveFlag = 1; //	����  ��ǰ���ӵ����ݼ�¼

            // 2.  initial
            memset(VdrData.H_08 + 6, 0, 120); // Ĭ���� 0x0
        }
        VdrData.H_08_counter++;
        if(VdrData.H_08_counter >= 60)
            VdrData.H_08_counter = 0;



        //------------------------09H ---------  min=0;	sec=0	----------------------------
        //  1 .	��д����λ��	1-59 min
        if(VdrData.H_09_seconds_counter >= 56) // 59 ��ʱ�������������0 ��ô���Ӿͱ���
        {
            //   �� A.20		  (��ǰ���ӵ�λ��  +	��ǰ���ӵ�ƽ���ٶ�)
            memcpy( VdrData.H_09 + 6 + MINUTE( mytime_now ) * 11, VdrData.Longi, 4); // ����
            memcpy( VdrData.H_09 + 6 + MINUTE( mytime_now ) * 11 + 4, VdrData.Lati, 4); //γ��
            VdrData.H_09[6 + MINUTE( mytime_now ) * 11 + 8] = (gps_alti >> 8);
            VdrData.H_09[6 + MINUTE( mytime_now ) * 11 + 9] = gps_alti;
            //  ��ǰ���ӵ�ƽ���ٶ�	 ��AvrgSpd_MintProcess()  ���õı���
            VdrData.H_09[6 + MINUTE( mytime_now ) * 11 + 10] = VdrData.H_09_spd_accumlate / 60;

            //  ͣ��ǰ15 ����ƽ���ٶȼ�¼
#if 1
            if(VdrData.H_09_seconds_counter == 58)
            {
                avrg_spd = VdrData.H_09_spd_accumlate / 58;
                if(avrg_spd > 5)
                    Avrg15_min_generate(VdrData.H_09_spd_accumlate / 58);
                else
                {
                    if(Avrg_15minSpd.savefull_state == 1)
                    {
                        Avrg15_min_save();
                    }
                    Avrg_15minSpd.Ram_counter = 0;
                    Avrg_15minSpd.savefull_state = 0;
                }
            }
#endif

        }
        //   ��ǰ��������Ϳ�Сʱ��
        if((MINUTE( mytime_now ) == 59) && (VdrData.H_09_seconds_counter == 57))
        {
            //  2.  ������ ʱ����	 |	 ��  ����0
            VdrData.H_09[0] = ((YEAR( mytime_now ) / 10) << 4) + (YEAR( mytime_now ) % 10);
            VdrData.H_09[1] = ((MONTH( mytime_now ) / 10) << 4) + (MONTH( mytime_now ) % 10);
            VdrData.H_09[2] = ((DAY( mytime_now ) / 10) << 4) + (DAY( mytime_now ) % 10);
            VdrData.H_09[3] = ((HOUR( mytime_now ) / 10) << 4) + (HOUR( mytime_now ) % 10);
            VdrData.H_09[4] = 0;
            VdrData.H_09[5] = 0;
            //  ���һ���ӵ��ж�         �ٶ��Ѿ�
            // VdrData.H_09[6+MINUTE( mytime_now )*11+10]=current_speed_10x/10;//VdrData.H_09_spd_accumlate/(VdrData.H_09_seconds_counter+1);
            // 1.	�ж��Ƿ�����Ҫ�洢
            spd_usefull_counter = 0;
            pos_V = 0;
            for(i = 0; i < 60; i++)
            {
                if(VdrData.H_09[6 + i * 11 + 10]) // ���ٶ��ۼ�
                    spd_usefull_counter++;
                if((VdrData.H_09[6 + i * 11] == 0x7F) && (VdrData.H_09[6 + i * 11 + 4] == 0x7F))
                    pos_V++;   // λ����Ч���ۼ�
            }

            //  ����  60 �����ٶȶ�Ϊ0  �� ��û�ж�λ������������
            if(!((spd_usefull_counter == 0) && (pos_V == 60)))
                VdrData.H_09_saveFlag = 1;	//	 ����  ��ǰ���ӵ����ݼ�¼
        }

        //---------09  tick --------------
        VdrData.H_09_seconds_counter++;
        if(VdrData.H_09_seconds_counter >= 60)
        {
            VdrData.H_09_seconds_counter = 0;
            VdrData.H_09_spd_accumlate = 0;
        }
        VdrData.H_09_spd_accumlate += current_speed_10x / 10;

        // ÿ��Сʱprocess
        if((MINUTE( mytime_now ) == 0) && (SEC( mytime_now ) == 2))
        {
            //   ��һСʱ��������Ҫ��ʼ��һ��
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
        }
    }

}


void VDR_product_11H_Start(void)
{
    //    11 H   ������ʻ��ʼʱ��   ����ʼλ��     ��60s ��ʼ���ʻ��ʼ
    {
        //  1.   ��������ʻ֤��
        memcpy(VdrData.H_11, jt808_param.id_0xF009, 18);
        //   2.   ��ʼʱ��
        mytime_to_bcd(VdrData.H_11 + 18, mytime_now);

        //   3.  ��ʼλ��
        memcpy( VdrData.H_11 + 30, VdrData.Longi, 4); // ����
        memcpy( VdrData.H_11 + 30 + 4, VdrData.Lati, 4); //γ��
        VdrData.H_11[30 + 8] = (gps_alti >> 8);
        VdrData.H_11[30 + 9] = gps_alti;
        VdrData.H_11_start = 1; //  start
        VdrData.H_11_lastSave = 0;

    }
}

void VDR_product_11H_End(u8 value)
{
    //    4.   save
    VdrData.H_11_saveFlag = value; // 1: �ۼ�  2: ���ۼ�
#if 1
    //     5. clear  state
    if(value == 1)
    {
        VdrData.H_11_lastSave = 0; // clear
        //VdrData.H_11_start=0;
    }
    else if(value == 2)
        VdrData.H_11_lastSave = 1; // �Ѿ��洢��
#endif
}




void VDR_product_12H(u8  value)    // ��ο���¼
{
    //   1. ʱ�䷢����ʱ��
    mytime_to_bcd(VdrData.H_12, mytime_now);

    //  2.   ��������ʻ֤��
    memcpy(VdrData.H_12 + 6, jt808_param.id_0xF009, 18);
    //  3.  ���
    VdrData.H_12[24] = value;

    // 4.  save
    VdrData.H_12_saveFlag = 1;
}


//  Note :  �����˵���û���ã������������£��ϵ����SPI DF  Σ����ܴ�
void VDR_product_13H(u8  value)    //  �ⲿ�����¼   01  ͨ��    02  �ϵ�
{
    //   1. ʱ�䷢����ʱ��
    mytime_to_bcd(VdrData.H_13, mytime_now);
    //  2.  value
    VdrData.H_13[6] = value;
    //  3. save
    VdrData.H_13_saveFlag = 1;
}


void VDR_product_14H(u8 cmdID)    // ��¼�ǲ����޸ļ�¼
{

    //   1. ʱ�䷢����ʱ��
    mytime_to_bcd(VdrData.H_14, mytime_now);
    //  2.  value
    VdrData.H_14[6] = cmdID;
    //  3. save
    VdrData.H_14_saveFlag = 1;
}



// note  :    ��Ҫ��������   08 H �����ݼ�¼ �����ﲻ��Ҫ��䡣
void VDR_product_15H(u8  value)    //  �ɼ�ָ���ٶȼ�¼
{
    // �ٶ��쳣 ��ʾʱ��    5  min





}

void  VDR_get_15H_StartEnd_Time(u8  *Start, u8 *End)
{
    u8  i = 0;

    for(i = 0; i < 6; i++)
    {
        VdrData.H_15_Set_StartDateTime[i] = Start[i];
        VdrData.H_15_Set_EndDateTime[i] = End[i];
    }

}


void  drv_query(void)
{
    rt_kprintf("\r\n       08   max=%d  index=%d    full=%d", VDR_08_MAXindex, Vdr_Wr_Rd_Offset.V_08H_Write, Vdr_Wr_Rd_Offset.V_08H_full);
    rt_kprintf("\r\n       09   max=%d  index=%d    full=%d", VDR_09_MAXindex, Vdr_Wr_Rd_Offset.V_09H_Write, Vdr_Wr_Rd_Offset.V_09H_full);
    rt_kprintf("\r\n       10   max=%d  index=%d    full=%d", VDR_10_MAXindex, Vdr_Wr_Rd_Offset.V_10H_Write, Vdr_Wr_Rd_Offset.V_10H_full);
    rt_kprintf("\r\n       11   max=%d  index=%d    full=%d", VDR_11_MAXindex, Vdr_Wr_Rd_Offset.V_11H_Write, Vdr_Wr_Rd_Offset.V_11H_full);
    rt_kprintf("\r\n       12   max=%d  index=%d    full=%d", VDR_12_MAXindex, Vdr_Wr_Rd_Offset.V_12H_Write, Vdr_Wr_Rd_Offset.V_12H_full);
    rt_kprintf("\r\n       13   max=%d  index=%d    full=%d", VDR_13_MAXindex, Vdr_Wr_Rd_Offset.V_13H_Write, Vdr_Wr_Rd_Offset.V_13H_full);
    rt_kprintf("\r\n       14   max=%d  index=%d    full=%d", VDR_14_MAXindex, Vdr_Wr_Rd_Offset.V_14H_Write, Vdr_Wr_Rd_Offset.V_14H_full);
    rt_kprintf("\r\n       15   max=%d  index=%d    full=%d", VDR_15_MAXindex, Vdr_Wr_Rd_Offset.V_15H_Write, Vdr_Wr_Rd_Offset.V_15H_full);

}
FINSH_FUNCTION_EXPORT(drv_query, drv_query );
//---------------------------------------------------------------------------------

//------- stuff     user data       ---------------
u16  stuff_drvData(u8 type, u16 Start_recNum, u16 REC_nums, u8 *dest)
{
    u16  res_len = 0, get_len = 0;
    u16  i = 0;
    u16  get_indexnum = 0;


    if(Start_recNum >= 1) //û����
        get_indexnum = Start_recNum - 1 - i;

    for(i = 0; i < REC_nums; i++)
    {
        if(get_indexnum >= (Start_recNum - 1 - i))
            get_indexnum = Start_recNum - 1 - i;
        else
            continue;

        switch(type)
        {
        case 0x08:
            get_len = gb_vdr_get_08h(get_indexnum, dest + res_len);
            break;
        case 0x09:
            get_len = gb_vdr_get_09h(get_indexnum, dest + res_len);
            break;
        case 0x10:
            get_len = gb_vdr_get_10h(get_indexnum, dest + res_len);
            break;
        case 0x11:
            get_len = gb_vdr_get_11h(get_indexnum, dest + res_len);
            break;
        case 0x12:
            get_len = gb_vdr_get_12h(get_indexnum, dest + res_len);
            break;
        case 0x13:
            get_len = gb_vdr_get_13h(get_indexnum, dest + res_len);
            break;
        case 0x14:
            get_len = GB_VDR_READ_14H(get_indexnum, dest + res_len);
            break;
        case 0x15:
            get_len = gb_vdr_get_15h(get_indexnum, dest + res_len);
            break;
        default:
            return 0;

        }

        res_len += get_len;
    }
    return   res_len;
}



/*
   ��ʻ�ٶȼ�¼
   ���������Լ������ݣ��Լ���
   ����126�ֽ�ÿ����
   48*60=2880��

    ÿһ��   128 �ֽ�

 */

u16 gb_vdr_get_08h( u16 indexnum, u8 *p)
{
    int			i;
    uint8_t		buf[128];
    u32  addr = 0;
    u8  FCS = 0;



    //	  1.  get  address
    addr = VDR_08H_START + indexnum * 128;

    //    2. read
    sst25_read(addr, buf, 128 );
    // OutPrint_HEX("08H content",buf,128); // debug
    //     3.  FCS  check
    FCS = 0;
    for(i = 0; i < 127; i++)
        FCS ^= buf[i];
    if(buf[127] != FCS)
    {
        // rt_kprintf("\r\n  08H read fcs error  save:%X  cacu: %X \r\n",buf[127],FCS);
        return  0;
    }
    if(buf[0] == MarkByte)
    {
        memcpy(p, buf + 1, 126);
        return  126;
    }
    else
        return 0;
}



//FINSH_FUNCTION_EXPORT( gb_vdr_get_08h, get_08 );


/*
   λ����Ϣ
   360Сʱ   ��ÿСʱ666�ֽ�
 */

u16  gb_vdr_get_09h( u16 indexnum, u8 *p)
{
    int		 i;
    uint8_t	 buf[669];
    u32  addr = 0;
    u8  FCS = 0;



    //    1.  get	address
    addr = VDR_09H_START + indexnum * 768;

    //	  2. read
    sst25_read(addr, buf, 669 ); //2+content
    // OutPrint_HEX("09H content",buf,669); // debug
    //	   3.  FCS	check
    FCS = 0;
    for(i = 0; i < 667; i++)
        FCS ^= buf[i];
    if(buf[667] != FCS)
    {
        //rt_kprintf("\r\n	09H read fcs error	save:%X  cacu: %X \r\n",buf[667],FCS);
        return  0;
    }
    if(buf[0] == MarkByte)
    {
        memcpy(p, buf + 1, 666);
        return  666;
    }
    else
        return 0;

}

//FINSH_FUNCTION_EXPORT( gb_vdr_get_09h, get_09 );


/*
   �¹��ɵ�
   234Byte
   ��100��
 */
u16 gb_vdr_get_10h( u16 indexnum, u8 *p)
{
    int		 i;
    uint8_t	 buf[237];
    u32  addr = 0;
    u8   FCS = 0;



    //    1.  get	address
    addr = VDR_10H_START + indexnum * 256;

    //	  2. read
    sst25_read(addr, buf, 236 );
    //OutPrint_HEX("10H content",buf,236); // debug
    //	   3.  FCS	check
    FCS = 0;
    for(i = 0; i < 235; i++)
        FCS ^= buf[i];
    if(buf[235] != FCS)
    {
        //rt_kprintf("\r\n	10H read fcs error	save:%X  cacu: %X \r\n",buf[235],FCS);
        return  0;
    }
    if(buf[0] == MarkByte)
    {
        memcpy(p, buf + 1, 234);
        return  234;
    }
    else
        return 0;

}

//FINSH_FUNCTION_EXPORT( gb_vdr_get_10h, get_10 );


/*��ʱ��ʻ��¼
   50Bytes   100��

      ÿ����¼   64   ���ֽ�
 */

u16 gb_vdr_get_11h( u16 indexnum, u8 *p)
{

    int		 i;
    uint8_t	 buf[60];
    u32  addr = 0;
    u8  FCS = 0;



    //    1.  get	address
    addr = VDR_11H_START + indexnum * 64;

    //	  2. read
    sst25_read(addr, buf, 52 );
    //OutPrint_HEX("11H content",buf,52); // debug
    //	   3.  FCS	check
    FCS = 0;
    for(i = 0; i < 51; i++)
        FCS ^= buf[i];
    if(buf[51] != FCS)
    {
        //rt_kprintf("\r\n	11H read fcs error	save:%X  cacu: %X \r\n",buf[51],FCS);
        return  0;
    }
    if(buf[0] == MarkByte)
    {
        memcpy(p, buf + 1, 50);
        return  50;
    }
    else
        return 0;


}

//FINSH_FUNCTION_EXPORT( gb_vdr_get_11h, get_11 );


/*��ʻԱ��ݵ�¼
   25Bytes   200��
 */

u16 gb_vdr_get_12h( u16 indexnum, u8 *p)
{
    int		 i;
    uint8_t	 buf[30];
    u32  addr = 0;
    u8  FCS = 0;



    //    1.  get	address
    addr = VDR_12H_START + indexnum * 32;

    //	  2. read
    sst25_read(addr, buf, 27 );
    // OutPrint_HEX("12H content",buf,27); // debug
    //	   3.  FCS	check
    FCS = 0;
    for(i = 0; i < 26; i++)
        FCS ^= buf[i];
    if(buf[26] != FCS)
    {
        //rt_kprintf("\r\n	11H read fcs error	save:%X  cacu: %X \r\n",buf[26],FCS);
        return  0;
    }
    if(buf[0] == MarkByte)
    {
        memcpy(p, buf + 1, 25);
        return  25;
    }
    else
        return 0;
}

//FINSH_FUNCTION_EXPORT( gb_vdr_get_12h, get_12 );


/*�ⲿ�����¼
   7�ֽ� 100��**/
u16 gb_vdr_get_13h( u16 indexnum, u8 *p)
{
    uint8_t	 buf[10];
    u32  addr = 0;



    //    1.  get	address
    addr = VDR_13H_START + indexnum * 8;

    //rt_kprintf("\r\n  14 addr=0x%00000008X \r\n",addr);
    //	  2.  read
    sst25_read(addr, buf, 8 );
    // OutPrint_HEX("13H content",buf,8); // debug
    if(buf[0] == MarkByte)
    {
        memcpy(p, buf + 1, 7);
        return  7;
    }
    else
        return 0;

}
//FINSH_FUNCTION_EXPORT( gb_vdr_get_13h, get_13 );


/*��¼�ǲ���
   7�ֽ� 100��    8 */
u16 GB_VDR_READ_14H(u16 indexnum, u8 *p)
{
    u32         addr = 0;
    uint8_t	 buf[10];

    //    1.  get	address
    addr = VDR_14H_START + indexnum * 8;
    // rt_kprintf("\r\n  14 addr=0x%00000008X \r\n",addr);
    //	  2.  read
    memset(buf, 0, sizeof(buf));
    sst25_read(addr, buf, 8 );
    //OutPrint_HEX("14H content",buf,8); // debug
    if(buf[0] == MarkByte)
    {
        memcpy(p, buf + 1, 7);
        return  7;
    }
    else
        return 0;

}
//FINSH_FUNCTION_EXPORT( GB_VDR_READ_14H, get_14 );


/*
   �ٶ�״̬��־
   133Byte    256
   ��10��
 */
u16 gb_vdr_get_15h( u16 indexnum, u8 *p)
{

    int		 i;
    uint8_t	 buf[150];
    u32  addr = 0;
    u8  FCS = 0;



    //    1.  get	address
    addr = VDR_15H_START + indexnum * 256;

    //	  2. read
    sst25_read(addr, buf, 135 );
    //OutPrint_HEX("15H content",buf,135); // debug
    //	   3.  FCS	check
    for(i = 0; i < 134; i++)
        FCS ^= buf[i];
    if(buf[134] != FCS)
    {
        //rt_kprintf("\r\n	15H read fcs error	save:%X  cacu: %X \r\n",buf[134],FCS);
        return  0;
    }
    if(buf[0] == MarkByte)
    {
        memcpy(p, buf + 1, 133);
        return  133;
    }
    else
        return 0;

}

//FINSH_FUNCTION_EXPORT( gb_vdr_get_15h, get_15 );







//==========  write   ------------------------------------------------------------------------------
u16  vdr_creat_08h( u16 indexnum, u8 *p, u16 inLen)
{
    u8  inbuf[128];
    u8  FCS = 0, i = 0;
    u32 inaddress = 0; //  д���ַ

    //  1.  Stuff  Head
    inbuf[0] = MarkByte;         //
    //  2.   Index  get  address
    if(indexnum >= VDR_08_MAXindex)
    {
        Vdr_Wr_Rd_Offset.V_08H_Write = 0;
        indexnum = 0;
    }
    memcpy(inbuf + 1, p, inLen); //  126 bytes
    //   3.   caculate  fcs
    for(i = 0; i < 127; i++)
        FCS ^= inbuf[i];
    inbuf[127] = FCS;
    //    4.  get  address
    inaddress = VDR_08H_START + indexnum * 128;
    //     5 .  write
    sst25_write_back(inaddress, inbuf,  128 );
    return  indexnum;
}


//	666=>>768 rec_size
u16  vdr_creat_09h( u16 indexnum, u8 *p, u16 inLen)
{
    u8  inbuf[669];
    u8  FCS = 0;
    u16 i = 0;
    u32 inaddress = 0; //  д���ַ

    //  1.  Stuff  Head
    inbuf[0] = MarkByte;         //
    //  2.   Index  get  address
    if(indexnum >= VDR_09_MAXindex)
    {
        Vdr_Wr_Rd_Offset.V_09H_Write = 0;
        indexnum = 0;
    }
    memcpy(inbuf + 1, p, inLen); // 666 bytes
    //   3.   caculate  fcs
    for(i = 0; i < 667; i++)
        FCS ^= inbuf[i];

    inbuf[667] = FCS;
    //    4.  get  address
    inaddress = VDR_09H_START + indexnum * 768;

    //  �жϲ����������
    //     5 .  write
    sst25_write_back(inaddress, inbuf,  668 ); // 2+content
    return  indexnum;

}

/*
   �¹��ɵ� Accident Point    234
*/
u16  vdr_creat_10h( u16 indexnum, u8 *p, u16 inLen)
{

    u8  inbuf[256];
    u8  FCS = 0, i = 0;
    u32 inaddress = 0; //  д���ַ

    //  1.  Stuff  Head
    inbuf[0] = MarkByte	;	 //
    //  2.   Index  get  address
    if(indexnum >= VDR_10_MAXindex)
    {
        Vdr_Wr_Rd_Offset.V_10H_Write = 0;
        indexnum = 0;
    }
    memcpy(inbuf + 1, p, inLen);	 // 666 bytes
    //   3.   caculate  fcs
    for(i = 0; i < 235; i++)
        FCS ^= inbuf[i];
    inbuf[235] = FCS;
    //    4.  get	address
    inaddress = VDR_10H_START + indexnum * 256;

    //  �жϲ����������
    // 	5 .  write
    sst25_write_back(inaddress, inbuf, 236 ); // 2+content
    return  indexnum;

}

/***********************************************************
* Function:
* Description:       recordsize 5==>> 64 size
* Input:             100   ��
* Input:
* Output:
* Return:
* Others:
***********************************************************/

u16  vdr_creat_11h( u16 indexnum, u8 *p, u16 inLen)
{


    u8	inbuf[60];
    u8	FCS = 0, i = 0;
    u32 inaddress = 0; //	д���ַ

    //	1.	Stuff  Head
    inbuf[0] = MarkByte;		 //
    //	2.	 Index	get  address
    if(indexnum >= VDR_11_MAXindex)
    {
        Vdr_Wr_Rd_Offset.V_11H_Write = 0;
        indexnum = 0;
    }
    memcpy(inbuf + 1, p, inLen); // 666 bytes
    //	3.	 caculate  fcs
    for(i = 0; i < 51; i++)
        FCS ^= inbuf[i];
    inbuf[51] = FCS;
    //	 4.  get  address
    inaddress = VDR_11H_START + indexnum * 64;

    //	  5 .  write
    sst25_write_back(inaddress, inbuf, 52 ); // 2+content     ����д
    return  indexnum;

}
/*
   �ⲿ�����¼
   ������4k�ļ�¼�1�ζ���

      25 =>>   32
 */

u16  vdr_creat_12h( u16 indexnum, u8 *p, u16 inLen)
{

    u8  inbuf[30];
    u8  FCS = 0, i = 0;
    u32 inaddress = 0; //  д���ַ

    //  1.  Stuff  Head
    inbuf[0] = MarkByte;		//
    //  2.	Index  get	address
    if(indexnum >= VDR_12_MAXindex)
    {
        Vdr_Wr_Rd_Offset.V_12H_Write = 0;
        indexnum = 0;
    }
    memcpy(inbuf + 1, p, inLen); // 666 bytes
    //   3.	caculate  fcs
    for(i = 0; i < 26; i++)
        FCS ^= inbuf[i];
    inbuf[26] = FCS;
    //	4.	get  address
    inaddress = VDR_12H_START + indexnum * 32;

    //	 5 .  write
    sst25_write_back(inaddress, inbuf,  27 ); // 2+content	   ����д
    return	indexnum;

}


/*������4k�ļ�¼�1�ζ���
     7 bytes    >> 8       record size

*/
u16  vdr_creat_13h( u16 indexnum, u8 *p, u16 inLen)
{
    u8  inbuf[10];
    u32 inaddress = 0; //  д���ַ

    //  1.  Stuff  Head
    inbuf[0] = MarkByte;		//
    //  2.	Index  get	address
    if(indexnum >= VDR_13_MAXindex)
    {
        Vdr_Wr_Rd_Offset.V_13H_Write = 0;
        indexnum = 0;
    }
    memcpy(inbuf + 1, p, inLen); // 666 bytes
    //	4.	get  address
    inaddress = VDR_13H_START + indexnum * 8;

    //	 5 .  write
    sst25_write_back( inaddress, inbuf, 8 ); // 2+content	   ����д
    return	indexnum;


}

/*������4k�ļ�¼�1�ζ���
     7 bytes    >> 8 	  record size

*/
u16  vdr_creat_14h( u16 indexnum, u8 *p, u16 inLen)
{
    u8  inbuf[10];
    u32 inaddress = 0; //  д���ַ

    //  1.  Stuff  Head
    inbuf[0] = MarkByte;		//
    //  2.	Index  get	address
    if(indexnum >= VDR_14_MAXindex)
    {
        Vdr_Wr_Rd_Offset.V_14H_Write = 0;
        indexnum = 0;
    }
    memcpy(inbuf + 1, p, inLen); // 666 bytes
    //	4.	get  address
    inaddress = VDR_14H_START + indexnum * 8;
    // rt_kprintf("\r\n write 14 inaddr=0x%00000008X \r\n",inaddress);
    //	 5 .  write
    sst25_write_back( inaddress, inbuf, 8 ); // 2+content	   ����д

    return	indexnum;

}

/*������4k�ļ�¼�1�ζ���
       133   >>  256
*/
u16  vdr_creat_15h( u16 indexnum, u8 *p, u16 inLen)
{

    u8  inbuf[260];
    u8  FCS = 0, i = 0;
    u32 inaddress = 0; //  д���ַ

    //  1.  Stuff	Head
    inbuf[0] = MarkByte; 	 //
    //  2.   Index  get  address
    if(indexnum >= VDR_15_MAXindex)
    {
        Vdr_Wr_Rd_Offset.V_15H_Write = 0;
        indexnum = 0;
    }
    memcpy(inbuf + 1, p, inLen);	// 666 bytes
    //	 3.   caculate	fcs
    for(i = 0; i < 134; i++)
        FCS ^= inbuf[i];
    inbuf[134] = FCS;
    //	  4.  get  address
    inaddress = VDR_15H_START + indexnum * 256;

    //	   5 .	write
    sst25_write_back(inaddress, inbuf,  135); // 2+content	 ����д
    return   indexnum;

}

/*
   �洢08-15  ÿ���������ݴ洢��ʲôλ����
*/
u8  vdr_cmd_writeIndex_save(u8 CMDType, u32 value)
{
    u8     head[448];
    u16    offset = 0;
    u32    Addr_base = 0;
    u32    Save_Addr = 0;
    // u16    InpageOffset=0;  // ҳ��ƫ��
    u8     reg[9];
    u8     Flag_used = 0x01;


    //  1.   Classify
    switch(CMDType)
    {
    case 0x08:
        Addr_base = VDR_WRITEINDEX;
        break;
    case 0x09:
        Addr_base = VDR_WRITEINDEX + SECTORSIZE;
        break;
    case 0x10:
        Addr_base = VDR_WRITEINDEX + 2 * SECTORSIZE;
        break;
    case 0x11:
        Addr_base = VDR_WRITEINDEX + 3 * SECTORSIZE;
        break;
    case 0x12:
        Addr_base = VDR_WRITEINDEX + 4 * SECTORSIZE;
        break;
    case 0x13:
        Addr_base = VDR_WRITEINDEX + 5 * SECTORSIZE;
        break;
    case 0x14:
        Addr_base = VDR_WRITEINDEX + 6 * SECTORSIZE;
        break;
    case 0x15:
        Addr_base = VDR_WRITEINDEX + 7 * SECTORSIZE;
        break;
    default :
        return false;
    }
    //  2 .  Excute
    sst25_read(Addr_base, head, 448); //��ȡǰ448 ���ֽ�
    /*
         448 �ֽ�(ռ512���ֽ�)����״̬��ʾ�Ƿ�д����
         ��3584 ������ addr=base+512+n*8
    */
    for(offset = 0; offset < 448; offset++)
    {
        if(0xFF == head[offset])
            break;
    }

    if(offset == 448)
    {
        sst25_erase_4k(Addr_base); // Erase block
        offset = 0;
    }
    Save_Addr = Addr_base + 512 + (offset << 3);

    memset(reg, 0xff, sizeof(reg));
    // ֻ��4���ֽ�
    reg[0] = (value >> 24);
    reg[1] = (value >> 16);
    reg[2] = (value >> 8);
    reg[3] = (value);

    sst25_write_through(Addr_base + offset, &Flag_used, 1); //  ����״̬λ
    sst25_write_through(Save_Addr, reg, 4);

    return true;
    //                 The  End
}


u32 vdr_cmd_writeIndex_read(u8 CMDType)
{
    u8	   head[448];
    u16    offset = 0;
    u32    Addr_base = 0;
    u32    Read_Addr = 0;
    u32    value = 0;
    u8     reg[9];


    //Wr_Address, Rd_Address  , û��ʲô��ֻ�Ǳ�ʾ���룬����д�۲����
    //  1.   Classify

    //  1.	Classify
    switch(CMDType)
    {
    case 0x08:
        Addr_base = VDR_WRITEINDEX;
        break;
    case 0x09:
        Addr_base = VDR_WRITEINDEX + SECTORSIZE;
        break;
    case 0x10:
        Addr_base = VDR_WRITEINDEX + 2 * SECTORSIZE;
        break;
    case 0x11:
        Addr_base = VDR_WRITEINDEX + 3 * SECTORSIZE;
        break;
    case 0x12:
        Addr_base = VDR_WRITEINDEX + 4 * SECTORSIZE;
        break;
    case 0x13:
        Addr_base = VDR_WRITEINDEX + 5 * SECTORSIZE;
        break;
    case 0x14:
        Addr_base = VDR_WRITEINDEX + 6 * SECTORSIZE;
        break;
    case 0x15:
        Addr_base = VDR_WRITEINDEX + 7 * SECTORSIZE;
        break;
    default :
        return false;
    }
    //  2 .	Excute
    sst25_read(Addr_base, head, 448); //��ȡǰ448 ���ֽ�
    /*
      448 �ֽ�(ռ512���ֽ�)����״̬��ʾ�Ƿ�д����
      ��3584 ������ addr=base+512+n*8
    */

    for(offset = 0; offset < 448; offset++)
    {
        if(0xFF == head[offset])
            break;
    }
    offset--;  // ��һ������Ϊ0xFF
    Read_Addr = Addr_base + 512 + (offset << 3);
    sst25_read(Read_Addr, reg, 4);
    value = (reg[0] << 24) + (reg[1] << 16) + (reg[2] << 8) + reg[3];
    return value;
}

//---------------------------------------------------------------------------------
#ifdef JT808_TEST_JTB

/*
��ѹ�����ݣ���7bitתΪ8bit
*/

static void gb_decompress_data( uint8_t *src, uint8_t *dst )
{
    uint8_t *psrc	= src;
    uint8_t *pdst	= dst;
    uint8_t c1, c2, c3, c4, c5, c6, c7;
    uint8_t i;
    /*�ֽھ���7bit �ٶ�ѹ����ı���53byte*/
    for( i = 0; i < 7; i++ )
    {
        c1	= *psrc++;
        c2	= *psrc++;
        c3	= *psrc++;
        c4	= *psrc++;
        c5	= *psrc++;
        c6	= *psrc++;
        c7	= *psrc++;

        *pdst++ = c1 & 0xfe;
        *pdst++ = ( ( c1 & 0x01 ) << 7 ) | ( ( c2 >> 1 ) & 0x7e );
        *pdst++ = ( ( c2 & 0x03 ) << 6 ) | ( ( c3 >> 2 ) & 0x3e );
        *pdst++ = ( ( c3 & 0x07 ) << 5 ) | ( ( c4 >> 3 ) & 0x1e );
        *pdst++ = ( ( c4 & 0x0f ) << 4 ) | ( ( c5 >> 4 ) & 0x0e );
        *pdst++ = ( ( c5 & 0x1f ) << 3 ) | ( ( c6 >> 5 ) & 0x06 );
        *pdst++ = ( ( c6 & 0x3f ) << 2 ) | ( ( c7 >> 6 ) & 0x02 );
        *pdst++ = ( ( c7 & 0x7f ) << 1 );
    }
    for( i = 0; i < 74; i++ )
    {
        *pdst++ = *psrc++;
    }
}



/*
��ʻ�ٶȼ�¼
���������Լ������ݣ��Լ���
����126�ֽ�ÿ����
48*60=2880��

ÿ5�� ��һ��  630�ֽ�
ÿ6�� ��һ��  756�ֽ�
*/

uint8_t gb_get_08h( uint8_t *pout, u16 packet_in )
{
    //  2014 �� 2 �·�
    static uint8_t	month_08 = STRT_MON, day_08_const = STRT_DAY; // ��ʼ����
    u32  minute_total = 0;
    uint8_t day_08, hour_08 = 0, min_08 = 0;
    static uint32_t addr_08		= GuoJian_VDR_08H_09H_START;
    uint32_t count_08	= 0;
    uint8_t			buf[128], data[135];
    uint8_t			*p = RT_NULL, *pdump;
    int				i, j, k;
#ifdef DBG_VDR
    p = testbuf;
#else
    p = pout;
#endif
    // ��������İ���Ż�ȡ��Ӧ����Ϣ
    // 1. ������ת��Ϊ����
    minute_total = day_08_const * 24 * 60 - (packet_in - 1) * 4; //      2-28  00:00

    min_08 = minute_total % 60; // ����
    day_08 = minute_total / 1440; //  �� ÿ��1440 ����
    hour_08 = (minute_total - day_08 * 1440) / 60; // Сʱ


    count_08 = (packet_in - 1) * 4; // ÿ����ʼ

    for( i = 0; i < 4; i++ )//5
    {
        addr_08 = 0x00300000 + hour_08 * 8192 + min_08 * 128 + 128;
        if( addr_08 >= GuoJian_VDR_08H_09H_END )
        {
            addr_08 = GuoJian_VDR_08H_09H_START + 128;
        }

        sst25_read(addr_08, buf, 128 );
        gb_decompress_data( buf, data );
        buf[0]	= HEX_TO_BCD( STRT_YEAR);         /*year*/
        buf[1]	= HEX_TO_BCD( month_08 );   /*month*/
        buf[2]	= HEX_TO_BCD( day_08 );     /*day*/
        buf[3]	= HEX_TO_BCD( hour_08 );    /*hour*/
        buf[4]	= HEX_TO_BCD( min_08 );     /*miniute*/
        buf[5]	= HEX_TO_BCD( 0 );          /*sec*/

        for( j = 0; j < 60; j++ )
        {
            buf[j * 2 + 6]	= data[j];
            buf[j * 2 + 7]	= data[60 + j];
        }
        memcpy( pout + i * 126, buf, 126 );

        count_08++;


        //rt_kprintf( "\r\nVDR>08H(%d) 14-%02d-%02d %02d:%02d \r\n", count_08, month_08, day_08, hour_08, min_08 );
        if( min_08 == 0 )
        {
            min_08 = 59;
            if( hour_08 == 0 )
            {
                day_08--;
                hour_08 = 24;
            }
            hour_08--;
        }
        else
        {
            min_08--;
        }

        /*�������*/
        pdump = buf;
#ifdef  DBG_VDR
        for( j = 0; j < 6; j++ )
        {
            rt_kprintf( "%02x ", *pdump++ );
        }
        for( j = 0; j < 6; j++ )
        {
            rt_kprintf( "\r\n>" );
            for( k = 0; k < 20; k++ )
            {
                rt_kprintf( "%02x ", *pdump++ );
            }
        }
#endif
        //addr_08 += 128;
    }
    return 126 * 4;//6
}

//FINSH_FUNCTION_EXPORT( gb_get_08h, gb_get_08 );


/*
λ����Ϣ
360Сʱ   ��ÿСʱ666�ֽ�
*/

uint8_t gb_get_09h( uint8_t *pout, u16 packet_in )
{
    //  2014 �� 2 �·�
    static uint8_t	month_09 = STRT_MON, day_09_const = STRT_DAY, hour_09_const = 23;  //true
    uint8_t day_09 = 0, hour_09 = 0; //true
    //static uint8_t	month_09 = 4, day_09 =18, hour_09 = 6;//half change
    u32  hour_total = 0;
    static uint32_t addr_09		= GuoJian_VDR_08H_09H_START;
    static uint32_t count_09	= 0;
    uint8_t			buf[128], data[135];
    uint8_t			*p = RT_NULL;
    int				i, j, k;
    //  39.906678 =           116.192092   +
    u32 lati_ini = 0x16D5B47, longi_ini = 0x424C537;
    u32 lati = 0, longi = 0;
    u8  count = 0;
#ifdef DBG_VDR
    pout = testbuf;
#endif
    p = pout;


    count_09 = packet_in; // ÿ����ʼ
    //  ����ʼ����ת����Сʱ
    hour_total = day_09_const * 24 + hour_09_const - count_09;

    day_09 = hour_total / 24;
    hour_09 = hour_total % 24;


    addr_09 = 0x00300000 + hour_09 * 8192 + 128;                /*��λ��Сʱ�ĵ�һ��������*/
    if( addr_09 >= GuoJian_VDR_08H_09H_END )
    {
        addr_09 = GuoJian_VDR_08H_09H_START + 128;
    }

    *p++	= HEX_TO_BCD( STRT_YEAR );                                 /*year*/
    *p++	= HEX_TO_BCD( month_09 );                           /*month*/
    *p++	= HEX_TO_BCD( day_09 );                             /*day*/
    *p++	= HEX_TO_BCD( hour_09 );                            /*hour*/
    *p++	= HEX_TO_BCD( 0 );                                  /*miniute*/
    *p++	= HEX_TO_BCD( 0 );                                  /*sec*/

    count = 0;
    for( i = addr_09; i < addr_09 + ( 60 * 128 ); i += 128 )    /*����60����������*/
    {
        //SST25V_BufferRead( buf, i, 128 );
        sst25_read(i, buf, 128);
        gb_decompress_data( buf, data );
        for( j = 0; j < 60; j++ )
        {
            if( data[60 + j] & 0x01 )                           /*���ǲ������ٶ�*/
            {
                break;
            }
        }
        if( j < 60 )
        {
            /*λ����Ϣ*/
            /*Note: �н�ĵ�������Ϊ����γ39��26����41��03��������115��25���� 117��30����
            115��25��=0x0420ABD0                                    117��30��=0x0433BEA0
            39��26��=2366��=0x016905E0                               41��03��=0x0177D2F0
            �߶�30-40 ��

            1�� <=>  111 ����     0.0001 �� =0.18 ��         ÿ��8����λ��1.44�� ���ٶ���5.2KM/H
            ÿ������ʻ 1.46x60=90��==500   ÿ��������500
            longi �� lati����

            ���ﾭγ��һ���滻 �������
            note2:    10���ֽڵ����
            4 ���ֽ�  ����
            4 ���ֽ�γ��
            2 ���ֽں���
            */


#if 1
            //  longitude
            if(packet_in % 2)
            {
                longi = longi_ini + count * 6000; // ÿ������100��<=>    500 ����λ

                lati = lati_ini + count * 6000; // lati
            }
            else
            {
                longi = longi_ini + (62 - count) * 6000;
                lati = lati_ini + (62 - count) * 6000; // lati
            }

            *p++ = (u8)(longi >> 24);
            *p++ = (u8)(longi >> 16);
            *p++ = (u8)(longi >> 8);
            *p++ = (u8)(longi);
            //  lati
            *p++ = (u8)(lati >> 24);
            *p++ = (u8)(lati >> 16);
            *p++ = (u8)(lati >> 8);
            *p++ = (u8)(lati);
            //  Height
            *p++ = 0x00;
            *p++ = 30 + (rt_tick_get() % 5);
#endif
            //----------------------------------------------------------------------------
            *p++ = 5 + (rt_tick_get() % 10); // data[j];                                     /*�ٶ�*/
        }
        else
        {
            memcpy( p, "\x7F\xFF\xFF\xFF\x7F\xFF\xFF\xFF\x00\x00\x00", 11 );
            p += 11;
        }
        count++;
    }
    rt_kprintf( "\r\nVDR>09H(%d) 14-%02d-%02d %02d \r\n", count_09, month_09, day_09, hour_09 );

    if( hour_09 == 0 )
    {
        day_09--;
        hour_09 = 24;
    }
    hour_09--;

#ifdef  DBG_VDR

    p = pout;
    for( i = 0; i < 6; i++ )
    {
        rt_kprintf( "%02x ", *p++ );
    }
    for( i = 0; i < 60; i++ )
    {
        rt_kprintf( "\r\n>" );
        for( j = 0; j < 11; j++ )
        {
            rt_kprintf( "%02x ", *p++ );
        }
    }
#endif
    return 666;
}

//FINSH_FUNCTION_EXPORT( gb_get_09h, get_09 );


/*
�¹��ɵ�
234Byte
��100��
*/
uint8_t gb_get_10h( uint8_t *pout )
{
    static uint32_t addr_10 = GuoJian_VDR_10H_START;
    uint8_t			buf[240];
    uint8_t			*p;
    uint32_t		i;
#ifdef DBG_VDR
    pout = testbuf;
#endif
    p = pout;

    if( addr_10 > GuoJian_VDR_10H_END )
    {
        addr_10 = GuoJian_VDR_10H_START;
    }
    //SST25V_BufferRead( buf, addr_10, 234 );
    sst25_read(addr_10, buf, 234);
    //--------------------------------------------------
    // �����滻һ��	 14 ��2 �·�
    buf[0] = HEX_TO_BCD( STRT_YEAR ); //year
    buf[1] = HEX_TO_BCD( STRT_MON); // month

    //  ״̬���б仯
    for(i = 0; i < 80; i = i + 2)
        buf[25 + i] = buf[25 + i] & (rt_tick_get() % 7);



    // �滻��γ��
    buf[224] = 0x04; // longi
    buf[225] = 0x24 + (rt_tick_get() % 6);
    buf[226] = buf[226] + (rt_tick_get() % 3) * 3;

    buf[228] = 0x01; //lati
    buf[229] = 0x6D + rt_tick_get() % 3;
    buf[230] = buf[230] + 3 * (rt_tick_get() % 5);

    buf[232] = 0x00; // height
    buf[233] = 30 + (rt_tick_get() % 5);
    //--------------------------------------------------
    memcpy( p, buf, 234 );
    addr_10 += 256;

#ifdef  DBG_VDR
    rt_kprintf( "\r\nVDR>10H" );
    p = pout;
    for( i = 0; i < 234; i++ )
    {
        if( i % 8 == 0 )
        {
            rt_kprintf( "\r\n" );
        }
        rt_kprintf( "%02x ", *p++ );
    }
#endif
    return 234;
}

//FINSH_FUNCTION_EXPORT( gb_get_10h, get_10 );


/*��ʱ��ʻ��¼
50Bytes   100��
*/

uint8_t gb_get_11h( uint8_t *pout, u16 packet_in )
{
    static uint8_t	month_11 = STRT_MON, day_11_const = STRT_DAY; // ��ʼ����
    u32  minute_total = 0;
    static uint32_t addr_11 = GuoJian_VDR_11H_START;
    uint8_t			buf[64];
    uint8_t			*p = RT_NULL;
    uint32_t		i, j;
    uint8_t day_11, hour_11 = 0, min_11 = 0;
    u8  value_reg = 0;


#ifdef DBG_VDR
    pout = testbuf;
#endif
    p = pout;

    if( addr_11 > GuoJian_VDR_11H_END )
    {
        addr_11 = GuoJian_VDR_11H_START;
    }
    for( j = 0; j < 1; j++ )
    {
        //SST25V_BufferRead( buf, addr_11, 50 );
        sst25_read(addr_11, buf, 50);
        //--------------------------------------------------
        // �����滻һ��	 14 ��2 �·�
        /*



        */
        //��ʼʱ������
        buf[18] = HEX_TO_BCD( STRT_YEAR );
        buf[19] = HEX_TO_BCD( STRT_MON); //month

        minute_total = day_11_const * 24 * 60 - (packet_in) * 241 - packet_in * 30; //	  2-28	00:00   241+30=

        min_11 = minute_total % 60; // ����
        day_11 = minute_total / 1440; //  �� ÿ��1440 ����
        hour_11 = (minute_total - day_11 * 1440) / 60; // Сʱ

        buf[20] = HEX_TO_BCD(day_11); // day
        buf[21] = HEX_TO_BCD(hour_11); // hour
        buf[22] = HEX_TO_BCD(min_11); // min

        //
        // ����ʱ������
        buf[24] = HEX_TO_BCD( STRT_YEAR );
        buf[25] = HEX_TO_BCD( STRT_MON);

        minute_total = minute_total + 241;


        min_11 = minute_total % 60; // ����
        day_11 = minute_total / 1440; //  �� ÿ��1440 ����
        hour_11 = (minute_total - day_11 * 1440) / 60; // Сʱ

        buf[26] = HEX_TO_BCD(day_11); // day
        buf[27] = HEX_TO_BCD(hour_11); // hour
        buf[28] = HEX_TO_BCD(min_11); // min

        // ��ʼlongi
        buf[30] = 0x04;
        buf[31] = 0x25 + (rt_tick_get() % 2);

        // ��ʼ laiti
        buf[34] = 0x01;
        buf[35] = 0x6D - (rt_tick_get() % 3);

        //  ��ʼ�߳�
        buf[38] = 0x00;
        buf[39] = 30 + (rt_tick_get() % 5);

        // ����longi
        buf[40] = 0x04;
        buf[41] = 0x27;

        // ���� laiti
        buf[44] = 0x01;
        buf[45] = 0x6D;

        //  �����߳�
        buf[48] = 0x00;
        buf[49] = 30 + (rt_tick_get() % 5);

        //--------------------------------------------------
        for(i = 0; i < 50; i++)		/*ƽ̨����ת�塣SHIT....*/
        {
            if(buf[i] == 0x7d) buf[i] = 0x7C;
            if(buf[i] == 0x7E) buf[i] = 0x7F;
        }
        memcpy( p + j * 50, buf, 50 );
        addr_11 += 64;
    }

#ifdef  DBG_VDR
    rt_kprintf( "\r\nVDR>11H @0x%08x", addr_11 );
    p = pout;
    for( i = 0; i < 500; i++ )
    {
        //WatchDog_Feed( );
        if( i % 8 == 0 )
        {
            rt_kprintf( "\r\n" );
        }
        rt_kprintf( "%02x ", *p++ );
    }
#endif
    return 50;
}

//FINSH_FUNCTION_EXPORT( gb_get_11h, get_11 );


/*��ʻԱ��ݵ�¼
25Bytes   200��
*/

uint8_t gb_get_12h( uint8_t *pout, u16 packet_in )
{

    static uint8_t	month_11 = STRT_MON, day_11_const = STRT_DAY; // ��ʼ����
    u32  minute_total = 0, reg_min = 0;
    static uint32_t addr_12 = GuoJian_VDR_12H_START;
    uint8_t			buf[26];
    uint8_t			*p;
    uint32_t		i, j;

    uint8_t day_12, hour_12 = 0, min_12 = 0;
    u8  value_reg = 0;
#ifdef DBG_VDR
    pout = testbuf;
#endif
    p = pout;

    if( addr_12 > GuoJian_VDR_12H_END )
    {
        addr_12 = GuoJian_VDR_12H_START;
    }

    reg_min = day_11_const * 24 * 60 - (packet_in - 1) * 2401;
    for( j = 0; j < 20; j++ )
    {
        //SST25V_BufferRead( buf, addr_12, 25 );
        sst25_read(addr_12, buf, 25);
        //--------------------------------------------------
        // �����滻һ��	 14 ��2 �·�

        buf[0] = HEX_TO_BCD( STRT_YEAR );
        buf[1] = HEX_TO_BCD( STRT_MON); //month

        minute_total = reg_min - (j) * 120; //	  ÿ2Сʱһ��

        min_12 = minute_total % 60; // ����
        day_12 = minute_total / 1440; //  �� ÿ��1440 ����
        hour_12 = (minute_total - day_12 * 1440) / 60; // Сʱ

        buf[2] = HEX_TO_BCD(day_12); // day
        buf[3] = HEX_TO_BCD(hour_12); // hour
        buf[4] = HEX_TO_BCD(min_12); // min
        //--------------------------------------------------
        memcpy( p + j * 25, buf, 25 );
        addr_12 += 32;
    }
#ifdef DBG_VDR
    rt_kprintf( "\r\nVDR>12H @0x%08x", addr_12 );
    p = pout;
    for( i = 0; i < 500; i++ )
    {
        if( i % 8 == 0 )
        {
            rt_kprintf( "\r\n" );
        }
        rt_kprintf( "%02x ", *p++ );
    }
#endif
    return 500;
}

//FINSH_FUNCTION_EXPORT( gb_get_12h, get_12 );


/*�ⲿ�����¼
7�ֽ� 100��**/
uint8_t gb_get_13h( uint8_t *pout )
{
    static uint32_t addr_13 = GuoJian_VDR_13H_START;
    uint8_t			buf[8];
    uint8_t			*p;
    uint32_t		i, j;
    u8  day = 0;
#ifdef DBG_VDR
    pout = testbuf;
#endif
    p = pout;

    if( addr_13 > GuoJian_VDR_13H_END )
    {
        addr_13 = GuoJian_VDR_13H_START;
    }
    for( j = 0; j < 100; j++ )
    {
        //SST25V_BufferRead( buf, addr_13, 8 );
        sst25_read(addr_13, buf, 8);
        //--------------------------------------------------
        // �����滻һ��	 14 ��2 �·�
        buf[0] = HEX_TO_BCD( STRT_YEAR );
        if(j < 13)
            buf[1] = HEX_TO_BCD( STRT_MON) + 1;
        else
        {
            buf[1] = HEX_TO_BCD( STRT_MON);
            day = (buf[2] >> 4) * 10 + (buf[2] & 0x0F);
            if(day >= 5)
                day = day - 2;
            buf[2] = HEX_TO_BCD(day);
        }
        //--------------------------------------------------
        memcpy( p + j * 7, buf, 7 );

        addr_13 += 8;
    }
#ifdef DBG_VDR
    rt_kprintf( "\r\nVDR>13H @0x%08x\r\n", addr_13 );
    p = pout;
    for( i = 0; i < 700; i++ )
    {
        if( i % 7 == 0 )
        {
            rt_kprintf( "\r\n" );
        }
        rt_kprintf( "%02x ", *p++ );
    }
#endif

    return 700;
}

//FINSH_FUNCTION_EXPORT( gb_get_13h, get_13 );


/*��¼�ǲ���
7�ֽ� 100��*/
uint8_t gb_get_14h( uint8_t *pout )
{
    static uint32_t addr_14 = GuoJian_VDR_14H_START;
    uint8_t			buf[8];
    uint8_t			*p;
    uint32_t		i, j;
    u8 day = 0;
#ifdef DBG_VDR
    pout = testbuf;
#endif
    p = pout;
    if( addr_14 > GuoJian_VDR_14H_END )
    {
        addr_14 = GuoJian_VDR_14H_START;
    }
    for( j = 0; j < 100; j++ )
    {
        //SST25V_BufferRead( buf, addr_14, 8 );
        sst25_read(addr_14, buf, 8);
        //--------------------------------------------------
        // �����滻һ��	 14 ��2 �·�
        buf[0] = HEX_TO_BCD( STRT_YEAR );
        buf[1] = HEX_TO_BCD( STRT_MON);
        day = (buf[2] >> 4) * 10 + (buf[2] & 0x0F);
        day = day + 12;
        buf[2] = HEX_TO_BCD(day);
        //--------------------------------------------------
        memcpy( p + j * 7, buf, 7 );
        addr_14 += 8;
    }

#ifdef  DBG_VDR
    rt_kprintf( "\r\nVDR>14H @0x%08x\r\n", addr_14 );
    p = pout;
    for( i = 0; i < 700; i++ )
    {
        if( i % 7 == 0 )
        {
            rt_kprintf( "\r\n" );
        }
        rt_kprintf( "%02x ", *p++ );
    }
#endif
    return 700;
}

//FINSH_FUNCTION_EXPORT( gb_get_14h, get_14 );


/*
�ٶ�״̬��־
133Byte
��10��
*/
uint8_t gb_get_15h( uint8_t *pout , u16 packet_in )
{
    static uint32_t addr_15 = GuoJian_VDR_15H_START;
    uint8_t			buf[140];
    uint8_t			*p;
    uint32_t		i, j;
    u8  value_reg = 0;
    u8  day = 0;
#ifdef DBG_VDR
    pout = testbuf;
#endif
    p = pout;

    if( addr_15 > GuoJian_VDR_15H_END )
    {
        addr_15 = GuoJian_VDR_15H_START;
    }
    for( i = 0; i < 1; i++ )
    {
        //SST25V_BufferRead( buf, addr_15, 134 );
        sst25_read(addr_15, buf, 134);
        //--------------------------------------------------
        // �����滻һ��	 14 ��2 �·�
        if(packet_in % 2)
            buf[0] = 0x02; // �쳣 �������5  ����
        else
            buf[0] = 0x01; // ���� �������5  ����


        buf[1] = HEX_TO_BCD( STRT_YEAR ); // ��ʼʱ��
        if(packet_in <= 4)
            buf[2] = HEX_TO_BCD( STRT_MON ) + 1;
        else
        {
            buf[2] = HEX_TO_BCD( STRT_MON);
            day = (buf[3] >> 4) * 10 + (buf[3] & 0x0F);
            if(day >= 5)
                day = day - 2;
            buf[3] = HEX_TO_BCD(day);
        }

        // �жϷ���
        value_reg = ((buf[5] >> 4) * 10) + (buf[5] & 0x0F);
        if(value_reg >= 55)
        {
            value_reg = value_reg - 5;
            buf[5] = ((value_reg / 10) << 4) + (value_reg % 10);
        }
        buf[6] = 0; // ����0

        buf[7] = 0x14; // ����ʱ��
        buf[8] = buf[2];
        buf[9] = buf[3];

        value_reg = value_reg + 5;
        buf[11] = ((value_reg / 10) << 4) + (value_reg % 10); // 5 ����
        buf[12] = 0; // ����0
        //--------------------------------------------------
        value_reg = 13; // ��һ���ٶ��ֶ�

        for(j = 0; j < 60; j++) // �ٶ�Ҫ����40
        {
            if(buf[0] == 0x02) // �쳣
            {
                buf[value_reg] = 51 + (rt_tick_get() % 3);
            }
            else if(buf[0] == 0x01) // �ٶ�����
            {
                buf[value_reg] = 45 + (rt_tick_get() % 3);
            }
            value_reg++;
            buf[value_reg++] = 45; //�ο��ٶ�
        }
        //

        memcpy( p + i * 133, buf, 133 );

#ifdef  DBG_VDR
        rt_kprintf( "\r\nVDR>15H @0x%08x", addr_15 );
        p = pout;
        for( i = 0; i < 133; i++ )
        {
            if( i % 8 == 0 )
            {
                rt_kprintf( "\r\n" );
            }
            rt_kprintf( "%02x ", *p++ );
        }
#endif
        addr_15 += 256;
    }
    return 133;
    //	return 133 * 5;
}

//FINSH_FUNCTION_EXPORT( gb_get_15h, get_15 );

#endif
/************************************** The End Of File **************************************/
