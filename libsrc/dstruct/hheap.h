//		HHEAD.H		Homogenous heap
//		Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/dstruct/RCS/hheap.h 1.6 1996/10/21 09:35:12 TOML Exp $
* $Log: hheap.h $
 * Revision 1.6  1996/10/21  09:35:12  TOML
 * msvc
 * 
 * Revision 1.5  1996/01/22  15:39:05  DAVET
 *
 * Added cplusplus stuff.
 *
 * Revision 1.4  1993/05/04  09:30:20  rex
 * Added prototype for HheapFreeAll().
 *
 * Revision 1.3  1993/04/30  11:03:36  rex
 * Modified HheapHead struct in change to new 'free list' allocation technique.
 *
 * Revision 1.2  1993/04/23  18:03:52  rex
 * Added numItems to heap header, and macro to access it
 *
 * Revision 1.1  1993/04/19  09:28:51  rex
 * Initial revision
 *
*/

#ifndef HHEAP_H
#define HHEAP_H

#include <llist.h>

//	Homogenous heap header


#ifdef __cplusplus
extern "C"  {
#endif


typedef struct {
	llist_head chunkList;		// list of heap chunks
	ushort sizeItem;				// size of each item
	ushort numItemsPerChunk;	// # items per heap chunk
	bool shrink;					// if 1, shrink heap when a chunk is empty
	long sizeChunk;				// sizeof of chunk of items
	long numItems;					// number of items in heap
	void *pfree;					// next free item in heap
} HheapHead;

//	Handy macros

#define HheapGetNumItems(phh) ((phh)->numItems)		// don't store to this!

//	Prototypes

void HheapInit(HheapHead *phh, ushort sizeItem, short numItemsPerChunk,
	bool shrink);									// initialize a hheap
void HheapFreeAll(HheapHead *phh);			// free all items, deallocate
void *HheapAlloc(HheapHead *phh);			// allocate item from hheap
void *HheapCalloc(HheapHead *phh);			// allocate item from hheap, clear
void HheapFree(HheapHead *phh, void *p);	// free item from hheap
void HheapAllocChunk(HheapHead *phh);		// allocate a chunk of items

#ifdef __cplusplus
}
#endif

#endif

