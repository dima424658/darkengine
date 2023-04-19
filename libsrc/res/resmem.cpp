//      ResMem.C      Resource Manager memory management
//      Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/res/RCS/resmem.cpp 1.58 1998/01/07 10:46:14 TOML Exp $
*/

#ifdef _WIN32
#include <windows.h>
#else
#define max(x, y) ( ((x) > (y)) ? (x) : (y) )
#define min(x, y) ( ((x) < (y)) ? (x) : (y) )
#endif

#include <lg.h>
#include <comtools.h>
#include <allocapi.h>
#include <res.h>
#include <res_.h>
#include <resthred.h>
#include <coremutx.h>
#include <rescache.h>
#include <string.h>
#include <mprintf.h>
#include <initguid.h>

#undef Malloc
#undef Free
#undef Realloc

#pragma code_seg("lgalloc")

///////////////////////////////////////////////////////////////////////////////

BOOL g_fPagingEnabled;

///////////////////////////////////////////////////////////////////////////////

DEFINE_LG_GUID(GUID_ResCache, 0x8b);
ICache * g_pResCache;

///////////////////////////////////////

HRESULT LGAPI ResCacheCallbackFunc(const sCacheMsg * pMsg)
{
    switch (pMsg->message)
    {
        case kCM_DeleteOnAge:
        case kCM_DeleteOnFlush:
            DoResDrop((Id)pMsg->itemId);
            return S_OK;

        default:
            return S_FALSE;
    }
}

///////////////////////////////////////

void ResCacheRemove(Id id)
{
    if (!g_fPagingEnabled)
        return;

    void * pDummy;

    if (g_pResCache->Remove(id, &pDummy) != S_OK)
        Warning(("Tried to remove an item expected but not in the cache (0x%x)", id));
}

///////////////////////////////////////

void ResCacheAdd(Id id)
{
    if (!g_fPagingEnabled)
        return;

    AssertMsg(RESDESC(id)->lock == 0, "Locked item added to cache");

    g_pResCache->Add(id, NULL, RESDESC(id)->size);
}

///////////////////////////////////////

void ResCacheTouch(Id id)
{
    if (g_fPagingEnabled)
        g_pResCache->Touch(id);
}

///////////////////////////////////////////////////////////////////////////////

ulong LGAPI ResPager(ulong size, sAllocLimits * pLimits);

// No longer used?  8/15/97 -JF
//static bool resPushedAllocators;                 // did we push our allocators?

void ResMemInit()
{
   static BOOL fInited;

   if (!fInited)
   {
      fInited = TRUE;
      sCacheClientDesc cacheClientDesc;

      cacheClientDesc.pID         = &GUID_ResCache;
      cacheClientDesc.pContext    = NULL;
      cacheClientDesc.pfnCallback = ResCacheCallbackFunc;
      cacheClientDesc.nMaxBytes   = (unsigned)-1;
      cacheClientDesc.nMaxItems   = (unsigned)-1;
      cacheClientDesc.flags       = 0;

      g_pResSharedCache->AddClient(&cacheClientDesc, &g_pResCache);
   }

//  Set the default allocation cap
   ResMemSetCap(ResPickAllocCap());
   g_fPagingEnabled = AllocSetPageFunc(ResPager);

}

///////////////////////////////////////

void ResMemTerm()
{
#ifndef SHIP
   g_pResSharedCache->FlushAll();
#endif
   SafeRelease(g_pResCache);
   AllocSetPageFunc(NULL);
}

///////////////////////////////////////////////////////////////////////////////

void ResGetMemStats(sResMemStats * pStats)
{
#ifdef _WIN32
    MEMORYSTATUS memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    GlobalMemoryStatus(&memoryStatus);
    memcpy(pStats, &memoryStatus, sizeof(memoryStatus));
#endif

    sCacheState state;
    sAllocLimits limits;

    g_pResSharedCache->GetState(&state);
    AllocGetLimits(&limits);

    pStats->allocCap = limits.allocCap;
    pStats->totalMalloc = limits.totalAlloc;
    pStats->lockedMalloc = limits.totalAlloc - state.nBytes;
    pStats->cachedMalloc = state.nBytes;
}

///////////////////////////////////////////////////////////////////////////////

#define ResInternalMalloc(n)     malloc(n)
#define ResInternalFree(p)       free(p);
#define ResInternalRealloc(p, n) realloc(p, n)
#define ResInternalGetSize(p)    _msize(p)

///////////////////////////////////////////////////////////////////////////////

ulong ResPickAllocCap()
{
   return AllocPickAllocCap();
}

ulong ResMemSetCap(ulong cap)
{
   return AllocSetAllocCap(cap);
}

///////////////////////////////////////////////////////////////////////////////

BOOL g_fResPage = TRUE;

void ResEnablePaging(BOOL b)
{
    g_fResPage = b;
}

///////////////////////////////////////////////////////////////////////////////

void ResCompact(void)
{
    g_pResSharedCache->FlushAll();
    ResMemSetCap(ResPickAllocCap());
    _heapmin();
}

///////////////////////////////////////////////////////////////////////////////

// Contains the amount of bytes we flushed from the cache in order to get more
// memory.  Must be cleared by application each frame.
#ifdef DBG_ON

#include <memstat.h>

ResPageStats resPageStats;                       // paging statistics

#endif
Id idBeingLoaded = ID_NULL;                      // for res pager: which id is being loaded,
                                                 // or ID_NULL (0) for general Malloc()

//   ---------------------------------------------------------
//
//   ResPage() pages out block objects to scare up memory.
//
//      size = # bytes we need to scare up
//
//   Returns: ptr to allocated memory, or NULL if couldn't ResInternalFree up room enough


#ifdef DBG_ON
void ResDumpLRU()
{
#if TODO
   Id id;
   ResDesc *prd;
   Spew(DSRC_RES_Stat, ("---LRU LIST (newest to oldest) ---\n"));
   id = gResDesc[ID_TAIL].prev;
   while (id != ID_HEAD)
   {
      prd = RESDESC(id);
      Spew(DSRC_RES_Stat, ("id: $%x  lock: %d\n", id, prd->lock));
      id = prd->prev;
   }
   Spew(DSRC_RES_Stat, ("--------------\n"));
#endif
}
#endif


///////////////////////////////////////////////////////////////////////////////

//   --------------------------------------------------------------
//      DEFAULT PAGER - INSTALL A REPLACEMENT, IF YOU DARE
//   --------------------------------------------------------------
//
//   ResDefaultPager() is default pager.  This routine can be replaced
//   by a call to ResInstallPager().  Such a replacement routine, to be
//   a good citizen, should emulate this routine with respect to maintenance
//   of paging statistics variables.
//
//   The default pager ignores the global idBeingLoaded, which is set to
//   the id of the resource which was trying to be loaded when this paging
//   happened, or ID_NULL (0) if it happened during a general Malloc().
//   A replacement pager could look at this id, and thereby have access
//   to its type, or maybe some other user category based on id.  The
//   replacement pager could then make use of this information in some
//   magical way to decide who to throw out.

ulong LGAPI ResPager(ulong size, sAllocLimits * pLimits)
{
   // If the game has explicitly disabled paging...
   if (!g_fResPage)
     return 0;

   #define     kResPageMinTriggerSize   1024
   #define     kResPageMaxTriggerSkip   (kResPageMinTriggerSize * 32)
   #define     kResPageExtraPct         10
   #define     kResPageMaxExtra         (100*1024)
   #define     kResPageMinExtra         (10*1024)
   #define     kResAllocCapMinIncrement (16 * 1024)
   #define     kResPageHighAllocLevel   (1024 * 1024 * 24)

   long        nBytesFreed;
   long        nBytesWanted;
   long        nBytesExtra;
   long        nBytesOverCap;

   static long triggerSkipped;

   // If it's a really small block, don't bother paging (limiting the consecutive skipped paging requests)
   // We're intentionally allowing possibly less-correct (but safe) behavior by not holding on to any mutex
   // while we do this.  This is to save time on small blocks.
   if (size < kResPageMinTriggerSize && size != 1 && triggerSkipped < kResPageMaxTriggerSkip) // @Note (toml 03-07-97): Size '1' is reserved to force a page
   {
      triggerSkipped += size;
      return 1;
   }
   else
      triggerSkipped = 0;

   ResThreadLock();
   CoreThreadLock();

   const ulong allocCap = pLimits->allocCap;
   const ulong totalAlloc = pLimits->totalAlloc;

#ifndef SHIP
   static ulong highWarnBytes = kResPageHighAllocLevel;
   if (totalAlloc > highWarnBytes)
   {
      mprintf("WARNING: Memory allocation suspiciously high (%d bytes)!\n", totalAlloc);
      highWarnBytes += (kResPageHighAllocLevel / 2);
   }
#endif

//   Set up for memory freeing loop:  here, we decide to scare up an
//   extra 10% (restricted to 10K - 100k) above and beyond what we're looking
//   for, just so we don'tgo thru this pain every millisecond.  Is this really
//   a good idea? I dunno.  Plus, remember that just freeing up the needed
//   amount of memory is not guarantee of success, because we're really looking
//   for a contiguous block of that size.  This is another reason for getting
//   more than we think we need, because once we get that point we go into
//   a loop of trying a ResInternalMalloc(), freeing a single additional item, trying
//   ResInternalMalloc() again, etc.  We don't want to enter this loop too early for
//   performance reasons, or so I figured when I wrote this.

   nBytesFreed    = 0;
   nBytesOverCap  = (totalAlloc > allocCap) ? totalAlloc - allocCap : 0;

   nBytesExtra    = (size * kResPageExtraPct) / 100;
   nBytesExtra    = max(kResPageMinExtra, min(kResPageMaxExtra, nBytesExtra));

// Aim to page down to cap, plus target size, plus extra bytes
   nBytesWanted   = nBytesOverCap + size + nBytesExtra;
   nBytesWanted   = max(nBytesWanted, nBytesOverCap);

   nBytesFreed = g_pResSharedCache->Purge(nBytesWanted);

   // Calculate a new cap
   long newTargetCap = 0;

   // If totally over the cap, loosen cap by 1/16 to reduce paging
   if (size > nBytesFreed)
   {
      newTargetCap = allocCap + (size - nBytesFreed) + pLimits->initAllocCap / 16;
      newTargetCap = newTargetCap + (kResAllocCapMinIncrement - newTargetCap % kResAllocCapMinIncrement);
   }

   if (newTargetCap)
   {
      // Snap the cap to initial cap, max initial +50 %
      if (newTargetCap < pLimits->initAllocCap)
         newTargetCap = pLimits->initAllocCap;
      else if (newTargetCap > pLimits->initAllocCap + pLimits->initAllocCap / 2)
         newTargetCap = pLimits->initAllocCap + pLimits->initAllocCap / 2;

      pLimits->allocCap = newTargetCap;
   }

   DBG(DSRC_RES_CumStat, { resPageStats.totSizeGotten += nBytesFreed; });

   CoreThreadUnlock();
   ResThreadUnlock();

   return nBytesFreed;                                     // if successful, we're done!
}


///////////////////////////////////////////////////////////////////////////////

extern "C" {

#if TODO
void ResValidateList()
{
   Id id, idNext;
   ResDesc *prd;

   id = gResDesc[ID_HEAD].next;     // let's start with oldest resource

//   Go from head (oldest) to tail (newest) of LRU chain,
//   looking for corrupt resources

   while (id != ID_TAIL)
   {

//   Get ptr to resource descriptor, preload 'next' field in case we drop
//   this item

      prd = RESDESC(id);
      idNext = prd->next;

//   Check for bad resource in LRU list, warn

#ifdef DBG_ON
      if (prd->ptr == NULL)
      {
         Error(1, "ResValidateList: id $%x in lru list but not in ram\n", id);
      }
      else if (prd->lock)
      {
         Error(1, "ResValidateList: id $%x in lru list but locked, lock count: %d\n", id, prd->lock);
      }
#endif

//   Go on to next resource

      id = idNext;                               // go to next block in LRU order
   }
}
#endif
}
