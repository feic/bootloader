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




/*************************************************************/
#include "bootpara.h"



/*************************************************************/

void Main(void)
{
	

	Port_Init();
	


	
	Uart_Init(0,115200/*boot_params.serial_baud.val*/);
	

	//MMU_Init();	

	Uart_SendByte('\n');
  
	
   Uart_SendString("bootloader for test\n");
	
	
	Uart_SendString(" +------------------------------------------------------------+\n");	
	

	
	InitNandFlash(1);
	LoadRun();    //boot linux  added @5.8 
}




