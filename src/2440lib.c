//===================================================================
// File Name : 2440lib.c
// Function  : S3C2440 PLL,Uart, LED, Port Init
// Modified by Allium for RAM saving.
//===================================================================

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

extern char Image$$RW_RAM1$$RW$$Limit[];
void *mallocPt=Image$$RW_RAM1$$RW$$Limit;

/*****************************************[Port]*********************************************************/
//port init
void Port_Init(void)
{         
    rGPACON = 0x7fffff;     
    rGPBCON = 0x000151;
    rGPBUP  = 0x7ff;     // The pull up function is disabled GPB[10:0]
	  rGPBDAT &= ~0x1;    
    rGPCCON = 0xaaaaaaaa;       
    rGPCUP  = 0xffff;     // The pull up function is disabled GPC[15:0]     
    rGPDCON = 0xaaaaaaaa;       
    rGPDUP  = 0xffff;     // The pull up function is disabled GPD[15:0]    
	  rGPECON = 0xa02aa800; // For added AC97 setting      
    rGPEUP  = 0xffff;     
	
     //*** PORT F GROUP
    rGPFCON = 0xC151;
    rGPFUP  = 0xff;     // The pull up function is disabled GPF[7:0]

    //*** PORT G GROUP
    rGPGCON = 0x00a25550;// GPG9 input without pull-up
    rGPGUP  = 0xffff;    // The pull up function is disabled GPG[15:0]    
    rGPHCON = 0x00faaa;
    rGPHUP  = 0x7ff;    // The pull up function is disabled GPH[10:0]	
    rGPJCON = 0x02aaaaaa;
    rGPJUP  = 0x1fff;    // The pull up function is disabled GPH[10:0]
    
    //External interrupt will be falling edge triggered. 
    rEXTINT0 = 0x22222222;    // EINT[7:0]
    rEXTINT1 = 0x22222222;    // EINT[15:8]
    rEXTINT2 = 0x22222222;    // EINT[23:16]
}

/*************************************************[Uart]****************************************************************/
//UART Init
void Uart_Init(int pclk,int baud)
{
    
    if(pclk == 0)
    rUFCON0 = 0x0;   //UART channel 0 FIFO control register, FIFO disable
    rUFCON1 = 0x0;   //UART channel 1 FIFO control register, FIFO disable
    rUFCON2 = 0x0;   //UART channel 2 FIFO control register, FIFO disable
    rUMCON0 = 0x0;   //UART chaneel 0 MODEM control register, AFC disable
    rUMCON1 = 0x0;   //UART chaneel 1 MODEM control register, AFC disable
		
    //init UART0  only
    rULCON0 = 0x3;   //Line control register : Normal,No parity,1 stop,8 bits     
    rUCON0  = 0x245;   // Control register
    rUBRDIV0=26;   //Baud rate divisior register 0
    
}




//Uart SendByte
void Uart_SendByte(char data)
{
    
   if(data=='\n')
    {
     while(!(rUTRSTAT0 & 0x2));      
      rUTXH0='\r';
     }
     while(!(rUTRSTAT0 & 0x2));   //Wait until THR is empty.
			 rUTXH0=data;
    
}               

// Uart SendingString
void Uart_SendString(char *pt)
{
    while(*pt)
        Uart_SendByte(*pt++);
}

//********************** BOARD LCD backlight ]****************************
void LcdBkLtSet(U32 HiRatio)
{
#define FREQ_PWM1		1000

	if(!HiRatio)
	{
		rGPBCON  = rGPBCON & (~(3<<2)) | (1<<2) ;	//GPB1设置为output
		rGPBDAT &= ~(1<<1);
		return;
	}
	rGPBCON = rGPBCON & (~(3<<2)) | (2<<2) ;		//GPB1设置为TOUT1
	
	if( HiRatio > 100 )
		HiRatio = 100 ;	
	
	rTCON = rTCON & (~(0xf<<8)) ;			// clear manual update bit, stop Timer1

	rTCFG0 	&= 0xffffff00;					// set Timer 0&1 prescaler 0
	rTCFG0 |= 15;							//prescaler = 15+1

	rTCFG1 	&= 0xffffff0f;					// set Timer 1 MUX 1/16
	rTCFG1  |= 0x00000030;					// set Timer 1 MUX 1/16

	rTCNTB1	 = ( 100000000>>8 )/FREQ_PWM1;		//if set inverter off, when TCNT2<=TCMP2, TOUT is high, TCNT2>TCMP2, TOUT is low
	rTCMPB1  = ( rTCNTB1*(100-HiRatio))/100 ;	//if set inverter on,  when TCNT2<=TCMP2, TOUT is low,  TCNT2>TCMP2, TOUT is high

	rTCON = rTCON & (~(0xf<<8)) | (0x0e<<8) ;
	//自动重装,输出取反关闭,更新TCNTBn、TCMPBn,死区控制器关闭
	rTCON = rTCON & (~(0xf<<8)) | (0x0d<<8) ;		//开启背光控制
}
