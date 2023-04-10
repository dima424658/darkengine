/*
 * $Source: x:/prj/tech/libsrc/lg/RCS/memstat.c $
 * $Revision: 1.13 $
 * $Author: TOML $
 * $Date: 1996/10/10 12:23:20 $
 *
 * $Header: x:/prj/tech/libsrc/lg/RCS/memstat.c 1.13 1996/10/10 12:23:20 TOML Exp $
 */

#ifdef _WIN32
#include <windows.h>
#else
#include <dpmi.h>
#endif

#include <malloc.h>
#include <string.h>
#include <dos.h>

#include <lg.h>
#include <memstat.h>
#include <mprintf.h>
#include <_lg.h>

// our secret heapwalker...
extern int _oheapwalk( struct _heapinfo *__entry );

#ifdef TARGET_BADNESS
// final 0 must stay here
short MemStats_targidx[8]={0,0,0,0,0,0,0,0};  // ok, initially, no values
short MemStats_watchid[8]={-1,-1,-1,-1,-1,-1,-1,-1};  // ok, initially, no values
short (*MemStats_watchset)(short watchid, void __far *watchloc) = NULL;
#endif

int   MemStatsFlags = MS_GETSTAT;

#define MSG_BASE (0x0b0000+320-20)     // right of second line...

#define MAX_HEAP_ITEMS   0x20000

//	--------------------------------------------------------------
//
//	MemStats() fills in structure with memory stats
//
//		pms = ptr to MemStat struct
//
//	Returns: 0 if heap ok, -1 if heap is bad

int MemStats(MemStat *pms)
{
#ifdef _WIN32
    return 0;
#else
	struct _heapinfo hinfo;
	int hstat;
#ifdef VERBALIZE
   char i;
#endif

//	Init stat struct

	memset(pms, 0, sizeof(MemStat));

//	Get DPMI stats

   dpmi_free_mem_info((dpmis_mem *)&pms->info);

//	Walk heap, collecting info

	Spew(DSRC_LG_Memwalk, ("Heapwalk:\n"));

	hinfo._pentry = NULL;

#ifdef VERBALIZE
   if (MemStatsFlags&MS_VERBALIZE)
   {
	   mput('S',69,1);
	   for (i=1; i <= 6; i++)
   	   mput(' ',69+i,1);
   }
#endif
#ifdef MEMORIZE
   if (MemStatsFlags&MS_MEMORIZE)
   {
	   *(ulong *)(MSG_BASE+0)=0;
	   *(ulong *)(MSG_BASE+4)=0;
	   *(ulong *)(MSG_BASE+8)=0;
	   *(uchar *)(MSG_BASE+9)='S';
   }
#endif

	while (TRUE)
   {
#ifdef TARGET_BADNESS
      if (MemStatsFlags&MS_SETBREAK)
      {
         int i;
         for (i=0; MemStats_targidx[i]!=0; i++)
	         if (pms->all.num==MemStats_targidx[i])
			   {
		         Warning(("watch %x:%x sz %x for %o\n",FP_SEG(hinfo._pentry),FP_OFF(hinfo._pentry),*(ulong __far *)(hinfo._pentry),pms->all.num));
               if (MemStats_watchset!=NULL)
                  MemStats_watchid[i]=(*MemStats_watchset)(MemStats_watchid[i],hinfo._pentry);
            }
       }
#endif

#ifdef VERBALIZE
      if (MemStatsFlags&MS_VERBALIZE)
	      mput('H',68,1);
#endif
#ifdef MEMORIZE
      if (MemStatsFlags&MS_MEMORIZE)
         *(uchar *)(MSG_BASE+8)='H';
#endif

   	hstat = _oheapwalk(&hinfo);

#ifdef VERBALIZE
      if (MemStatsFlags&MS_VERBALIZE)
	   {
	      mput(' ',68,1);
	      mput('0'+((pms->all.num>>12)&0x7) ,70,1);
	      mput('0'+((pms->all.num>>9)&0x7) ,71,1);
	      mput('0'+((pms->all.num>>6)&0x7) ,72,1);
	      mput('0'+((pms->all.num>>3)&0x7) ,73,1);
	      mput('0'+((pms->all.num)&0x7) ,74,1);
      }
#endif

#ifdef MEMORIZE
      if (MemStatsFlags&MS_MEMORIZE)
      {
	      *(uchar *)(MSG_BASE+8)=' ';
	      *(ulong *)(MSG_BASE+0)=FP_OFF(hinfo._pentry);
	      *(ulong *)(MSG_BASE+4)=pms->all.num;
      }
#endif

		Spew(DSRC_LG_Memwalk, ("%c pentry: 0x%x:0x%x, size: 0x%x, use: %d (stat = %d) idx: %d\n",
			(hinfo._useflag==_USEDENTRY?'*':'.'), FP_SEG(hinfo._pentry), FP_OFF(hinfo._pentry), hinfo._size, hinfo._useflag, hstat, pms->all.num));

		if (hstat != _HEAPOK)
			break;

      if (++pms->all.num > MAX_HEAP_ITEMS)
      {
         Warning(("*** HEY, THE HEAP IS REALLY REALLY BIG ***\n"));
         break;
      }

#ifdef GETSTAT
      if (MemStatsFlags&MS_GETSTAT)
      {
			pms->all.sizeTot += hinfo._size;
			if (hinfo._size > pms->all.sizeMax)
				pms->all.sizeMax = hinfo._size;

			if (hinfo._useflag == _USEDENTRY)
			{
				pms->used.num++;
				pms->used.sizeTot += hinfo._size;
				if (hinfo._size > pms->used.sizeMax)
					pms->used.sizeMax= hinfo._size;
			}
			else
			{
				pms->free.num++;
				pms->free.sizeTot += hinfo._size;
				if (hinfo._size > pms->free.sizeMax)
				pms->free.sizeMax = hinfo._size;
			}
      }
#endif
   }
#ifdef VERBALIZE
   if (MemStatsFlags&MS_VERBALIZE)
   {
	   mput(' ',69,1);
	   mput('F',75,1);
   }
#endif

#ifdef TARGET_BADNESS
   if (MemStatsFlags&MS_SETBREAK)
      MemStatsFlags&=~MS_SETBREAK;
#endif

#ifdef MEMORIZE
   if (MemStatsFlags&MS_MEMORIZE)
	   *(uchar *)(MSG_BASE+0x9)='F';
#endif

//	Compute stats

#ifdef GETSTAT
	pms->all.sizeAvg = pms->all.num ? (pms->all.sizeTot / pms->all.num) : 0;
	pms->used.sizeAvg = pms->used.num ? (pms->used.sizeTot / pms->used.num) : 0;
	pms->free.sizeAvg = pms->free.num ? (pms->free.sizeTot / pms->free.num) : 0;
#endif

//	Return 0 or error (-1)

	if (hstat != _HEAPEND)
		{
		Warning(("MemStats: heap bad %x hid %x, at 0x%x:0x%x size %x\n",hstat,pms->all.num,FP_SEG(hinfo._pentry),FP_OFF(hinfo._pentry),*(ulong __far *)(hinfo._pentry)));
		return(-1);
		}
	return(0);
#endif
}

// Routine to determine the largest available block of at
// most uVal size.  Return the size and set the ptr to point
// to the malloced memory.  uVal should be a multiple of 4
// on entry.

int DetermineLargest(int uVal,uchar **ptrPtr)
{
   uchar *ptr = NULL;
   uchar *newPtr;
   int curVal = uVal;
   int lVal = 0;

   do {
      if (ptr)
      {
         newPtr = (uchar *)realloc(ptr,curVal);
         if (newPtr) ptr = newPtr;
      }
      else
      {
         ptr = newPtr = (uchar *)malloc(curVal);
      }

      if (newPtr) lVal = curVal;
      else uVal = curVal;

      curVal = ((lVal + uVal) >> 1) & ~0x3;

   } while(lVal < uVal - 4);

   *ptrPtr = ptr;

   return lVal;

} // DetermineLargest

// lowMemReserved is in Kb !

int MallocableTotal(int lowMemReserved,uchar val)
{
#ifdef WIN32

   MEMORYSTATUS memoryStatus;
   const ulong kMinCap = 0x600000;               // 6mb
   const ulong kMaxCap = 0x1000000;              // 16 mb
   const ulong kNonDynReserved = 0x300000;       // 3mb
   ulong targetCap;
   memoryStatus.dwLength = sizeof(memoryStatus);
   GlobalMemoryStatus(&memoryStatus);
   targetCap = memoryStatus.dwTotalPhys - kNonDynReserved;

   return max(kMinCap, min(kMaxCap, targetCap));

#else

   dpmis_block lowMem;
   uchar *newPtr;
   uchar *headPtr = NULL;
   int maxMem = 0x40000000;   // start with 1 gig
   int totalMem = 0;

// First allocate some low mem directly from dpmi so that
// our slorker can't get at it.

   if (lowMemReserved != 0)
   {
   // Ok, shifting by 6 mean mult by 64 = 1024/16 is num of paras in Kb
      if (dpmi_alloc_dos_mem(&lowMem,(lowMemReserved << 6)) != 0)
      {
         Error(1,"MallocableTotal: Can't allocate low mem!\n");
         return 0;
      }
   } // have something to allocate low

// First allocate all of the memory
   while((maxMem = DetermineLargest(maxMem,&newPtr)) > 0)
   {
      memset(newPtr,val,maxMem);
      Spew(DSRC_LG_Memwalk,(
         "MallocableTotal: Found 0x%8x bytes at 0x%8x\n",
         maxMem,
         (uint)newPtr
      ));
      totalMem += maxMem;     // add in the next value
      *(uchar **)newPtr = headPtr;  // save the old head
      headPtr = newPtr;       // and point to new head
   } // while have another block

// Now deallocate everything
   while(headPtr)
   {
      uchar *next = *(uchar **)headPtr;
      free(headPtr);
      headPtr = next;
   } // while still points at something

   Spew(DSRC_LG_Memwalk,(
      "\nMallocableTotal: Total 0x%8x bytes\n\n",
      totalMem
   ));

// Finally return the low mem alloced above

   if (lowMemReserved != 0)
   {
      if (dpmi_free_dos_mem(&lowMem) != 0)
      {
         Error(1,"MallocableTotal: Can't dealloc low memory, abort!\n");
         return 0;
      } // free it
   } // have something to return

   return totalMem;
#endif
} // MallocableTotal

