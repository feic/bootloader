/****************************************************************
 NAME: u2440mon.c
 4.0
 	��������ͼƬ
 	����NOR����֧��
 	֧����WINCE���ں���д

 ****************************************************************/
#define	GLOBAL_CLK		1

#include <stdlib.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "Nand.h"




/*************************************************************/
#include "bootpara.h"



/*************************************************************/

void Bootloader(void)
{
	

	Port_Init();
	


	
	Uart_Init(0,115200);
	

	//MMU_Init();	

	
  
	
   Uart_SendString("bootloader for test\n");
	
	
	//Uart_SendString(" +------------------------------------------------------------+\n");	
	

	
	InitNandFlash();
	LoadRun();    //boot linux  added @5.8 
}




