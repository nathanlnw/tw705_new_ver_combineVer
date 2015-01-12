#include <rtthread.h>
#include "ffconf.h"
#include "ff.h"
#include "SPI_SD_driver.h"
#include <finsh.h>
#include "sst25.h"
#include "diskio.h"
#include "jt808.h"



static FATFS fs;
static FIL 	file;

void tf_open(char *name)
{
    FRESULT res;
    int  len;
    char buffer[512];

    rt_kprintf("\r\ntf_open enter;");
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    sprintf(buffer, "%s", name);
    res = f_open(&file, buffer, FA_READ | FA_WRITE);
    rt_kprintf("\r\n f_open = %d", res);
    if(res == 0)
    {
        rt_kprintf("\r\n %s=", buffer);
        while(1)
        {
            memset(buffer, 0, sizeof(buffer));
            res = f_read(&file, buffer, sizeof(buffer), &len);
            if(res || len == 0)
            {
                break;
            }
            rt_kprintf("%s", buffer);
        }
        rt_kprintf("\r\n");
        f_close(&file);
    }
    rt_sem_release(&sem_dataflash);
}
FINSH_FUNCTION_EXPORT( tf_open, open a tf card );



void tf_write( char *name, char *str)
{
    FRESULT res;
    int  len;
    char buffer[512];

    rt_kprintf("\r\ntf_open enter;");
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    sprintf(buffer, "%s", name);
    res = f_open(&file, buffer, FA_READ | FA_WRITE);
    rt_kprintf("\r\n f_open = %d", res);
    if(res == 0)
    {
        rt_kprintf("\r\n %s=", buffer);
        while(1)
        {
            memset(buffer, 0, sizeof(buffer));
            res = f_read(&file, buffer, sizeof(buffer), &len);
            if(res || len == 0)
            {
                break;
            }
            rt_kprintf("%s", buffer);
        }
        rt_kprintf("\r\n");
        res = f_write(&file, str, strlen(str), &len);
        if(res == 0)
        {
            rt_kprintf("\r\n f_write len =%d", len);
        }
        f_sync(&file);
        rt_thread_delay( RT_TICK_PER_SECOND / 20 );
        f_close(&file);
    }
    rt_sem_release(&sem_dataflash);
}
FINSH_FUNCTION_EXPORT( tf_write, open a tf card );


void tf_new( char *name, char *str)
{
    FRESULT res;
    int  len;
    char buffer[512];

    rt_kprintf("\r\ntf_open enter;");
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    sprintf(buffer, "%s", name);
    res = f_open(&file, buffer, FA_READ | FA_WRITE | FA_OPEN_ALWAYS );
    rt_kprintf("\r\n f_open = %d", res);
    if(res == 0)
    {
        rt_kprintf("\r\n %s=", buffer);
        while(1)
        {
            memset(buffer, 0, sizeof(buffer));
            res = f_read(&file, buffer, sizeof(buffer), &len);
            if(res || len == 0)
            {
                break;
            }
            rt_kprintf("%s", buffer);
        }
        rt_kprintf("\r\n");
        res = f_write(&file, str, strlen(str), &len);
        if(res == 0)
        {
            rt_kprintf("\r\n f_write len =%d", len);
        }
        f_sync(&file);
        rt_thread_delay( RT_TICK_PER_SECOND / 20 );
        f_close(&file);
    }
    rt_sem_release(&sem_dataflash);
}
FINSH_FUNCTION_EXPORT( tf_new, open a tf card );


ALIGN( RT_ALIGN_SIZE )
static char thread_sd_test_stack[1024];
struct rt_thread thread_sd_test;



/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_sd_test( void *parameter )
{
    FRESULT res;
    char tempbuf[64];
    rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
    SPI_Configuration();
    res = SD_Init();
    if(0 == res)
    {
        rt_kprintf("\r\n SD CARD INIT OK!");
        memset(tempbuf, 0, sizeof(tempbuf));
        SD_GetCID(tempbuf);
        rt_kprintf("\r\n CID=");
        printf_hex_data(tempbuf, 16);
        SD_GetCSD(tempbuf);
        rt_kprintf("\r\n SID=");
        printf_hex_data(tempbuf, 16);

        rt_kprintf("\r\n SD 容量=%d", SD_GetCapacity());
    }
    else
    {
        rt_kprintf("\r\n SD CARD INIT ERR = %d", res);
    }
    if(0 == f_mount(MMC, &fs))
    {
        rt_kprintf("\r\n f_mount SD OK!");
    }
    rt_sem_release(&sem_dataflash);
    while( 1 )
    {

        rt_thread_delay( RT_TICK_PER_SECOND / 20 );
    }
}

/*RS485设备初始化*/
void SD_test_init( void )
{
    //rt_sem_init( &sem_RS485, "sem_RS485", 0, 0 );
    rt_thread_init( &thread_sd_test,
                    "sd_test",
                    rt_thread_entry_sd_test,
                    RT_NULL,
                    &thread_sd_test_stack[0],
                    sizeof( thread_sd_test_stack ), 9, 5 );
    rt_thread_startup( &thread_sd_test );
}

