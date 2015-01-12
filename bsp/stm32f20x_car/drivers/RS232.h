#ifndef _H_RS232
#define _H_RS232

typedef __packed struct
{
    u8		IC_Status;           		///IC卡状态，0表示没有插入，1表示已经插入
    u8		pro_step;           		///IC卡检测状态，从0开始一直到6
    MYTIME	Time;               		///记录IC卡改变的时间
} TypeDF_ICCARD;


extern rt_tick_t 	last_tick_oil;		///最后一次收到油耗数据的时刻，单位为tick
extern uint32_t 	oil_value;			///邮箱油量，单位为1/10升
extern uint16_t 	oil_high;			///邮箱油量高度实时值，单位为1/10mm
extern uint16_t 	oil_high_average;	///邮箱油量高度平均实时值，单位为1/10mm

extern void rs232_init( void );
extern void rs232_proc(void);

extern void  iccard_send_data(u8 CMD_TYPE, u8 *Srcstr, u16 inlen );
#endif

