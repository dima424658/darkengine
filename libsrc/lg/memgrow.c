//		MemGrow.C - Routine to grow heap like mad
//		Rex E. Bradford

/*
 * $Source: x:/prj/tech/libsrc/lg/RCS/memgrow.c $
 * $Revision: 1.1 $
 * $Author: rex $
 * $Date: 1993/09/30 18:37:11 $
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>

char heapLockCount = 0;								// heap locked if > 0

void *__ExpandDGROUP(int need);					// we're patching this sucker
uchar shutOffExpand[3] = {0x31,0xC0,0xC3};	// XOR EAX,EAX and RET and 00
uchar oldExpandCode[3];								// holds old 1st 4 bytes in code

//	-------------------------------------------------------------
//
//	MemGrowHeap() grows heap by amt supplied.  Since the WATCOM
//	heap manager never returns memory to DPMI, we have to be careful
//	not to grab too much.  The recommended method is to determine
//	on startup how much we would like, either apriori or by examining
//	the amount of memory DPMI has available to us and calculating some
//	percentage of that, clamped to a reasonably generous maximum.
//	Then that memory is captured and the heap is locked from further
//	growth.  For instance:
//
//		MemStats(&memInfo);					// get stats on memory situation
//		availK = memInfo.info.totFreePages * 4;	// number of K available
//		wantK = (availK < 4000) ? 3000 : ((availK * 3) / 4); // computed desired
//		wantK = min(wantK,12000);			// clamp to max (12 Meg in this case)
//		gotK = MemGrowHeap(wantK);			// ok, grow to that amt
//		if (gotK != wantK)
//			Warning(("Hey! I was sure I could get %dK!!\n", wantK));
//		MemLockHeap();							// set lock flag & disable grow routine
//
//	wantK = memory desired, in K
//
//	Returns: memory gotten

int MemGrowHeap(int wantK)
{
	int i;
	int gotK;
	char *ptrs[16];

//	Try to get blocks from possible 32 Meg block down to 1K block

	for (i = 15; i >= 0; i--)
		{
		if (wantK & (1 << i))
			ptrs[i] = malloc(1 << (i + 10));
		else
			ptrs[i] = NULL;
		}

//	Free memory and count it

	gotK = 0;
	for (i = 15; i >= 0; i--)
		{
		if (ptrs[i])
			{
			free(ptrs[i]);
			gotK += (1 << i);
			}
		}

//	Return amt gotten

	return(gotK);
}

//	----------------------------------------------------------
//
//	MemLockHeap() locks the heap.

void MemLockHeap()
{
	if (++heapLockCount == 1)
		{
		memcpy(oldExpandCode, (uchar *) __ExpandDGROUP, sizeof(oldExpandCode));
		memcpy((uchar *) __ExpandDGROUP, shutOffExpand, sizeof(shutOffExpand));
		}
}

//	-----------------------------------------------------------
//
//	MemUnlockHeap() unlocks the heap.

void MemUnlockHeap()
{
	if (--heapLockCount == 0)
		memcpy((uchar *) __ExpandDGROUP, oldExpandCode, sizeof(oldExpandCode));
}

