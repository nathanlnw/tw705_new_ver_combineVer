/*
 * File      : startup.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-08-31     Bernard      first implementation
 * 2011-06-05     Bernard      modify for STM32F107 version
 */

#include <rthw.h>
#include <rtthread.h>

#include "include.h"
#include "board.h"
#include "jt808.h"
#include "hmi.h"
#include "sle4442.h"
#include "printer.h"
#include "rs485.h"

#include "gps.h"
#include "jt808_param.h"
#include "jt808_GB19056.h"
#include <finsh.h>


#define PCB_VER2_GPIO				GPIOA
#define PCB_VER2_PIN				GPIO_Pin_14

#define PCB_VER1_GPIO				GPIOA
#define PCB_VER1_PIN				GPIO_Pin_13

#define PCB_VER0_GPIO				GPIOB
#define PCB_VER0_PIN				GPIO_Pin_3

/**
 * @addtogroup STM32
 */

/*@{*/

//extern int  rt_application_init(void);
#ifdef RT_USING_FINSH
extern uint16_t	mem_ID;				//�洢оƬ�ĳ��Ҵ���,SST25VF032(0xBF4A),W25Q64FV(0xEF16)
extern void finsh_system_init( void );


extern void finsh_set_device( const char *device );


#endif

#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define STM32_SRAM_BEGIN ( &Image$$RW_IRAM1$$ZI$$Limit )
#elif __ICCARM__
#pragma section="HEAP"
#define STM32_SRAM_BEGIN ( __segment_end( "HEAP" ) )
#else
extern int __bss_end;
#define STM32_SRAM_BEGIN ( &__bss_end )
#endif
extern unsigned char			printf_on;		///Ϊ1��ʾ��ӡ������Ϣ��


/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed( u8 *file, u32 line )
{
    rt_kprintf( "\nWrong parameter value detected on\n" );
    rt_kprintf( "       file  %s\n", file );
    rt_kprintf( "       line  %d\n", line );

    while( 1 )
    {
        ;
    }
}


void printf_set(uint8_t num)
{
    printf_on = num;
    rt_kprintf("\n printf_on = %d", printf_on);
}
FINSH_FUNCTION_EXPORT( printf_set, printf_set );

/*********************************************************************************
  *��������:void printf_pro( void )
  *��������:��ΪĬ���ǲ���ӡlog��Ϣ�ģ��ú����Ĺ����ǵ�����ʱ��⵽�˽������������������Ϳ�����ӡlog��Ϣ
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2014-05-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void printf_pro( void )
{
    uint8_t st;
    GPIO_InitTypeDef	GPIO_InitStructure;

    printf_on = 0;

    GPIO_StructInit(&GPIO_InitStructure);
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE );

    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_25MHz;
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_3;
    GPIO_Init( GPIOD, &GPIO_InitStructure );
    st = GPIO_ReadInputDataBit( GPIOD, GPIO_Pin_3 );	///����в˵������������ӡ������Ϣ
    if(st == 0)
    {
        printf_on = 1;
        if(donotsleeptime < 600)
            donotsleeptime = 600;
    }
    st = 0;
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_8;
    GPIO_Init( GPIOA, &GPIO_InitStructure );
    st += GPIO_ReadInputDataBit( GPIOA, GPIO_Pin_8 );	///�����down��������
    GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_9;
    GPIO_Init( GPIOC, &GPIO_InitStructure );
    st += GPIO_ReadInputDataBit( GPIOC, GPIO_Pin_9 );	///�����up��������
    if(st == 0)
    {
        device_factory_oper = 1;
    }
    else
    {
        device_factory_oper = 0;
    }
}

/*********************************************************************************
  *��������:void PowerON_OFF_GSM_GPS(void)
  *��������:������ʱ���ȿ���һ��GSMģ�飬Ȼ���ٹرգ�Ŀ���Ƿ�ֹ��ص�ѹ���ͣ����flash������ʧ
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
void PowerON_OFF_GSM_GPS(void)
{
    uint32_t			counter = 0;
    GPIO_InitTypeDef	GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );

    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
    ///��ʼ��GPSģ���Դ��Ϊ���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init( GPIOD, &GPIO_InitStructure );
    GPIO_SetBits( GPIOD, GPIO_Pin_10 );			///��GPSģ��

    ///��ʼ��GPSģ���Դ��Ϊ���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init( GPIOD, &GPIO_InitStructure );

    while(counter++ < 7000000)
    {
        ;
    }
    counter = 0;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init( GPIOD, &GPIO_InitStructure );
    GPIO_SetBits( GPIOD, GPIO_Pin_13 );			///��GSMģ��


    while(counter++ < 7000000)
    {
        ;
    }
    GPIO_ResetBits( GPIOD, GPIO_Pin_13 );		///�ر�GSMģ��
    while(counter++ < 3000000)
    {
        ;
    }
}

/*********************************************************************************
  *��������:void check_pcb_version(void)
  *��������:���pcb�汾��Ϣ
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
void check_pcb_version(void)
{
    uint8_t  			st;
    GPIO_InitTypeDef	GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE );

    GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed 	= GPIO_Speed_25MHz;

    GPIO_InitStructure.GPIO_Pin 	= PCB_VER2_PIN;
    GPIO_Init( PCB_VER2_GPIO, &GPIO_InitStructure );
    GPIO_InitStructure.GPIO_Pin 	= PCB_VER1_PIN;
    GPIO_Init( PCB_VER1_GPIO, &GPIO_InitStructure );
    GPIO_InitStructure.GPIO_Pin 	= PCB_VER0_PIN;
    GPIO_Init( PCB_VER0_GPIO, &GPIO_InitStructure );

    if(GPIO_ReadInputDataBit( PCB_VER2_GPIO, PCB_VER2_PIN ))	///PCB VER BIT(2)λ
    {
        HardWareVerion |= BIT(2);
    }
    if(GPIO_ReadInputDataBit( PCB_VER1_GPIO, PCB_VER1_PIN ))	///PCB VER BIT(2)λ
    {
        HardWareVerion |= BIT(1);
    }
    if(GPIO_ReadInputDataBit( PCB_VER0_GPIO, PCB_VER0_PIN ))	///PCB VER BIT(2)λ
    {
        HardWareVerion |= BIT(0);
    }
    rt_kprintf("\n HardWareVerion=%d", HardWareVerion);
}


/**
 * This function will startup RT-Thread RTOS.
 */
void rtthread_startup( void )
{
    rt_hw_board_init( );
    printf_pro( );
    rt_show_version( );
    PowerON_OFF_GSM_GPS( );
    check_pcb_version();
    rt_system_tick_init( );
    rt_system_object_init( );
    rt_system_timer_init( );

    rt_system_heap_init( (void *)STM32_SRAM_BEGIN, (void *)STM32_SRAM_END );
    rt_system_scheduler_init( );

    rt_device_init_all( );
    rt_kprintf( "\nrcc.csr=%08x", RCC->CSR );

    sst25_init( );      /*�ڴ˳�ʼ��,gsm���ܶ�ȡ����������app_thread�в�����ִ��*/
    if(mem_ID == 0xBF4A)
    {
        rt_kprintf("\n �洢оƬ:SST25VF032");
    }
    else
    {
        rt_kprintf("\n �洢оƬ:W25Q64FV");
    }
#ifdef TJ_MIXERS_V01
    rt_kprintf("\n ���賵����!");
#endif
#ifdef BJ_XGX_V01
    rt_kprintf("\n �����¹��߳���!");
#endif
#ifdef TJ_TAXI_V01
    rt_kprintf("\n �����⳵����!");
#endif
    //rt_kprintf("\n MEM_ID=%04X",mem_ID);
    rt_sem_init( &sem_dataflash, "sem_df", 0, RT_IPC_FLAG_FIFO );
    rt_sem_release( &sem_dataflash );
    key_lcd_port_init( );

    //  GB19056   init  �ź���
    rt_sem_init(&GB_RX_sem, "gbRxSem", 0, 0);


    param_load();		/*����ϵͳ������û��ʹ���ź���*/

    //   GB19056  ��س�ʼ�� ����DF ������ȡ
    GB_Drv_init();

    data_load();
    bkpsram_init( );
    //rt_kprintf("\n%d>�����ͺ�:TD%04x",rt_tick_get(),jt808_param.id_0xF013);
    send_info();
    jt808_gps_data_init( );
    gps_init( );

    printer_driver_init();
    usbh_init( );
    hmi_init( );

    //spi_sd_init( );
    //SD_test_init();
    rt_application_init( );
    RS485_init( );

    rs232_init( );
    gsm_init( );
    jt808_init( );

#ifdef RT_USING_FINSH
    /* init finsh */
    finsh_system_init( );
    finsh_set_device( FINSH_DEVICE_NAME );
#endif

    /* init timer thread */
    rt_system_timer_thread_init( );

    /* init idle thread */
    rt_thread_idle_init( );

    /* start scheduler */
    rt_system_scheduler_start( );

    /* never reach here */
    return;
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
int main( void )
{
    /* disable interrupt first */
    rt_hw_interrupt_disable( );
    /* startup RT-Thread RTOS */
    rtthread_startup( );

    return 0;
}

/*@}*/
