#ifndef _LARGE_LCD
#define _LARGE_LCD



#define   LCD_5inch


typedef struct  _LARGE_LCD
{
    u8  Enable_Flag;   //  ʹ�ܷ��ͱ�?
    u16 CounterTimer;  // ������
    u8   Txinfo[300];   //  �Ĵ�������
    u16  TxInfolen;    //   ���� ��Ϣ����
    u8   TXT_content[256];
    u16  TXT_contentLen;
    u8    RxInfo[100];    // ��������
    u8    Rx_wr;//
    u16  RxInfolen;     // ������Ϣ����
    u8   Type;    //  01  ��ʱ �� 02   �ٶȷ��� 03 �ı���Ϣ
    u8   Process_Enable; //  ���ͱ�־λ

} LARGELCD;

extern LARGELCD DwinLCD;

extern void  large_lcd_process(void);






#endif
