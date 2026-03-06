#ifndef __AUDIOPLAY_H
#define __AUDIOPLAY_H
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
 *	音乐播放器 应用代码
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

#define AUDIO_BUF_NUM	4

//音乐播放控制器
typedef __packed struct
{
    u8 *saibuf[AUDIO_BUF_NUM];   //AI音频缓冲区
    volatile u8 saisavebuf; //即将保存的音频帧缓冲编号
    volatile u8 saiplaybuf;	//即将播放的音频帧缓冲编号
    u8 *tbuf;				//零时数组,仅在24bit解码的时候需要用到
    FIL *file;				//音频文件指针

    u8 status;				//bit0:0,暂停播放;1,继续播放
} __audiodev;
extern __audiodev audiodev;	//音乐播放控制


void wav_sai_dma_callback(void);

void audio_start(void);
void audio_stop(void);
u16 audio_get_tnum(u8 *path);
void audio_index_show(u16 index, u16 total);
void audio_msg_show(u32 totsec, u32 cursec, u32 bitrate);
void audio_play(void);
u8 audio_play_song(u8* fname);


#endif












