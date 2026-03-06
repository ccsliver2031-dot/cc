#include "wavplay.h"
#include "audioplay.h"
#include "usart.h"
#include "delay.h"
#include "malloc.h"
#include "ff.h"
#include "sai.h"
#include "es8388.h"
#include "key.h"
#include "led.h"

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
 *	WAV解码代码
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

__wavctrl wavctrl;		//WAV控制结构体
vu8 wavtransferend = 0;	//sai传输完成标志
vu8 wavwitchbuf = 0;		//saibufx指示标志

/**
 * @brief	WAV解析初始化
 *
 * @param   fname	文件路径+文件名
 * @param   wavx	wav信息存放结构体指针
 *
 * @return  u8		0,成功;1,打开文件失败;2,非WAV文件;3,DATA区域未找到
 */
u8 wav_decode_init(u8* fname, __wavctrl* wavx)
{
    FIL*ftemp;
    u8 *buf;
    u32 br = 0;
    u8 res = 0;

    ChunkRIFF *riff;
    ChunkFMT *fmt;
    ChunkFACT *fact;
    ChunkDATA *data;
    ftemp = (FIL*)mymalloc(SRAM1, sizeof(FIL));
    buf = mymalloc(SRAM1, 512);

    if(ftemp && buf)	//内存申请成功
    {
        res = f_open(ftemp, (TCHAR*)fname, FA_READ); //打开文件

        if(res == FR_OK)
        {
            f_read(ftemp, buf, 512, &br);	//读取512字节在数据
            riff = (ChunkRIFF *)buf;		//获取RIFF块

            if(riff->Format == 0X45564157) //是WAV文件
            {
                fmt = (ChunkFMT *)(buf + 12);	//获取FMT块
                fact = (ChunkFACT *)(buf + 12 + 8 + fmt->ChunkSize); //读取FACT块

                if(fact->ChunkID == 0X74636166 || fact->ChunkID == 0X5453494C)wavx->datastart = 12 + 8 + fmt->ChunkSize + 8 + fact->ChunkSize; //具有fact/LIST块的时候(未测试)

                else wavx->datastart = 12 + 8 + fmt->ChunkSize;

                data = (ChunkDATA *)(buf + wavx->datastart);	//读取DATA块

                if(data->ChunkID == 0X61746164) //解析成功!
                {
                    wavx->audioformat = fmt->AudioFormat;		//音频格式
                    wavx->nchannels = fmt->NumOfChannels;		//通道数
                    wavx->samplerate = fmt->SampleRate;		//采样率
                    wavx->bitrate = fmt->ByteRate * 8;			//得到位速
                    wavx->blockalign = fmt->BlockAlign;		//块对齐
                    wavx->bps = fmt->BitsPerSample;			//位数,16/24/32位
                    wavx->datasize = data->ChunkSize;			//数据块大小
                    wavx->datastart = wavx->datastart + 8;		//数据流开始的地方.

                    printf("wavx->audioformat:%d\r\n", wavx->audioformat);
                    printf("wavx->nchannels:%d\r\n", wavx->nchannels);
                    printf("wavx->samplerate:%d\r\n", wavx->samplerate);
                    printf("wavx->bitrate:%d\r\n", wavx->bitrate);
                    printf("wavx->blockalign:%d\r\n", wavx->blockalign);
                    printf("wavx->bps:%d\r\n", wavx->bps);
                    printf("wavx->datasize:%d\r\n", wavx->datasize);
                    printf("wavx->datastart:%d\r\n", wavx->datastart);
                }

                else res = 3; //data区域未找到.
            }

            else res = 2; //非wav文件

        }

        else res = 1; //打开文件错误
    }

    f_close(ftemp);
    myfree(SRAM1, ftemp); //释放内存
    myfree(SRAM1, buf);
    return 0;
}


/**
 * @brief	填充buf
 *
 * @param   buf		数据区
 * @param   size	填充数据量
 * @param   bits	位数(16/24)
 *
 * @return  u32		读到的数据个数
 */
u32 wav_buffill(u8 *buf, u16 size, u8 bits)
{
    u16 readlen = 0;
    u32 bread;
    u16 i;
    u32 *p, *pbuf;

    if(bits == 24) //24bit音频,需要处理一下
    {
        readlen = (size / 4) * 3;		//此次要读取的字节数
        f_read(audiodev.file, audiodev.tbuf, readlen, (UINT*)&bread); //读取数据
        pbuf = (u32*)buf;

        for(i = 0; i < size / 4; i++)
        {
            p = (u32*)(audiodev.tbuf + i * 3);
            pbuf[i] = p[0];
        }

        bread = (bread * 4) / 3;		//填充后的大小.
    }

    else
    {
        f_read(audiodev.file, buf, size, (UINT*)&bread); //16bit音频,直接读取数据

        if(bread < size) //不够数据了,补充0
        {
            for(i = bread; i < size - bread; i++)buf[i] = 0;
        }
    }

    return bread;
}

/**
 * @brief	WAV播放时,SAI DMA传输回调函数
 *
 * @param   void
 *
 * @return  void
 */
void wav_sai_dma_tx_callback(void)
{
    audiodev.saiplaybuf++;

    if(audiodev.saiplaybuf >= AUDIO_BUF_NUM)audiodev.saiplaybuf = 0;

    if(wavctrl.bps == 16)
        HAL_SAI_Transmit_DMA(&SAI1A_Handler, audiodev.saibuf[audiodev.saiplaybuf], WAV_SAI_TX_DMA_BUFSIZE / 2);
}

/**
 * @brief	得到当前播放时间
 *
 * @param   fx		文件指针
 * @param   wavx	wav播放控制器
 *
 * @return  void
 */
void wav_get_curtime(FIL*fx, __wavctrl *wavx)
{
    long long fpos;
    wavx->totsec = wavx->datasize / (wavx->bitrate / 8);	//歌曲总长度(单位:秒)
    fpos = fx->fptr - wavx->datastart; 					//得到当前文件播放到的地方
    wavx->cursec = fpos * wavx->totsec / wavx->datasize;	//当前播放到第多少秒了?
}

/**
 * @brief	播放某个WAV文件
 *
 * @param   fname	wav文件路径
 *
 * @return  u8		KEY0_PRES:下一曲/KEY1_PRES:上一曲/其他:错误
 */
u8 wav_play_song(u8* fname)
{
    u8 key;
    u8 t = 0;
    u8 res;
    u32 fillnum;
    u32 nr;
    u8 cnt;

    audiodev.file = (FIL*)mymalloc(SRAM1, sizeof(FIL));
    audiodev.tbuf = mymalloc(SRAM1, WAV_SAI_TX_DMA_BUFSIZE);

    for(cnt = 0; cnt < AUDIO_BUF_NUM; cnt++)
    {
        audiodev.saibuf[cnt] = mymalloc(SRAM2, WAV_SAI_TX_DMA_BUFSIZE);

        if(audiodev.saibuf[cnt] == NULL)
            break;
    }

    if(audiodev.file && audiodev.tbuf && (cnt == AUDIO_BUF_NUM))
    {
        res = wav_decode_init(fname, &wavctrl); //得到文件的信息

        if(res == 0) //解析文件成功
        {
            if(wavctrl.bps == 16)
            {
                ES8388_I2S_Cfg(0, 3); //飞利浦标准,16位数据长度
                SAIA_Init(SAI_MODEMASTER_TX, SAI_CLOCKSTROBING_RISINGEDGE, SAI_DATASIZE_16, wavctrl.samplerate);
                SAIA_TX_DMA_Init(1);//配置TX DMA,16位
            }

            audiodev.saiplaybuf = 0;
            audiodev.saisavebuf = 0;

            sai_tx_callback = wav_sai_dma_tx_callback;			//回调函数指wav_sai_dma_callback
            audio_stop();
            res = f_open(audiodev.file, (TCHAR*)fname, FA_READ);	//打开文件

            if(res == 0)
            {
                f_lseek(audiodev.file, wavctrl.datastart);		//跳过文件头
                fillnum = wav_buffill(audiodev.saibuf[audiodev.saisavebuf], WAV_SAI_TX_DMA_BUFSIZE, wavctrl.bps);

                audio_start();

                if(wavctrl.bps == 16)
                    HAL_SAI_Transmit_DMA(&SAI1A_Handler, audiodev.saibuf[audiodev.saiplaybuf], WAV_SAI_TX_DMA_BUFSIZE / 2);

                else if(wavctrl.bps == 24)
                    HAL_SAI_Transmit_DMA(&SAI1A_Handler, audiodev.saibuf[audiodev.saiplaybuf], WAV_SAI_TX_DMA_BUFSIZE / 4);

                while(res == 0)
                {
                    if(fillnum != WAV_SAI_TX_DMA_BUFSIZE) //播放结束?
                    {
                        res = KEY0_PRES;
                        break;
                    }

                    audiodev.saisavebuf++;
                    if(audiodev.saisavebuf >= AUDIO_BUF_NUM)audiodev.saisavebuf = 0;
                    do
                    {
                        nr = audiodev.saiplaybuf;

                        if(nr)nr--;

                        else nr = AUDIO_BUF_NUM - 1;

                    }
                    while(audiodev.saisavebuf == nr); //碰撞等待.

                    if(audiodev.status & 0X01)
                    {
                        //非暂停时，填充音频数据
                        fillnum = wav_buffill(audiodev.saibuf[audiodev.saisavebuf], WAV_SAI_TX_DMA_BUFSIZE, wavctrl.bps);
                    }
                    else
                    {
                        //暂停时，填充0
                        for(u16 i = 0; i < WAV_SAI_TX_DMA_BUFSIZE; i++)
                            audiodev.saibuf[audiodev.saisavebuf][i] = 0;
                    }

                    key = KEY_Scan(0);

                    if(key == WKUP_PRES) //暂停+继续播放
                    {
                        if(audiodev.status & 0X01)audiodev.status &= ~(1 << 0);

                        else audiodev.status |= 0X01;
                    }

                    if(key == KEY2_PRES || key == KEY0_PRES) //下一曲/上一曲
                    {
                        res = key;
                        break;
                    }

                    wav_get_curtime(audiodev.file, &wavctrl); //得到总时间和当前播放的时间
                    audio_msg_show(wavctrl.totsec, wavctrl.cursec, wavctrl.bitrate); //显示播放时间

                    t++;
                    if(t == 20)
                    {
                        t = 0;
                        LED_B_TogglePin;
                    }
                }
                audio_stop();
            }
            else res = 0XF1;
        }
        else res = 0XF2;
    }
    else res = 0XF3;

    myfree(SRAM1, audiodev.tbuf);	//释放内存
    myfree(SRAM1, audiodev.file);	//释放内存

    for(cnt = 0; cnt < AUDIO_BUF_NUM; cnt++)
        myfree(SRAM2, audiodev.saibuf[cnt]);	//释放内存

    return res;
}










