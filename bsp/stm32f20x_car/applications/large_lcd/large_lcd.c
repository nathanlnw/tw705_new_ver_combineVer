#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "include.h"
#include "rs485.h"
#include "jt808.h"
#include "camera.h"
#include "large_lcd.h"

#include "jt808_gps.h"
#include "jt808_util.h"
#include "jt808_param.h"


#include <finsh.h>

#ifdef	LCD_5inch

//   SD_Type  LCD
#define   LCD_IDLE               0
#define   LCD_SETTIME        1
#define   LCD_SETSPD          2
#define   LCD_SDTXT            3
#define   LCD_GPSNUM         4
#define   LCD_GSMNUM        5
#define   LCD_VechInfo       6
#define   LCD_PAGE			8



LARGELCD     DwinLCD;


void DwinLCD_init(void)
{
    DwinLCD.TxInfolen = 0;	// 信息长度
    DwinLCD.Rx_wr = 0;	// clear write
    memset(DwinLCD.Txinfo, 0, sizeof(DwinLCD.Txinfo));
}



/******************************************************************************
 * Function: Lcd_write ()
 * DESCRIPTION: - 向LCD写数据 (密码没有输入完全时使用此函数要在判定车台配置是否完成)
 * Input: statu	LCD状态	type 写数据类型
 * Input: data 数据
 * Output:
 * Returns:
 *
 * -----------------------------------
* Created By wxg 19-jan-2014
* -------------------------------
 ******************************************************************************/
u8  Lcd_write(u8 statu, u8 type, u8 data)
{
    //TX_485const;

    memset(DwinLCD.Txinfo, 0, sizeof(DwinLCD.Txinfo));
    DwinLCD.TxInfolen = 0;
    DwinLCD.Txinfo[0] = 0x88; //协议头
    DwinLCD.Txinfo[1] = 0x55;
    switch (type)
    {
    case  LCD_SETTIME:    //  ??
        /*  88 55 0A 80 1F 5A 12 09 01 03 11 53 01     -----??
        	?: 12 09 01 03 11 53 01 ??  12? 09?01? ??3 11:53:01
        */
#if 0
        DwinLCD.Txinfo[2] = 0x0A;
        DwinLCD.Txinfo[3] = 0x80;
        DwinLCD.Txinfo[4] = 0x1F; //  ???
        DwinLCD.Txinfo[5] = 0x5A;
        DwinLCD.Txinfo[6] = ((((YEAR( mytime_now ))) / 10) << 4) + (((YEAR( mytime_now ))) % 10);
        DwinLCD.Txinfo[7] = ((MONTH( mytime_now ) / 10) << 4) + (MONTH( mytime_now ) % 10);
        DwinLCD.Txinfo[8] = ((DAY( mytime_now ) / 10) << 4) + (DAY( mytime_now ) % 10);
        DwinLCD.Txinfo[9] = 0x01;
        DwinLCD.Txinfo[10] = ((HOUR( mytime_now ) / 10) << 4) + (HOUR( mytime_now ) % 10);
        DwinLCD.Txinfo[11] = ((MINUTE( mytime_now ) / 10) << 4) + (MINUTE( mytime_now ) % 10);
        DwinLCD.Txinfo[12] = ((SEC( mytime_now ) / 10) << 4) + (SEC( mytime_now ) % 10);
        //rt_device_write(&Device_485,0,( const char*)DwinLCD.Txinfo,13);
        RS485_write(DwinLCD.Txinfo, 13);
#endif
        break;
    case  LCD_SETSPD:   //   ????
#if 0
        // AA 55 07 82 00 01 01 30  00 F3
        DwinLCD.Txinfo[2] = 0x05;
        DwinLCD.Txinfo[3] = 0x82;
        DwinLCD.Txinfo[4] = 0x10;
        DwinLCD.Txinfo[5] = 0x00;
        DwinLCD.Txinfo[6] = ((gps_speed / 10) >> 8); // ??
        DwinLCD.Txinfo[7] = (u8)(gps_speed / 10);
        RS485_write(DwinLCD.Txinfo, 8);
#endif
        break;
    case  LCD_GPSNUM:  //  GPS ??   AA 55 05 82 10 01 00 03  ???20
        DwinLCD.Txinfo[2] = 5; // ??
        DwinLCD.Txinfo[3] = 0x82;
        DwinLCD.Txinfo[4] = 0x10; // LCD ID
        DwinLCD.Txinfo[5] = 0x01;
        DwinLCD.Txinfo[6] = 0x00; //Data
        DwinLCD.Txinfo[7] = data; //Satelite_num;

        //rt_device_write(&Device_485,0,( const char*)DwinLCD.Txinfo,8);
        RS485_write(DwinLCD.Txinfo, 8);
        break;
    case    LCD_GSMNUM://   GSM ??  AA 55 05 82 10 02 00 09  ???31
        DwinLCD.Txinfo[2] = 5; // ??
        DwinLCD.Txinfo[3] = 0x82;
        DwinLCD.Txinfo[4] = 0x10; // LCD ID
        DwinLCD.Txinfo[5] = 0x02;
        DwinLCD.Txinfo[6] = 0x00; //Data
        DwinLCD.Txinfo[7] = data; //ModuleSQ;
        //rt_device_write(&Device_485,0,( const char*)DwinLCD.Txinfo,8);
        RS485_write(DwinLCD.Txinfo, 8);
        break;
    case LCD_PAGE:
        DwinLCD.Txinfo[2] = 0X04;
        DwinLCD.Txinfo[3] = 0X80;
        DwinLCD.Txinfo[4] = 0X03;
        DwinLCD.Txinfo[5] = 0X00;
        DwinLCD.Txinfo[6] = data;
        //rt_device_write(&Device_485,0,( const char*)DwinLCD.Txinfo,7);
        RS485_write(DwinLCD.Txinfo, 7);
        break;
#if 0
    case LCD_Text_clear:
        DwinLCD.Txinfo[2] = 0X05;
        DwinLCD.Txinfo[3] = 0X82;
        DwinLCD.Txinfo[4] = 0X44;
        DwinLCD.Txinfo[5] = 0X00;
        DwinLCD.Txinfo[6] = 0x00;
        DwinLCD.Txinfo[7] = 0x00;
        //rt_device_write(&Device_485,0,( const char*)DwinLCD.Txinfo,8);
        RS485_write(DwinLCD.Txinfo, 8);
        break;
#endif
    case 20:
        DwinLCD.Txinfo[2] = 0X03;
        DwinLCD.Txinfo[3] = 0X81;
        DwinLCD.Txinfo[4] = 0X03;
        DwinLCD.Txinfo[5] = 0X02;
        //rt_device_write(&Device_485,0,( const char*)DwinLCD.Txinfo,8);
        RS485_write(DwinLCD.Txinfo, 6);
        break;
    }
    //-------------------------

    DwinLCD.TxInfolen = 0; //长度清零
    return RT_EOK;


}

FINSH_FUNCTION_EXPORT( Lcd_write,  Lcdwrite );

void  Lcd_Data_Process(void)
{
    u16 data_id = 0, i = 0;
    u16 ContentLen = 0;
    u8  dwin_reg[150];
    //加入调试的信息
    while(DwinLCD.Rx_wr != 0)
    {
        rt_kprintf(",%02x\n", DwinLCD.RxInfo[i]);
        i++;
        DwinLCD.Rx_wr--;
    }
    //AA 55 08 83 30 00 02 1E FF FF 00
    if((DwinLCD.RxInfo[0] != 0x88) || (DwinLCD.RxInfo[1] != 0x55) || (DwinLCD.RxInfo[2] < 3)\
            || ((DwinLCD.RxInfo[6] < 1)))
    {
        return ;
    }

    //是有效数据
    if((DwinLCD.RxInfo[2] >= 3) && (DwinLCD.RxInfo[6] > 0))
    {
        ContentLen  = DwinLCD.RxInfo[6] * 2; //内容的长度
        data_id = ((u16)DwinLCD.RxInfo[4] << 8) + (u16)DwinLCD.RxInfo[5];

        //找到地址以后有用的数据
        memset(dwin_reg, 0, sizeof(dwin_reg));
        i = 0;
        while((0xFF != DwinLCD.RxInfo[7 + i]) && (2 * ContentLen > i) && (0 != DwinLCD.RxInfo[7 + i])) //找到后面的数据
        {
            dwin_reg[i] = DwinLCD.RxInfo[7 + i];
            i++;
        }
        rt_kprintf("data_id.......%x\r\n", data_id);
        //清零中断里的接受计数和存储数据
        //Big_lcd.rx_counter =0;
        memset(DwinLCD.RxInfo, 0, sizeof(DwinLCD.RxInfo));
        switch(data_id)
        {

            //-------------------------------------------------------车辆初始化-----------------------
            //++++++++++++++++++++++++++++++++++++++++++++++++++++++++使用前锁定2快速开户相关++++++
            //终端手机号
        case 0x4220:
            if(strlen(dwin_reg) == 11)
            {
                memset(jt808_param.id_0xF006, 0, sizeof(jt808_param.id_0xF006));
                jt808_param.id_0xF006[0] = 0x30;
                memcpy(jt808_param.id_0xF006 + 1, dwin_reg, 11);
                rt_thread_delay(4);
                Lcd_write(0, LCD_PAGE, 5);
            }
            else
            {
                Lcd_write(0, LCD_PAGE, 0x0A);
            }
            break;
            //开户
        case 0x4234:
            //Stuff_AccountPacket_0110H(0,1);
            param_save(1);
            jt808_tx_lock(mast_socket->linkno);
            break;
            //车主手机号
        case 0x4240:
            if(strlen(dwin_reg) == 11)
            {
#if 0
                memset(Big_lcd.chezhu, 0, 12);
                memcpy(Big_lcd.chezhu, dwin_reg, 11);
                memset(dwin_reg, 0, 150);
                memcpy(dwin_reg, Big_lcd.chezhu, 11);
                dwin_reg[11] = 0x30;
                memset(Big_lcd.chezhu, 0, 12);
                StrToBCD(dwin_reg, Big_lcd.chezhu, 6);
#endif
                memset(jt808_param.id_0xF014, 0, sizeof(jt808_param.id_0xF014));
                memcpy(jt808_param.id_0xF014, dwin_reg, 11);
                rt_thread_delay(4);
                Lcd_write(0, LCD_PAGE, 5);
            }
            else
            {
                rt_thread_delay(4);
                Lcd_write(0, LCD_PAGE, 0x0A);
            }
            break;
            //省域ID
        case 0x4260:
            jt808_param.id_0x0081  = atoi(dwin_reg);
            Lcd_write(0, LCD_PAGE, 5);
            break;
            //车牌加颜色
        case 4280:
            if(strlen(dwin_reg) == 9)
            {
                memset(jt808_param.id_0x0083, 0, sizeof(jt808_param.id_0x0083));
                memcpy(jt808_param.id_0x0083, dwin_reg, 8);
                jt808_param.id_0x0084 = atoi(dwin_reg + 8);
                rt_thread_delay(4);
                Lcd_write(0, LCD_PAGE, 0x5);
            }
            else
            {
                rt_thread_delay(4);
                Lcd_write(0, LCD_PAGE, 0x0A);
            }
            break;
            //市域ID
        case 0x4320:
            jt808_param.id_0x0082 = atoi(dwin_reg);
            Lcd_write(0, LCD_PAGE, 0x5);
            break;
            //车辆VIN
        case 0x4300:
            if(strlen(dwin_reg) == 17)
            {
                rt_kprintf("da yin vin.... \n");
                memset(jt808_param.id_0xF005, 0, sizeof(jt808_param.id_0xF005));
                memcpy(jt808_param.id_0xF005, dwin_reg, 17);
                Lcd_write(0, LCD_PAGE, 0x5);
            }
            else
            {
                rt_thread_delay(4);
                Lcd_write(0, LCD_PAGE, 0x0A);
            }

            break;

            //++++++++++++++++++++++++++++++++++++++++++++++++++使用前锁定2快速开户相关++++++++++++++++++++++++++++++++
        default:
            break;

        }

    }
    else
    {
        //Lcd_write(0,LCD_PAGE,0x0A);
        return ;
    }

}





void  large_lcd_process(void)
{

    u8	ch;
    u8	i = 0;
    static u8 send_msg_count = 0;
    static u8 device_lock_ok = 0;

    if((send_msg_count < 6)
            && (rt_tick_get() > 500))
    {
        if( send_msg_count % 2 == 0)
        {
            if(device_unlock)
            {
                Lcd_write(0, LCD_PAGE, 0x01);
            }
            else
            {
                Lcd_write(0, LCD_PAGE, 0x05);
            }
        }
        send_msg_count++;
    }
    if(device_lock_ok != device_unlock)
    {
        send_msg_count = 0;
        device_lock_ok = device_unlock;
    }
    //  1. 处理数据
    if(DwinLCD.Process_Enable == 1)
    {

        //DwinLCD_Data_Process();
        Lcd_Data_Process();
        //---------------------------------------
        DwinLCD.Process_Enable = 0; // clear
    }
    // 2. 接收数据
    while( (RS485_read_char( &ch )) && (0 == DwinLCD.Process_Enable) )
    {
        DwinLCD.RxInfo[DwinLCD.Rx_wr++] = ch;
        // 2.1 check  1st  Byte
        if(DwinLCD.RxInfo[0] != 0x88)
            DwinLCD.Rx_wr = 0;
        // 2.2  head   get
        if(DwinLCD.Rx_wr >= 3)
        {

            //----------  Check  LCD  --------------------------
            if((0x88 == DwinLCD.RxInfo[0]) && (0x55 == DwinLCD.RxInfo[1]))
            {
                //AA 55 08 83 30 00 02 1E FF FF 00 					08 是长度(包含1BYTE 指令2 BYTES  地址)
                if(DwinLCD.Rx_wr == 3)
                {
                    DwinLCD.RxInfolen = DwinLCD.RxInfo[2];
                    //rt_kprintf("\r\n  co--\r\n");
                }
            }
            else
            {
                DwinLCD.Rx_wr = 0; // clear
                DwinLCD.RxInfolen = 0;
            }

            // get info
            if(DwinLCD.Rx_wr >= (DwinLCD.RxInfolen + 3)) //  88  55  08
            {
                DwinLCD.Process_Enable = 1;
                DwinLCD.Rx_wr = 0; //clear
            }


        }

        //  3 large_lcd send
        //DwinLCD_Send();


    }

}
#endif
