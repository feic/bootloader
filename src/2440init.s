;=========================================
; NAME: 2440INIT.S
; DESC: C start up codes
;       Configure memory, ISR ,stacks
;	Initialize C-variables
; HISTORY:
; 2002.02.25:kwtark: ver 0.0
; 2002.03.20:purnnamu: Add some functions for testing STOP,Sleep mode
; 2003.03.14:DonGo: Modified for 2440.
;=========================================

	GET option.inc
	GET memcfg.inc
	GET 2440addr.inc

BIT_SELFREFRESH EQU	(1<<22)

;Pre-defined constants
USERMODE    EQU 	0x10
FIQMODE     EQU 	0x11
IRQMODE     EQU 	0x12
SVCMODE     EQU 	0x13
ABORTMODE   EQU 	0x17
UNDEFMODE   EQU 	0x1b
MODEMASK    EQU 	0x1f
NOINT       EQU 	0xc0

;The location of stacks
UserStack	EQU	(_STACK_BASEADDRESS-0x3800)	;0x33ff4800 ~
SVCStack	EQU	(_STACK_BASEADDRESS-0x2800)	;0x33ff5800 ~
UndefStack	EQU	(_STACK_BASEADDRESS-0x2400)	;0x33ff5c00 ~
AbortStack	EQU	(_STACK_BASEADDRESS-0x2000)	;0x33ff6000 ~
IRQStack	EQU	(_STACK_BASEADDRESS-0x1000)	;0x33ff7000 ~
FIQStack	EQU	(_STACK_BASEADDRESS-0x0)	;0x33ff8000 ~

;Check if tasm.exe(armasm -16 ...@ADS 1.0) is used.
	GBLL    THUMBCODE
	[ {CONFIG} = 16
THUMBCODE SETL  {TRUE}
	    CODE32
 		|
THUMBCODE SETL  {FALSE}
    ]

 		MACRO
	MOV_PC_LR
 		[ THUMBCODE
	    bx lr
 		|
	    mov	pc,lr
 		]
	MEND

 		MACRO
	MOVEQ_PC_LR
 		[ THUMBCODE
        bxeq lr
 		|
	    moveq pc,lr
 		]
	MEND

 		

	IMPORT  |Image$$ER_ROM1$$RO$$Base|	; Base of ROM code
	IMPORT  |Image$$ER_ROM1$$RO$$Limit|  ; End of ROM code (=start of ROM data)
	IMPORT  |Image$$RW_RAM1$$RW$$Base|   ; Base of RAM to initialise
	IMPORT  |Image$$RW_RAM1$$ZI$$Base|   ; Base and limit of area
	IMPORT  |Image$$RW_RAM1$$ZI$$Limit|  ; to zero initialise
	
	IMPORT	MMU_SetAsyncBusMode
	IMPORT	MMU_SetFastBusMode	

	IMPORT  Main    ; The main entry of mon program


	AREA    RESET,CODE,READONLY

	ENTRY
	
	EXPORT	__ENTRY
	
	PRESERVE8
	
__ENTRY
;========
;复位
;========
ResetEntry
	;1)The code, which converts to Big-endian, should be in little endian code.
	;2)The following little endian code will be compiled in Big-Endian mode.
	;  The code byte order should be changed as the memory bus width.
	;3)The pseudo instruction,DCD can t be used here because the linker generates error.
	ASSERT	:DEF:ENDIAN_CHANGE
	[ ENDIAN_CHANGE
	    ASSERT  :DEF:ENTRY_BUS_WIDTH
	    [ ENTRY_BUS_WIDTH=32
		b	ChangeBigEndian	    ;DCD 0xea000007
	    ]

	    [ ENTRY_BUS_WIDTH=16
		andeq	r14,r7,r0,lsl #20   ;DCD 0x0007ea00
	    ]

	    [ ENTRY_BUS_WIDTH=8
		streq	r0,[r0,-r10,ror #1] ;DCD 0x070000ea
	    ]
	|
	    b	ResetHandler
    ]
	b	.	;handler for Undefined mode
	b	.	;handler for SWI interrupt
	b	.	;handler for PAbort
	b	.	;handler for DAbort
	b	.		;reserved
	b	.	;handler for IRQ interrupt
	b	.   ;handler for FIQ interrupt


ChangeBigEndian
;@0x24
	[ ENTRY_BUS_WIDTH=32
	    DCD	0xee110f10	;0xee110f10 => mrc p15,0,r0,c1,c0,0
	    DCD	0xe3800080	;0xe3800080 => orr r0,r0,#0x80;  //Big-endian
	    DCD	0xee010f10	;0xee010f10 => mcr p15,0,r0,c1,c0,0
	]
	[ ENTRY_BUS_WIDTH=16
	    DCD 0x0f10ee11
	    DCD 0x0080e380
	    DCD 0x0f10ee01
	]
	[ ENTRY_BUS_WIDTH=8
	    DCD 0x100f11ee
	    DCD 0x800080e3
	    DCD 0x100f01ee
    ]
	DCD 0xffffffff  ;swinv 0xffffff is similar with NOP and run well in both endian mode.
	DCD 0xffffffff
	DCD 0xffffffff
	DCD 0xffffffff
	DCD 0xffffffff
	b ResetHandler
	





	LTORG

;=======
; ENTRY
;=======
ResetHandler
	ldr	r0,=WTCON       ;watch dog disable
	ldr	r1,=0x0
	str	r1,[r0]

	ldr	r0,=INTMSK
	ldr	r1,=0xffffffff  ;all interrupt disable
	str	r1,[r0]

	ldr	r0,=INTSUBMSK
	ldr	r1,=0x7fff		;all sub interrupt disable
	str	r1,[r0]

	;led显示
	[ {FALSE}
	; rGPFDAT = (rGPFDAT & ~(0xf<<4)) | ((~data & 0xf)<<4);
	; Led_Display
	ldr	r0,=GPFCON
	ldr	r1,=0x5500
	str	r1,[r0]
	ldr	r0,=GPFDAT
	ldr	r1,=0x10
	str	r1,[r0]
	]

	;To reduce PLL lock time, adjust the LOCKTIME register.
	ldr	r0,=LOCKTIME
	ldr	r1,=0xffffff
	str	r1,[r0]

    [ PLL_ON_START
	; Added for confirm clock divide. for 2440.
	; Setting value Fclk:Hclk:Pclk
	ldr	r0,=CLKDIVN
	ldr	r1,=CLKDIV_VAL		; 0=1:1:1, 1=1:1:2, 2=1:2:2, 3=1:2:4, 4=1:4:4, 5=1:4:8, 6=1:3:3, 7=1:3:6.
	str	r1,[r0]
	
	[ CLKDIV_VAL>1 		; means Fclk:Hclk is not 1:1.
	mrc p15,0,r0,c1,c0,0
	orr r0,r0,#0xc0000000;R1_nF:OR:R1_iA
	mcr p15,0,r0,c1,c0,0
	|
	mrc p15,0,r0,c1,c0,0
	bic r0,r0,#0xc0000000;R1_iA:OR:R1_nF
	mcr p15,0,r0,c1,c0,0
	]

	;Configure UPLL
	ldr	r0,=UPLLCON
	ldr	r1,=((U_MDIV<<12)+(U_PDIV<<4)+U_SDIV)  
	str	r1,[r0]
	nop	; Caution: After UPLL setting, at least 7-clocks delay must be inserted for setting hardware be completed.
	nop
	nop
	nop
	nop
	nop
	nop
	;Configure MPLL
	ldr	r0,=MPLLCON
	ldr	r1,=((M_MDIV<<12)+(M_PDIV<<4)+M_SDIV)  ;Fin=16.9344MHz
	str	r1,[r0]
    ]
    
	

;	EXPORT StartPointAfterSleepWakeUp
;StartPointAfterSleepWakeUp

	;Set memory control registers
 	;ldr	r0,=SMRDATA
 	adrl	r0, SMRDATA	
	ldr	r1,=BWSCON	;BWSCON Address
	add	r2, r0, #52	;End address of SMRDATA

0
	ldr	r3, [r0], #4
	str	r3, [r1], #4
	cmp	r2, r0
	bne	%B0
	
	;delay
	mov	r0, #&1000
1
	subs	r0, r0, #1
	bne	%B1
	;===

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;       When EINT0 is pressed,  Clear SDRAM 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; check if EIN0 button is pressed

;	ldr	r0,=GPFCON
;	ldr	r1,=0x0
;	str	r1,[r0]
;	ldr	r0,=GPFUP
;	ldr	r1,=0xff
;	str	r1,[r0]
;
;	ldr	r1,=GPFDAT
;	ldr	r0,[r1]
;	bic	r0,r0,#(0x1e<<1)  ; bit clear
;	tst	r0,#0x1
;	bne %F1

; Clear SDRAM Start
  
;	ldr	r0,=GPFCON
;	ldr	r1,=0x55aa
;	str	r1,[r0]
;	ldr	r0,=GPFUP
;	ldr	r1,=0xff
;	str	r1,[r0]
;	ldr	r0,=GPFDAT
;	ldr	r1,=0x0
;	str	r1,[r0]	;LED=****


;Clear SDRAM End

1

	;Initialize stacks
	bl	InitStacks

;==========================================================
  	; Setup IRQ handler//建立中断表
	
;===========================================================
;// 判断是从nor启动还是从nand启动
;===========================================================
	;bl	Led_Test
	
	ldr	r0, =BWSCON
	ldr	r0, [r0]
	ands	r0, r0, #6		;OM[1:0] != 0, NOR FLash boot
	bne	NORRwCopy		;don t read nand flash
	adr	r0, ResetEntry		;OM[1:0] == 0, NAND FLash boot // ADR 装载参照的地址=sub r0,pc,#0x268;
	cmp	r0, #0				;if use Multi-ice,//JTAG调试时是直接下载到内存中运行，不需要再从nand拷贝 
	bne	InitRamZero		;don t read nand flash for boot
	;nop
	

	
;===========================================================
;//将程序从nandflash拷贝到sdram
;===========================================================
nand_boot_beg
	mov	r5, #NFCONF			;DsNandFlash
	ldr	r0, [r5, #4]
	bic r0, r0, #1
	str	r0, [r5, #4]
	ldr	pc, =InitRamZero;此处跳转到内存空间 LDR 装载数据，寻址灵活。 但不改变PSR
						  ;要装载一个被存储的‘状态’并正确的恢复它 可以这样写：ldr r0, [base] 换行  moves pc, r0
;=============================================================================================
;若是从NAND启动，则拷贝工作已经在nand_boot_beg中完成，所以直接跳转到main
;若是从NOR启动，则将RO和RW部分都拷贝到内存，然后跳转到内存运行（也可在NOR中运行，只是速度稍慢）
;
;注：若在NOR中直接运行，需把RO/BASE改为0并定义RW/BASE 会跳过RO拷贝
;=============================================================================================
	
		
NORRwCopy	
	ldr	r0, TopOfROM
	ldr r1, BaseOfROM
	sub r0, r0, r1			;TopOfROM-BaseOfROM得到从0开始RW的偏移地址
	ldr	r2, BaseOfBSS		;将RW部分的数据从ROM拷贝到RAM
	ldr	r3, BaseOfZero	
0
	cmp	r2, r3
	ldrcc	r1, [r0], #4
	strcc	r1, [r2], #4
	bcc	%B0	
	
InitRamZero
	mov	r0,	#0
	ldr r2, BaseOfZero
	ldr	r3,	EndOfBSS
1	
	cmp	r2,	r3				;初始化Zero部分 不管从哪里启动，这部分都需要执行
	strcc	r0, [r2], #4
	bcc	%B1
	
	ldr	pc, =CEntry		;goto compiler address

	
;	[ CLKDIV_VAL>1 		; means Fclk:Hclk is not 1:1.
;	bl	MMU_SetAsyncBusMode
;	|
;	bl MMU_SetFastBusMode	; default value.
;	]
	


CEntry
 	bl	Main	;Don t use main() because ......
 	b	.


;=========================================================
ClearSdram
	mov r1,#0
	mov r2,#0
	mov r3,#0
	mov r4,#0
	mov r5,#0
	mov r6,#0
	mov r7,#0
	mov r8,#0
	
	ldr	r9,=0x00700000   ;for wince
	ldr	r0,=0x30000000
0	
	stmia	r0!,{r1-r8}
	subs	r9,r9,#32 
	bne	%B0
	bx lr
	
;===========================================================	
;function initializing stacks
InitStacks
	;Don t use DRAM,such as stmfd,ldmfd......
	;SVCstack is initialized before
	;Under toolkit ver 2.5, 'msr cpsr,r1' can be used instead of 'msr cpsr_cxsf,r1'
	mrs	r0,cpsr
	bic	r0,r0,#MODEMASK
	orr	r1,r0,#UNDEFMODE|NOINT
	msr	cpsr_cxsf,r1		;UndefMode
	ldr	sp,=UndefStack		; UndefStack=0x33FF_5C00

	orr	r1,r0,#ABORTMODE|NOINT
	msr	cpsr_cxsf,r1		;AbortMode
	ldr	sp,=AbortStack		; AbortStack=0x33FF_6000

	orr	r1,r0,#IRQMODE|NOINT
	msr	cpsr_cxsf,r1		;IRQMode
	ldr	sp,=IRQStack		; IRQStack=0x33FF_7000

	orr	r1,r0,#FIQMODE|NOINT
	msr	cpsr_cxsf,r1		;FIQMode
	ldr	sp,=FIQStack		; FIQStack=0x33FF_8000

	bic	r0,r0,#MODEMASK|NOINT
	orr	r1,r0,#SVCMODE
	msr	cpsr_cxsf,r1		;SVCMode
	ldr	sp,=SVCStack		; SVCStack=0x33FF_5800

	;USER mode has not be initialized.

	bx lr
	;The LR register won t be valid if the current mode is not SVC mode.
	
;===========================================================


	


;--------------------LED test
	EXPORT	Led_Test
Led_Test
	mov	r0, #0x56000000
	mov	r1, #0x5500
	str	r1, [r0, #0x50]
	
0	
	mov	r1, #0x50
	str	r1, [r0, #0x54]
	mov	r2, #0x100000
1
	subs	r2, r2, #1
	bne	%B1
	
	mov	r1, #0xa0
	str	r1, [r0, #0x54]
	mov	r2, #0x100000
2
	subs	r2, r2, #1
	bne	%B2
	b	%B0
	bx  lr

;===========================================================

	LTORG

;GCS0->SST39VF1601
;GCS1->16c550
;GCS2->IDE
;GCS3->CS8900
;GCS4->DM9000
;GCS5->CF Card
;GCS6->SDRAM
;GCS7->unused

SMRDATA DATA
; Memory configuration should be optimized for best performance
; The following parameter is not optimized.
; Memory access cycle parameter strategy
; 1) The memory settings is  safe parameters even at HCLK=75Mhz.
; 2) SDRAM refresh period is for HCLK<=75Mhz.

	DCD (0+(B1_BWSCON<<4)+(B2_BWSCON<<8)+(B3_BWSCON<<12)+(B4_BWSCON<<16)+(B5_BWSCON<<20)+(B6_BWSCON<<24)+(B7_BWSCON<<28))
	DCD ((B0_Tacs<<13)+(B0_Tcos<<11)+(B0_Tacc<<8)+(B0_Tcoh<<6)+(B0_Tah<<4)+(B0_Tacp<<2)+(B0_PMC))   ;GCS0
	DCD ((B1_Tacs<<13)+(B1_Tcos<<11)+(B1_Tacc<<8)+(B1_Tcoh<<6)+(B1_Tah<<4)+(B1_Tacp<<2)+(B1_PMC))   ;GCS1
	DCD ((B2_Tacs<<13)+(B2_Tcos<<11)+(B2_Tacc<<8)+(B2_Tcoh<<6)+(B2_Tah<<4)+(B2_Tacp<<2)+(B2_PMC))   ;GCS2
	DCD ((B3_Tacs<<13)+(B3_Tcos<<11)+(B3_Tacc<<8)+(B3_Tcoh<<6)+(B3_Tah<<4)+(B3_Tacp<<2)+(B3_PMC))   ;GCS3
	DCD ((B4_Tacs<<13)+(B4_Tcos<<11)+(B4_Tacc<<8)+(B4_Tcoh<<6)+(B4_Tah<<4)+(B4_Tacp<<2)+(B4_PMC))   ;GCS4
	DCD ((B5_Tacs<<13)+(B5_Tcos<<11)+(B5_Tacc<<8)+(B5_Tcoh<<6)+(B5_Tah<<4)+(B5_Tacp<<2)+(B5_PMC))   ;GCS5
	DCD ((B6_MT<<15)+(B6_Trcd<<2)+(B6_SCAN))    ;GCS6
	DCD ((B7_MT<<15)+(B7_Trcd<<2)+(B7_SCAN))    ;GCS7
	DCD ((REFEN<<23)+(TREFMD<<22)+(Trp<<20)+(Tsrc<<18)+(Tchr<<16)+REFCNT)

	DCD 0x32	    ;SCLK power saving mode, BANKSIZE 128M/128M
	;DCD 0x02	    ;SCLK power saving disable, BANKSIZE 128M/128M

	DCD 0x20	    ;MRSR6 CL=2clk
	DCD 0x20	    ;MRSR7 CL=2clk
	
BaseOfROM	DCD	|Image$$ER_ROM1$$RO$$Base|
TopOfROM	DCD	|Image$$ER_ROM1$$RO$$Limit|
BaseOfBSS	DCD	|Image$$RW_RAM1$$RW$$Base|
BaseOfZero	DCD	|Image$$RW_RAM1$$ZI$$Base|
EndOfBSS	DCD	|Image$$RW_RAM1$$ZI$$Limit|

	ALIGN
	
	
;=====================================================================
; Clock division test
; Assemble code, because VSYNC time is very short
;=====================================================================
	EXPORT CLKDIV124
	EXPORT CLKDIV144
	
CLKDIV124
	
	ldr     r0, = CLKDIVN
	ldr     r1, = 0x3		; 0x3 = 1:2:4
	str     r1, [r0]
;	wait until clock is stable
	nop
	nop
	nop
	nop
	nop

	ldr     r0, = REFRESH
	ldr     r1, [r0]
	bic		r1, r1, #0xff
	bic		r1, r1, #(0x7<<8)
	orr		r1, r1, #0x470	; REFCNT135
	str     r1, [r0]
	nop
	nop
	nop
	nop
	nop
	bx  lr

CLKDIV144
	ldr     r0, = CLKDIVN
	ldr     r1, = 0x4		; 0x4 = 1:4:4
	str     r1, [r0]
;	wait until clock is stable
	nop
	nop
	nop
	nop
	nop

	ldr     r0, = REFRESH
	ldr     r1, [r0]
	bic		r1, r1, #0xff
	bic		r1, r1, #(0x7<<8)
	orr		r1, r1, #0x630	; REFCNT675 - 1520
	str     r1, [r0]
	nop
	nop
	nop
	nop
	nop
	bx  lr


	
	END
