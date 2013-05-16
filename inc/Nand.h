#ifndef __NAND_H
#define __NAND_H


static U32 StartPage, BlockCnt;
void InitNandFlash(void);
void LoadRun(void);
void ReadPage(U32 addr, U8 *buf);

struct Partition{
	U32 offset;
	U32 size;
	char *name;
};





#endif /*__NAND_H*/

