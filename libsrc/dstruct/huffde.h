//		HUFFDE.H		Huffman flash-decoding macro
//		Rex E. Bradford

/*
* $Header: r:/prj/lib/src/dstruct/rcs/huffde.h 1.1 1994/08/22 17:13:11 rex Exp $
* $Log: huffde.h $
 * Revision 1.1  1994/08/22  17:13:11  rex
 * Initial revision
 * 
*/

#ifndef __HUFF_H
#include <huff.h>
#endif

//	This header file can be included multiple times by the power programmer.

#undef HUFF_VALUE_MASK
#undef HUFF_VALUE
#undef HUFF_NUMBITS_SHIFT
#undef HUFF_NUMBITS
#undef HuffReadTok
#undef HuffReadFixedTok

//	To use this header file, you must include this .h file AFTER defining
//	the following constants:
//
//	HUFF_BITS_PRI	  - width of primary flash table indices, in bits
//	HUFF_BITS_SEC	  - width of secondary flash table indices, in bits
//	HUFF_ENTRY_WIDTH - width of flash table entries, in bytes (must be 2 to 4)
//
//	HUFF_BITS_PRI may range from 1 to 16
//	HUFF_BITS_SEC may range from 1 to 16
//	HUFF_ENTRY_WIDTH may range from 2 to 4 (save room for 4 numbits bits!)

#define HUFF_VALUE_MASK (0xFFFFFFFFL >> (32 - (HUFF_ENTRY_WIDTH * 8)))
#define HUFF_VALUE(tok) ((tok) & HUFF_VALUE_MASK)

#define HUFF_NUMBITS_SHIFT ((HUFF_ENTRY_WIDTH * 8) - 4)
#define HUFF_NUMBITS(tok) (((tok) >> HUFF_NUMBITS_SHIFT) & 15)

//	Use this macro to reset the read ptr.

#define HuffResetReadPtr(ph,pbits) { \
	(ph)->bits=(pbits);	\
	(ph)->ibit=0;			\
}

//	This macro reads the next variable-length token from a huffman
//	stream.  You must set up the following globals:
//
//	HuffPtr huffPtr	- ptr into huffman stream; init with HuffResetReadPtr()
//	ulong huffTokBuff	- zero before start reading from stream
//	ulong huffVal     - return value (no need to initialize)

#define HuffReadTok() 															\
	{																					\
	ulong *phuffTab;																\
	while (huffPtr.ibit < HUFF_BITS_PRI)									\
		{																				\
		huffTokBuff = (huffTokBuff << 8) | *huffPtr.bits++;			\
		huffPtr.ibit += 8;														\
		}																				\
	huffVal = (huffTokBuff >> (huffPtr.ibit - HUFF_BITS_PRI)) &		\
		(0xFFFF >> (16 - HUFF_BITS_PRI));									\
	phuffTab = (ulong *)((uchar *) gHuffFlashTable +					\
		(huffVal * HUFF_ENTRY_WIDTH));										\
	if (HUFF_NUMBITS(*phuffTab) == 0)										\
		{																				\
		huffPtr.ibit -= HUFF_BITS_PRI;										\
		while (TRUE)																\
			{																			\
			phuffTab = (ulong *)((uchar *) gHuffFlashTable +			\
				(HUFF_VALUE(*phuffTab) * HUFF_ENTRY_WIDTH));				\
			if (huffPtr.ibit < HUFF_BITS_SEC)								\
				{																		\
				huffTokBuff = (huffTokBuff << 8) | *huffPtr.bits++;	\
				huffPtr.ibit += 8;												\
				}																		\
			huffVal = (huffTokBuff >> (huffPtr.ibit - HUFF_BITS_SEC)) &	\
				(0xFFFF >> (16 - HUFF_BITS_SEC));							\
			phuffTab = (ulong *)((uchar *) phuffTab +						\
				(huffVal * HUFF_ENTRY_WIDTH));								\
			if (HUFF_NUMBITS(*phuffTab) != 0)								\
				break;																\
			huffPtr.ibit -= HUFF_BITS_SEC;									\
			}																			\
		}																				\
	huffPtr.ibit -= HUFF_NUMBITS(*phuffTab);								\
	huffVal = HUFF_VALUE(*phuffTab);											\
	}

//	This macro reads the next fixed number of bits from a huffman stream.
//	You must set up the following globals:
//
//	HuffPtr huffPtr	- same as used in HuffReadTok()
//	ulong huffTokBuff	- same as used in HuffReadTok()
//	ulong huffVal		- return value, no need to init
//
//	The number of bits must range from 1 to 16.

#define HuffReadFixedTok(numbits)									\
	if (huffPtr.ibit < numbits)										\
		{																		\
		huffTokBuff = (huffTokBuff << 8) | *huffPtr.bits++;	\
		huffPtr.ibit += 8;												\
		}																		\
	huffVal = (huffTokBuff >> (huffPtr.ibit - numbits)) &		\
		(0xFFFF >> (16 - numbits));									\
	huffPtr.ibit -= numbits;

