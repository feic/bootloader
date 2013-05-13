/**************************************************************
 NAME: option.h
 DESC: To measuure the USB download speed, the WDT is used.
       To measure up to large time, The WDT interrupt is used.
 HISTORY:
 Feb.20.2002:Shin, On Pil: Programming start
 Mar.25.2002:purnnamu: S3C2400X profile.c is ported for S3C2440X.
 **************************************************************/
 
#ifndef __OPTION_H__
#define __OPTION_H__

#define MEGA	(1000000)

#define FIN 	(12000000)	



#ifdef GLOBAL_CLK
U32 FCLK;
U32 HCLK;
U32 PCLK;
U32 UCLK;
#else
extern unsigned int FCLK;
extern unsigned int HCLK;
extern unsigned int PCLK;
extern unsigned int UCLK;
#endif

// BUSWIDTH : 16,32
#define BUSWIDTH    (32)



#define _RAM_STARTADDRESS 	0x30000000
#define _ISR_STARTADDRESS 	0x33ffff00     
#define _MMUTT_STARTADDRESS	0x33ff8000
#define _STACK_BASEADDRESS	0x33ff8000
#define HEAPEND		  	0x33ff0000
#define _NONCACHE_STARTADDRESS	0x31000000



#endif /*__OPTION_H__*/
