/**************************************************************
The initial and control for 320×240 16Bpp TFT LCD----LCD_LTV350QV_F05
**************************************************************/

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"

#include "bootpara.h"
//#include "LCD_LTV350QV_F05.h"
//#define LCD_320_240	1
//#define LCD_640_480	0
#define M5D(n) ((n) & 0x1fffff)	// To get lower 21bits

#define MVAL		(13)
#define MVAL_USED 	(0)		//0=each frame   1=rate by MVAL
#define INVVDEN		(1)		//0=normal       1=inverted
#define BSWP		(0)		//Byte swap control
#define HWSWP		(1)		//Half word swap control

/***************************************************
  320*240
****************************************************/
#define LCD_XSIZE_320_240 	(320)
#define LCD_YSIZE_320_240 	(240)

#define SCR_XSIZE_320_240 	(320)
#define SCR_YSIZE_320_240 	(240)

#define HOZVAL_TFT_320_240	(LCD_XSIZE_320_240-1)//分辨率
#define LINEVAL_TFT_320_240	(LCD_YSIZE_320_240-1)

#define VBPD_320_240		(3)		//垂直同步信号的后肩
#define VFPD_320_240		(5)		//垂直同步信号的前肩
#define VSPW_320_240		(15)		//垂直同步信号的脉宽

#define HBPD_320_240		(58)		//8水平同步信号的后肩
#define HFPD_320_240		(15)		//8水平同步信号的前肩
#define HSPW_320_240		(8)		//6水平同步信号的脉宽

#define CLKVAL_TFT_320_240	(9) //3	


/***************************************************
****************************************************/



//volatile static unsigned short LCD_BUFFER[SCR_YSIZE][SCR_XSIZE];
#define LCD_BUFFER 0x30100000               //图片存储地址
#define PIC_BUFFER LCD_BUFFER - 0x44		//跳过图片头信息		

/**************************************************************
320×240 16Bpp TFT LCD功能模块初始化
**************************************************************/
static void Lcd_Init_320_240(void)
{
		//0x30000000;
  rGPCUP=0xffffffff; // Disable Pull-up register
  rGPCCON=0xaaaa56a9; //Initialize VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND 

  rGPDUP=0xffffffff; // Disable Pull-up register
  rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]

	rLCDCON1=(CLKVAL_TFT_320_240<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,12bpp TFT,ENVID=off
	rLCDCON2=(VBPD_320_240<<24)|(LINEVAL_TFT_320_240<<14)|(VFPD_320_240<<6)|(VSPW_320_240);
	rLCDCON3=(HBPD_320_240<<19)|(HOZVAL_TFT_320_240<<8)|(HFPD_320_240);
	rLCDCON4=(MVAL<<8)|(HSPW_320_240);
	rLCDCON5=(1<<11)|(1<<9)|(1<<8)|(1<<3)|(BSWP<<1)|(HWSWP);
	//rLCDCON5=(1<<11)|(0<<9)|(0<<8)|(0<<6)|(BSWP<<1)|(HWSWP);	//FRM5:6:5,HSYNC and VSYNC are inverted

	rLCDSADDR1=(((U32)LCD_BUFFER>>22)<<21)|M5D((U32)LCD_BUFFER>>1);
	rLCDSADDR2=M5D( ((U32)LCD_BUFFER+(SCR_XSIZE_320_240*LCD_YSIZE_320_240*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE_320_240-LCD_XSIZE_320_240)/1)<<11)|(LCD_XSIZE_320_240/1);
	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
    //rTCONSEL|=((1<<4)|1); // Disable LCC3600, LPC3600
	rTPAL=0; // Disable Temp Palette
}

/**************************************************************
LCD视频和控制信号输出或者停止，1开启视频输出
**************************************************************/
static void Lcd_EnvidOnOff(int onoff)
{
    if(onoff==1)
	rLCDCON1|=1; // ENVID=ON
    else
	rLCDCON1 =rLCDCON1 & 0x3fffe; // ENVID Off
}

/**************************************************************
320×240 8Bpp TFT LCD 电源控制引脚使能
**************************************************************/
static void Lcd_PowerEnable(int invpwren,int pwren)
{
    //GPG4 is setted as LCD_PWREN
    rGPGUP=rGPGUP&(~(1<<4))|(1<<4); // Pull-up disable
    rGPGCON=rGPGCON&(~(3<<8))|(3<<8); //GPG4=LCD_PWREN
    //Enable LCD POWER ENABLE Function
    rLCDCON5=rLCDCON5&(~(1<<3))|(pwren<<3);   // PWREN
    rLCDCON5=rLCDCON5&(~(1<<5))|(invpwren<<5);   // INVPWREN
}

/**************************************************************
**************************************************************/
extern void LoadPic(U32 PicBuffer);
void Lcd_Tft_LTV350QV_F05_Init(void)
{
	Lcd_Init_320_240();
  LcdBkLtSet( 70 ) ;
	Lcd_PowerEnable(0, 1);
  Lcd_EnvidOnOff(1);		//turn on vedio
  LoadPic(PIC_BUFFER);

}


