;=========================================
; NAME: 2440INIT.S
; DESC: C start up codes
;       Configure memory, ISR ,stacks
;	Initialize C-variables
; HISTORY:
; 2002.02.25:kwtark: ver 0.0
; 2002.03.20:purnnamu: Add some functions for testing STOP,Sleep mode
; 2003.03.14:DonGo: Modified for 2440.
; Modified by Allium for RAM saving
;=========================================

	GET option.inc
	GET memcfg.inc
	GET 2440addr.inc

BIT_SELFREFRESH EQU	(1<<22)


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

 

 		

	IMPORT  |Image$$ER_ROM1$$RO$$Base|	; Base of ROM code
	IMPORT  |Image$$ER_ROM1$$RO$$Limit|  ; End of ROM code (=start of ROM data)
	IMPORT  |Image$$RW_RAM1$$RW$$Base|   ; Base of RAM to initialise
	IMPORT  |Image$$RW_RAM1$$ZI$$Base|   ; Base and limit of area
	IMPORT  |Image$$RW_RAM1$$ZI$$Limit|  ; to zero initialise
	
	

	IMPORT  Bootloader    ; The main entry of mon program


	AREA    RESET,CODE,READONLY

	
	PRESERVE8

	ENTRY






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

	; rGPFDAT = (rGPFDAT & ~(0xf<<4)) | ((~data & 0xf)<<4);
	; Led_Display
	LDR	R3,	=0x56000010		;把GPB的控制寄存器地址保存在r3中
	LDR	R4,	=0x555555		;
	STR	R4,	[R3]			;初始化GPB口的控制寄存器，
							;使LED0-LED3对应的管脚为输出
	
	LDR R1,	=0x56000014		;GPB的DAT寄存器地址
	MOV	R2,	#0x00001		;
	STR R2,	[R1]			;点亮led0-led3,蜂鸣器蜂鸣


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
	
	

	bl	InitStacks


;===========================================================
;// 判断是从nor启动还是从nand启动
;===========================================================
	;bl	Led_Test
	
	ldr	r0, =BWSCON
	ldr	r0, [r0]
	ands	r0, r0, #6		;OM[1:0] != 0, NOR FLash boot
	blne	NORRwCopy		;don t read nand flash
	
;=============================================================================================
;若是从NAND启动，不进行拷贝，直接进行ZI部分初始化
;若是从NOR启动，则将RW部分拷贝到内存
;=============================================================================================

InitRamZero
	mov	r0,	#0
	ldr r2, BaseOfZero
	ldr	r3,	EndOfBSS
1	
	cmp	r2,	r3				;初始化Zero部分 不管从哪里启动，这部分都需要执行
	strcc	r0, [r2], #4
	bcc	%B1
	
	ldr	pc, =CEntry		;goto compiler address

	

CEntry
 	bl	Bootloader	;Don t use main() because ......
 

;=========================================================
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

	mov	pc,lr
	;The LR register won t be valid if the current mode is not SVC mode.
	

	
;===========================================================
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
	mov pc,lr 
	

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
	
	END
