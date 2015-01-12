/*
gb_rec.c
汽车行驶记录仪相关操作
*/
#include <stdio.h>

#include <board.h>
#include <rtthread.h>
#include <finsh.h>
#include <rtdevice.h>

#include "include.h"
#include "jt808.h"
#include "msglist.h"
#include "jt808_sprintf.h"
#include "sst25.h"

#include "rtc.h"
#include "jt808_gps.h"
#include "jt808_util.h"
#include "jt808_param.h"


#include "common.h"
#include "m66.h"
#include "gb_inject.h"
#include "Jt808_config.h"
#include "Usbh_conf.h"

#include "ffconf.h"
#include "ff.h"
#include "SPI_SD_driver.h"
#include <finsh.h>
#include "diskio.h"


#ifdef JT808_TEST_JTB

static GB_INJECT  gb_data_inject;
static u8    gb_write_buf[1024];
static u8    gb_read_buf[1024];
static FIL 	file;




void  gb_injection(void)
{
    /*
    执行导入行驶记录仪数据
    */
    if( rt_device_find( "udisk" ) == RT_NULL ) /*没有找到*/
    {
        rt_kprintf("\r\n 没有找到udisk\r\n");
        rt_thread_delay( RT_TICK_PER_SECOND );
    }
    else
    {
        rt_kprintf("\r\n 找到udisk，ch12.mp3 ok\r\n");
        gb_data_inject.write_flag = 1;
        gb_data_inject.run_step = 1;
        gb_data_inject.Pkg_count = 0;
        gb_data_inject.File_size = FileSize_GBinput; //固定文件大小
    }
}
FINSH_FUNCTION_EXPORT(gb_injection, gb_injection );

void gb_erase_DataArea(void)
{
    /*
    擦除掉，记录仪的存储区域
    */
    uint8_t  i = 0;

#if 0
    /*行驶记录仪VDR数据存储区域*/
#define ADDR_DF_VDR_BASE 			0x1D0000				///行驶记录仪VDR数据存储区域开始位置
#define ADDR_DF_VDR_END				0x2AC000				///行驶记录仪VDR数据存储区域结束位置
#define ADDR_DF_VDR_SECT			220						///多给分配了一些空间，实际需要182
#endif

    uint32_t addr = 0;

    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    addr = ADDR_DF_VDR_BASE;

    for(i = 0; i < 10; i++)
    {
        sst25_erase_64k(ADDR_DF_VDR_BASE + (i << 16));
    }

    rt_sem_release( &sem_dataflash );
}

uint8_t gb_write_check(uint32_t addr, char *instr, uint16_t inLen)
{
    //char *check_read;
    uint8_t   resualt = RT_EOK;
    uint32_t   i = 0;

    // check_read=rt_malloc(inLen);
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    sst25_write_through(addr, instr, inLen);
    sst25_read(addr, gb_read_buf, inLen);


    for(i = 0; i < inLen; i++)
    {
        if(instr[i] != gb_read_buf[i])
        {
            rt_kprintf("\r\n	error at pkg=%d,i=%d,write=%X,read=%X", gb_data_inject.Pkg_count, i, instr[i], gb_read_buf[i]);
            gb_data_inject.write_flag = 0;
            gb_data_inject.run_step = 0;
            gb_data_inject.File_size = 0;
            gb_data_inject.Pkg_count = 0;
            rt_kprintf("\r\n gbdata 文件读取错误");
            resualt = RT_ERROR;
            break;
        }
    }
    rt_sem_release( &sem_dataflash );
    // free(check_read);

    return  resualt;
}

void gb_inject_run(void)
{
    /*
    执行导入 数据操作
    */

    rt_err_t	res;
    u16            read_size, lastpacket = 0, i = 0;
    int  len;

    switch(gb_data_inject.run_step)
    {
    case  1:
        gb_erase_DataArea();   //clear  mp3  DF  area
        rt_kprintf("\n GB area erase over!\n");
        gb_data_inject.run_step = 2;
        break;
    case 2:
        res = f_open(&file, "1:ch12.mp3", FA_READ);
        if( res == 0 )
        {
            //gb_data_inject.run_step=3;
            rt_kprintf("\r\n 读取ch12.mp3文件\r\n");

            //-------------------------------------
            rt_enter_critical();
            while(1)
            {
                if((gb_data_inject.File_size - gb_data_inject.File_left) >= 1024)
                    read_size = 1024;
                else
                {
                    read_size = gb_data_inject.File_size - gb_data_inject.File_left;
                    lastpacket = 1;
                }

                memset(gb_write_buf, 0, sizeof(gb_write_buf));
                res = f_read(&file, gb_write_buf, read_size, &len);

                if(res || len == 0)
                {
                    rt_kprintf("\r\n f_read = %d", res);
                    gb_data_inject.write_flag = 0;
                    gb_data_inject.run_step = 0;
                    gb_data_inject.File_size = 0;
                    gb_data_inject.Pkg_count = 0;
                    rt_kprintf("\r\n ch12.mp3写入完毕!");
                    rt_exit_critical();
                    break;

                }
                if(res)
                {
                    gb_data_inject.write_flag = 0;
                    gb_data_inject.run_step = 0;
                    gb_data_inject.File_size = 0;
                    gb_data_inject.Pkg_count = 0;
                    rt_kprintf("\r\n ch12.mp3文件读取错误");
                    f_close(&file);
                    rt_exit_critical();
                    break;
                }
                else           /*判断是否为最后一包*/
                {
                    gb_data_inject.File_left += read_size;
                    // rt_kprintf("\r\n gb_read:");
                    // printf_hex_data(gb_write_buf, read_size);
                    rt_kprintf("\r\n  write page= %d", gb_data_inject.Pkg_count);
                    // -----------
                    if(gb_write_check((ADDR_DF_VDR_BASE + (gb_data_inject.Pkg_count << 10)), gb_write_buf, read_size))
                    {
                        f_close(&file);
                        gb_data_inject.write_flag = 0;
                        gb_data_inject.run_step = 0;
                        gb_data_inject.File_size = 0;
                        gb_data_inject.Pkg_count = 0;
                        rt_kprintf("\r\n ch12.mp3写入完毕!");
                        rt_exit_critical();
                        return;
                    }
                    gb_data_inject.Pkg_count++;

                }
                if(1 == lastpacket)
                {
                    // DF_WriteFlashDirect(DF_MP3_offset,0,"mp3",3); //  写标志
                    gb_data_inject.write_flag = 0;
                    gb_data_inject.run_step = 0;
                    gb_data_inject.File_size = 0;
                    gb_data_inject.Pkg_count = 0;
                    rt_kprintf("\r\n ch12.mp3写入完毕--last!");
                    f_close(&file);
                    break;
                }

            }
            rt_exit_critical();

            //-------------------------------------
        }
        else
        {
            gb_data_inject.write_flag = 0;
            gb_data_inject.run_step = 0;
            gb_data_inject.File_size = 0;
            gb_data_inject.Pkg_count = 0;
            rt_kprintf("\r\n ch12.mp3文件不存在");
            return;
        }
        break;
    default:
        gb_data_inject.write_flag = 0;
        gb_data_inject.run_step = 0;
        gb_data_inject.File_size = 0;
        gb_data_inject.Pkg_count = 0;
        break;
    }
}

#endif
