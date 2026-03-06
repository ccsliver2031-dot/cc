#include "sai.h"
#include "delay.h"
#include "wavplay.h"
#include "recorder.h"
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
 *	SAIA/B驱动代码
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


SAI_HandleTypeDef SAI1A_Handler;        //SAI1 Block A句柄
DMA_HandleTypeDef SAI1_TXDMA_Handler;   //DMA发送句柄

SAI_HandleTypeDef SAI1B_Handler;        //SAI1 Block B句柄
DMA_HandleTypeDef SAI1_RXDMA_Handler;   //DMA发送句柄

/**
 * @brief	SAI Block A初始化,I2S,飞利浦标准
 *
 * @param   mode		工作模式,可以设置:SAI_MODEMASTER_TX/SAI_MODEMASTER_RX/SAI_MODESLAVE_TX/SAI_MODESLAVE_RX
 * @param   cpol		数据在时钟的上升/下降沿选通，可以设置：SAI_CLOCKSTROBING_FALLINGEDGE/SAI_CLOCKSTROBING_RISINGEDGE
 * @param   datalen		数据大小,可以设置：SAI_DATASIZE_8/10/16/20/24/32
 * @param   samplerate	采样率：HZ
 *
 * @return  u8		0:初始化成功，其他:失败
 */
u8 SAIA_Init(u32 mode, u32 cpol, u32 datalen, u32 samplerate)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /* Configure and enable PLLSAI1 clock to generate 11.294MHz */
    PeriphClkInit.PeriphClockSelection 		= RCC_PERIPHCLK_SAI1;
    PeriphClkInit.Sai1ClockSelection      	= RCC_SAI1CLKSOURCE_PLLSAI1;
    PeriphClkInit.PLLSAI1.PLLSAI1Source 	= RCC_PLLSOURCE_HSE;
    PeriphClkInit.PLLSAI1.PLLSAI1M 			= 1;
    PeriphClkInit.PLLSAI1.PLLSAI1N 			= 24;
    PeriphClkInit.PLLSAI1.PLLSAI1P 			= RCC_PLLP_DIV17;
    PeriphClkInit.PLLSAI1.PLLSAI1Q 			= RCC_PLLQ_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1R 			= RCC_PLLR_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1ClockOut 	= RCC_PLLSAI1_SAI1CLK;

    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
    
    SAI1A_Handler.Instance = SAI1_Block_A;  //SAI1 Bock A
    HAL_SAI_DeInit(&SAI1A_Handler);         //清除以前的配置

    /* Initialize SAI */
    SAI1A_Handler.Instance 				= SAI1_Block_A;                   //SAI1 Bock A
    SAI1A_Handler.Init.AudioMode 		= mode;                     //设置SAI1工作模式
    SAI1A_Handler.Init.Synchro 			= SAI_ASYNCHRONOUS;           //音频模块异步
    SAI1A_Handler.Init.OutputDrive 		= SAI_OUTPUTDRIVE_ENABLE; //立即驱动音频模块输出
    SAI1A_Handler.Init.NoDivider 		= SAI_MASTERDIVIDER_ENABLE; //使能主时钟分频器(MCKDIV)
    SAI1A_Handler.Init.AudioFrequency 	= samplerate;			//设置采样率
    SAI1A_Handler.Init.FIFOThreshold 	= SAI_FIFOTHRESHOLD_1QF; //设置FIFO阈值,1/4 FIFO
    SAI1A_Handler.Init.MonoStereoMode 	= SAI_STEREOMODE;      //立体声模式
    SAI1A_Handler.Init.Protocol 		= SAI_FREE_PROTOCOL;         //设置SAI1协议为:自由协议(支持I2S/LSB/MSB/TDM/PCM/DSP等协议)
    SAI1A_Handler.Init.DataSize 		= datalen;                   //设置数据大小
    SAI1A_Handler.Init.FirstBit 		= SAI_FIRSTBIT_MSB;          //数据MSB位优先
    SAI1A_Handler.Init.ClockStrobing 	= cpol;                 //数据在时钟的上升/下降沿选通

    SAI1A_Handler.Init.SynchroExt     	= SAI_SYNCEXT_DISABLE;
    SAI1A_Handler.Init.Mckdiv         	= 1; 					/* N.U */			// MCLK = PLLSAI1/Mckdiv*2 == 5.65M
    SAI1A_Handler.Init.CompandingMode 	= SAI_NOCOMPANDING;
    SAI1A_Handler.Init.TriState       	= SAI_OUTPUT_NOTRELEASED;

    //帧设置
    SAI1A_Handler.FrameInit.FrameLength 		= 64;                //设置帧长度为64,左通道32个SCK,右通道32个SCK.
    SAI1A_Handler.FrameInit.ActiveFrameLength 	= 32;          //设置帧同步有效电平长度,在I2S模式下=1/2帧长.
    SAI1A_Handler.FrameInit.FSDefinition 		= SAI_FS_CHANNEL_IDENTIFICATION; //FS信号为SOF信号+通道识别信号
    SAI1A_Handler.FrameInit.FSPolarity 			= SAI_FS_ACTIVE_LOW;  //FS低电平有效(下降沿)
    SAI1A_Handler.FrameInit.FSOffset 			= SAI_FS_BEFOREFIRSTBIT; //在slot0的第一位的前一位使能FS,以匹配飞利浦标准

    //SLOT设置
    SAI1A_Handler.SlotInit.FirstBitOffset 		= 0;               //slot偏移(FBOFF)为0
    SAI1A_Handler.SlotInit.SlotSize 			= SAI_SLOTSIZE_32B;      //slot大小为32位
    SAI1A_Handler.SlotInit.SlotNumber		 	= 2;                   //slot数为2个
    SAI1A_Handler.SlotInit.SlotActive 			= (SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1); //使能slot0和slot1

    HAL_SAI_Init(&SAI1A_Handler);                            //初始化SAI
    __HAL_SAI_ENABLE(&SAI1A_Handler);                        //使能SAI
    SAIA_DMA_Enable();										//开启SAI的DMA功能

    return 0;
}

/**
 * @brief	SAI Block B初始化,I2S,飞利浦标准，MCLK：输出44100HZ
 *
 * @param   mode		工作模式,可以设置:SAI_MODEMASTER_TX/SAI_MODEMASTER_RX/SAI_MODESLAVE_TX/SAI_MODESLAVE_RX
 * @param   cpol		数据在时钟的上升/下降沿选通，可以设置：SAI_CLOCKSTROBING_FALLINGEDGE/SAI_CLOCKSTROBING_RISINGEDGE
 * @param   datalen		数据大小,可以设置：SAI_DATASIZE_8/10/16/20/24/32
 * @param   samplerate	采样率：HZ
 *
 * @return  u8		0:初始化成功，其他:失败
 */
u8 SAIB_Init(u32 mode,u32 cpol,u32 datalen)
{
    SAI1B_Handler.Instance = SAI1_Block_B;  //SAI1 Bock B
    HAL_SAI_DeInit(&SAI1B_Handler);         //清除以前的配置

    SAI1B_Handler.Instance=SAI1_Block_B;                     //SAI1 Bock A
    SAI1B_Handler.Init.AudioMode=mode;                       //设置SAI1工作模式
    SAI1B_Handler.Init.Synchro=SAI_SYNCHRONOUS;             //音频模块同步
    SAI1B_Handler.Init.OutputDrive=SAI_OUTPUTDRIVE_ENABLE;   //立即驱动音频模块输出
    SAI1B_Handler.Init.NoDivider=SAI_MASTERDIVIDER_ENABLE;   //使能主时钟分频器(MCKDIV)
    SAI1B_Handler.Init.FIFOThreshold=SAI_FIFOTHRESHOLD_1QF;  //设置FIFO阈值,1/4 FIFO
    SAI1B_Handler.Init.MonoStereoMode=SAI_STEREOMODE;        //立体声模式
    SAI1B_Handler.Init.Protocol=SAI_FREE_PROTOCOL;           //设置SAI1协议为:自由协议(支持I2S/LSB/MSB/TDM/PCM/DSP等协议)
    SAI1B_Handler.Init.DataSize=datalen;                     //设置数据大小
    SAI1B_Handler.Init.FirstBit=SAI_FIRSTBIT_MSB;            //数据MSB位优先
    SAI1B_Handler.Init.ClockStrobing=cpol;                   //数据在时钟的上升/下降沿选通
    
	SAI1B_Handler.Init.SynchroExt     = SAI_SYNCEXT_DISABLE;
	SAI1B_Handler.Init.CompandingMode = SAI_NOCOMPANDING;
//    SAI1B_Handler.Init.Mckdiv         	= 0; 					/* N.U */			// MCLK = PLLSAI1/2/512 == 11.3M
	SAI1B_Handler.Init.TriState       = SAI_OUTPUT_NOTRELEASED;
	
    //帧设置
    SAI1B_Handler.FrameInit.FrameLength=64;                  //设置帧长度为64,左通道32个SCK,右通道32个SCK.
    SAI1B_Handler.FrameInit.ActiveFrameLength=32;            //设置帧同步有效电平长度,在I2S模式下=1/2帧长.
    SAI1B_Handler.FrameInit.FSDefinition=SAI_FS_CHANNEL_IDENTIFICATION;//FS信号为SOF信号+通道识别信号
    SAI1B_Handler.FrameInit.FSPolarity=SAI_FS_ACTIVE_LOW;    //FS低电平有效(下降沿)
    SAI1B_Handler.FrameInit.FSOffset=SAI_FS_BEFOREFIRSTBIT;  //在slot0的第一位的前一位使能FS,以匹配飞利浦标准	

    //SLOT设置
    SAI1B_Handler.SlotInit.FirstBitOffset=0;                 //slot偏移(FBOFF)为0
    SAI1B_Handler.SlotInit.SlotSize=SAI_SLOTSIZE_32B;        //slot大小为32位
    SAI1B_Handler.SlotInit.SlotNumber=2;                     //slot数为2个    
    SAI1B_Handler.SlotInit.SlotActive=(SAI_SLOTACTIVE_0|SAI_SLOTACTIVE_1);//使能slot0和slot1

    HAL_SAI_Init(&SAI1B_Handler);                            //初始化SAI
    __HAL_SAI_ENABLE(&SAI1B_Handler);                        //使能SAI 
	SAIB_DMA_Enable();										//开启SAI的DMA功能

	return 0;
}

/**
 * @brief	SAI底层驱动，引脚配置，时钟使能,此函数会被HAL_SAI_Init()调用
 *
 * @param   hsdram	SAI句柄
 *
 * @return  void
 */
void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_SAI1_CLK_ENABLE();                //使能SAI1时钟
    __HAL_RCC_GPIOE_CLK_ENABLE();               //使能GPIOE时钟

    //初始化PE2,3,4,5,6
    GPIO_Initure.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_Initure.Mode = GPIO_MODE_AF_PP;        //推挽复用
    GPIO_Initure.Pull = GPIO_PULLUP;            //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;       //高速
    GPIO_Initure.Alternate = GPIO_AF13_SAI1;     //复用为SAI
    HAL_GPIO_Init(GPIOE, &GPIO_Initure);        //初始化
}

/**
 * @brief	开启SAI_A的DMA功能,HAL库没有提供此函数，因此我们需要自己操作寄存器编写一个
 *
 * @param   void
 *
 * @return  void
 */
void SAIA_DMA_Enable(void)
{
    u32 tempreg = 0;
    tempreg = SAI1_Block_A->CR1;        //先读出以前的设置
    tempreg |= 1 << 17;					 //使能DMA
    SAI1_Block_A->CR1 = tempreg;		  //写入CR1寄存器中
}

/**
 * @brief	开启SAI_B的DMA功能,HAL库没有提供此函数，因此我们需要自己操作寄存器编写一个
 *
 * @param   void
 *
 * @return  void
 */
void SAIB_DMA_Enable(void)
{
    u32 tempreg=0;
    tempreg=SAI1_Block_B->CR1;          //先读出以前的设置			
	tempreg|=1<<17;					    //使能DMA
	SAI1_Block_B->CR1=tempreg;		    //写入CR1寄存器中
}

/**
 * @brief	SAIA TX DMA配置
 *
 * @param   width	位宽(存储器和外设,同时设置),0,8位;1,16位;2,32位
 *
 * @return  void
 */
void SAIA_TX_DMA_Init(u8 width)
{
    u32 memwidth = 0, perwidth = 0; //外设和存储器位宽

    switch(width)
    {
        case 0:         //8位
            memwidth = DMA_MDATAALIGN_BYTE;
            perwidth = DMA_PDATAALIGN_BYTE;
            break;

        case 1:         //16位
            memwidth = DMA_MDATAALIGN_HALFWORD;
            perwidth = DMA_PDATAALIGN_HALFWORD;
            break;

        case 2:         //32位
            memwidth = DMA_MDATAALIGN_WORD;
            perwidth = DMA_PDATAALIGN_WORD;
            break;

    }

    __HAL_RCC_DMA2_CLK_ENABLE();                                    //使能DMA2时钟

    __HAL_LINKDMA(&SAI1A_Handler, hdmatx, SAI1_TXDMA_Handler);       //将DMA与SAI联系起来

    SAI1_TXDMA_Handler.Instance = DMA2_Channel1;                     //DMA2数据流1
    SAI1_TXDMA_Handler.Init.Request = DMA_REQUEST_1;                 //通道1
    SAI1_TXDMA_Handler.Init.Direction = DMA_MEMORY_TO_PERIPH;       //存储器到外设模式
    SAI1_TXDMA_Handler.Init.PeriphInc = DMA_PINC_DISABLE;           //外设非增量模式
    SAI1_TXDMA_Handler.Init.MemInc = DMA_MINC_ENABLE;               //存储器增量模式
    SAI1_TXDMA_Handler.Init.PeriphDataAlignment = perwidth;         //外设数据长度:16/32位
    SAI1_TXDMA_Handler.Init.MemDataAlignment = memwidth;            //存储器数据长度:16/32位
    SAI1_TXDMA_Handler.Init.Mode = DMA_NORMAL;						//使用循环模式
    SAI1_TXDMA_Handler.Init.Priority = DMA_PRIORITY_HIGH;           //高优先级
    HAL_DMA_DeInit(&SAI1_TXDMA_Handler);                            //先清除以前的设置
    HAL_DMA_Init(&SAI1_TXDMA_Handler);	                            //初始化DMA

    __HAL_DMA_DISABLE(&SAI1_TXDMA_Handler);                         //先关闭DMA
    delay_us(10);                                                   //10us延时，防止-O2优化出问题
    __HAL_DMA_ENABLE_IT(&SAI1_TXDMA_Handler, DMA_IT_TC);            //开启传输完成中断

    __HAL_DMA_CLEAR_FLAG(&SAI1_TXDMA_Handler, DMA_FLAG_TC1);     	//清除DMA传输完成中断标志位

    HAL_NVIC_SetPriority(DMA2_Channel1_IRQn, 0, 0);                  //DMA中断优先级
    HAL_NVIC_EnableIRQ(DMA2_Channel1_IRQn);
}

/**
 * @brief	SAIB RX DMA配置
 *
 * @param   width	位宽(存储器和外设,同时设置),0,8位;1,16位;2,32位
 *
 * @return  void
 */
void SAIB_RX_DMA_Init(u8 width)
{ 
    u32 memwidth=0,perwidth=0;      //外设和存储器位宽
    switch(width)
    {
        case 0:         //8位
            memwidth=DMA_MDATAALIGN_BYTE;
            perwidth=DMA_PDATAALIGN_BYTE;
            break;
        case 1:         //16位
            memwidth=DMA_MDATAALIGN_HALFWORD;
            perwidth=DMA_PDATAALIGN_HALFWORD;
            break;
        case 2:         //32位
            memwidth=DMA_MDATAALIGN_WORD;
            perwidth=DMA_PDATAALIGN_WORD;
            break;
    }
	
    __HAL_RCC_DMA2_CLK_ENABLE();                                    //使能DMA2时钟
	
    __HAL_LINKDMA(&SAI1B_Handler,hdmarx,SAI1_RXDMA_Handler);         //将DMA与SAI联系起来
	
    SAI1_RXDMA_Handler.Instance=DMA2_Channel2;                       //DMA2数据流1                 
    SAI1_RXDMA_Handler.Init.Request = DMA_REQUEST_1;                 //通道1
    SAI1_RXDMA_Handler.Init.Direction=DMA_PERIPH_TO_MEMORY;         //外设到存储器模式
    SAI1_RXDMA_Handler.Init.PeriphInc=DMA_PINC_DISABLE;             //外设非增量模式
    SAI1_RXDMA_Handler.Init.MemInc=DMA_MINC_ENABLE;                 //存储器增量模式
    SAI1_RXDMA_Handler.Init.PeriphDataAlignment=perwidth;           //外设数据长度:16/32位
    SAI1_RXDMA_Handler.Init.MemDataAlignment=memwidth;              //存储器数据长度:16/32位
    SAI1_RXDMA_Handler.Init.Mode=DMA_CIRCULAR;						//使用循环模式 
    SAI1_RXDMA_Handler.Init.Priority=DMA_PRIORITY_MEDIUM;             //高优先级
    HAL_DMA_DeInit(&SAI1_RXDMA_Handler);                            //先清除以前的设置
    HAL_DMA_Init(&SAI1_RXDMA_Handler);	                            //初始化DMA
    
    HAL_NVIC_SetPriority(DMA2_Channel2_IRQn,0,1);                    //DMA中断优先级
    HAL_NVIC_EnableIRQ(DMA2_Channel2_IRQn);
}


//SAI DMA回调函数指针
void (*sai_tx_callback)(void);	//TX回调函数
void (*sai_rx_callback)(u16 data);	//RX回调函数 

/**
 * @brief	DMA2_Channel1中断服务函数
 *
 * @param   void
 *
 * @return  void
 */
void DMA2_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&SAI1_TXDMA_Handler);

}

/**
 * @brief	SAI发送完成回调函数，此函数由HAL_DMA_IRQHandler调用
 *
 * @param   hsai	SAI句柄
 *
 * @return  void
 */
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    if(sai_tx_callback != NULL)
        sai_tx_callback();	//执行回调函数,读取数据等操作在这里面处理
}

/**
 * @brief	SAI开始播放
 *
 * @param   void
 *
 * @return  void
 */
void SAI_Play_Start(void)
{
    __HAL_DMA_ENABLE(&SAI1_TXDMA_Handler);//开启DMA TX传输
}
/**
 * @brief	SAI停止播放
 *
 * @param   void
 *
 * @return  void
 */
void SAI_Play_Stop(void)
{
    __HAL_DMA_DISABLE(&SAI1_TXDMA_Handler);  //关闭DMA传输，结束播放
}

/**
 * @brief	DMA2_Channel2中断服务函数
 *
 * @param   void
 *
 * @return  void
 */
void DMA2_Channel2_IRQHandler(void)
{
	if( __HAL_DMA_GET_FLAG(&SAI1_RXDMA_Handler,DMA_FLAG_TC2) != RESET )	
	{
		__HAL_DMA_CLEAR_FLAG(&SAI1_RXDMA_Handler,DMA_FLAG_TC2);
		if(sai_rx_callback!=NULL)sai_rx_callback(SAI_RX_DMA_BUF_SIZE);	//执行回调函数,读取数据等操作在这里面处理 
	}

	if( __HAL_DMA_GET_FLAG(&SAI1_RXDMA_Handler,DMA_FLAG_HT2) != RESET )
	{
		__HAL_DMA_CLEAR_FLAG(&SAI1_RXDMA_Handler,DMA_FLAG_HT2);
		if(sai_rx_callback!=NULL)sai_rx_callback(0);	//执行回调函数,读取数据等操作在这里面处理 	
	}

	HAL_DMA_IRQHandler(&SAI1_RXDMA_Handler);
}
/**
 * @brief	SAIB开始录音
 *
 * @param   void
 *
 * @return  void
 */
void SAI_Rec_Start(void)
{ 
    __HAL_DMA_ENABLE(&SAI1_RXDMA_Handler);//开启DMA RX传输      		
}
/**
 * @brief	SAIB停止录音
 *
 * @param   void
 *
 * @return  void
 */
void SAI_Rec_Stop(void)
{   
    __HAL_DMA_DISABLE(&SAI1_RXDMA_Handler);//结束录音    
}



