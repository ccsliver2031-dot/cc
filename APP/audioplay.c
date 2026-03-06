#include "audioplay.h"
#include "ff.h"
#include "malloc.h"
#include "usart.h"
#include "es8388.h"
#include "sai.h"
#include "led.h"
#include "lcd.h"
#include "delay.h"
#include "key.h"
#include "exfuns.h"  
#include "string.h"  
#include "text.h"

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
 
 

//音乐播放控制器
__audiodev audiodev;	  
 
/**
 * @brief	开始音频播放
 *
 * @param   void
 *
 * @return  void
 */
void audio_start(void)
{
	audiodev.status=1;//非暂停
	SAI_Play_Start();
} 
/**
 * @brief	关闭音频播放
 *
 * @param   void
 *
 * @return  void
 */
void audio_stop(void)
{
	audiodev.status=0;//暂停
	SAI_Play_Stop();
}

/**
 * @brief	得到path路径下,目标文件的总个数
 *
 * @param   void
 *
 * @return  u16		总有效文件数
 */
u16 audio_get_tnum(u8 *path)
{	  
	u8 res;
	u16 rval=0;
 	DIR tdir;	 		//临时目录
	FILINFO* tfileinfo;	//临时文件信息	 	
	tfileinfo=(FILINFO*)mymalloc(SRAM1,sizeof(FILINFO));//申请内存
    res=f_opendir(&tdir,(const TCHAR*)path); //打开目录 
	if(res==FR_OK&&tfileinfo)
	{
		while(1)//查询总的有效文件数
		{
	        res=f_readdir(&tdir,tfileinfo);       			//读取目录下的一个文件
	        if(res!=FR_OK||tfileinfo->fname[0]==0)break;	//错误了/到末尾了,退出	 		 
			res=f_typetell((u8*)tfileinfo->fname);	
			if((res&0XF0)==0X40)//取高四位,看看是不是音乐文件	
			{
				rval++;//有效文件数增加1
			}	    
		}  
	}  
	myfree(SRAM1,tfileinfo);//释放内存
	return rval;
}
/**
 * @brief	显示曲目索引
 *
 * @param   index	当前索引
 * @param   total	总文件数
 *
 * @return  u16		总有效文件数
 */
void audio_index_show(u16 index,u16 total)
{
	//显示当前曲目的索引,及总曲目数
	LCD_ShowxNum(30+0,216,index,3,16,0X80);		//索引
	LCD_ShowChar(30+24,216,'/',16);
	LCD_ShowxNum(30+32,216,total,3,16,0X80); 	//总曲目				  	  
}
/**
 * @brief	显示播放时间,比特率 信息
 *
 * @param   totsec	音频文件总时间长度
 * @param  	cursec	当前播放时间
 * @param  	bitrate	比特率(位速)
 *
 * @return  void
 */ 
void audio_msg_show(u32 totsec,u32 cursec,u32 bitrate)
{	
	static u16 playtime=0XFFFF;//播放时间标记	      
	if(playtime!=cursec)					//需要更新显示时间
	{
		playtime=cursec;
		//显示播放时间			 
		LCD_ShowxNum(30,199,playtime/60,2,16,0X80);		//分钟
		LCD_ShowChar(30+16,199,':',16);
		LCD_ShowxNum(30+24,199,playtime%60,2,16,0X80);	//秒钟		
 		LCD_ShowChar(30+40,199,'/',16); 	    	 
		//显示总时间    	   
 		LCD_ShowxNum(30+48,199,totsec/60,2,16,0X80);	//分钟
		LCD_ShowChar(30+64,199,':',16);
		LCD_ShowxNum(30+72,199,totsec%60,2,16,0X80);	//秒钟	  		    
		//显示位率			   
   		LCD_ShowxNum(30+110,199,bitrate/1000,4,16,0X80);//显示位率	 
		LCD_ShowString(30+110+32,199,200,16,16,"Kbps");	 
	}
}

/**
 * @brief	播放音乐
 *
 * @param   void
 *
 * @return  void
 */ 
void audio_play(void)
{
	u8 res;
 	DIR wavdir;	 		//目录
	FILINFO *wavfileinfo;//文件信息 
	u8 *pname;			//带路径的文件名
	u16 totwavnum; 		//音乐文件总数
	u16 curindex;		//当前索引
	u8 key;				//键值		  
 	u32 temp;
	u32 *wavoffsettbl;	//音乐offset索引表
 
	ES8388_ADDA_Cfg(0,1);	//开启DAC关闭ADC
	ES8388_Output_Cfg(1);	//DAC选择通道1输出
	
 	while(f_opendir(&wavdir,"0:/MUSIC"))//打开音乐文件夹
 	{		
		Show_Str(30,190,240,16,(u8*)"MUSIC文件夹错误!",16);
		delay_ms(200);				  
		LCD_Fill(30,190,240,206,WHITE);//清除显示	     
		delay_ms(200);				  				  		  
	} 									  
	totwavnum=audio_get_tnum((u8*)"0:/MUSIC"); //得到总有效文件数
  	while(totwavnum==NULL)//音乐文件总数为0		
 	{	    
		Show_Str(30,190,240,16,(u8*)"没有音乐文件!",16);
		delay_ms(200);				  
		LCD_Fill(30,190,240,146,WHITE);//清除显示	     
		delay_ms(200);							  			  
	}										   
	wavfileinfo=(FILINFO*)mymalloc(SRAM1,sizeof(FILINFO));	//申请内存
  	pname=mymalloc(SRAM1,_MAX_LFN*2+1);					//为带路径的文件名分配内存
 	wavoffsettbl=mymalloc(SRAM1,4*totwavnum);				//申请4*totwavnum个字节的内存,用于存放音乐文件off block索引
 	while(!wavfileinfo||!pname||!wavoffsettbl)//内存分配出错
 	{	    
		Show_Str(30,190,240,16,(u8*)"内存分配失败!",16);
		delay_ms(200);				  
		LCD_Fill(30,190,240,146,WHITE);//清除显示	     
		delay_ms(200);				  				  
	}  	 
 	//记录索引
    res=f_opendir(&wavdir,"0:/MUSIC"); //打开目录
	if(res==FR_OK)
	{
		curindex=0;//当前索引为0
		while(1)//全部查询一遍
		{						
			temp=wavdir.dptr;								//记录当前index 
	        res=f_readdir(&wavdir,wavfileinfo);       		//读取目录下的一个文件
	        if(res!=FR_OK||wavfileinfo->fname[0]==0)break;	//错误了/到末尾了,退出 		 
			res=f_typetell((u8*)wavfileinfo->fname);	
			if((res&0XF0)==0X40)//取高四位,看看是不是音乐文件	
			{
				wavoffsettbl[curindex]=temp;//记录索引
				curindex++;
			}	    
		} 
	}   
   	curindex=0;											//从0开始显示
   	res=f_opendir(&wavdir,(const TCHAR*)"0:/MUSIC"); 	//打开目录
	while(res==FR_OK)//打开成功
	{	
		dir_sdi(&wavdir,wavoffsettbl[curindex]);				//改变当前目录索引	   
        res=f_readdir(&wavdir,wavfileinfo);       				//读取目录下的一个文件
        if(res!=FR_OK||wavfileinfo->fname[0]==0)break;			//错误了/到末尾了,退出		 
		strcpy((char*)pname,"0:/MUSIC/");						//复制路径(目录)
		strcat((char*)pname,(const char*)wavfileinfo->fname);	//将文件名接在后面
 		LCD_Fill(30,182,LCD_Width-1,182+16,WHITE);			//清除之前的显示
		Show_Str(30,182,LCD_Width-1,16,(u8*)wavfileinfo->fname,16);//显示歌曲名字 
		audio_index_show(curindex+1,totwavnum); 
		key=audio_play_song(pname); 			 		//播放这个音频文件
		if(key==KEY2_PRES)		//上一曲
		{
			if(curindex)curindex--;
			else curindex=totwavnum-1;
 		}else if(key==KEY0_PRES)//下一曲
		{
			curindex++;		   	
			if(curindex>=totwavnum)curindex=0;//到末尾的时候,自动从头开始
 		}else break;	//产生了错误 	 
	} 											   		    
	myfree(SRAM1,wavfileinfo);			//释放内存			    
	myfree(SRAM1,pname);				//释放内存			    
	myfree(SRAM1,wavoffsettbl);		//释放内存	 
}

/**
 * @brief	播放某个音频文件
 *
 * @param   fname	WAV文件路径
 *
 * @return  u8		返回按键值
 */
u8 audio_play_song(u8* fname)
{
	u8 res;  
	res=f_typetell(fname); 
	switch(res)
	{
		case T_WAV:
			res=wav_play_song(fname);
			break;
		default://其他文件,自动跳转到下一曲
			printf("can't play:%s\r\n",fname);
			res=KEY0_PRES;
			break;
	}
	return res;
}































