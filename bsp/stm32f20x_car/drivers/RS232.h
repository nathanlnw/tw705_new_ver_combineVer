#ifndef _H_RS232
#define _H_RS232

typedef __packed struct
{
    u8		IC_Status;           		///IC��״̬��0��ʾû�в��룬1��ʾ�Ѿ�����
    u8		pro_step;           		///IC�����״̬����0��ʼһֱ��6
    MYTIME	Time;               		///��¼IC���ı��ʱ��
} TypeDF_ICCARD;


extern rt_tick_t 	last_tick_oil;		///���һ���յ��ͺ����ݵ�ʱ�̣���λΪtick
extern uint32_t 	oil_value;			///������������λΪ1/10��
extern uint16_t 	oil_high;			///���������߶�ʵʱֵ����λΪ1/10mm
extern uint16_t 	oil_high_average;	///���������߶�ƽ��ʵʱֵ����λΪ1/10mm

extern void rs232_init( void );
extern void rs232_proc(void);

extern void  iccard_send_data(u8 CMD_TYPE, u8 *Srcstr, u16 inlen );
#endif

