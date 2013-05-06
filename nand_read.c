/* File: 		nand_read.c
 *  Fuction:	构建最小的bootloader引导linux内核
 *		small Bios 精简了vivi  编译后小于3k
 *  By:	LiuNing		Date:2011-9-28
 *		注意此版本屏蔽了坏块判断
 */

#include "smdk2440.h"

#define __REGb(x) 	(*(volatile unsigned char *)(x))
#define __REGw(x) 	(*(volatile unsigned short *)(x))
#define __REGi(x) 	(*(volatile unsigned int *)(x))
#define NF_BASE 	0x4e000000

#define NFCONF		__REGi(NF_BASE + 0x0)	//配置寄存器
#define NFCONT		__REGi(NF_BASE + 0x4)	//控制寄存器
#define NFCMD		__REGb(NF_BASE + 0x8)	//命令寄存器
#define NFADDR		__REGb(NF_BASE + 0xc)	//地址寄存器
#define NFDATA		__REGb(NF_BASE + 0x10)	//数据寄存器
#define NFDATA16 	__REGw(NF_BASE + 0x10)
#define NFSTAT 		__REGb(NF_BASE + 0x20)	//操作状态
#define NFSTAT_BUSY 1
#define NAND_CMD_READSTART  0X30
#define nand_select() 		(NFCONT &= ~(1 << 1))
#define nand_deselect() 	(NFCONT |= (1 << 1))
#define nand_clear_RnB() 	(NFSTAT |= (1 << 2))

static inline void nand_wait(void)
{
  int i;
  while (!(NFSTAT & NFSTAT_BUSY))
  {
  	for (i=0; i<10; i++);
  }
}


#define NAND_CMD_READ0 		0
#define NAND_CMD_READ1 		1
#define NAND_CMD_RNDOUT 	5
#define NAND_5_ADDR_CYCLE
#define NAND_PAGE_SIZE 		2048	/* K9F2G08U0A是大页nand 2k字节每页*/
#define BAD_BLOCK_OFFSET 	NAND_PAGE_SIZE
#define NAND_BLOCK_MASK 	(NAND_PAGE_SIZE - 1)
#define NAND_BLOCK_SIZE 	(NAND_PAGE_SIZE * 64)	/* 64页组成一个block */
#if 0
static int isBadBlock(unsigned long i)
{
  unsigned char data;
  unsigned long page_num;
  
  nand_clear_RnB();
  page_num = i >> 11; /* addr / 2048 */
  NFCMD  = NAND_CMD_READ0;
  NFADDR = BAD_BLOCK_OFFSET & 0xff;
  NFADDR = (BAD_BLOCK_OFFSET >> 8) & 0xff;
  NFADDR = page_num & 0xff;
  NFADDR = (page_num >> 8) & 0xff;
  NFADDR = (page_num >> 16) & 0xff;
  NFCMD  = NAND_CMD_READSTART;
  nand_wait();
  data = (NFDATA & 0xff);
  if (data != 0xff)
  	return 1;
  return 0;
}
#endif
static int nand_read_page_ll(unsigned char *buf, unsigned long addr)
{
  unsigned int i, page_num;
  
  nand_clear_RnB();
  NFCMD = NAND_CMD_READ0;
  page_num = addr >> 11;
  NFADDR = 0;
  NFADDR = 0;
  NFADDR = page_num & 0xff;
  NFADDR = (page_num >> 8) & 0xff;
  NFADDR = (page_num >> 16) & 0xff;
  NFCMD  = NAND_CMD_READSTART;
  nand_wait();
  for (i = 0; i < NAND_PAGE_SIZE; i++)
  {
      *buf = (NFDATA & 0xff);
      buf++;
  }
  return NAND_PAGE_SIZE;
}

extern int nand_read_ll(unsigned char *buf, unsigned long start_addr, int size)
{
    int i, j;
    if ((start_addr & NAND_BLOCK_MASK) || (size & NAND_BLOCK_MASK))
    {
        return -1; 
    }
    nand_select();
    nand_clear_RnB();
    for (i=0; i<10; i++);
	
    for (i=start_addr; i < (start_addr + size);)
    {
        j = nand_read_page_ll(buf, i);
        i += j;
        buf += j;
    }
    nand_deselect();
    return 0;
}
