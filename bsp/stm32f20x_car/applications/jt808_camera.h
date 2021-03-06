#ifndef _H_JT808_CAMERA
#define _H_JT808_CAMERA
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


rt_err_t Cam_jt808_0x0800_ex(uint8_t linkno, u32 mdeia_id, u8 media_delete , u8 send_meda );
rt_err_t Cam_jt808_0x0801(uint8_t linkno, u32 mdeia_id, u8 media_delete );
rt_err_t Cam_jt808_0x0800(uint8_t linkno, u32 mdeia_id , u8 media_delete);
rt_err_t Cam_jt808_0x8801(uint8_t linkno, uint8_t *pmsg);
rt_err_t Cam_jt808_0x8802(uint8_t linkno, uint8_t *pmsg);
rt_err_t Cam_jt808_0x8803(uint8_t linkno, uint8_t *pmsg);
rt_err_t Cam_jt808_0x8805(uint8_t linkno, uint8_t *pmsg);
uint8_t  Cam_jt808_get( void );


void cam_ok( struct _Style_Cam_Requset_Para *para, uint32_t pic_id );
void cam_end( struct _Style_Cam_Requset_Para *para );


#endif
/************************************** The End Of File **************************************/
