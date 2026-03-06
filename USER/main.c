#include "sys.h"
#include "usart.h"
#include "usmart.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "malloc.h"
#include "w25qxx.h"
#include "sd_card.h"
#include "ff.h"
#include "exfuns.h"
#include "fontupd.h"
#include "text.h"
#include "es8388.h"
#include "audioplay.h"
#include "recorder.h"

/*********************************************************************************
			  ___   _     _____  _____  _   _  _____  _____  _   __
			 / _ \ | |   |_   _||  ___|| \ | ||_   _||  ___|| | / /
			/ /_\ \| |     | |  | |__  |  \| |  | |  | |__  | |/ /
			|  _  || |     | |  |  __| | . ` |  | |  |  __| |    \
			| | | || |_____| |_ | |___ | |\  |  | |  | |___ | |\  \
			\_| |_/\_____/\___/ \____/ \_| \_/  \_/  \____/ \_| \_/

 *	******************************************************************************
 *	ALIENTEK Pandora STM32L IOT开发板	实验31
 *	录音机实验		HAL库版本
 *	技术支持：www.openedv.com
 *	淘宝店铺：http://eboard.taobao.com
 *	关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 *	广州市星翼电子科技有限公司
 *	作者：正点原子 @ALIENTEK
 *	******************************************************************************/

/*

	使用注意事项：该例程只支持44.1KHz/16bit的WAV音频播放

*/
int main(void)
{
    HAL_Init();
    SystemClock_Config();		//初始化系统时钟为80M
    delay_init(80); 			//初始化延时函数    80M系统时钟
    uart_init(115200);			//初始化串口，波特率为115200

    LED_Init();					//初始化LED
    KEY_Init();					//初始化按键
    LCD_Init();					//初始化LCD
    W25QXX_Init();				//初始化W25Q256

    my_mem_init(SRAM1);			//初始化内部SRAM1内存池
    my_mem_init(SRAM2);			//初始化内部SRAM2内存池

    ES8388_Init();				//ES8388初始化
    ES8388_Set_Volume(33);		//设置耳机音量大小

    exfuns_init();		        //为fatfs相关变量申请内存
    f_mount(fs[0], "0:", 1);    //挂载SD卡
    f_mount(fs[1], "1:", 1);    //挂载SPI FLASH.

    POINT_COLOR = RED;
    Display_ALIENTEK_LOGO(0, 0);

    while(font_init()) 			//检查字库
    {
        LCD_ShowString(30, 100, 200, 16, 16, "Font Error!");
        delay_ms(200);
        LCD_Fill(30, 100, 240, 116, WHITE); //清除显示
        delay_ms(200);
    }

    while(SD_Init())			//检查SD卡
    {
        LCD_ShowString(30, 100, 200, 16, 16, "SD Error!");
        delay_ms(200);
        LCD_Fill(30, 100, 240, 116, WHITE); //清除显示
        delay_ms(200);
    }

    Show_Str(30, 80, 200, 16, (u8*)"潘多拉STM32L4 IOT开发板", 16);
    Show_Str(30, 97, 200, 16, (u8*)"录音机实验", 16);
    Show_Str(30, 114, 200, 16, (u8*)"正点原子@ALIENTEK", 16);
    Show_Str(30, 131, 200, 16, (u8*)"2018年10月27日", 16);

    while(1)
    {
        wav_recorder();
    }
}


