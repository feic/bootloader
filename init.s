/*
 * Changed from vivi
 *	By LiuNing
 *	Date: 2011-9-28
 */

#include "smdk2440.h"

@ Start of executable code 

ENTRY(_start)
ENTRY(ResetEntryPoint)

@
@ Exception vector table (physical address = 0x00000000)
@

@ 0x00: Reset
	b	Reset

@ 0x04: Undefined instruction exception
UndefEntryPoint:
	b	.

@ 0x08: Software interrupt exception
SWIEntryPoint:
	b	.

@ 0x0c: Prefetch Abort (Instruction Fetch Memory Abort)
PrefetchAbortEnteryPoint:
	b	.

@ 0x10: Data Access Memory Abort
DataAbortEntryPoint:
	bl	.

@ 0x14: Not used
NotUsedEntryPoint:
	b	.

@ 0x18: IRQ(Interrupt Request) exception
IRQEntryPoint:
	b	.

@ 0x1c: FIQ(Fast Interrupt Request) exception
FIQEntryPoint:
	b	.

@
@ VIVI magics
@

@ 0x20: magic number so we can verify that we only put 
	.long   0
@ 0x24:
	.long   0
@ 0x28: where this vivi was linked, so we can put it in memory in the right place
	.long   _start
@ 0x2C: this contains the platform, cpu and machine id
	.long   ARCHITECTURE_MAGIC
@ 0x30: vivi capabilities 
	.long   0

@
@ Start VIVI head
@
Reset:
	@ disable watch dog timer
	mov	r1, #0x53000000
	mov	r2, #0x0
	str	r2, [r1]

	@ disable all interrupts
	mov	r1, #INT_CTL_BASE
	mov	r2, #0xffffffff
	str	r2, [r1, #oINTMSK]
	ldr	r2, =0x7ff
	str	r2, [r1, #oINTSUBMSK]	

	@ initialise system clocks
	mov	r1, #CLK_CTL_BASE
	mvn	r2, #0xff000000
	str	r2, [r1, #oLOCKTIME]
	
	mov	r1, #CLK_CTL_BASE
	ldr	r2, clkdivn_value
	str	r2, [r1, #oCLKDIVN]

	mrc	p15, 0, r1, c1, c0, 0		@ read ctrl register 
	orr	r1, r1, #0xc0000000		@ Asynchronous  
	mcr	p15, 0, r1, c1, c0, 0		@ write ctrl register

	mov	r1, #CLK_CTL_BASE
	@ldr	r2, mpll_value			@ clock default
	ldr 	r2, =0x7f021	@mpll_value_USER 		@ clock user set
	str	r2, [r1, #oMPLLCON]
	bl	memsetup

	@ set GPIO for UART
	mov	r1, #GPIO_CTL_BASE
	add	r1, r1, #oGPIO_H
	ldr	r2, gpio_con_uart	
	str	r2, [r1, #oGPIO_CON]
	ldr	r2, gpio_up_uart
	str	r2, [r1, #oGPIO_UP]	
	
	bl	InitUART

	@ reset NAND
	mov	r1, #NAND_CTL_BASE
	ldr	r2, =( (7<<12)|(7<<8)|(7<<4)|(0<<0) )
	str	r2, [r1, #oNFCONF]
	ldr	r2, [r1, #oNFCONF]

	ldr	r2, =( (1<<4)|(0<<1)|(1<<0) ) @ Active low CE Control 
	str	r2, [r1, #oNFCONT]
	ldr	r2, [r1, #oNFCONT]

	ldr	r2, =(0x6)		@ RnB Clear
	str	r2, [r1, #oNFSTAT]
	ldr	r2, [r1, #oNFSTAT]
	
	mov	r2, #0xff		@ RESET command
	strb	r2, [r1, #oNFCMD]
	mov	r3, #0			@ wait 
1:	add	r3, r3, #0x1
	cmp	r3, #0xa
	blt	1b
2:	ldr	r2, [r1, #oNFSTAT]	@ wait ready
	tst	r2, #0x1
	beq	2b

	ldr	r2, [r1, #oNFCONT]
	orr	r2, r2, #0x2		@ Flash Memory Chip Disable
	str	r2, [r1, #oNFCONT]

	@ get read to call C functions (for nand_read())
	ldr	sp, DW_STACK_START	@ setup stack pointer
	mov	fp, #0			@ no previous frame, so fp=0

	mov	r1, #GPIO_CTL_BASE
	add	r1, r1, #oGPIO_F
	mov	r2, #0xe0
	str	r2, [r1, #oGPIO_DAT]

	
	@ get read to call C functions
	ldr	sp, DW_STACK_START	@ setup stack pointer
	mov	fp, #0			@ no previous frame, so fp=0
	mov	a2, #0			@ set argv to NULL 

	bl	main			@ call main 

	mov	pc, #0		@ otherwise, reboot

@
@ End VIVI head
@


@ Initialize UART
@
@ r0 = number of UART port
InitUART:
	ldr	r1, SerBase
	mov	r2, #0x0
	str	r2, [r1, #oUFCON]
	str	r2, [r1, #oUMCON]
	mov	r2, #0x3
	str	r2, [r1, #oULCON]
	ldr	r2, =0x245
	str	r2, [r1, #oUCON]
#define UART_BRD ((UART_PCLK  / (UART_BAUD_RATE * 16)) - 1)
	mov	r2, #UART_BRD
	str	r2, [r1, #oUBRDIV]

	mov	r3, #100
	mov	r2, #0x0
1:	sub	r3, r3, #0x1
	tst	r2, r3
	bne	1b

	mov	pc, lr
	
ENTRY(memsetup)
	@ initialise the static memory 

	@ set memory control registers
	mov	r1, #MEM_CTL_BASE
	adrl	r2, mem_cfg_val
	add	r3, r1, #52
1:	ldr	r4, [r2], #4
	str	r4, [r1], #4
	cmp	r1, r3
	bne	1b

	mov	pc, lr	
@
@ Data Area
@
@ Memory configuration values
.align 4
mem_cfg_val:
	.long	vBWSCON
	.long	vBANKCON0
	.long	vBANKCON1
	.long	vBANKCON2
	.long	vBANKCON3
	.long	vBANKCON4
	.long	vBANKCON5
	.long	vBANKCON6
	.long	vBANKCON7
	.long	vREFRESH
	.long	vBANKSIZE
	.long	vMRSRB6
	.long	vMRSRB7


@ Processor clock values
.align 4
clock_locktime:
	.long	vLOCKTIME
@mpll_value:
@	.long	vMPLLCON_NOW
mpll_value_USER:
	.long   vMPLLCON_NOW_USER
clkdivn_value:
	.long	vCLKDIVN_NOW

@ initial values for serial
uart_ulcon:
	.long	vULCON
uart_ucon:
	.long	vUCON
uart_ufcon:
	.long	vUFCON
uart_umcon:
	.long	vUMCON
@ inital values for GPIO
gpio_con_uart:
	.long	vGPHCON
gpio_up_uart:
	.long	vGPHUP

	.align	2
DW_STACK_START:
	.word	STACK_BASE+STACK_SIZE-4

.align 4
SerBase:
	.long UART0_CTL_BASE
