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
#include "profile.h"
#include "memtest.h"


#include "usbmain.h"
#include "usbout.h"
#include "usblib.h"
#include "2440usb.h"

//#include "ok2440_usb.a"


#include "norflash.h"

void Isr_Init(void);
void HaltUndef(void);
void HaltSwi(void);
void HaltPabort(void);
void HaltDabort(void);
void Lcd_Off(void);
void WaitDownload(void);
void Menu(void);
void ClearMemory(void);


void Clk0_Enable(int clock_sel);	
void Clk1_Enable(int clock_sel);
void Clk0_Disable(void);
void Clk1_Disable(void);

//#define DOWNLOAD_ADDRESS _RAM_STARTADDRESS
volatile U32 downloadAddress;

void (*restart)(void)=(void (*)(void))0x0;


volatile unsigned char *downPt;
volatile U32 downloadFileSize;
volatile U16 checkSum;
volatile unsigned int err=0;
volatile U32 totalDmaCount;

volatile int isUsbdSetConfiguration;

int download_run=0;
U32 tempDownloadAddress;
int menuUsed = 0;

extern char Image$$RW$$Limit[];
U32 *pMagicNum=(U32 *)Image$$RW$$Limit;
int consoleNum;

/*************************************************************/
#include "bootpara.h"

void LcdDisplay(void);
int find_camera(void);
void Led_Test(void);
void comdownload(void);
U32 GetFlashID(void);
int SectorProg(U32 begin, U16 *data, U32 size);

void NandErase(void);
void NandWrite(void);
void NandLoadRun(void);
void NandLoadRun_wince(void);

#define	printf	Uart_Printf

#define	DM9000_BASE			0x20000300
#define	DM9000_DATA_OFFSET	4

void mdelay(int ms)
{
	U32 val = (PCLK>>3)/1000-1;
	
	rTCFG0 &= ~(0xff<<8);
	rTCFG0 |= 3<<8;			//prescaler = 3+1
	rTCFG1 &= ~(0xf<<12);
	rTCFG1 |= 0<<12;		//mux = 1/2

/*	while(ms--) {
		rTCNTB3 = val;
		rTCMPB3 = val>>1;		// 50%
		rTCON &= ~(0xf<<16);
		rTCON |= 3<<16;			//one shot, inv-off, update TCNTB3&TCMPB3, start timer 3
		rTCON &= ~(2<<16);		//clear manual update bit
		while(rTCNTO3);
	}*/
	rTCNTB3 = val;
	rTCMPB3 = val>>1;		// 50%
	rTCON &= ~(0xf<<16);
	rTCON |= 0xb<<16;		//interval, inv-off, update TCNTB3&TCMPB3, start timer 3
	rTCON &= ~(2<<16);		//clear manual update bit
	while(ms--) {
		while(rTCNTO3>=val>>1);
		while(rTCNTO3<val>>1);
	};

}

static U8 dm9000_ior(int reg)
{
	*(volatile U8 *)DM9000_BASE = reg;
	return *(volatile U8 *)(DM9000_BASE+DM9000_DATA_OFFSET);
}

static void rd_dm9000_id(void)
{
	U16 id;
	
	id = dm9000_ior(0x28) | (dm9000_ior(0x29)<<8);
	printf("read dm9000 vid = 0x%x\n", id);
	
	id = dm9000_ior(0x2a) | (dm9000_ior(0x2b)<<8);
	printf("read dm9000 pid = 0x%x\n", id);
	
	id = dm9000_ior(0x8) | (dm9000_ior(0x9)<<8);
	printf("read dm9000 reg(0x09,0x08) = 0x%x\n", id);
	
	printf("dm9000 isr = 0x%x\n", dm9000_ior(0xfe));
}

static void buzzer(int freq, int ms)
{
	rGPBCON &= ~3;			//set GPB0 as tout0, pwm output
	rGPBCON |= 2;
		
	rTCFG0 &= ~0xff;
	rTCFG0 |= 15;			//prescaler = 15+1
	rTCFG1 &= ~0xf;
	rTCFG1 |= 2;			//mux = 1/8
	rTCNTB0 = (PCLK>>7)/freq;
	rTCMPB0 = rTCNTB0>>1;	// 50%
	rTCON &= ~0x1f;
	rTCON |= 0xb;			//disable deadzone, auto-reload, inv-off, update TCNTB0&TCMPB0, start timer 0
	rTCON &= ~2;			//clear manual update bit
	
	mdelay(ms);
	
	
	rGPBCON &= ~3;			//set GPB0 as output
	rGPBCON |= 1;
	rGPBDAT &= ~1;
}

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
	int i,j;
	U8 key;
	U32 mpll_val;
	
	//menuUsed=0;
    
	#if ADS10   
	__rt_lib_init(); //for ADS 1.0
	#endif

	Port_Init();
	
	Isr_Init();
	
	//Led_Test();
	

	i = search_params();
		
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
#if 1	
	Uart_Printf(" +------------------------------------------------------------+\n");
	Uart_Printf(" |   FL2440 Bootloader VER-5.1              www.witech.com.cn |\n");
	Uart_Printf(" |------------------------------------------------------------|\n");
	Uart_Printf(" |------------------------------------------------------------|\n");	
	Uart_Printf(" |CPU ID is 0x%08x                                        |\n", rGSTATUS1);
	if(i==-1) {
		Uart_Printf(" |  Fail to found boot params!                                |\n");

		save_params();

	}
	Uart_Printf(" |FCLK=%dMHz,HCLK=%dMHz,PCLK=%dMHz, CPU is running at %dMHz|\n",
					FCLK/MEGA, HCLK/MEGA, PCLK/MEGA, cpu_freq/MEGA);
	Uart_Printf(" |UPLL=%dMHz, UCLK=%dMHz                                      |\n", UPLL/MEGA, UCLK/MEGA);
	Uart_Printf(" |Serial port %d, Baud rate is %d.                         |\n", boot_params.serial_sel.val, boot_params.serial_baud.val);
	Uart_Printf(" |OS image stored in %s Flash.                              |\n","NAND");
	Uart_Printf(" |Autoboot delay is %d seconds.                                |\n", boot_params.boot_delay.val);
	//display_init();//added by pht	
	
	if(boot_params.boot_delay.val)
		init_autorun_timer(boot_params.boot_delay.val);
	Uart_Printf(" +------------------------------------------------------------+\n");	
	
#else
	Uart_Printf("FEILING 2440 Bootloader VER-5.0    www.witech.com.cn\n");
	if(i==-1) {
		Uart_Printf("Fail to found boot params!\n");

		save_params();

	}
	Uart_Printf("Autoboot delay is %d seconds.\n", boot_params.boot_delay.val);
	if(boot_params.boot_delay.val)
		init_autorun_timer(boot_params.boot_delay.val);

#endif
	
	
	//Uart_Printf("<*************************************************************>\n");

	rMISCCR=rMISCCR&~(1<<3); // USBD is selected instead of USBH1 
	rMISCCR=rMISCCR&~(1<<13); // USB port 1 is enabled.



	isUsbdSetConfiguration=0;
	
	rDSC0 = 0x2aa;
	rDSC1 = 0x2aaaaaaa;
	
	if(0) {
		int i;
		volatile U16 *p = (volatile U16 *)0x08000000;
		
		p[3] = 0xbf;
		p[2] = 0;
		p[3] = 0;
		p[2] = 1;
		printf("dr2=0x%04x\n", p[2]);
		
		for(i=0; i<8; i++)
			printf("0x%08x\n", p[i]);
		
	}


//	Delay(0);	//calibrate Delay() first
//	Uart_Printf("Check SST39VF160 Flash ID is 0x%08x\n", GetFlashID());
	UsbdMain(); 

	pISR_SWI=(_ISR_STARTADDRESS+0xf0);	//for pSOS

	Led_Display(0x6);
	
	
	
	

#if USBDMA
	mode="DMA";
#else
	mode="Int";
#endif

	Clk0_Disable();
	Clk1_Disable();
	
	mpll_val = rMPLLCON;
	
	//Delay(30000);
	Lcd_Tft_LTV350QV_F05_Init();//by pht.	

	download_run=1; //The default menu is the Download & Run mode.

	while(1)
	{
		if(!((rGPFDAT>>6)&0x1)){
			AutoNandLoadRun_key();
			autorun_ds = 1;//若无自启动，则进入Menu;
			Menu();
		}
		if(menuUsed==1)Menu();
		WaitDownload();	
	}

}



void Menu(void)
{
	
	U8 key;
	menuUsed=1;
	while(1)
	{
#if 0
		Uart_Printf("\n +------------------------------------------------------------+\n");
		Uart_Printf(" |                    BOOTLOADER菜单                          |\n");
		Uart_Printf(" +------------------------------------------------------------+\n");
		Uart_Printf(" | [0] 下载并运行                                             |\n");
		Uart_Printf(" | [1] 下载到FLASH                                            |\n");

		Uart_Printf(" | [2] 用串口下载                                             |\n");
		Uart_Printf(" | [3] 启动应用程序                                           |\n");
		Uart_Printf(" | [4] 启动LINUX                                              |\n");

		Uart_Printf(" | [5] 启动WINCE                                              |\n");	
		Uart_Printf(" | [6] 擦除FLASH                                              |\n");
		Uart_Printf(" | [7] 设置参数                                               |\n");
		Uart_Printf(" +------------------------------------------------------------+\n");
#else		
		Uart_Printf("\n +------------------------------------------------------------+\n");
		Uart_Printf(" |                     Select Menu                            |\n");
		Uart_Printf(" +------------------------------------------------------------+\n");
		Uart_Printf(" | [0] Download & Run                                         |\n");
		Uart_Printf(" | [1] Download to flash                                      |\n");

		Uart_Printf(" | [2] Download From UART                                     |\n");
		Uart_Printf(" | [3] Boot My Application                                    |\n");
		Uart_Printf(" | [4] Boot Linux                                             |\n");

		Uart_Printf(" | [5] Boot Wince                                             |\n");	
		Uart_Printf(" | [6] Erase Flash Partition                                  |\n");
		Uart_Printf(" | [7] Config parameters                                      |\n");
		Uart_Printf(" +------------------------------------------------------------+\n");
		
#endif
		key=Uart_Getch();
		
		switch(key)
		{
		case '0':
			Uart_Printf("\nDownload&Run is selected.\n\n");
			download_run=1;
			return;
		case '1':
			download_run=0;
			NandWrite();
		/*
			Uart_Printf("\nDownload Only is selected.\n");
			Uart_Printf("Enter a new temporary download address(0x3...):");
			tempDownloadAddress=Uart_GetIntNum();
			download_run=0;
			Uart_Printf("The temporary download address is 0x%x.\n\n",tempDownloadAddress);
			return;
		*/
			break;
		case '2':
			comdownload();
			break;
		case '3':
			NandLoadRun_App();

		case '4':
			NandLoadRun();
			break;
		case '5':
			NandLoadRun_wince();
			break;
		case '6':
			NandErase();
			break;
		case '7':
			set_params();
			break;
/*			
		case '8'
			MemoryTest();
			break;
*/
		default:
			break;
		}	
	}		

}



void WaitDownload(void)
{
	U32 i;
	U32 j;
	U16 cs;
	U32 temp;
	U16 dnCS;
	int first=1;
	float time;
	U8 tempMem[16];
	U8 key;
	
	checkSum=0;
	downloadAddress=(U32)tempMem; //_RAM_STARTADDRESS; 
	downPt=(unsigned char *)downloadAddress;
	//This address is used for receiving first 8 byte.
	downloadFileSize=0;
	
#if 0
	MMU_DisableICache(); 
		//For multi-ICE. 
		//If ICache is not turned-off, debugging is started with ICache-on.
#endif

	/*******************************/
	/*	Test program download	*/
	/*******************************/
	j=0;

	if(isUsbdSetConfiguration==0)
	{
		//Uart_Printf("  USB host is not connected yet.\n");
	}

	while(downloadFileSize==0)
	{
		if(first==1 && isUsbdSetConfiguration!=0)
		{
			//Uart_Printf("  USB host is connected. Waiting a download.\n");
			first=0;
		}

		if(j%0x100000==0)Led_Display(0x8);
		if(j%0x100000==0x80000)Led_Display(0x0);
		j++;

		key=Uart_GetKey();
		if(autorun_trig)
			AutoNandLoadRun();	//run it in svc mode
		if(key!=0)
		{
			autorun_ds = 1;
			//printf("disable autorun\n");
			Menu();
			first=1; //To display the message,"USB host ...."
		}
	}
	
	autorun_ds = 1;
	//printf("disable autorun\n");
	
	Timer_InitEx();	  
	Timer_StartEx();  

#if USBDMA	

	rINTMSK&=~(BIT_DMA2);  

	ClearEp3OutPktReady(); 
		// indicate the first packit is processed.
		// has been delayed for DMA2 cofiguration.

	if(downloadFileSize>EP3_PKT_SIZE)
	{
		if(downloadFileSize<=(0x80000))
		{
	  		ConfigEp3DmaMode(downloadAddress+EP3_PKT_SIZE-8,downloadFileSize-EP3_PKT_SIZE);
 
	  		//will not be used.
/*	   rDIDST2=(downloadAddress+downloadFileSize-EP3_PKT_SIZE);  
		   rDIDSTC2=(0<<1)|(0<<0);  
		rDCON2=rDCON2&~(0xfffff)|(0);				
*/
		}
	  	else
	  	{
	  		ConfigEp3DmaMode(downloadAddress+EP3_PKT_SIZE-8,0x80000-EP3_PKT_SIZE);
	  		//2440比2410的DIDSTCx寄存器多了中断产生条件的控制位,USB的DMA传输为字节计数
	  		//防止高频开cache运行时下载大于0x80000字节文件时IsrDma2出错!!!
	  		//while((rDSTAT2&0xfffff)==(0x80000-EP3_PKT_SIZE));
	  		while(!(rDSTAT2&(1<<20)));	//防止DMA传输尚未开始就写入下一次重装值!!!
			if(downloadFileSize>(0x80000*2))//for 1st autoreload
			{
				rDIDST2=(downloadAddress+0x80000-8);  //for 1st autoreload.
			 rDIDSTC2=(1<<2)|(0<<1)|(0<<0);  
				rDCON2=rDCON2&~(0xfffff)|(0x80000);			  

  		while(rEP3_DMA_TTC<0xfffff)
  		{
  			rEP3_DMA_TTC_L=0xff; 
  			rEP3_DMA_TTC_M=0xff;
  			rEP3_DMA_TTC_H=0xf;
  		}
			}	
 		else
 		{
 			rDIDST2=(downloadAddress+0x80000-8);  //for 1st autoreload.
	  			rDIDSTC2=(1<<2)|(0<<1)|(0<<0);  
 			rDCON2=rDCON2&~(0xfffff)|(downloadFileSize-0x80000); 		

  		while(rEP3_DMA_TTC<0xfffff)
  		{
  			rEP3_DMA_TTC_L=0xff; 
  			rEP3_DMA_TTC_M=0xff;
  			rEP3_DMA_TTC_H=0xf;
  		}
		}
	}
 	totalDmaCount=0;
	}
	else
	{
	totalDmaCount=downloadFileSize;
	}
#endif

	Uart_Printf("\nNow, Downloading [ADDRESS:%xh,TOTAL:%d]\n",
			downloadAddress,downloadFileSize);
	Uart_Printf("RECEIVED FILE SIZE:%8d",0);
   
#if USBDMA	
	j=0x10000;

	while(1)
	{
		if( (rDCDST2-(U32)downloadAddress+8)>=j)
	{
		Uart_Printf("\b\b\b\b\b\b\b\b%8d",j);
   		j+=0x10000;
		}
	if(totalDmaCount>=downloadFileSize)break;
	}

#else
	j=0x10000;

	while(((U32)downPt-downloadAddress)<(downloadFileSize-8))
	{
	if( ((U32)downPt-downloadAddress)>=j)
	{
		Uart_Printf("\b\b\b\b\b\b\b\b%8d",j);
   		j+=0x10000;
	}
	}
#endif

	time=Timer_StopEx();
	
	Uart_Printf("\b\b\b\b\b\b\b\b%8d",downloadFileSize);	
	Uart_Printf("\n(%5.1fKB/S,%3.1fS)\n",(float)(downloadFileSize/time/1000.),time);
	
#if USBDMA	
	/*******************************/
	/*	 Verify check sum		*/
	/*******************************/

	Uart_Printf("Now, Checksum calculation\n");

	cs=0;	
	i=(downloadAddress);
	j=(downloadAddress+downloadFileSize-10)&0xfffffffc;
	while(i<j)
	{
		temp=*((U32 *)i);
		i+=4;
		cs+=(U16)(temp&0xff);
		cs+=(U16)((temp&0xff00)>>8);
		cs+=(U16)((temp&0xff0000)>>16);
		cs+=(U16)((temp&0xff000000)>>24);
	}

	i=(downloadAddress+downloadFileSize-10)&0xfffffffc;
	j=(downloadAddress+downloadFileSize-10);
	while(i<j)
	{
  	cs+=*((U8 *)i++);
	}
	
	checkSum=cs;
#else
	//checkSum was calculated including dnCS. So, dnCS should be subtracted.
	checkSum=checkSum - *((unsigned char *)(downloadAddress+downloadFileSize-8-2))
		 - *( (unsigned char *)(downloadAddress+downloadFileSize-8-1) );	
#endif	  

	dnCS=*((unsigned char *)(downloadAddress+downloadFileSize-8-2))+
	(*( (unsigned char *)(downloadAddress+downloadFileSize-8-1) )<<8);

	if(checkSum!=dnCS)
	{
	Uart_Printf("Checksum Error!!! MEM:%x DN:%x\n",checkSum,dnCS);
	return;
	}

	Uart_Printf("Download O.K.\n\n");
	Uart_TxEmpty(consoleNum);


	if(download_run==1)
	{
		register void(*run)(void);	//使用寄存器变量以防止禁止DCACHE后数据不一致!!!
		rINTMSK=BIT_ALLMSK;
		run=(void (*)(void))downloadAddress;	//使用DCACHE且RW区也在CACHE区间downloadAddress会在cache中
		{	
			MMU_DisableDCache();	//download program must be in 
    		MMU_DisableICache();	//non-cache area
    		MMU_InvalidateDCache();	//使所有DCACHE失效,本程序的MMU_Init中将会刷新DCACHE到存储器,
    								//为使应用此MMU_Init方式的程序能被正确运行必须先使DCACHE失效!!!
    		MMU_DisableMMU();
    		//call_linux(0, 193, downloadAddress);	//或不用上面3个函数而直接使用call_linux
		}
	run();
	}
}




void Isr_Init(void)
{
	pISR_UNDEF=(unsigned)HaltUndef;
	pISR_SWI  =(unsigned)HaltSwi;
	pISR_PABORT=(unsigned)HaltPabort;
	pISR_DABORT=(unsigned)HaltDabort;
	rINTMOD=0x0;	  // All=IRQ mode
	rINTMSK=BIT_ALLMSK;	  // All interrupt is masked.

	//pISR_URXD0=(unsigned)Uart0_RxInt;	
	//rINTMSK=~(BIT_URXD0);   //enable UART0 RX Default value=0xffffffff

#if 1
	pISR_USBD =(unsigned)IsrUsbd;
	pISR_DMA2 =(unsigned)IsrDma2;
#else
	pISR_IRQ =(unsigned)IsrUsbd;	
		//Why doesn't it receive the big file if use this. (???)
		//It always stops when 327680 bytes are received.
#endif	
	ClearPending(BIT_DMA2);
	ClearPending(BIT_USBD);
	//rINTMSK&=~(BIT_USBD);  
   
	//pISR_FIQ,pISR_IRQ must be initialized
}


void HaltUndef(void)
{
	Uart_Printf("Undefined instruction exception!!!\n");
	while(1);
}

void HaltSwi(void)
{
	Uart_Printf("SWI exception!!!\n");
	while(1);
}

void HaltPabort(void)
{
	Uart_Printf("Pabort exception!!!\n");
	while(1);
}

void HaltDabort(void)
{
	Uart_Printf("Dabort exception!!!\n");
	while(1);
}


void ClearMemory(void)
{
	
	
	int memError=0;
	U32 *pt;
	
	//
	// memory clear
	//
	Uart_Printf("Clear Memory (%xh-%xh):WR",_RAM_STARTADDRESS,HEAPEND);

	pt=(U32 *)_RAM_STARTADDRESS;
	while((U32)pt < HEAPEND)
	{
		*pt=(U32)0x0;
		pt++;
	}
	
	if(memError==0)Uart_Printf("\b\bO.K.\n");
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

