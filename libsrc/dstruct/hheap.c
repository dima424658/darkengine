//		HHEAP.C		Homogenous heap module
//		Rex E. Bradford (REX)
//
/*
* $Header: x:/prj/tech/libsrc/dstruct/RCS/hheap.c 1.7 1996/10/21 09:35:09 TOML Exp $
* $Log: hheap.c $
 * Revision 1.7  1996/10/21  09:35:09  TOML
 * msvc
 * 
 * Revision 1.6  1996/10/10  13:16:22  TOML
 * msvc
 *
 * Revision 1.5  1993/09/28  18:38:42  rex
 * Added check for allocation failure
 *
 * Revision 1.4  1993/05/04  09:30:32  rex
 * Added HheapFreeAll() function
 *
 * Revision 1.3  1993/04/30  11:04:07  rex
 * Rewrote using 'free list' allocation technique, which is faster and
 * allows greater than 32 items per chunk.
 *
 * Revision 1.2  1993/04/23  18:04:07  rex
 * Added tracking of numItems in heap, and check for item already freed in
 * HheapFree
 *
 * Revision 1.1  1993/04/19  09:28:56  rex
 * Initial revision
 *
*/

#include <stdlib.h>
#include <string.h>

#include <lg.h>
#include <hheap.h>

//	Heap chunk (data follows immediately after chunk struct)

typedef struct {
	struct _llist;					// ptrs to next & prev chunks
	ushort numItems;				// # items in chunk
} HheapChunk;

//	When a node item is free, it is put into a linked list and
//	looks like this:

typedef struct HheapFreeNode_ {
	struct HheapFreeNode_ *pnext;	// thus minimum item size is 8 bytes
	HheapChunk *pchunk;				// chunk that it's in
} HheapFreeNode;

//	Macro to compute end of a chunk

#define HHEAP_CHUNK_END(phh,pchunk) ((uchar *)(pchunk) + (phh)->sizeChunk)

//	---------------------------------------------------------------
//
//	HheapInit() initializes a homogenous heap.
//
//		phh              = ptr to hheap header
//		sizeItem         = size of each hheap item (if less than 8, padded)
//		numItemsPerChunk = # items per allocation chunk
//		shrink           = if 1, shrink hheap when a chunk is empty

void HheapInit(HheapHead *phh, ushort sizeItem, short numItemsPerChunk,
	bool shrink)
{
//	Init list of chunks and related stuff

	llist_init(&phh->chunkList);
	phh->numItems = 0;
	phh->pfree = NULL;

//	Copy in params, clamping item size at minimum of 8 bytes

	phh->sizeItem = max(sizeItem, sizeof(HheapFreeNode));
	phh->numItemsPerChunk = numItemsPerChunk;
	phh->shrink = shrink;

//	Compute chunk size

	phh->sizeChunk = sizeof(HheapChunk) +
		((long) phh->sizeItem * (long) phh->numItemsPerChunk);
}

//	----------------------------------------------------------------
//
//	HheapFreeAll() frees all items in hheap and deallocates chunks.
//		Hheap may be used again without reinitializing.
//
//		phh = ptr to heap header

void HheapFreeAll(HheapHead *phh)
{
	HheapChunk *pchunk;
	HheapChunk *pchunknext;

//	Traverse heap chunk list, freeing all chunks

	pchunk = llist_head(&phh->chunkList);
	while (pchunk != llist_end(&phh->chunkList))
		{
		pchunknext = llist_next(pchunk);
		Free(pchunk);
		pchunk = pchunknext;
		}

//	Re-init heap header, in case heap used again

	llist_init(&phh->chunkList);
	phh->numItems = 0;
	phh->pfree = NULL;
}

//	----------------------------------------------------------------
//
//	HheapAlloc() allocates an item from a hheap.
//
//		phh = ptr to hheap header

void *HheapAlloc(HheapHead *phh)
{
	void *p;

//	If free ptr NULL, grow chunk list

	if (phh->pfree == NULL)
		{
		HheapAllocChunk(phh);
		if (phh->pfree == NULL)
			return(NULL);
		}

//	Grab free ptr

	p = phh->pfree;

//	Bump # items in the chunk it came from

	((HheapFreeNode *) p)->pchunk->numItems++;

//	Advance free ptr

	phh->pfree = ((HheapFreeNode *) phh->pfree)->pnext;

//	Bump # items in entire heap, and return ptr

	phh->numItems++;
	return(p);
}

//	---------------------------------------------------------------
//
//	HheapCalloc() allocates an item from a hheap and clears it.
//
//		phh = ptr to hheap header

void *HheapCalloc(HheapHead *phh)
{
	void *p;

	p = HheapAlloc(phh);
	if (p)
		memset(p, 0, phh->sizeItem);
	return(p);
}

//	---------------------------------------------------------------
//
//	HheapFree() frees item from a homogenous heap.
//
//		phh = ptr to hheap header
//		p   = ptr to allocated item

void HheapFree(HheapHead *phh, void *p)
{
	HheapChunk *pchunk;
	HheapFreeNode *pfnPrev;
	HheapFreeNode *pfnNext;

//	Search chunk list to find item

	forallinlist(HheapChunk, &phh->chunkList, pchunk)
	{
		if ((p >= (void *)pchunk) && (p < (void *)HHEAP_CHUNK_END(phh, pchunk)))
		{

//	Found, put at head of free list

			((HheapFreeNode *) p)->pnext = phh->pfree;
			((HheapFreeNode *) p)->pchunk = pchunk;
			phh->pfree = p;

//	Decrement # items in chunk, test if should free chunk

			pchunk->numItems--;

			if ((pchunk->numItems == 0) && phh->shrink)
				{

//	Freeing chunks is a pain.  First, remove from the chunk list.

				llist_remove(pchunk);

//	We know, since we just freed it, that the 1st free node is in the
//	current chunk.  Now let's find the first free node that isn't.
//	Set the free ptr to the first node to it (or NULL if there isn't one).

				pfnPrev = phh->pfree;
				while (pfnPrev && (pfnPrev->pchunk == pchunk))
					pfnPrev = pfnPrev->pnext;
				phh->pfree = pfnPrev;

//	If there is a free node which is not in this chunk, continue from it,
//	unlinking those that are in this chunk.  pfnPrev always points to
//	the last free node which is not in this chunk.

				while (pfnPrev)
					{
					pfnNext = pfnPrev->pnext;
					while (pfnNext && (pfnNext->pchunk == pchunk))
						pfnNext = pfnNext->pnext;
					pfnPrev->pnext = pfnNext;
					pfnPrev = pfnNext;
					}

//	Finally, we can free the stupid chunk

				Free(pchunk);
				}

//	Decrement # items in entire heap and return

			phh->numItems--;
			return;
			}
		}

//	If search of entire list failed

	Warning(("HheapFree: can't find: $%x in heap $%x\n", p, phh));
}

//	---------------------------------------------------------------
//
//	HheapAllocChunk() allocates a new chunk & adds to free list.
//
//		phh = ptr to heap header.

void HheapAllocChunk(HheapHead *phh)
{
	HheapChunk *pchunk;
	HheapFreeNode *pfnList;
	HheapFreeNode *pfn;
	int i;

//	Allocate chunk, return if failed

	pchunk = Malloc(phh->sizeChunk);
	if (pchunk == NULL)
		{
		Warning(("HheapAllocChunk: unable to allocate %d bytes\n",
			phh->sizeChunk));
		return;
		}

//	Add to chunk list and init # items

	llist_add_head(&phh->chunkList, pchunk);
	pchunk->numItems = 0;

//	Fill it with free ptrs, at head of free list

	pfn = pfnList = (HheapFreeNode *)(pchunk + 1);
	for (i = 0; i < (phh->numItemsPerChunk - 1); i++)
		{
		pfn->pnext = (HheapFreeNode *) (((char *) pfn) + phh->sizeItem);
		pfn->pchunk = pchunk;
		pfn = pfn->pnext;
		}
	pfn->pnext = phh->pfree;
	pfn->pchunk = pchunk;
	phh->pfree = pfnList;
}

