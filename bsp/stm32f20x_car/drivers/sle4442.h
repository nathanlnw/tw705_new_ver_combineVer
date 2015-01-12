/*SLE4442 初始化硬件*/
#ifndef _SLE4442_H_
#define _SLE4442_H_

#include <rtthread.h>
#include <rthw.h>
#include "include.h"
#include "usart.h"
#include "board.h"
#include <serial.h>

#include  <stdlib.h>//数字转换成字符串

typedef struct
{
    uint8_t	administrator_card;			/*非0表示为管理员卡*/
    uint32_t 	card_change_mytime; 		/*卡插入或拔出的时间,时间格式为MYTIME*/
    uint8_t 	IC_Card_valitidy[4]; 		/*卡片有效期，时间格式为BCD码YYYYMMDD*/
    uint8_t 	IC_Card_Checked; 			/*卡是否已检测*/
    uint8_t	card_state;					/*IC卡读取结果,
 										0:成功，读取成功;
 										1:失败，卡密码认证失败;
 										2:失败，卡被锁定;
 										3:失败，卡被拔出;
 										4:失败，数据校验错误;*/
} TYPE_IC_CARD_PARA;


extern TYPE_IC_CARD_PARA ic_card_para;

//#define  C_50Ms     1638 //50ms时钟中断
//#define  TRUE   1
//#define  FALSE  0


void Init_4442(void);
void CheckICCard(void);
extern unsigned char Rx_4442( unsigned char addr, unsigned char num, unsigned char *buf );
rt_err_t IC_CARD_jt808_0x0702_ex(uint8_t linkno, uint8_t is_ack, uint8_t is_794 );
rt_err_t IC_CARD_jt808_0x0702(uint8_t linkno, uint8_t is_ack );

#endif

