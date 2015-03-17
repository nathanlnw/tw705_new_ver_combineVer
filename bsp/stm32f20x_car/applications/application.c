/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */


/**
 * @addtogroup STM32
 */
/*@{*/

#include <stdio.h>

#include "include.h"
#include <board.h>
#include <rtthread.h>

#include "rtc.h"
#include "jt808_gps.h"
#include "jt808_util.h"
#include "jt808_param.h"
#include "jt808_GB19056.h"


#include "ffconf.h"
#include "ff.h"
#include "SPI_SD_driver.h"
#include <finsh.h>
#include "diskio.h"
#include "LCD_Driver.h"


extern void mma8451_process(void);
extern void jt808_vehicle_process(void);
extern void CAN_init(void);


/*
   通过gps语句触发的1秒定时
   未定位时也有疲劳驾驶
 */
static void adjust_mytime_now( void )
{
    uint8_t year, month, day, hour, minute, sec;

    sec		= SEC( mytime_now );
    minute	= MINUTE( mytime_now );
    hour	= HOUR( mytime_now );
    day		= DAY( mytime_now );
    month	= MONTH( mytime_now );
    year	= YEAR( mytime_now );
    sec++;

    if( sec == 60 )
    {
        sec = 0;
        minute++;
    }
    if( minute == 60 )
    {
        minute = 0;
        hour++;
    }
    if( hour == 24 )
    {
        hour = 0;
        day++;
    }
    if( ( month == 4 ) || ( month == 6 ) || ( month == 9 ) || ( month == 11 ) )
    {
        if( day == 31 )
        {
            day = 1;
            month++;
        }
    }
    else if( month == 2 )
    {
        if( year % 4 == 0 ) /*闰年29天*/
        {
            if( day == 30 )
            {
                day = 1;
                month++;
            }
        }
        else
        {
            if( day == 29 )
            {
                day = 1;
                month++;
            }
        }
    }
    else if( day == 32 )
    {
        day = 1;
        month++;
    }
    if( month == 13 )
    {
        month = 1;
        year++;
    }
    mytime_now = MYDATETIME( year, month, day, hour, minute, sec );
}


/**/
void synctime( void )
{
    RTC_TimeTypeDef ts;
    RTC_DateTypeDef ds;
    unsigned int	year, month, day;
    unsigned int	hour, minute, sec;
    uint32_t		utc;

    RTC_GetTime( RTC_Format_BIN, &ts );
    RTC_GetDate( RTC_Format_BIN, &ds );
    year	= ds.RTC_Year + 2000;
    month	= ds.RTC_Month;
    day		= ds.RTC_Date;
    hour	= ts.RTC_Hours;
    minute	= ts.RTC_Minutes;
    sec		= ts.RTC_Seconds;

    if( 0 >= (int)( month -= 2 ) )    /**//* 1..12 -> 11,12,1..10 */
    {
        month		+= 12;              /**//* Puts Feb last since it has leap day */
        year	-= 1;
    }

    utc = ( ( ( (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * month / 12 + day ) +
                year * 365 - 719499
              ) * 24 + hour      /**//* now have hours */
            ) * 60 + minute         /**//* now have minutes */
          ) * 60 + sec;          /**//* finally seconds */

    if( utc >= jt808_param_bk.utctime )
    {
        utc_now		= utc;
        mytime_now	= MYDATETIME( ds.RTC_Year, ds.RTC_Month, ds.RTC_Date, ts.RTC_Hours, ts.RTC_Minutes, ts.RTC_Seconds);
        /*
        if( utc - jt808_param_bk.utctime < 3600 * 72 )		//72小时以内，认为准确(259200 = 3600 * 72)
        {
        	utc_now		= utc;
        	mytime_now	= MYDATETIME( ds.RTC_Year, ds.RTC_Month, ds.RTC_Date, ts.RTC_Hours, ts.RTC_Minutes, ts.RTC_Seconds);
        	//rt_kprintf("\r\n RTC Time ok!");
        }
        else
        {
        	rt_kprintf ( "\n RTC=%d RTC_BKP=%d",utc,jt808_param_bk.utctime);
        }
        */
    }
}



static __IO uint32_t uwLsiFreq = 0;

__IO uint32_t uwTimingDelay = 0;
__IO uint32_t uwCaptureNumber = 0;
__IO uint32_t uwPeriodValue = 0;


/**
  * @brief  Configures TIM5 to measure the LSI oscillator frequency.
  * @param  None
  * @retval LSI Frequency
  */
static uint32_t GetLSIFrequency(void)
{
    NVIC_InitTypeDef   NVIC_InitStructure;
    TIM_ICInitTypeDef  TIM_ICInitStructure;
    RCC_ClocksTypeDef  RCC_ClockFreq;

    /* Enable the LSI oscillator ************************************************/
    RCC_LSICmd(ENABLE);

    /* Wait till LSI is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
    {
    }

    /* TIM5 configuration *******************************************************/
    /* Enable TIM5 clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

    /* Connect internally the TIM5_CH4 Input Capture to the LSI clock output */
    TIM_RemapConfig(TIM5, TIM5_LSI);

    /* Configure TIM5 presclaer */
    TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);

    /* TIM5 configuration: Input Capture mode ---------------------
       The LSI oscillator is connected to TIM5 CH4
       The Rising edge is used as active edge,
       The TIM5 CCR4 is used to compute the frequency value
    ------------------------------------------------------------ */
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
    TIM_ICInitStructure.TIM_ICFilter = 0;
    TIM_ICInit(TIM5, &TIM_ICInitStructure);

    /* Enable TIM5 Interrupt channel */
    NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable TIM5 counter */
    TIM_Cmd(TIM5, ENABLE);

    /* Reset the flags */
    TIM5->SR = 0;

    /* Enable the CC4 Interrupt Request */
    TIM_ITConfig(TIM5, TIM_IT_CC4, ENABLE);


    /* Wait until the TIM5 get 2 LSI edges (refer to TIM5_IRQHandler() in
      stm32f4xx_it.c file) ******************************************************/
    while(uwCaptureNumber != 2)
    {
    }
    /* Deinitialize the TIM5 peripheral registers to their default reset values */
    TIM_DeInit(TIM5);


    /* Compute the LSI frequency, depending on TIM5 input clock frequency (PCLK1)*/
    /* Get SYSCLK, HCLK and PCLKx frequency */
    RCC_GetClocksFreq(&RCC_ClockFreq);

    /* Get PCLK1 prescaler */
    if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0)
    {
        /* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
        return ((RCC_ClockFreq.PCLK1_Frequency / uwPeriodValue) * 8);
    }
    else
    {
        /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
        return (((2 * RCC_ClockFreq.PCLK1_Frequency) / uwPeriodValue) * 8) ;
    }
}


void iwdg_init(void)
{
    /* Get the LSI frequency:  TIM5 is used to measure the LSI frequency */
    uwLsiFreq = GetLSIFrequency();
    rt_kprintf("\nLSI Freq = % d ", uwLsiFreq);

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    /* IWDG counter clock: LSI/32 */
    IWDG_SetPrescaler(IWDG_Prescaler_64);

    /* Set counter reload value to obtain 250ms IWDG TimeOut.
     IWDG counter clock Frequency = LsiFreq/32
     Counter Reload Value = 250ms/IWDG counter clock period
    					  = 0.25s / (32/LsiFreq)
    					  = LsiFreq/(32 * 4)
    					  = LsiFreq/128

    如果改为4秒   LsiFreq*4/64=LsiFreq/16;
     */

    IWDG_SetReload(uwLsiFreq / 16); /*[0..4095]*/

    /* Reload IWDG counter */
    IWDG_ReloadCounter();

    /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
    IWDG_Enable();
}

static uint8_t sd_ok = 0;
static FATFS fs;
static FIL 	file;
static struct rt_semaphore sem_sd;


void sd_open(char *name)
{
    FRESULT res;
    int  len;
    char buffer[512];

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    res = f_open(&file, name, FA_READ | FA_WRITE);
    if(res == 0)
    {
        rt_kprintf("\r\n file\"%s:\"\r\n", name);
        while(1)
        {
            memset(buffer, 0, sizeof(buffer));
            res = f_read(&file, buffer, sizeof(buffer), &len);
            if(res || len == 0)
            {
                rt_kprintf("\r\n f_read = %d,len = %d", res, len);
                break;
            }
            rt_kprintf("%s", buffer);
        }
        rt_kprintf("\r\n");
        f_close(&file);
    }
    else
    {
        rt_kprintf("\r\n f_open = %d", res);
    }
    rt_sem_release(&sem_dataflash);
}
FINSH_FUNCTION_EXPORT( sd_open, open a tf card );



void sd_write( char *name, char *str)
{
    FRESULT res;
    int  len;

    //rt_kprintf("\r\n F_WR(%s):",name);
    if(rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY ) != RT_EOK)
    {
        return;
    }
    res = f_open(&file, name, FA_READ | FA_WRITE | FA_OPEN_ALWAYS );
    if(res == 0)
    {
        f_lseek( &file, file.fsize);
        res = f_write(&file, str, strlen(str), &len);
        if(res)
        {
            rt_kprintf("\r\n f_write = %d", res);
            sd_ok = 0;
        }
        else
        {
            //rt_kprintf("%s",str);
        }
        res = f_close(&file);
        if(res)
        {
            rt_kprintf("\r\n f_close = %d", res);
            sd_ok = 0;
        }
    }
    else
    {
        rt_kprintf("\r\n f_open = %d", res);
        sd_ok = 0;
    }
    rt_sem_release(&sem_dataflash);
}
FINSH_FUNCTION_EXPORT( sd_write, open a tf card );


void sd_write_console(char *str)
{
    static uint8_t	time_wr = 0;
    FRESULT res;
    int  len;
    char buffer[256];
    char file_name[32];

    if(sd_ok == 0)
    {
        rt_kprintf( "\n%d>%s", rt_tick_get( ), str);
        return;
    }
    if(time_wr == 0)
    {
        sprintf(file_name, "0:RESET.txt");
        sprintf( buffer, "20%02d-%02d-%02d-%02d:%02d:%02d->%s\r\n",
                 YEAR( mytime_now ),
                 MONTH( mytime_now ),
                 DAY( mytime_now ),
                 HOUR( mytime_now ),
                 MINUTE( mytime_now ),
                 SEC( mytime_now ),
                 "Reset system!"
               );
        sd_write(file_name, buffer);

        ///比较当前时间和最后一次的时间是否在同一天
        sprintf(file_name, "0:TW%02d%02d%02d.txt", \
                YEAR( mytime_now ),
                MONTH( mytime_now ),
                DAY( mytime_now )
               );
        sd_write(file_name, buffer);
    }

    ///比较当前时间和最后一次的时间是否在同一天
    sprintf(file_name, "0:TW%02d%02d%02d.txt", \
            YEAR( mytime_now ),
            MONTH( mytime_now ),
            DAY( mytime_now )
           );

    sprintf( buffer, "20%02d-%02d-%02d-%02d:%02d:%02d->%s\r\n",
             YEAR( mytime_now ),
             MONTH( mytime_now ),
             DAY( mytime_now ),
             HOUR( mytime_now ),
             MINUTE( mytime_now ),
             SEC( mytime_now ),
             str
           );
    sd_write(file_name, buffer);
    time_wr = 1;
}
FINSH_FUNCTION_EXPORT_ALIAS( sd_write_console, sd_write, debug sd write_console );


void sd_init_process(void)
{
    FRESULT res;
    char tempbuf[64];
    if(rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY ) != RT_EOK)
    {
        return;
    }

    //SPI_Configuration();
    res = SD_Init();
    if(0 == res)
    {
        rt_kprintf("\r\n SD CARD INIT OK!  SD 容量=%d", SD_GetCapacity());
        /*
        memset(tempbuf,0,sizeof(tempbuf));
        SD_GetCID(tempbuf);
        rt_kprintf("\r\n CID=");
        printf_hex_data(tempbuf, 16);
        SD_GetCSD(tempbuf);
        rt_kprintf("\r\n SID=");
        printf_hex_data(tempbuf, 16);
        */
        if(0 == f_mount(MMC, &fs))
        {
            //rt_kprintf("\r\n f_mount SD OK!");
            sd_ok = 1;
        }
    }
    else
    {
        rt_kprintf("\r\n SD CARD INIT ERR = %d", res);
    }
    //rt_sem_init( &sem_sd, "sem_sd", 0, RT_IPC_FLAG_FIFO );
    rt_sem_release( &sem_dataflash );
    sd_write_console("Init_SD_card!");
}


ALIGN( RT_ALIGN_SIZE )
static char thread_app_stack[1024];
struct rt_thread thread_app;



/*
   应该在此处初始化必要的设备和事件集
 */
void rt_thread_entry_app( void *parameter )
{
    uint8_t i;
    uint8_t need_rtc_init	= 1;
    uint8_t bkpsram_ok		= 0;
    uint32_t temp_counter = 0;		///定义的计数器


    if( rtc_init( ) == 0 ) /*初始化正确*/
    {
        need_rtc_init = 0;
        rt_kprintf("\n RTC_INIT_OK!");
        synctime( );
        //datetime( );
    }
    else
    {
        rt_kprintf("\n RTC_INIT_ERROR!");
    }
    for(i = 0; i < 3; i++)		//多次初始化时因为曾经发现没有正常初始化，导致误认为没有装卡
    {
        if(sd_ok == 0)
        {
            sd_init_process();
        }
    }
    CAN_init();

    while( 1 )
    {
        //  GB19056   process   related
        Virtual_thread_of_GB19056();

        ++temp_counter;
        if(temp_counter % 5 == 0)		//250 ms TIMER
        {
            synctime();
        }
        if(temp_counter % 20 == 0)		//1s TIMER
        {
            //synctime();
            if( need_rtc_init ) /*需要初始化*/
            {
                if( rtc_init( ) == 0 ) /*初始化正确*/
                {
                    need_rtc_init = 0;
                    synctime( );
                }
            }
            //adjust_mytime_now();
            //IWDG_ReloadCounter();
            if(jt808_param_bk.updata_utctime)
            {
                --jt808_param_bk.updata_utctime;
                if(jt808_param_bk.updata_utctime == 0)
                {
                    rt_kprintf("\n 连接升级服务器时间到，需要重新连接主服务器;");
                    gsmstate( GSM_POWEROFF );
                }
            }
            ///渣土车处理
            if(( gsm_socket[0].state == CONNECTED ) && (jt808_status & BIT_STATUS_FIXED))
            {
                if((jt808_param_bk.acc_check_state == 0x03) && ((jt808_status & BIT_STATUS_ACC) == 0))
                {
                    RELAY_SET( 0 );
                }
                else
                {
                    RELAY_SET( 1 );
                }
            }
            param_save_bksram();
        }
        if(temp_counter % 2 == 0)		//100ms TIMER
        {
            mma8451_process();
        }
        jt808_vehicle_process();
        wdg_thread_counter[0] = 1;

        ///设备没有定位，改变GPS数据中的时间信息，

        if( jt808_status & BIT_STATUS_FIXED )
        {
            if((jt808_data.id_0xFA00 > utc_now) || (utc_now - jt808_data.id_0xFA00 > 3600 * 8))
            {
                data_save();
            }
        }

#ifdef JT808_TEST_JTB
        gb_inject_run(); /* 导入记录仪数据*/
#endif
        // nathan  add start


        // nathan  add  over
        rt_thread_delay( RT_TICK_PER_SECOND / 20 );
    }
}

/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
int rt_application_init( void )
{
    rt_thread_init( &thread_app,
                    "init",
                    rt_thread_entry_app,
                    RT_NULL,
                    &thread_app_stack[0],
                    sizeof( thread_app_stack ), 6, 5 );
    rt_thread_startup( &thread_app );
    return 0;
}

/*@}*/

/************************************** The End Of File **************************************/
