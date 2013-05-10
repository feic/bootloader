/*
Support 512/page NAND Flash only
*/
#include <string.h>
#include <stdio.h>

#include "def.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"
#include "Nand.h"

//suppport boot params
#define	GLOBAL_PARAMS
#include "bootpara.h"

#define BPAGE_MAGIC_ADD 0x33ff0000
#define BPAGE_ADD 0x33ff0008
const char bpage_magic[8] = {'B', 'b', 'B', 'b', 'P', 'a', 'G', 'e'};

#define	puts	Uart_SendString
#define	printf	Uart_SendString
#define	getch	Uart_Getch
#define	putch	Uart_SendByte

#define	EnNandFlash()	(rNFCONT |= 1)
#define	DsNandFlash()	(rNFCONT &= ~1)
#define	NFChipEn()		(rNFCONT &= ~(1<<1))
#define	NFChipDs()		(rNFCONT |= (1<<1))
#define	InitEcc()		(rNFCONT |= (1<<4))
#define	MEccUnlock()	(rNFCONT &= ~(1<<5))
#define	MEccLock()		(rNFCONT |= (1<<5))
#define	SEccUnlock()	(rNFCONT &= ~(1<<6))
#define	SEccLock()		(rNFCONT |= (1<<6))

#define	WrNFDat8(dat)	(rNFDATA8 = (dat))
#define	WrNFDat32(dat)	(rNFDATA = (dat))
#define	RdNFDat8()		(rNFDATA8)	//byte access
#define	RdNFDat32()		(rNFDATA)	//word access

#define	WrNFCmd(cmd)	(rNFCMD = (cmd))
#define	WrNFAddr(addr)	(rNFADDR = (addr))
#define	WrNFDat(dat)	WrNFDat8(dat)
#define	RdNFDat()		RdNFDat8()	//for 8 bit nand flash, use byte access

#define	RdNFMEcc()		(rNFMECC0)	//for 8 bit nand flash, only use NFMECC0
#define	RdNFSEcc()		(rNFSECC)	//for 8 bit nand flash, only use low 16 bits

#define	RdNFStat()		(rNFSTAT)
#define	NFIsBusy()		(!(rNFSTAT&1))
#define	NFIsReady()		(rNFSTAT&1)

//#define	WIAT_BUSY_HARD	1
//#define	ER_BAD_BLK_TEST
//#define	WR_BAD_BLK_TEST

#define	READCMD0	0
#define	READCMD1	0x30
#define	ERASECMD0	0x60
#define	ERASECMD1	0xd0
#define	PROGCMD0	0x80
#define	PROGCMD1	0x10
#define	QUERYCMD	0x70
#define	RdIDCMD		0x90

static U16 NandAddr;
static int fs_yaffs;

// HCLK=100Mhz
#define TACLS		1//7	// 1-clk(0ns) 
#define TWRPH0		4//7	// 3-clk(25ns)
#define TWRPH1		1//7	// 1-clk(10ns)  //TACLS+TWRPH0+TWRPH1>=50ns

void InitNandFlash(int info);
void cpy_bpage(void);
void add_bpage(unsigned int seq);

static void InitNandCfg(void)
{
	// for S3C2440

	rNFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4)|(0<<0);	
	// TACLS		[14:12]	CLE&ALE duration = HCLK*TACLS.
	// TWRPH0		[10:8]	TWRPH0 duration = HCLK*(TWRPH0+1)
	// TWRPH1		[6:4]	TWRPH1 duration = HCLK*(TWRPH1+1)
	// AdvFlash(R)	[3]		Advanced NAND, 0:256/512, 1:1024/2048
	// PageSize(R)	[2]		NAND memory page size
	//						when [3]==0, 0:256, 1:512 bytes/page.
	//						when [3]==1, 0:1024, 1:2048 bytes/page.
	// AddrCycle(R)	[1]		NAND flash addr size
	//						when [3]==0, 0:3-addr, 1:4-addr.
	//						when [3]==1, 0:4-addr, 1:5-addr.
	// BusWidth(R/W) [0]	NAND bus width. 0:8-bit, 1:16-bit.
	
	rNFCONT = (0<<13)|(0<<12)|(0<<10)|(0<<9)|(0<<8)|(1<<6)|(1<<5)|(1<<4)|(1<<1)|(1<<0);
	// Lock-tight	[13]	0:Disable lock, 1:Enable lock.
	// Soft Lock	[12]	0:Disable lock, 1:Enable lock.
	// EnablillegalAcINT[10]	Illegal access interupt control. 0:Disable, 1:Enable
	// EnbRnBINT	[9]		RnB interrupt. 0:Disable, 1:Enable
	// RnB_TrandMode[8]		RnB transition detection config. 0:Low to High, 1:High to Low
	// SpareECCLock	[6]		0:Unlock, 1:Lock
	// MainECCLock	[5]		0:Unlock, 1:Lock
	// InitECC(W)	[4]		1:Init ECC decoder/encoder.
	// Reg_nCE		[1]		0:nFCE=0, 1:nFCE=1.
	// NANDC Enable	[0]		operating mode. 0:Disable, 1:Enable.

}

#ifdef	WIAT_BUSY_HARD
#define	WaitNFBusy()	while(NFIsBusy())
#else
static U32 WaitNFBusy(void)	// R/B 未接好?
{
	U8 stat;
	
	WrNFCmd(QUERYCMD);
	do {
		stat = RdNFDat();
		//printf("%x\n", stat);
	}while(!(stat&0x40));
	WrNFCmd(READCMD0);
	return stat&1;
}
#endif

static U32 ReadChipId(void)
{
	U32 id;
	
	NFChipEn();	
	WrNFCmd(RdIDCMD);
	WrNFAddr(0);
	while(NFIsBusy());	
	id  = RdNFDat()<<8;
	id |= RdNFDat();		
	NFChipDs();		
	
	return id;
}

static U16 ReadStatus(void)
{
	U16 stat;
	
	NFChipEn();	
	WrNFCmd(QUERYCMD);		
	stat = RdNFDat();	
	NFChipDs();
	
	return stat;
}


U32 EraseBlock(U32 addr)
{
	U8 stat;

	addr &= ~0x3f;
	
	/*先检测是否为坏块，若是坏块的话则不再擦除，厂家标记的坏块不要动  pht 090422*/
	if(CheckBadBlk(addr))
		return 1;

	NFChipEn();	
	WrNFCmd(ERASECMD0);		
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
	WrNFCmd(ERASECMD1);		
	stat = WaitNFBusy();
	NFChipDs();
	

	putch('.');
	//printf("Erase block 0x%x %s\n", addr, stat?"fail":"ok");
		
	return stat;
}

//addr = page address
void ReadPage(U32 addr, U8 *buf)
{
	U16 i;
	
	NFChipEn();
	WrNFCmd(READCMD0);
	WrNFAddr(0);
	WrNFAddr(0);
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);	
	WrNFCmd(READCMD1);
	InitEcc();
	WaitNFBusy();
	for(i=0; i<2048; i++)
		buf[i] = RdNFDat();
	NFChipDs();
}
int CheckBadBlk(U32 addr);

void MarkBadBlk(U32 addr)
{
	addr &= ~0x3f;
	
	
	NFChipEn();
	

	WrNFCmd(PROGCMD0);
	//mark offset 2048
	WrNFAddr(0);		//2048&0xff
	WrNFAddr(8);		//(2048>>8)&0xf
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
	WrNFDat(0);			//mark with 0
	WrNFCmd(PROGCMD1);
	WaitNFBusy();		//needn't check return status
		
	NFChipDs();
}

int CheckBadBlk(U32 addr)
{
	U8 dat;
	
	addr &= ~0x3f;


	NFChipEn();
	WrNFCmd(READCMD0);
	WrNFAddr(0);		//2048&0xff
	WrNFAddr(8);		//(2048>>8)&0xf
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
	WrNFCmd(READCMD1);
	WaitNFBusy();
	dat = RdNFDat();
	
	
	NFChipDs();
	return (dat!=0xff);
}

/************************************************************/




extern U32 downloadAddress; 
extern U32 downloadFileSize;

/*
void wince_rewrite()
{
	U32 launch;
	if(!RelocateNKBIN(downloadAddress, (U32 *)&downloadAddress, (U32 *)&downloadFileSize, &launch)) {
		boot_params.run_addr.val    = launch;
		boot_params.initrd_addr.val = downloadAddress;
		boot_params.initrd_len.val  = downloadFileSize;
		save_params();	//save initrd_len.val
		InitNandFlash(1);
	}
}
*/


/************************************************************/
int have_nandflash;
void InitNandFlash(int info)
{	
	U32 i;
	
	InitNandCfg();
	i = ReadChipId();
	
		
	if(i==0xecda){
		have_nandflash = 1;	
		NandAddr = 1;
	}
	else if(i==0xecf1){	
		have_nandflash = 1;
	}
	else{
		have_nandflash = 0;
		printf("unsupported nandflash id \n");
	}

	//if(info)
		//printf("Nand flash status = %x\n", ReadStatus());
}

static void MarkGoodBlk(U32 addr)
{
	addr &= ~0x3f;
	
	NFChipEn();
	

	WrNFCmd(PROGCMD0);
	//mark offset 2048
	WrNFAddr(0);		//2048&0xff
	WrNFAddr(8);		//(2048>>8)&0xf
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
	WrNFDat(0xff);			//mark with 0xff
	WrNFCmd(PROGCMD1);
	WaitNFBusy();		//needn't check return status
		
	NFChipDs();
}


//==================================================================
#if 1
void cpy_bpage()
{
	if(boot_params.bpage[0]>20)
	{
		boot_params.bpage[0]=0;
	}	

	memcpy((char *)BPAGE_MAGIC_ADD,bpage_magic,8);
	memcpy((unsigned int *)BPAGE_ADD,boot_params.bpage,boot_params.bpage[0]*4+4);
}
//添加坏块信息
void add_bpage(unsigned int seq)
{
		int i, j;
		i = 1;
		
		while (i < boot_params.bpage[0] && boot_params.bpage[i] < seq)
			i++;
		if (boot_params.bpage[i] == seq)
			return;
		else if (i == boot_params.bpage[0])
			boot_params.bpage[i+1] = seq;
		else
		{
			j = boot_params.bpage[0];
			while (j >= i)
			{
				boot_params.bpage[j + 1] = boot_params.bpage[j];
				j--;
			}
			
			boot_params.bpage[i] = seq;
		}
		boot_params.bpage[0]++;
}

#endif
//======================================================================
#include "norflash.h"
#define	NOR_PARAMS_OFFSET	0x20000

//======================================================================
int search_params(void)
{
	U8 dat[0x800];
	int ret=-1;
	BootParams *pBP = (BootParams *)dat;
	
	if(rBWSCON&0x6){//nor flash
		memcpy(dat, (void *)(NOR_PARAMS_OFFSET), sizeof(boot_params));	//now mmu is not set, so use original address
		if(!strncmp(boot_params.start.flags, pBP->start.flags, 10))
			ret = 0;

	}
	else{//nand
	
		U32 i,page, page_cnt;
		struct Partition *nf_part;
		
		nf_part = NandSelPart_2("bootParam");	
		if(!nf_part)
			return ret;
		page = nf_part->offset>>11;
		page_cnt = nf_part->size>>11;
		
		InitNandFlash(0);	//don't show info in InitNandFlash!
		//search from the last page
		for(i=0; i<page_cnt; i++) {
			if(!(i&0x3f)) 
				if(CheckBadBlk(page)) {
					i += 64;
					continue;
				}
			ReadPage(page+i, dat);
			if(!strncmp(boot_params.start.flags, pBP->start.flags, 10)) {
				ret = 0;
				break;
			}
		}
		if(i>=page_cnt)
			ret = -1;
		DsNandFlash();
	}
		
	
	
	
	if(!ret) {
		ParamItem *pPIS = &pBP->start, *pPID = &boot_params.start;
		
		for(; pPID<=&boot_params.user_params; pPIS++, pPID++)
			if(!strncmp(pPID->flags, pPIS->flags, sizeof(pPID->flags)))
				pPID->val = pPIS->val;
		strncpy(boot_params.string, pPIS->flags, boot_params.user_params.val+1);
		if(boot_params.user_params.val!=strlen(pPID->flags)) {
			memset(boot_params.string, 0, sizeof(boot_params.string));
			boot_params.user_params.val = 0;
		}
		
	} else {
		//printf("Fail to find boot params! Use Default parameters.\n");
		//don't printf anything before serial initialized!
	}
	return ret;
}





//=========================================================================
