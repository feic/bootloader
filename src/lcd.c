/**************************************************************
The initial and control for 320×240 16Bpp TFT LCD----LCD_LTV350QV_F05
**************************************************************/

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h" 
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
  480*272
****************************************************/
#define LCD_XSIZE_480_272 	(480)
#define LCD_YSIZE_480_272 	(272)

#define SCR_XSIZE_480_272 	(480)
#define SCR_YSIZE_480_272 	(272)

#define HOZVAL_TFT_480_272	(LCD_XSIZE_480_272-1)//分辨率
#define LINEVAL_TFT_480_272	(LCD_YSIZE_480_272-1)

#define VBPD_480_272		(3)		//垂直同步信号的后肩
#define VFPD_480_272		(5)		//垂直同步信号的前肩
#define VSPW_480_272		(5)		//垂直同步信号的脉宽

#define HBPD_480_272		(33)		//8水平同步信号的后肩
#define HFPD_480_272		(15)		//8水平同步信号的前肩
#define HSPW_480_272		(8)		//6水平同步信号的脉宽

#define CLKVAL_TFT_480_272	(3) //3	
/***************************************************
  640*480
****************************************************/
#define LCD_XSIZE_640_480 	(640)
#define LCD_YSIZE_640_480 	(480)

#define SCR_XSIZE_640_480 	(640)
#define SCR_YSIZE_640_480 	(480)

#define HOZVAL_TFT_640_480	(LCD_XSIZE_640_480-1)//分辨率
#define LINEVAL_TFT_640_480	(LCD_YSIZE_640_480-1)

#define VBPD_640_480		25
#define VFPD_640_480		5
#define VSPW_640_480		1

#define HBPD_640_480		67
#define HFPD_640_480		40
#define HSPW_640_480		31

#define CLKVAL_TFT_640_480	(1) 
/***************************************************
  800*600
****************************************************/
#define LCD_XSIZE_800_600 	(800)
#define LCD_YSIZE_800_600 	(600)

#define SCR_XSIZE_800_600 	(800)
#define SCR_YSIZE_800_600 	(600)

#define HOZVAL_TFT_800_600	(LCD_XSIZE_800_600-1)//分辨率
#define LINEVAL_TFT_800_600	(LCD_YSIZE_800_600-1)

#define VBPD_800_600		22//8//(25)		//垂直同步信号的后肩
#define VFPD_800_600		36//5//(5)		//垂直同步信号的前肩
#define VSPW_800_600		5//(1)		//垂直同步信号的脉宽

#define HBPD_800_600		63//(110)//67		//8水平同步信号的后肩
#define HFPD_800_600		55//(40)		//8水平同步信号的前肩
#define HSPW_800_600		119//(31)		//6水平同步信号的脉宽

#define CLKVAL_TFT_800_600	(1) //3	
/***************************************************
  800*480
****************************************************/
#define LCD_XSIZE_800_480 	(800)
#define LCD_YSIZE_800_480 	(480)

#define SCR_XSIZE_800_480 	(800)
#define SCR_YSIZE_800_480 	(480)

#define HOZVAL_TFT_800_480	(LCD_XSIZE_800_480-1)//分辨率
#define LINEVAL_TFT_800_480	(LCD_YSIZE_800_480-1)

#define VBPD_800_480		25//8//(25)		//垂直同步信号的后肩
#define VFPD_800_480		5//5//(5)		//垂直同步信号的前肩
#define VSPW_800_480		1//(1)		//垂直同步信号的脉宽

#define HBPD_800_480		67//(110)//67		//8水平同步信号的后肩
#define HFPD_800_480		40//(40)		//8水平同步信号的前肩
#define HSPW_800_480		31//(31)		//6水平同步信号的脉宽

#define CLKVAL_TFT_800_480	(1) //3	

/***************************************************
****************************************************/

extern void Uart_Printf(char *f, ...) ;

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
480×272 16Bpp TFT LCD功能模块初始化
**************************************************************/
static void Lcd_Init_480_272(void)
{
		//0x30000000;
  rGPCUP=0xffffffff; // Disable Pull-up register
  rGPCCON=0xaaaa56a9; //Initialize VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND 

  rGPDUP=0xffffffff; // Disable Pull-up register
  rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]

	rLCDCON1=(CLKVAL_TFT_480_272<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,12bpp TFT,ENVID=off
	rLCDCON2=(VBPD_480_272<<24)|(LINEVAL_TFT_480_272<<14)|(VFPD_480_272<<6)|(VSPW_480_272);
	rLCDCON3=(HBPD_480_272<<19)|(HOZVAL_TFT_480_272<<8)|(HFPD_480_272);
	rLCDCON4=(MVAL<<8)|(HSPW_480_272);
	rLCDCON5=(1<<11)|(1<<9)|(1<<8)|(1<<3)|(BSWP<<1)|(HWSWP);
	//rLCDCON5=(1<<11)|(0<<9)|(0<<8)|(0<<6)|(BSWP<<1)|(HWSWP);	//FRM5:6:5,HSYNC and VSYNC are inverted

	rLCDSADDR1=(((U32)LCD_BUFFER>>22)<<21)|M5D((U32)LCD_BUFFER>>1);
	rLCDSADDR2=M5D( ((U32)LCD_BUFFER+(SCR_XSIZE_480_272*LCD_YSIZE_480_272*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE_480_272-LCD_XSIZE_480_272)/1)<<11)|(LCD_XSIZE_480_272/1);
	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
    //rTCONSEL|=((1<<4)|1); // Disable LCC3600, LPC3600
	rTPAL=0; // Disable Temp Palette
}
/**************************************************************
640×480 16Bpp TFT LCD功能模块初始化
**************************************************************/
static void Lcd_Init_640_480(void)
{
		//0x30000000;
  rGPCUP=0xffffffff; // Disable Pull-up register
  rGPCCON=0xaaaa56a9; //Initialize VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND 

  rGPDUP=0xffffffff; // Disable Pull-up register
  rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]

	rLCDCON1=(CLKVAL_TFT_640_480<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,12bpp TFT,ENVID=off
	rLCDCON2=(VBPD_640_480<<24)|(LINEVAL_TFT_640_480<<14)|(VFPD_640_480<<6)|(VSPW_640_480);
	rLCDCON3=(HBPD_640_480<<19)|(HOZVAL_TFT_640_480<<8)|(HFPD_640_480);
	rLCDCON4=(MVAL<<8)|(HSPW_640_480);
	rLCDCON5=(1<<11)|(1<<9)|(1<<8)|(1<<3)|(BSWP<<1)|(HWSWP);
	//rLCDCON5=(1<<11)|(0<<9)|(0<<8)|(0<<6)|(BSWP<<1)|(HWSWP);	//FRM5:6:5,HSYNC and VSYNC are inverted

	rLCDSADDR1=(((U32)LCD_BUFFER>>22)<<21)|M5D((U32)LCD_BUFFER>>1);
	rLCDSADDR2=M5D( ((U32)LCD_BUFFER+(SCR_XSIZE_640_480*LCD_YSIZE_640_480*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE_640_480-LCD_XSIZE_640_480)/1)<<11)|(LCD_XSIZE_640_480/1);
	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
    //rTCONSEL|=((1<<4)|1); // Disable LCC3600, LPC3600
	rTPAL=0; // Disable Temp Palette
}
/**************************************************************
800×600 16Bpp TFT LCD功能模块初始化
**************************************************************/
static void Lcd_Init_800_600(void)
{
		//0x30000000;
  rGPCUP=0xffffffff; // Disable Pull-up register
  rGPCCON=0xaaaa56a9; //Initialize VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND 

  rGPDUP=0xffffffff; // Disable Pull-up register
  rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]

	rLCDCON1=(CLKVAL_TFT_800_600<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,12bpp TFT,ENVID=off
	rLCDCON2=(VBPD_800_600<<24)|(LINEVAL_TFT_800_600<<14)|(VFPD_800_600<<6)|(VSPW_800_600);
	rLCDCON3=(HBPD_800_600<<19)|(HOZVAL_TFT_800_600<<8)|(HFPD_800_600);
	rLCDCON4=(MVAL<<8)|(HSPW_800_600);
	rLCDCON5=(1<<11)|(1<<10)|(0<<9)|(0<<8)|(1<<3)|(BSWP<<1)|(HWSWP);
	//rLCDCON5=(1<<11)|(0<<9)|(0<<8)|(0<<6)|(BSWP<<1)|(HWSWP);	//FRM5:6:5,HSYNC and VSYNC are inverted

	rLCDSADDR1=(((U32)LCD_BUFFER>>22)<<21)|M5D((U32)LCD_BUFFER>>1);
	rLCDSADDR2=M5D( ((U32)LCD_BUFFER+(SCR_XSIZE_800_600*LCD_YSIZE_800_600*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE_800_600-LCD_XSIZE_800_600)/1)<<11)|(LCD_XSIZE_800_600/1);
	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
    //rTCONSEL|=((1<<4)|1); // Disable LCC3600, LPC3600
	rTPAL=0; // Disable Temp Palette
}
/**************************************************************
800×480 16Bpp TFT LCD功能模块初始化
**************************************************************/
static void Lcd_Init_800_480(void)
{
		//0x30000000;
  rGPCUP=0xffffffff; // Disable Pull-up register
  rGPCCON=0xaaaa56a9; //Initialize VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND 

  rGPDUP=0xffffffff; // Disable Pull-up register
  rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]

	rLCDCON1=(CLKVAL_TFT_800_480<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,12bpp TFT,ENVID=off
	rLCDCON2=(VBPD_800_480<<24)|(LINEVAL_TFT_800_480<<14)|(VFPD_800_480<<6)|(VSPW_800_480);
	rLCDCON3=(HBPD_800_480<<19)|(HOZVAL_TFT_800_480<<8)|(HFPD_800_480);
	rLCDCON4=(MVAL<<8)|(HSPW_800_480);
	rLCDCON5=(1<<11)|(1<<9)|(1<<8)|(1<<3)|(BSWP<<1)|(HWSWP);
	//rLCDCON5=(1<<11)|(0<<9)|(0<<8)|(0<<6)|(BSWP<<1)|(HWSWP);	//FRM5:6:5,HSYNC and VSYNC are inverted

	rLCDSADDR1=(((U32)LCD_BUFFER>>22)<<21)|M5D((U32)LCD_BUFFER>>1);
	rLCDSADDR2=M5D( ((U32)LCD_BUFFER+(SCR_XSIZE_800_480*LCD_YSIZE_800_480*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE_800_480-LCD_XSIZE_800_480)/1)<<11)|(LCD_XSIZE_800_480/1);
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
#if 0
/**************************************************************
320×240 16Bpp TFT LCD移动观察窗口
**************************************************************/
static void Lcd_MoveViewPort(int vx,int vy)
{
    U32 addr;

    SET_IF(); 
	#if (LCD_XSIZE<32)
    	    while((rLCDCON1>>18)<=1); // if x<32
	#else	
    	    while((rLCDCON1>>18)==0); // if x>32
	#endif
    
    addr=(U32)LCD_BUFFER+(vx*2)+vy*(SCR_XSIZE*2);
	rLCDSADDR1= ( (addr>>22)<<21 ) | M5D(addr>>1);
	rLCDSADDR2= M5D(((addr+(SCR_XSIZE*LCD_YSIZE*2))>>1));
	CLR_IF();
}    

/**************************************************************
320×240 16Bpp TFT LCD移动观察窗口
**************************************************************/
static void MoveViewPort(void)
{
    int vx=0,vy=0,vd=1;

    Uart_Printf("\n*Move the LCD view windos:\n");
    Uart_Printf(" press 8 is up\n");
    Uart_Printf(" press 2 is down\n");
    Uart_Printf(" press 4 is left\n");
    Uart_Printf(" press 6 is right\n");
    Uart_Printf(" press Enter to exit!\n");

    while(1)
    {
    	switch( Uart_GetKey() )
    	{
    	case '8':
	    if(vy>=vd)vy-=vd;    	   	
        break;

    	case '4':
    	    if(vx>=vd)vx-=vd;
    	break;

    	case '6':
                if(vx<=(SCR_XSIZE-LCD_XSIZE-vd))vx+=vd;   	    
   	    break;

    	case '2':
                if(vy<=(SCR_YSIZE-LCD_YSIZE-vd))vy+=vd;   	    
   	    break;

    	case '\r':
   	    return;

    	default:
	    break;
		}
	Uart_Printf("vx=%3d,vy=%3d\n",vx,vy);
	Lcd_MoveViewPort(vx,vy);
    }
}

/**************************************************************
320×240 16Bpp TFT LCD单个象素的显示数据输出
**************************************************************/
static void PutPixel(U32 x,U32 y, U32 c )
{
	if ( (x < SCR_XSIZE) && (y < SCR_YSIZE) )
		LCD_BUFFER[(y)][(x)] = c;
}

/**************************************************************
320×240 16Bpp TFT LCD全屏填充特定颜色单元或清屏
**************************************************************/
static void Lcd_ClearScr( U32 c)
{
	unsigned int x,y ;
		
    for( y = 0 ; y < SCR_YSIZE ; y++ )
    {
    	for( x = 0 ; x < SCR_XSIZE ; x++ )
    	{
			LCD_BUFFER[y][x] = c ;
    	}
    }
}

/**************************************************************
LCD屏幕显示垂直翻转
// LCD display is flipped vertically
// But, think the algorithm by mathematics point.
//   3I2
//   4 I 1
//  --+--   <-8 octants  mathematical cordinate
//   5 I 8
//   6I7
**************************************************************/
static void Glib_Line(int x1,int y1,int x2,int y2,int color)
{
	int dx,dy,e;
	dx=x2-x1; 
	dy=y2-y1;
    
	if(dx>=0)
	{
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 1/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}	
					x1+=1;
					e+=dy;
				}
			}
			else		// 2/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}	
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 8/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}	
					x1+=1;
					e+=dy;
				}
			}
			else		// 7/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}	
					y1-=1;
					e+=dx;
				}
			}
		}	
	}
	else //dx<0
	{
		dx=-dx;		//dx=abs(dx)
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 4/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}	
					x1-=1;
					e+=dy;
				}
			}
			else		// 3/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}	
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 5/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}	
					x1-=1;
					e+=dy;
				}
			}
			else		// 6/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}	
					y1-=1;
					e+=dx;
				}
			}
		}	
	}
}

/**************************************************************
在LCD屏幕上画一个矩形
**************************************************************/
static void Glib_Rectangle(int x1,int y1,int x2,int y2,int color)
{
    Glib_Line(x1,y1,x2,y1,color);
    Glib_Line(x2,y1,x2,y2,color);
    Glib_Line(x1,y2,x2,y2,color);
    Glib_Line(x1,y1,x1,y2,color);
}

/**************************************************************
在LCD屏幕上用颜色填充一个矩形
**************************************************************/
static void Glib_FilledRectangle(int x1,int y1,int x2,int y2,int color)
{
    int i;

    for(i=y1;i<=y2;i++)
	Glib_Line(x1,i,x2,i,color);
}

/**************************************************************
在LCD屏幕上指定坐标点画一个指定大小的图片
**************************************************************/
static void Paint_Bmp(int x0,int y0,int h,int l,unsigned char bmp[])
{
	int x,y;
	U32 c;
	int p = 0;
	
    for( y = y0 ; y < l ; y++ )
    {
    	for( x = x0 ; x < h ; x++ )
    	{
    		c = bmp[p+1] | (bmp[p]<<8) ;

			if ( ( (x0+x) < SCR_XSIZE) && ( (y0+y) < SCR_YSIZE) )
				LCD_BUFFER[y0+y][x0+x] = c ;
			
    		p = p + 2 ;
    	}
    }
}
#endif
/**************************************************************
**************************************************************/
extern void LoadPic(U32 PicBuffer);
void Lcd_Tft_LTV350QV_F05_Init(void)
{
	switch(boot_params.display_sel.val){
  	case 0:
  		Lcd_Init_320_240();
  		break;
  	case 1:
  		Lcd_Init_640_480();
  		break;
  	case 2:
  		Lcd_Init_800_600();
  		break;
  	case 3:
  		Lcd_Init_800_480();
  		break;	
  	case 4:
  		Lcd_Init_480_272();
  		break;  					
  	default:
  		Lcd_Init_320_240();
  		break;
	}
	LcdBkLtSet( 70 ) ;
	Lcd_PowerEnable(0, 1);
  Lcd_EnvidOnOff(1);		//turn on vedio
  LoadPic(PIC_BUFFER);
/*    
	Lcd_ClearScr(0xdddd);		//fill all screen with some color
	#define LCD_BLANK		12
	#define C_UP		( LCD_XSIZE - LCD_BLANK*2 )
	#define C_RIGHT		( LCD_XSIZE - LCD_BLANK*2 )
	#define V_BLACK		( ( LCD_YSIZE - LCD_BLANK*4 ) / 6 )
	
	Glib_FilledRectangle( LCD_BLANK, LCD_BLANK, ( LCD_XSIZE - LCD_BLANK ), ( LCD_YSIZE - LCD_BLANK ),0x0000);		//fill a Rectangle with some color

	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*0), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*1),0x001f);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*1), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*2),0x07e0);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*2), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*3),0xf800);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*3), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*4),0xffe0);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*4), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*5),0xf81f);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*5), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*6),0x07ff);		//fill a Rectangle with some color

	Glib_Line( LCD_BLANK,LCD_BLANK, (LCD_XSIZE-LCD_BLANK), (LCD_YSIZE-LCD_BLANK), 0x0000 ) ;
	Glib_Line( LCD_BLANK,(LCD_YSIZE-LCD_BLANK), (LCD_XSIZE-LCD_BLANK), LCD_BLANK, 0x0000 ) ;
	Glib_Line( (LCD_XSIZE/2),(LCD_BLANK*2 + V_BLACK*0), (LCD_XSIZE/2), (LCD_BLANK*2 + V_BLACK*6), 0x0000 ) ;
*/	
	//Paint_Bmp( 0, 0, 320, 240, xyx_320_240 ) ;//by pht.
}


