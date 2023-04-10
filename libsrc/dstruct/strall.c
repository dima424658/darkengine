//		STRALL.C		String allocator
//		Rex E. Bradford (REX)
//
//		This module implements a string storage manager, based on
//		the homogenous heap module.  An array of homogenous heaps
//		for various string sizes is kept.  If a string is larger
//		than the item size of the largest string heap, Malloc() and
//		Free() are used for it.  Otherwise, the appropriate homogenous
//		heap is used for the string's storage.
//
//		Note that, on average, 1/4 of the space allocated for a string
//		is excess space and is wasted.  Hey, life's rough.

/*
* $Header: r:/prj/lib/src/dstruct/rcs/strall.c 1.3 1993/04/30 11:04:51 rex Exp $
* $Log: strall.c $
 * Revision 1.3  1993/04/30  11:04:51  rex
 * Made each of 5 string heaps use a different # of items per chunk, so that
 * they are roughly balanced in total bytes per string chunk (.5K or 1K each)
 * 
 * Revision 1.2  1993/04/27  09:53:02  rex
 * Fixed bug, StringAlloc() didn't return value for large strings!
 * 
 * Revision 1.1  1993/04/19  09:29:04  rex
 * Initial revision
 * 
*/

#include <string.h>

#include "lg.h"
#include "hheap.h"
#include "strall.h"

//	This module maintains 5 homogenous heaps, for string sizes
//	including of '\0' of up to 8, 16, 32, 64, and 128 bytes.  Larger
//	strings are handled with Malloc() and Free().

#define NUM_STRING_HEAP_SIZES 5
short strallSizeStrings[NUM_STRING_HEAP_SIZES] = {8,16,32,64,128};
short strallNumItems[NUM_STRING_HEAP_SIZES] = {64,32,32,16,8};
HheapHead hheapStrings[NUM_STRING_HEAP_SIZES];
HheapHead *pheapHeadStrings[NUM_STRING_HEAP_SIZES];

//	---------------------------------------------------------
//		STRING STORAGE ALLOCATOR & DEALLOCATOR
//	---------------------------------------------------------
//
//	StringAlloc() allocates a string according to its size.
//
//		s = ptr to string to be copied into string storage.
//
//	Returns: ptr to allocated string in string block (or Malloc()'ed)
//		if larger than largest slot size.

char *StringAlloc(char *s)
{
static bool heapsNotInitialized = TRUE;
	HheapHead *phh;
	short len;
	int itable;
	char *p;

//	Make sure our heaps are initialized

	if (heapsNotInitialized)
		{
		phh = &hheapStrings[0];
		for (itable = 0; itable < NUM_STRING_HEAP_SIZES; itable++, phh++)
			{
			pheapHeadStrings[itable] = phh;
			HheapInit(phh, strallSizeStrings[itable], strallNumItems[itable],
				TRUE);
			}
		heapsNotInitialized = FALSE;
		}

//	Calculate length of string, find proper table, alloc string

	len = strlen(s) + 1;
	for (itable = 0; itable < NUM_STRING_HEAP_SIZES; itable++)
		{
		if (len <= strallSizeStrings[itable])
			{
			p = HheapAlloc(pheapHeadStrings[itable]);
			memcpy(p, s, len);
			return(p);
			}
		}

//	If too big for biggest table, just Malloc() it & copy string in

	p = Malloc(len);
	memcpy(p, s, len);
	return(p);
}

//	--------------------------------------------------------------
//
//	StringFree() frees a string.
//
//		s = ptr to string to be freed, gotten from StringAlloc()

void StringFree(char *s)
{
	short len;
	int itable;

//	Calculate length of string, find proper heap table, free string

	len = strlen(s) + 1;
	for (itable = 0; itable < NUM_STRING_HEAP_SIZES; itable++)
		{
		if (len <= strallSizeStrings[itable])
			{
			HheapFree(pheapHeadStrings[itable], s);
			return;
			}
		}

//	If larger than largest table size, just use Free()

	Free(s);
}

