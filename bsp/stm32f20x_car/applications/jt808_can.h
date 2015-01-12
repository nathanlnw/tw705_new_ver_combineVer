/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// 文件名
 * Author:			// 作者
 * Date:			// 日期
 * Description:		// 模块描述
 * Version:			// 版本信息
 * Function List:	// 主要函数及其功能
 *     1. -------
 * History:			// 历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#ifndef _H_JT808_CAN_H_
#define _H_JT808_CAN_H_

extern CanRxMsg 			can_message_rxbuf[16];
extern uint8_t				can_message_rxbuf_rd;
extern volatile uint8_t		can_message_rxbuf_wr;

extern uint8_t jt808_can_get( void );
extern void can_proc(void);

#endif
