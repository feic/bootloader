/* File: main.c
 *  Fuction:	构建最小的bootloader引导linux内核
 *		small Bios 精简了vivi  编译后小于3k
 *  By:	LiuNing		Date:2011-9-28
 *  qq: 	363894801
 *		注意此版本屏蔽了坏块判断
 *           未对gpio初始化
 */
#include "smdk2440.h"



#define BOOT_MEM_BASE		0x30000000
#define LINUX_KERNEL_OFFSET	0x8000
#define LINUX_PARAM_OFFSET	0x100
#define SZ_4K				0x00001000

#define LINUX_PAGE_SIZE		SZ_4K
#define LINUX_PAGE_SHIFT	12
#define LINUX_ZIMAGE_MAGIC	0x016f2818
#define COMMAND_LINE_SIZE	1024


/* This is the old deprecated way to pass parameters to the kernel */
struct param_struct {
union {
	struct {
		unsigned long page_size; /* 0 */
		unsigned long nr_pages; /* 4 */
		unsigned long ramdisk_size; /* 8 */
		unsigned long flags; /* 12 */
#define FLAG_READONLY 1
#define FLAG_RDLOAD 4
#define FLAG_RDPROMPT 8
		unsigned long rootdev; /* 16 */
		unsigned long video_num_cols; /* 20 */
		unsigned long video_num_rows; /* 24 */
		unsigned long video_x; /* 28 */
		unsigned long video_y; /* 32 */
		unsigned long memc_control_reg; /* 36 */
		unsigned char sounddefault; /* 40 */
		unsigned char adfsdrives; /* 41 */
		unsigned char bytes_per_char_h; /* 42 */
		unsigned char bytes_per_char_v; /* 43 */
		unsigned long pages_in_bank[4]; /* 44 */
		unsigned long pages_in_vram; /* 60 */
		unsigned long initrd_start; /* 64 */
		unsigned long initrd_size; /* 68 */
		unsigned long rd_start; /* 72 */
		unsigned long system_rev; /* 76 */
		unsigned long system_serial_low; /* 80 */
		unsigned long system_serial_high; /* 84 */
		unsigned long mem_fclk_21285; /* 88 */
	} s;
	char unused[256];
} u1;

union{
	char paths[8][128];
	struct {
		unsigned long magic;
		char n[1024 - sizeof(unsigned long)];
	} s;
} u2;
char commandline[COMMAND_LINE_SIZE];
};


#define UART_CTL_BASE	0x50000000
#define oUTRSTAT		0x10	
#define oUTXHL			0x20	

#define __REGl(x)	(*(volatile unsigned long *)(x))
#define __REGb(x)	(*(volatile unsigned char *)(x))

#define bUART(x, Nb)		__REGl(UART_CTL_BASE + (x)*0x4000 + (Nb))
#define bUARTb(x, Nb)		__REGb(UART_CTL_BASE + (x)*0x4000 + (Nb))

#define UTRSTAT0		bUART(0, oUTRSTAT)
#define UTXH0			bUARTb(0, oUTXHL)
#define UTRSTAT_TX_EMPTY	(1 << 2)

#define SERIAL_WRITE_READY()	((UTRSTAT0) & UTRSTAT_TX_EMPTY)
#define SERIAL_WRITE_CHAR(c)	((UTXH0) = (c))

#define PROC_SERIAL_PUTC(c)	\
	({ while (!SERIAL_WRITE_READY()); \
	   SERIAL_WRITE_CHAR(c); })

void putnstr(const char *str, int n)
{
	if (str == "")
		return;

	while (n && *str != '\0')
	{
		PROC_SERIAL_PUTC(*str);
		str++;
		n--;
	}
}

int strlen1(const char* str)
{
	int i=0;
	while(*str++) i++;
	return i;
} 

void putstr(const char *str)
{
	putnstr(str, strlen1(str));
	PROC_SERIAL_PUTC('\r');
	PROC_SERIAL_PUTC('\n');
}   


void Port_Init(void)
{
    GPACON = 0x7fffff; 
    GPBCON = 0x055555;
    GPBUP  = 0x6Bf;     // The pull up function is disabled GPB[10:0]

    GPCCON = 0xaaaaaaaa;       
    GPCUP  = 0xffff;     // The pull up function is disabled GPC[15:0] 

    GPDCON = 0xaaaaaaaa;       
    GPDUP  = 0xffff;     // The pull up function is disabled GPD[15:0]

    //rGPEUP  = 0xffff;     // The pull up function is disabled GPE[15:0]
	GPECON = 0xa02aa800; // For added AC97 setting      
    GPEUP  = 0xffff;     

    GPFCON = 0x55aa;
    GPFUP  = 0xff;     // The pull up function is disabled GPF[7:0]

    GPGCON = 0x00aaa9aa;// GPG9 input without pull-up
    GPGUP  = 0xffff;    // The pull up function is disabled GPG[15:0]
    GPGDAT|= (1 << 4);	//Open lcd backlight 

    //*** PORT H GROUP
    GPHCON = 0x00faaa;
    GPHUP  = 0x7ff;    // The pull up function is disabled GPH[10:0]

    
    //External interrupt will be falling edge triggered. 
    //EXTINT0 = 0x22222222;    // EINT[7:0]
    //EXTINT1 = 0x22222222;    // EINT[15:8]
    //EXTINT2 = 0x22222222;    // EINT[23:16]
}

void  call_linux(long a0, long a1, long a2)
{
	//cache_clean_invalidate();
	__asm__(
		"	mov r1, #0\n"
		"	mov r1, #7 << 5\n"		  /* 8 segments */
		"1: orr r3, r1, #63 << 26\n"	  /* 64 entries */
		"2: mcr p15, 0, r3, c7, c14, 2\n" /* clean & invalidate D index */
		"	subs	r3, r3, #1 << 26\n"
		"	bcs 2b\n"			  /* entries 64 to 0 */
		"	subs	r1, r1, #1 << 5\n"
		"	bcs 1b\n"			  /* segments 7 to 0 */
		"	mcr p15, 0, r1, c7, c5, 0\n"  /* invalidate I cache */
		"	mcr p15, 0, r1, c7, c10, 4\n" /* drain WB */
		);	
	//tlb_invalidate();
	__asm__(
		"mov	r0, #0\n"
		"mcr	p15, 0, r0, c7, c10, 4\n"	/* drain WB */
		"mcr	p15, 0, r0, c8, c7, 0\n"	/* invalidate I & D TLBs */
		);

	__asm__(
		"mov	r0, %0\n"
		"mov	r1, %1\n"
		"mov	r2, %2\n"
		"mov	ip, #0\n"
		"mcr	p15, 0, ip, c13, c0, 0\n"	/* zero PID */
		"mcr	p15, 0, ip, c7, c7, 0\n"	/* invalidate I,D caches */
		"mcr	p15, 0, ip, c7, c10, 4\n"	/* drain write buffer */
		"mcr	p15, 0, ip, c8, c7, 0\n"	/* invalidate I,D TLBs */
		"mrc	p15, 0, ip, c1, c0, 0\n"	/* get control register */
		"bic	ip, ip, #0x0001\n"		/* disable MMU */
		"mcr	p15, 0, ip, c1, c0, 0\n"	/* write control register */
		"mov	pc, r2\n"
		"nop\n"
		"nop\n"
		: /* no outpus */
		: "r" (a0), "r" (a1), "r" (a2)
		);
}

/*
 * pram_base: base address of linux paramter
 */
static void setup_linux_param(long param_base)
{
	struct param_struct *params = (struct param_struct *)param_base; 
	char *linux_cmd;

	/*printk("Setup linux parameters at 0x%08lx\n", param_base);*/
	memset(params, 0, sizeof(struct param_struct));


	params->u1.s.page_size = LINUX_PAGE_SIZE;
	params->u1.s.nr_pages = (DRAM_SIZE >> LINUX_PAGE_SHIFT);
#if 0
	params->u1.s.page_size = LINUX_PAGE_SIZE;
	params->u1.s.nr_pages = (dram_size >> LINUX_PAGE_SHIFT);
	params->u1.s.ramdisk_size = 0;
	params->u1.s.rootdev = rootdev;
	params->u1.s.flags = 0;

	/* TODO */
	/* If use ramdisk */
	/*
	params->u1.s.initrd_start = ?;
	params->u1.s.initrd_size = ?;
	params->u1.s.rd_start = ?;
	*/

#endif

	/* set linux command line */
	linux_cmd= "noinitrd root=/dev/mtdblock2 init=/linuxrc console=ttySAC0";
	memcpy(params->commandline, linux_cmd, strlen1(linux_cmd) + 1);
		
	//putstr("loading linux boot args is ok !"); 
}

extern int nand_read_ll(unsigned char *buf, unsigned long start_addr, int size);

int main(int argc, char *argv[])
{
	putstr ("\nHello small bootloader");
	Port_Init();
	if(!nand_read_ll((unsigned char *)0x30008000, 0x200000,0x280000))	//read kernel from nand
		putstr("read ok!\n");
	else
		putstr("read err!\n");
		
	setup_linux_param(BOOT_MEM_BASE + LINUX_PARAM_OFFSET);
	putstr ("MACH_TYPE = 168");
	putstr("NOW, Booting Linux...");  
	call_linux(0, 168, BOOT_MEM_BASE + LINUX_KERNEL_OFFSET);

	return 0;
}

