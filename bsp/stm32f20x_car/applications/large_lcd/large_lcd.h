#ifndef _LARGE_LCD
#define _LARGE_LCD



#define   LCD_5inch


typedef struct  _LARGE_LCD
{
    u8  Enable_Flag;   //  使能发送标?
    u16 CounterTimer;  // 计数器
    u8   Txinfo[300];   //  寄存器内容
    u16  TxInfolen;    //   发送 信息长度
    u8   TXT_content[256];
    u16  TXT_contentLen;
    u8    RxInfo[100];    // 接收内容
    u8    Rx_wr;//
    u16  RxInfolen;     // 接收信息长度
    u8   Type;    //  01  授时 ， 02   速度方向 03 文本信息
    u8   Process_Enable; //  发送标志位

} LARGELCD;

extern LARGELCD DwinLCD;

extern void  large_lcd_process(void);






#endif
