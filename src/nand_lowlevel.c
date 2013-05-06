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

#define	puts	Uart_Printf
#define	printf	Uart_Printf
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
U32 WritePage(U32 addr, U8 *buf)
{
	U32 i, mecc;
	U8 stat, tmp[7];
	
	
	NFChipEn();
	WrNFCmd(PROGCMD0);
	WrNFAddr(0);
	WrNFAddr(0);
	WrNFAddr(addr);
	WrNFAddr(addr>>8);
	if(NandAddr)
		WrNFAddr(addr>>16);
	InitEcc();	//reset mecc and secc
	MEccUnlock();
	for(i=0; i<2048; i++)
		WrNFDat(buf[i]);
	MEccLock();
	
	mecc = RdNFMEcc();
		
	tmp[0] = mecc&0xff;
    tmp[1] = (mecc>>8)&0xff;
    tmp[2] = (mecc>>16)&0xff;
    tmp[3] = (mecc>>24)&0xff;
    tmp[5] = 0xff;	//mark good block

	 if(fs_yaffs==1){
	 	WrNFDat(0xff);
	 	for(i=2049;i<2112;i++)
	 	{
	 		WrNFDat(buf[i]);
	 	}
	 }else{
	 	
		WrNFDat(0xff);//2048，坏块标志
	    SEccUnlock();
		WrNFDat(tmp[0]);//ECC校验码
		WrNFDat(tmp[1]);
		WrNFDat(tmp[2]);
		WrNFDat(tmp[3]);
		SEccLock();
	}

	WrNFCmd(PROGCMD1);
	stat = WaitNFBusy();
	NFChipDs();
	

	if(stat)
		printf("Write nand flash 0x%x fail\n", addr);
	else {	
	
#if 1
		U8 RdDat[2048];
		
		ReadPage(addr, RdDat);		
		for(i=0; i<2048; i++)
			if(RdDat[i]!=buf[i]) {
				printf("Check data at page 0x%x, offset 0x%x fail\n", addr, i);
				stat = 1;
				break;
			}
#endif			
	}
	

	return stat;	
}

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

void WrFileToNF(void)
{
	int i ,size, skip_blks;
	U32 ram_addr;
	struct Partition *nf_part;

	fs_yaffs=0;
	nf_part = NandSelPart("write");
	if(!nf_part)
		return;	
	StartPage = nf_part->offset>>11;
	BlockCnt  = nf_part->size>>17;

//	printf("\n    飞凌嵌入式      www.witech.com.cn\n");
	if(!strcmp(nf_part->name,"wince"))//strcmp 串比较 不管大小写
	{
		puts("\nThe 'wince' partition is reserved for wince. please use eboot\n");
		return;
	}	
	printf("\nNow download and write nand flash part [ %s ] \n",nf_part->name);
	printf("press [USB Port-->transmit] to choose the file \n");

	WaitDownload();
	
	
	if(downloadFileSize>nf_part->size) {
		puts("Download file size is more large than selected partition size!!!\n");
		return;
	}
	
	//if(!strcmp(nf_part->name,"wince"))//strcmp 串比较 不管大小写
		//wince_rewrite();
	if(!strcmp(nf_part->name,"fs_yaffs"))
		fs_yaffs=2;
	printf("Now write nand flash page 0x%x from ram address 0x%x, filesize = %d\n", StartPage, downloadAddress, downloadFileSize);

	skip_blks = 0;
	ram_addr = downloadAddress;
	size = downloadFileSize-10;
	for(i=0; size>0; )	{	
		if(!(i&0x3f)) {
			
			if(EraseBlock(i+StartPage)) {
				/*标记坏块并跳过改块*/
				nf_part->size -= 64<<11;	//partition available size - 1 block size
				if(downloadFileSize>nf_part->size) {
					puts("Program nand flash fail\n");
					return;
				}
				MarkBadBlk(i+StartPage);
				//printf("bpage %x\n",i+StartPage);
				add_bpage(i+StartPage);
				skip_blks++;				
				i += 64;				
				continue;
			}
		}
		if(fs_yaffs==2){
			//i+=64;
			fs_yaffs=1;
			continue;
		}
		if(WritePage(i+StartPage, (U8 *)ram_addr)) {
			ram_addr -= (i&0x3f)<<11;//i pages size
			size += (i&0x3f)<<11;
			if(fs_yaffs==1)
			{
				ram_addr -=(i&0x3f)*64;
				size +=(i&0x3f)*64;			
			}
			i &= ~0x3f;				
			nf_part->size -= 64<<11;	//partition available size - 1 block size
			if(downloadFileSize>nf_part->size) {
				puts("Program nand flash fail\n");
				return;
			}			
			MarkBadBlk(i+StartPage);
			//printf("bpage %x\n",i+StartPage);
			add_bpage(i+StartPage);
			skip_blks++;			
			i += 64;			
			continue;
		}
		ram_addr += 2048;
		size -= 2048;
		if(fs_yaffs==1){
			ram_addr +=64;
			size -=64;
		}
		i++;
		
	}
	fs_yaffs=0;
	puts("Program nand flash partition success\n");
	if(skip_blks)
	{
		printf("Skiped %d bad block(s)\n", skip_blks);
		save_params();
		//cpy_bpage();
	}
}


/************************************************************/
int have_nandflash;
void InitNandFlash(int info)
{	
	U32 i;
	
	InitNandCfg();
	i = ReadChipId();
	if(info)
		printf("NAND ID is 0x%04x \n", i);
		
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

void NandErase(void)
{
	int i, err = 0;

	struct Partition *nf_part; 
	
	InitNandFlash(1);
	if(!have_nandflash)
		return;
	
	nf_part = NandSelPart("erase");
	if(!nf_part)
		return;	
	StartPage = nf_part->offset>>11;
	BlockCnt  = nf_part->size>>17;

	
	printf("Are you sure to erase nand flash from page 0x%x, block count 0x%x ? [y/n]\n", StartPage, BlockCnt);
	while(1) {
		char c;
		
		c = getch();
		if((c=='y')||(c=='Y'))
			break;
		if((c=='n')||(c=='N'))
			return;
	}	
	
	for(i=0; BlockCnt; BlockCnt--, i+=64) {
		if(EraseBlock(i+StartPage)) {
			err ++;
			add_bpage(i+StartPage);
			//puts("Press any key to continue...\n");
			//getch();
		}
		else if(!strcmp(nf_part->name,"fs_yaffs"))
			MarkGoodBlk(i+StartPage);
	}	

	DsNandFlash();		//disable nand flash interface
	puts("Erase Nand partition completed ");
	
	if(err)
	{
		save_params();
		//cpy_bpage();
		printf("with %d bad block(s)\n", err);
	}
	else
		puts("success\n");
}

void NandWrite(void)
{
	InitNandFlash(1);
	if(!have_nandflash)
		return;
	WrFileToNF();
	DsNandFlash();		//disable nand flash interface
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


//flash的写操作必须从一块的第一页开始写
int save_params(void)
{
	
	U8 dat[0x800];
	int ret = 0;
	struct Partition *nf_part;
	
	if(rBWSCON&0x6){//nor
		//memcpy(dat, (void *)0x08000000, NOR_PARAMS_OFFSET);
		memcpy(dat, &boot_params, sizeof(boot_params));
		ret = ProgNorFlash(NOR_PARAMS_OFFSET, (U32)dat, sizeof(dat));	//28F128
	}
	else{
	
		U32 i,page, page_cnt;
		nf_part = NandSelPart_2("bootParam");	
		if(!nf_part)
			return -1;	
		page = nf_part->offset>>11;
		page_cnt= nf_part->size>>11;
		InitNandFlash(0);
	
		memset(dat, 0, 0x800/*sizeof(boot_params)*/);
		memcpy(dat, &boot_params, sizeof(boot_params));

		for(i=0; i<page_cnt; i++) {
			if(!(i&0x3f))//擦除flash，并检测是否是坏块
				if(EraseBlock(page)){
					i+=64;
					continue;
				}
			if(!WritePage(page+i, dat))
				break;
		}
		if(i>=page_cnt)
			ret = -1;
		
		
		DsNandFlash();
	}
	printf("Save boot params %s.\n", ret?"fail":"success");
	return ret;
}

int set_params(void)
{
	int i, key, chg=0;
	ParamItem *pPID;
	printf(" +------------------------------------------------------------+\n");
	printf(" |                     Config parameters                      |\n");
	printf(" +------------------------------------------------------------+\n");
	//printf("\n#####Config parameters#####\n");
	
	do {
		pPID = &boot_params.start;
		for(i=0; pPID<=&boot_params.user_params; pPID++, i++)
			printf("  [%2d] %-10s : 0x%08x (%d)\n",
						i, pPID->flags, pPID->val, pPID->val);
		printf("  [%2d] : Exit\n", i);
		if(boot_params.user_params.val)
			printf("User parameters is : \"%s\"\n", boot_params.string);
		
		printf("\nplease select item:");
		key = Uart_GetIntNum();
		if(key>=0&&key<i) {
			chg = 1;
			printf("please enter value:");
			i = key;
			if((&boot_params.start + i)==&boot_params.user_params) {
				//确保输入的字节数不超过127!
				char cmd[128];
				memset(cmd, 0, sizeof(cmd));
				Uart_GetString(cmd);
				strncpy(boot_params.string, cmd, strlen(cmd)+1);
				boot_params.user_params.val = strlen(cmd);
			} else {
				key = Uart_GetIntNum();
				(&boot_params.start + i)->val = key;
			}
		} else
			break;
	} while(1);
	
	if(chg) {
		printf("Do you want to save parameters? press y or Y for save.\n");
		key = getch();
		if(key=='y'||key=='Y')
			save_params();
	}
	
	return 0;
}
//=========================================================================
