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
#include "norflash.h"





extern char Image$$RW_RAM1$$RW$$Limit[];
U32 *pMagicNum=(U32 *)Image$$RW_RAM1$$RW$$Limit;
int consoleNum;

/*************************************************************/
#include "bootpara.h"



/*************************************************************/

void Main(void)
{
	

	Port_Init();
	

	//autorun_trig=0;
	consoleNum=boot_params.serial_sel.val&3;	// Uart 1 select for debug.
	if(consoleNum>1)consoleNum=0;
	Uart_Init(0,115200/*boot_params.serial_baud.val*/);
	Uart_Select(consoleNum);//Uart_Select(consoleNum) 默认用串口0，如果用户要用别的串口的话请修改这里

	//MMU_Init();	

	Uart_SendByte('\n');
  
	
	Uart_SendString("bootloader for test\n");
	

	

	
	
	//display_init();//added by pht	
	
	
	Uart_SendString(" +------------------------------------------------------------+\n");	
	

	
	
	//Uart_SendString("<*************************************************************>\n");

	


	Led_Display(0x6);
	
	
	
	//Delay(30000);
	//Lcd_Tft_LTV350QV_F05_Init();//by pht.	

	

	
	InitNandFlash(1);
	LoadRun();    //boot linux  added @5.8 
}




