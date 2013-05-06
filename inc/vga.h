#ifndef _VGA_H_
#define _VGA_H_

#define __REGb(x)	(*(volatile unsigned char *)(x))

#define IICCON	__REGb(0x54000000)
#define IICSTAT	__REGb(0x54000004)
#define IICADD	__REGb(0x54000008)
#define IICDS	__REGb(0x5400000c)

#define ACK_ENA		(0x1 << 7)
#define CLK_512		(0x1 << 6)
#define nRESUME		(~(0x1 << 4))
#define INT_PEND	(0x1 << 4)


#define MSR_RX		(0x2 << 6)
#define MSR_TX		(0x3 << 6)
#define BUS_BSY		(0x1 << 5)
#define TR_START	(0x1 << 5)
#define nTR_STOP	(~(0x1 << 5))
#define OUT_ENA		(0x1 << 4)
#define ACK_REV		(0x0)
#define ACK_NREV	(0x1)

//#define VGA_ADDR	0xEC
#define VGA_ADDR	0xEA

#define nWR			(~0x01)
#define RD			0x01
#define VGA_REG_ADDR_MASK	0x3f
#define VGA_REG_ADDR_MAX	0x3f


typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long	U32;

#define rIICCON  (*(volatile unsigned *)0x54000000) //IIC control
#define rIICSTAT (*(volatile unsigned *)0x54000004) //IIC status
#define rIICADD  (*(volatile unsigned *)0x54000008) //IIC address
#define rIICDS   (*(volatile unsigned *)0x5400000c) //IIC data shift

#define BANKCON6  (*(volatile unsigned *)0x4800001c) 
#define BANKCON7 (*(volatile unsigned *)0x48000020)
#define REFRESH  (*(volatile unsigned *)0x48000024)
#define MRSRB6   (*(volatile unsigned *)0x4800002c)
#define MRSRB7   (*(volatile unsigned *)0x48000030)


#define WRDATA      (1)
#define POLLACK     (2)
#define RDDATA      (3)
#define SETRDADDR   (4)

#define IICBUFSIZE 0x20

#define rGPECON    (*(volatile unsigned *)0x56000040) //Port E control
#define rGPEDAT    (*(volatile unsigned *)0x56000044) //Port E data
#define rGPEUP     (*(volatile unsigned *)0x56000048) //Pull-up control E
#endif

