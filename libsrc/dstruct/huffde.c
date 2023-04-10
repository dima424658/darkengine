//		HUFFDE.C		Huffman decompression routines
//		Rex E. Bradford

/*
* $Header: r:/prj/lib/src/dstruct/rcs/huffde.c 1.1 1994/08/22 17:13:01 rex Exp $
* $Log: huffde.c $
 * Revision 1.1  1994/08/22  17:13:01  rex
 * Initial revision
 * 
*/

#include <string.h>

#include <dbg.h>
#include <huff.h>

//	----------------------------------------------------------------
//		DECOMPRESS HUFFMAN MULTI-TABLES
//	----------------------------------------------------------------
//
//	HuffExpandFlashTables() compresses huffman flash-decoder tables.

void HuffExpandFlashTables(uchar *pFlashTab, ulong lenTab, ulong *pc,
	int tokSize)
{
	uchar *pft;
	ulong token,runCount;
	int runShift;

//	Setup

	pft = pFlashTab;
	runShift = tokSize * 8;

//	While still inside dest table, keep going

	while (pft < (pFlashTab + lenTab))
		{

//	Get next token, extract run count

		token = *pc++;
		runCount = token >> runShift;

//	Copy that many times into dest

		while (runCount-- != 0)
			{
			memcpy(pft, &token, tokSize);
			pft += tokSize;
			}
		}
}


