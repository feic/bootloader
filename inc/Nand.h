#ifndef __NAND_H
#define __NAND_H

////////////////////////////// 8-bit ////////////////////////////////
// Main function


//int NandSelPart(char *info);
static U32 StartPage, BlockCnt;

struct Partition{
	U32 offset;
	U32 size;
	char *name;
};





#endif /*__NAND_H*/

