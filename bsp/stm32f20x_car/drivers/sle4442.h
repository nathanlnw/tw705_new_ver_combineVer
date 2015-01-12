/*SLE4442 ��ʼ��Ӳ��*/
#ifndef _SLE4442_H_
#define _SLE4442_H_

#include <rtthread.h>
#include <rthw.h>
#include "include.h"
#include "usart.h"
#include "board.h"
#include <serial.h>

#include  <stdlib.h>//����ת�����ַ���

typedef struct
{
    uint8_t	administrator_card;			/*��0��ʾΪ����Ա��*/
    uint32_t 	card_change_mytime; 		/*�������γ���ʱ��,ʱ���ʽΪMYTIME*/
    uint8_t 	IC_Card_valitidy[4]; 		/*��Ƭ��Ч�ڣ�ʱ���ʽΪBCD��YYYYMMDD*/
    uint8_t 	IC_Card_Checked; 			/*���Ƿ��Ѽ��*/
    uint8_t	card_state;					/*IC����ȡ���,
 										0:�ɹ�����ȡ�ɹ�;
 										1:ʧ�ܣ���������֤ʧ��;
 										2:ʧ�ܣ���������;
 										3:ʧ�ܣ������γ�;
 										4:ʧ�ܣ�����У�����;*/
} TYPE_IC_CARD_PARA;


extern TYPE_IC_CARD_PARA ic_card_para;

//#define  C_50Ms     1638 //50msʱ���ж�
//#define  TRUE   1
//#define  FALSE  0


void Init_4442(void);
void CheckICCard(void);
extern unsigned char Rx_4442( unsigned char addr, unsigned char num, unsigned char *buf );
rt_err_t IC_CARD_jt808_0x0702_ex(uint8_t linkno, uint8_t is_ack, uint8_t is_794 );
rt_err_t IC_CARD_jt808_0x0702(uint8_t linkno, uint8_t is_ack );

#endif

