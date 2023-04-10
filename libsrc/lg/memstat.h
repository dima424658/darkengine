/*
 * $Source: x:/prj/tech/libsrc/lg/RCS/memstat.h $
 * $Revision: 1.9 $
 * $Author: DAVET $
 * $Date: 1996/01/24 10:49:08 $
 *
 * $Header: x:/prj/tech/libsrc/lg/RCS/memstat.h 1.9 1996/01/24 10:49:08 DAVET Exp $
 */

#ifndef __MEMSTAT_H
#define __MEMSTAT_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

typedef struct {
	ulong num;
	ulong sizeTot;
	ulong sizeMax;
	ulong sizeAvg;
} MemCatInfo;

typedef struct {
	long largestFreeBlock;			// largest available free block in bytes
	long maxUnlockedPageAlloc;		// maximum unlocked page allocation in pages
	long maxLockedPageAlloc;		// maximum locked page allocation in pages
	long linearAddrSpace;			// linear address space size in pages
	long totUnlockedPages;			// total number of unlocked pages
	long totFreePages;				// total number of free pages
	long totPhysPages;				// total number of physical pages
	long freeLinearSpace;			// free linear address space in pages
	long sizePartition;				// size of paging file/partition in pages
	char reserved[12];
} MemDpmiInfo;

typedef struct {
	MemCatInfo all;			// info on all blocks in heap
	MemCatInfo used;			// info on used blocks in heap
	MemCatInfo free;			// info on free blocks in heap
	MemDpmiInfo info;			// info from DPMI call 0500
} MemStat;

// This routine attempts to allocate all memory available to
// the system, though it uses realloc, which may have a bug
// with some compilers.

int MallocableTotal(int lowMemReserved,uchar val);

//	This function collects stats on the current heap.  Note that
//	malloc() will grow the heap automatically, so the stats on
//	free memory blocks do not reflect whether there is enough memory
//	to satisfy an allocation request.  This information is unavailable.

int MemStats(MemStat *pms);	// walk heap, fill in stats
										// returns 0 if heap ok, -1 if bad heap

// flags for memstat. default is GETSTAT only
#define MS_VERBALIZE 0x1
#define MS_MEMORIZE  0x2
#define MS_SETBREAK  0x4
#define MS_GETSTAT   0x8

extern int   MemStatsFlags;

// possible flag defines for the heapwalkers build
// do we have ability to check for heap corruption/watchpointing
#ifndef WIN32
#define TARGET_BADNESS 
#endif
// verbalize mputs the status of heap to screen in a readable way
#ifdef DBG_ON
#define VERBALIZE
#endif
// memorize puts current loc and stuff in monomemory in hex form
//#define MEMORIZE 
// are we set up to gather various stats
#define GETSTAT

#ifdef TARGET_BADNESS
// final 0 must stay here
extern short MemStats_targidx[8];  // ok, initially, no values
extern short (*MemStats_watchset)(short watchid, void __far *watchloc);
#endif

#ifdef __cplusplus
}
#endif  // cplusplus

#endif /* !__MEMSTAT_H */
