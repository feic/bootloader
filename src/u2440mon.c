/****************************************************************
 NAME: u2440mon.c
 4.0
 	增加启动图片
 	增加NOR启动支持
 	支持新WINCE的内核烧写

 ****************************************************************/
#define	GLOBAL_CLK		1

#include <stdlib.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"
#include "mmu.h"




#include "norflash.h"


void Clk0_Enable(int clock_sel);	
void Clk1_Enable(int clock_sel);
void Clk0_Disable(void);
void Clk1_Disable(void);

//#define DOWNLOAD_ADDRESS _RAM_STARTADDRESS


//void (*restart)(void)=(void (*)(void))0x0;


//volatile unsigned char *downPt;

//volatile U16 checkSum;
//volatile unsigned int err=0;
//volatile U32 totalDmaCount;

//volatile int isUsbdSetConfiguration;





extern char Image$$RW_RAM1$$RW$$Limit[];
U32 *pMagicNum=(U32 *)Image$$RW_RAM1$$RW$$Limit;
int consoleNum;

/*************************************************************/
#include "bootpara.h"





static U32 autorun_10ms;
static U16 autorun_ds;
static U16 autorun_trig;

static __irq void autorun_proc(void)
{
	ClearPending(BIT_TIMER4);

	if(autorun_ds)
		DisableIrq(BIT_TIMER4);
	
	autorun_10ms--;
	if(!autorun_10ms) {
		DisableIrq(BIT_TIMER4);
		//CLR_IF();	//in irq service routine, irq is disabled
		autorun_trig = 1;
		//NandLoadRun();
	}
}

static void init_autorun_timer(int sec)
{
	U32 val = (PCLK>>4)/100-1;
	
	autorun_10ms = sec*100;
	
	pISR_TIMER4 = (U32)autorun_proc;
	
	rTCFG0 &= ~(0xff<<8);
	rTCFG0 |= 3<<8;			//prescaler = 3+1
	rTCFG1 &= ~(0xf<<16);
	rTCFG1 |= 1<<16;		//mux = 1/4

	rTCNTB4 = val;
	rTCON &= ~(0xf<<20);
	rTCON |= 7<<20;			//interval, inv-off, update TCNTB4&TCMPB4, start timer 4
	rTCON &= ~(2<<20);		//clear manual update bit
	EnableIrq(BIT_TIMER4);
}

static U32 cpu_freq;
static U32 UPLL;
static void cal_cpu_bus_clk(void)
{
	U32 val;
	U8 m, p, s;
	
	val = rMPLLCON;
	m = (val>>12)&0xff;
	p = (val>>4)&0x3f;
	s = val&3;

	//(m+8)*FIN*2 不要超出32位数!
	FCLK = ((m+8)*(FIN/100)*2)/((p+2)*(1<<s))*100;
	
	val = rCLKDIVN;
	m = (val>>1)&3;
	p = val&1;	
	val = rCAMDIVN;
	s = val>>8;
	
	switch (m) {
	case 0:
		HCLK = FCLK;
		break;
	case 1:
		HCLK = FCLK>>1;
		break;
	case 2:
		if(s&2)
			HCLK = FCLK>>3;
		else
			HCLK = FCLK>>2;
		break;
	case 3:
		if(s&1)
			HCLK = FCLK/6;
		else
			HCLK = FCLK/3;
		break;
	}
	
	if(p)
		PCLK = HCLK>>1;
	else
		PCLK = HCLK;
	
	if(s&0x10)
		cpu_freq = HCLK;
	else
		cpu_freq = FCLK;
		
	val = rUPLLCON;
	m = (val>>12)&0xff;
	p = (val>>4)&0x3f;
	s = val&3;
	UPLL = ((m+8)*FIN)/((p+2)*(1<<s));
	if(UPLL==96*MEGA)
		rCLKDIVN |= 8;	//UCLK=UPLL/2
	UCLK = (rCLKDIVN&8)?(UPLL>>1):UPLL;
}


/*************************************************************/

void Main(void)
{
	char *mode;
	int j;
	U8 key;
	U32 mpll_val;
	
 

	Port_Init();
	


	j=2;
	if(boot_params.display_sel.val==2)j=3;//TV mod

	switch(j) {
	case 0:	//240
		key = 14;
		mpll_val = (112<<12)|(4<<4)|(1);
		break;
	case 1:	//320
		key = 14;
		mpll_val = (72<<12)|(1<<4)|(1);
		break;
	case 2:	//400
		key = 14;
		mpll_val = (92<<12)|(1<<4)|(1);
		break;
	case 3:	//420!!!
		key = 14;
		mpll_val = (97<<12)|(1<<4)|(1);
		break;
	default:
		key = 14;
		mpll_val = (92<<12)|(1<<4)|(1);
		break;
	}
#if 1
	//init FCLK=400M, so change MPLL first
	ChangeMPllValue((mpll_val>>12)&0xff, (mpll_val>>4)&0x3f, mpll_val&3);
	ChangeClockDivider(key, 12);
	cal_cpu_bus_clk();
	if(PCLK<(40*MEGA)) {
		ChangeClockDivider(key, 11);
		cal_cpu_bus_clk();
	}
#else
	cal_cpu_bus_clk();
#endif
	//autorun_trig=0;
	consoleNum=boot_params.serial_sel.val&3;	// Uart 1 select for debug.
	if(consoleNum>1)consoleNum=0;
	Uart_Init(0,115200/*boot_params.serial_baud.val*/);
	Uart_Select(consoleNum);//Uart_Select(consoleNum) 默认用串口0，如果用户要用别的串口的话请修改这里

	MMU_Init();	

	Uart_SendByte('\n');
  
	
	Uart_SendString("bootloader for test\n");
	

	

	
	
	//display_init();//added by pht	
	
	if(boot_params.boot_delay.val)
		init_autorun_timer(boot_params.boot_delay.val);
	Uart_SendString(" +------------------------------------------------------------+\n");	
	

	
	
	//Uart_SendString("<*************************************************************>\n");

	
	pISR_SWI=(_ISR_STARTADDRESS+0xf0);	//for pSOS

	Led_Display(0x6);
	
	
	
	

	mode="Int";


	Clk0_Disable();
	Clk1_Disable();
	
	mpll_val = rMPLLCON;
	
	//Delay(30000);
	//Lcd_Tft_LTV350QV_F05_Init();//by pht.	

	

	
	InitNandFlash(1);
	LoadRun();    //boot linux  added @5.8 
}



void Clk0_Enable(int clock_sel)	
{	// 0:MPLLin, 1:UPLL, 2:FCLK, 3:HCLK, 4:PCLK, 5:DCLK0
	rMISCCR = rMISCCR&~(7<<4) | (clock_sel<<4);
	rGPHCON = rGPHCON&~(3<<18) | (2<<18);
}
void Clk1_Enable(int clock_sel)
{	// 0:MPLLout, 1:UPLL, 2:RTC, 3:HCLK, 4:PCLK, 5:DCLK1	
	rMISCCR = rMISCCR&~(7<<8) | (clock_sel<<8);
	rGPHCON = rGPHCON&~(3<<20) | (2<<20);
}
void Clk0_Disable(void)
{
	rGPHCON = rGPHCON&~(3<<18);	// GPH9 Input
}
void Clk1_Disable(void)
{
	rGPHCON = rGPHCON&~(3<<20);	// GPH10 Input
}

