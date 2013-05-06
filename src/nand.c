#include <string.h>
#include <stdio.h>

#include "def.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"
#include "Nand.h"

//suppport boot params
//#define	GLOBAL_PARAMS
#include "bootpara.h"

//可更改删除分区，分区名字不可改
static struct Partition NandPart[] = {
	{0, 		 0x00020000, "boot"},		//128K one block
	{0x00020000, 0x00060000, "bootParam"},	//384K three blocks
	{0x00080000, 0x00100000, "pic"},		//1M
	{0x00180000, 0x00380000, "MyApp"},		//3.5M
	{0x00500000, 0x00300000, "kernel"},		//3M
	{0x00800000, 0x03c00000, "fs_yaffs"},	//60M
	{0x04400000, 0x00080000, "eboot"},		//512K
	{0x04480000, 0x03b80000, "wince"},		//59.5M
	{0,			 0         , 0}
};

#define	puts	Uart_Printf
#define	printf	Uart_Printf
#define	getch	Uart_Getch
#define	putch	Uart_SendByte
#define	EnNandFlash()	(rNFCONT |= 1)
#define	DsNandFlash()	(rNFCONT &= ~1)
struct Partition * NandSelPart(char *info)
{
	U16 i, max_sel;
	struct Partition *ptr = NandPart;
	
	printf("\nPlease select which region to %s : Esc to abort\n", info);
	
	for(i=0; ptr->size!=0; i++, ptr++)
		printf("%d : offset 0x%08x, size 0x%08x [%s]\n", i, ptr->offset, ptr->size, ptr->name);
		
	max_sel = i;
	
	while(1) {
		i = getch();
		if(i==0x1b)
			return NULL;
		if((i>='0')&&(i<(max_sel+'0'))) {
			i -= '0';
			//StartPage = NandPart[i].offset>>11;
			//BlockCnt  = NandPart[i].size>>17;
			//return i;
			return &NandPart[i];
		}
	}	
}

struct Partition * NandSelPart_2(char *info)
{
	U16 i, max_sel;
	struct Partition *ptr = NandPart;
	
	//printf("\nPlease select which region to %s : Esc to abort\n", info);
	
	for(i=0; ptr->size!=0; i++, ptr++)
		//printf("%d : offset 0x%08x, size 0x%08x [%s]\n", i, ptr->offset, ptr->size, ptr->name);
		if(!strcmp(ptr->name,info))
			return ptr;

	printf(" ERROR:there is no %s part\n",info);
	return NULL;
		

}

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

/*************************************************************/
static __inline void cpu_arm920_cache_clean_invalidate_all(void)
{
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
}
void cache_clean_invalidate(void)
{
	cpu_arm920_cache_clean_invalidate_all();
}

static __inline void cpu_arm920_tlb_invalidate_all(void)
{
	__asm{
		mov	r0, #0
		mcr	p15, 0, r0, c7, c10, 4	/* drain WB */
		mcr	p15, 0, r0, c8, c7, 0	/* invalidate I & D TLBs */
	}
}

void tlb_invalidate(void)
{
	cpu_arm920_tlb_invalidate_all();
}

void disable_irq(void);

void call_linux(U32 a0, U32 a1, U32 a2)
{
	void (*goto_start)(U32, U32);
	
	rINTMSK=BIT_ALLMSK;
	
	cache_clean_invalidate();
	tlb_invalidate();	

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
static void LoadRun(void)
{
	U32 i, ram_addr, buf = 0x30200000;//boot_params.run_addr.val;
	struct param_struct *params = (struct param_struct *)0x30000100;
	struct Partition *nf_part;
	int size;
	
	memset(params, 0, sizeof(struct param_struct));


	if(1/*boot_params.start.val*/) {
		char parameters[512];
		char *rootfs;
		char initrd[32];
		char *tty_sel;
		char *devfs_sel;
		char *display_sel;
		
		switch (boot_params.root_sel.val) {
		case 0:
			rootfs = "/dev/ram";
			break;
		case 1:
			rootfs = "nfs";
			break;
		case 2:
			rootfs = "/dev/mtdblock2"; //    by pht.
			break;
		case 3:
			rootfs = "/dev/mtdblock3";
			break;
		case 4:
			rootfs = "/dev/mtdblock4";
			break;
		default:
			rootfs="";	//user define
			break;
		}
		
		switch (boot_params.display_sel.val) {
		case 0:
			display_sel = "display=sam320";
			break;
		case 1:
			display_sel = "display=vga640";
			break;
		case 2:
			display_sel = "display=vga800";
			break;			
		case 3:
			display_sel = "display=qch800";
			break;	
		case 4:
			display_sel = "display=lcd480";
			break;	
											
		default:
			display_sel = "display=sam640";	
			break;
		}
	
	
	
	
	
		if(boot_params.root_sel.val)
			sprintf(initrd, "load_ramdisk=0");
		else
			sprintf(initrd, "initrd=0x%08x,0x%08x",
						boot_params.initrd_addr.val,
						boot_params.initrd_len.val);
	
		switch (boot_params.tty_sel.val) {
		case 0:
			tty_sel="ttySAC0";
			break;
		case 1:
			tty_sel="ttySAC1";
			break;
		case 2:
			tty_sel="ttyS0";
			break;
		case 3:
			tty_sel="ttyS1";
			break;
		case 4:
			tty_sel="tty0";
			break;
		default:
			tty_sel="ttySAC0";	//user define
			break;
		}
	

		devfs_sel = "devfs=mount";

		
		memset(parameters, 0, sizeof(parameters));
		sprintf(parameters,
					"root=%s init=/linuxrc %s console=%s,%d mem=%dK %s %s %s",
					rootfs,
					initrd,
					tty_sel,
					boot_params.serial_baud.val,
					boot_params.mem_cfg.val>>10,
					devfs_sel,
					display_sel,
					boot_params.string);
		
		params->u1.s.page_size = LINUX_PAGE_SIZE;
		params->u1.s.nr_pages = (boot_params.mem_cfg.val >> LINUX_PAGE_SHIFT);
		memcpy(params->commandline, parameters, strlen(parameters));
	
		printf("Set boot params = %s\n", params->commandline);
	}

	printf("Load Kernel...\n");	

	
	
	nf_part = NandSelPart_2("kernel");	
	if(!nf_part)
		return;	
	StartPage = nf_part->offset>>11;
	size = nf_part->size;
	ram_addr = buf;	

	for(i=0; size>0; ) {
		if(!(i&0x3f)) {
			if(CheckBadBlk(i+StartPage)) {
				printf("Skipped bad block at 0x%x\n", i+StartPage);
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

/*	
	if(!boot_params.root_sel.val) {
		int ramdisk_sz;
		
		printf("Load Ramdisk...\n");
		
		StartPage = NandPart[3].offset>>11;
		size = NandPart[3].size;
		
		ram_addr = boot_params.initrd_addr.val;
		ramdisk_sz = boot_params.initrd_len.val;
		
		for(i=0; size>0&&ramdisk_sz>0; ) {
			if(!(i&0x3f)) {
				if(CheckBadBlk(i+StartPage)) {
					printf("Skipped bad block at 0x%x\n", i+StartPage);
					i += 64;
					size -= 64<<9;
					continue;
				}
			}
			ReadPage((i+StartPage), (U8 *)ram_addr);
			i++;
			size -= 2048;
			ram_addr += 2048;
			ramdisk_sz -= 2048;
		}
	}
*/	
	DsNandFlash();

		//ChangeClockDivider(13,12);
		//ChangeMPllValue(67,1,1);	//300MHz,2440A!
		//rCLKCON = 0x7fff0;
	call_linux(0, boot_params.machine.val, buf);
}

/***************** LOAD PIC ***********************/

void LoadPic(U32 PicBuffer)
{
	U32 i, ram_addr;
	int size;
	struct Partition *nf_part;
	nf_part = NandSelPart_2("pic");	
	if(!nf_part)
		return;	
	StartPage = nf_part->offset>>11;
	size = nf_part->size;
	ram_addr = PicBuffer;
	
	InitNandFlash(0);

	for(i=0; size>0; ) {
		if(!(i&0x3f)) {
			if(CheckBadBlk(i+StartPage)) {
				printf("Skipped bad block at 0x%x\n", i+StartPage);
				i += 64;
				//size -= 32<<9;
				continue;
			}
		}
		ReadPage((i+StartPage), (U8 *)ram_addr);
		i++;
		size -= 2048;
		ram_addr += 2048;
	}
}
/***************** boot APP ***********************/
static void LoadRunApp()
{
	U32 i, ram_addr, buf = boot_params.initrd_addr.val;
	int size;
	struct Partition *nf_part;
	
	MemoryTest();
	printf("Load Application...\n");	

	
	
	nf_part = NandSelPart_2("MyApp");	
	if(!nf_part)
		return;	
	StartPage = nf_part->offset>>11;
	size = nf_part->size;
	//StartPage = NandPart[1].offset>>11;	//part 2,3...
	//size = NandPart[1].size;//NandPart[boot_params.root_sel.val].size;
	ram_addr = boot_params.AppRun_addr.val;

	for(i=0; size>0; ) {
		if(!(i&0x3f)) {
			if(CheckBadBlk(i+StartPage)) {
				printf("Skipped bad block at 0x%x\n", i+StartPage);
				i += 64;
				//size -= 32<<9;
				continue;
			}
		}
		ReadPage((i+StartPage), (U8 *)ram_addr);
		i++;
		size -= 2048;
		ram_addr += 2048;
	}

	
	printf("run 0x%08x...\n",boot_params.AppRun_addr.val);
	//RelocateNKBIN(buf, &buf, &i, &boot_params.run_addr.val);
	call_linux(0, 0, boot_params.AppRun_addr.val);
}

/***************** boot wince ***********************/
extern MemoryTest(void);
int RelocateNKBIN(U32 img_src, U32 *pStart, U32 *pLength, U32 *pLaunch);

static void LoadRunWince(void)
{
	U32 i, ram_addr;
	int size;
	struct Partition *nf_part;
	
	U32 EBOOT_RUN_ADDR = 0x30038000;
	U32 EBOOT_SIZE	= 0x40000;
	
	//MemoryTest();
	printf("Load eboot...\n");	

	
	nf_part = NandSelPart_2("eboot");	
	if(!nf_part)
		return;	
	StartPage = nf_part->offset>>11;
	//size = nf_part->size;
	//StartPage = NandPart[5].offset>>11;	//part 2,3...
	size = EBOOT_SIZE;//NandPart[boot_params.root_sel.val].size;
	ram_addr = EBOOT_RUN_ADDR;

	for(i=0; size>0; ) {
		if(!(i&0x3f)) {
			if(CheckBadBlk(i+StartPage)) {
				printf("Skipped bad block at 0x%x\n", i+StartPage);
				i += 64;
				//size -= 32<<9;
				continue;
			}
		}
		ReadPage((i+StartPage), (U8 *)ram_addr);
		i++;
		size -= 2048;
		ram_addr += 2048;
	}

	
	printf("run 0x%08x...\n", EBOOT_RUN_ADDR);
	//RelocateNKBIN(buf, &buf, &i, &boot_params.run_addr.val);
	call_linux(0, 0, EBOOT_RUN_ADDR);
}

void NandLoadRun(void)
{
	InitNandFlash(1);
	LoadRun();
	
}
void NandLoadRun_wince(void)
{
	InitNandFlash(1);
	LoadRunWince();
}

void NandLoadRun_App()
{
	InitNandFlash(1);
	LoadRunApp();
}

void AutoNandLoadRun()
{
	InitNandFlash(0);
	switch(boot_params.start.val){
	case 1:
		LoadRunApp();
		break;
	case 2:
		LoadRun();
		break;
	case 3:
		LoadRunWince();	
		break;
	default:
		break;
	}

		
}

void AutoNandLoadRun_key()
{
	InitNandFlash(0);

	if(!((rGPFDAT>>5)&0x1))
		LoadRunWince();	
	else if(!((rGPFDAT>>1)&0x1))
		LoadRun();
	else if(!((rGPGDAT>>1)&0x1))
		LoadRunApp();
		
}