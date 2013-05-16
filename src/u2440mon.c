/****************************************************************
 NAME: u2440mon.c
 Modified by Allium for RAM saving.

 ****************************************************************/
#define	GLOBAL_CLK		1

#include <stdlib.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "Nand.h"
#include "bootpara.h"



void Bootloader(void)
{
	Port_Init();	
	Uart_Init(0,115200);
	Uart_SendString("bootloader for test\n");
	InitNandFlash();
	LoadRun();    
}

