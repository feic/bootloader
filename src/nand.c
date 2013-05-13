#include <string.h>
#include <stdio.h>

#include "def.h"
#include "2440addr.h"
#include "2440lib.h"
#include "Nand.h"

//suppport boot params
//#define	GLOBAL_PARAMS
#include "bootpara.h"

//可更改删除分区，分区名字不可改



#define	getch	Uart_Getch

#define	EnNandFlash()	(rNFCONT |= 1)
#define	DsNandFlash()	(rNFCONT &= ~1)




/************** boot linux ***************************/

#define LINUX_PAGE_SHIFT	12
#define LINUX_PAGE_SIZE		(1<<LINUX_PAGE_SHIFT)
#define COMMAND_LINE_SIZE 	1024

struct param_struct {
    union {
	struct {
	    unsigned long page_size;			/*  0 */
	    unsigned long nr_pages;				/*  4 */
	    unsigned long ramdisk_size;			/*  8 */
	    unsigned long flags;				/* 12 */
#define FLAG_READONLY	1
#define FLAG_RDLOAD		4
#define FLAG_RDPROMPT	8
	    unsigned long rootdev;				/* 16 */
	    unsigned long video_num_cols;		/* 20 */
	    unsigned long video_num_rows;		/* 24 */
	    unsigned long video_x;				/* 28 */
	    unsigned long video_y;				/* 32 */
	    unsigned long memc_control_reg;		/* 36 */
	    unsigned char sounddefault;			/* 40 */
	    unsigned char adfsdrives;			/* 41 */
	    unsigned char bytes_per_char_h;		/* 42 */
	    unsigned char bytes_per_char_v;		/* 43 */
	    unsigned long pages_in_bank[4];		/* 44 */
	    unsigned long pages_in_vram;		/* 60 */
	    unsigned long initrd_start;			/* 64 */
	    unsigned long initrd_size;			/* 68 */
	    unsigned long rd_start;				/* 72 */
	    unsigned long system_rev;			/* 76 */
	    unsigned long system_serial_low;	/* 80 */
	    unsigned long system_serial_high;	/* 84 */
	    unsigned long mem_fclk_21285;       /* 88 */
	} s;
	char unused[256];
    } u1;
    union {
	char paths[8][128];
	struct {
	    unsigned long magic;
	    char n[1024 - sizeof(unsigned long)];
	} s;
    } u2;
    char commandline[COMMAND_LINE_SIZE];
};

extern void  call_linux(U32 a0, U32 a1, U32 a2);





void call_linux(U32 a0, U32 a1, U32 a2)
{
	void (*goto_start)(U32, U32);
	
	rINTMSK=BIT_ALLMSK;
	
	__asm{
		mov	r1, #0		
		mov	r1, #7 << 5			  	/* 8 segments */
cache_clean_loop1:		
		orr	r3, r1, #63UL << 26	  	/* 64 entries */
cache_clean_loop2:	
		mcr	p15, 0, r3, c7, c14, 2	/* clean & invalidate D index */
		subs	r3, r3, #1 << 26
		bcs	cache_clean_loop2		/* entries 64 to 0 */
		subs	r1, r1, #1 << 5
		bcs	cache_clean_loop1		/* segments 7 to 0 */
		mcr	p15, 0, r1, c7, c5, 0	/* invalidate I cache */
		mcr	p15, 0, r1, c7, c10, 4	/* drain WB */
	}
	
	
	
	__asm{
		mov	r0, #0
		mcr	p15, 0, r0, c7, c10, 4	/* drain WB */
		mcr	p15, 0, r0, c8, c7, 0	/* invalidate I & D TLBs */
	}
	
	

	__asm{
//		mov	r0, a0//%0
//		mov	r1, a1//%1
//		mov	r2, a2//%2
		mov	ip, #0
		mcr	p15, 0, ip, c13, c0, 0	/* zero PID */
		mcr	p15, 0, ip, c7, c7, 0	/* invalidate I,D caches */
		mcr	p15, 0, ip, c7, c10, 4	/* drain write buffer */
		mcr	p15, 0, ip, c8, c7, 0	/* invalidate I,D TLBs */
		mrc	p15, 0, ip, c1, c0, 0	/* get control register */
		bic	ip, ip, #0x0001			/* disable MMU */
		mcr	p15, 0, ip, c1, c0, 0	/* write control register */
		//mov	pc, r2
		//nop
		//nop
		/* no outpus */
		//: "r" (a0), "r" (a1), "r" (a2)
	}
//	SetClockDivider(1, 1);
//	SetSysFclk(FCLK_200M);		//start kernel, use 200M
	//SET_IF();
	goto_start = (void (*)(U32, U32))a2;
	(*goto_start)(a0, a1);	
}

extern int sprintf(char * /*s*/, const char * /*format*/, ...);
extern int CheckBadBlk(U32 addr);
void LoadRun(void)
{
	U32 i, ram_addr, buf = 0x30200000;//boot_params.run_addr.val;
	struct param_struct *params = (struct param_struct *)0x30000100;

	int size;
	char *parameters;
	memset(params, 0, sizeof(struct param_struct));
	
			
		memset(parameters, 0, sizeof(parameters));
		parameters="root=/dev/mtdblock3 init=/linuxrc load_ramdisk=0 console=ttySAC0,115200 mem=65536K devfs=mount display=sam320 DEFAULT_USER_PARAMS";
						
		
		params->u1.s.page_size = LINUX_PAGE_SIZE;
		params->u1.s.nr_pages = (boot_params.mem_cfg.val >> LINUX_PAGE_SHIFT);
		memcpy(params->commandline, parameters, strlen(parameters));
	
		
	
	StartPage = 0x00500000>>11;
	size = 0x00300000;
	ram_addr = buf;	

	for(i=0; size>0; ) {
		if(!(i&0x3f)) {
			if(CheckBadBlk(i+StartPage)) {
				
				i += 64;
				size -= 64<<11;
				continue;
			}
		}
		ReadPage((i+StartPage), (U8 *)ram_addr);
		i++;
		size -= 2048;
		ram_addr += 2048;
	}


	DsNandFlash();

		
	call_linux(0, boot_params.machine.val, buf);
}


