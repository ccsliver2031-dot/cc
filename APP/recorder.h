#ifndef __RECORDER_H
#define __RECORDER_H
#include "sys.h"
#include "ff.h"
#include "wavplay.h"

/*********************************************************************************
			  ___   _     _____  _____  _   _  _____  _____  _   __
			 / _ \ | |   |_   _||  ___|| \ | ||_   _||  ___|| | / /
			/ /_\ \| |     | |  | |__  |  \| |  | |  | |__  | |/ /
			|  _  || |     | |  |  __| | . ` |  | |  |  __| |    \
			| | | || |_____| |_ | |___ | |\  |  | |  | |___ | |\  \
			\_| |_/\_____/\___/ \____/ \_| \_/  \_/  \____/ \_| \_/

 *	******************************************************************************
 *	本程序只供学习使用，未经作者许可，不得用于其它任何用途
 *	ALIENTEK Pandora STM32L IOT开发板
 *	录音机 应用代码
 *	正点原子@ALIENTEK
 *	技术论坛:www.openedv.com
 *	创建日期:2018/10/27
 *	版本：V1.0
 *	版权所有，盗版必究。
 *	Copyright(C) 广州市星翼电子科技有限公司 2014-2024
 *	All rights reserved
 *	******************************************************************************
 *	初始版本
 *	******************************************************************************/

#define SAI_RX_DMA_BUF_SIZE    	(1024 * 6)		//定义RX DMA 数组大小

#define SAI_RX_SRAM1_FIFO_SIZE	5
#define SAI_RX_SRAM2_FIFO_SIZE	5
#define SAI_RX_FIFO_SIZE		(SAI_RX_SRAM1_FIFO_SIZE+SAI_RX_SRAM2_FIFO_SIZE)			//定义接收FIFO大小

#define REC_SAMPLERATE			22050		//采样率,22.05Khz

u8 rec_sai_fifo_read(u8 **buf);
u8 rec_sai_fifo_write(u16 loc);

void rec_sai_dma_rx_callback(u16 data);
void recoder_enter_rec_mode(void);
void recoder_wav_init(__WaveHeader* wavhead);
void recoder_msg_show(u32 tsec, u32 kbps);
void recoder_remindmsg_show(u8 mode);
void recoder_new_pathname(u8 *pname);
void wav_recorder(void);

#endif












