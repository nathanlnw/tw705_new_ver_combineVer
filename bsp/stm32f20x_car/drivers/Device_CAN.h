/*
     Device_CAN.h
*/
#ifndef  _RT_Dev_CAN
#define _RT_Dev_CAN

#include <rthw.h>
#include <rtthread.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "board.h"
#include <serial.h>




#define CANTESTInit 1
#define CANTESTTX 1
#define CANTESTRX 0

#define CANBufSize   100

typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;

extern TestStatus TestRx;
extern u8	CAN_initOver;
extern CanRxMsg RxMessageData;

extern u32   CANBuffer_wr;//--------- Debug ----------
extern u32   Can_RXnum;
extern u32   Can_sdnum;
extern u32   Can_same;
extern u32   Can_loudiao;
extern u32   Can_notsame;







extern void CANGPIO_Configuration(void);
extern void CAN_App_Init(void);
extern TestStatus CAN_TX(void);
extern TestStatus CAN_RX(void);
extern u8 CANTXData(u8 *Instr, u8 len); //len=0~8
extern u8 CANTXStr(u8 *Instr, u32 len);
//extern u8 CANRXStr(void);
extern u8  CAN1_Rx_Process(void);









#endif

