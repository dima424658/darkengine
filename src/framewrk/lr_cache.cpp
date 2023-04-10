/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/lr_cache.cpp,v 1.2 2000/02/19 13:16:24 toml Exp $
// interface to the system cache for LR

// when an LR entry is unlocked, LR tells this cache it is free
// if the cache needs it, it tells LR, who really gets rid of it
// if LR needs it before the cache hoses it, then LR takes it back

// NOTE: currently only unlocked data is in the cache!

#include <lg.h>
#include <mprintf.h>
#include <comtools.h>
#include <appagg.h>
#include <cacheapi.h>

#include <lr_cache.h>

#include <initguid.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
DEFINE_LG_GUID(GUID_lrCache,0x4f);

// global cache data
static ICache *     g_pLRCache = NULL;
static int          g_HndInUse;
static lr_callback  g_pKillCallback = NULL;

/////////
#ifdef DBG_ON
#define lrCache_VerifyState() if (g_pLRCache==NULL) return FALSE
#else
#define lrCache_VerifyState()
#endif

//#define LR_TALK

#ifdef LR_TALK
#define lrCacheTalk(x)  mprintf x
#define lrCacheRun(x)   x
static int kill_count;
#else
#define lrCacheTalk(x)
#define lrCacheRun(x)
#endif

////////////
// cache control callback

// return S_FALSE to mean dont do the delete....
HRESULT LGAPI _lrCache_CallbackFunc(const sCacheMsg * pMsg)
{
   BOOL do_delete=TRUE;
   lrCacheTalk(("KillCall %x (pos %d) (item %x) (%x)..",pMsg->message,pMsg->itemId,pMsg->pItem,kCM_DeleteOnAge));
   lrCacheRun(kill_count++);
   switch (pMsg->message)
   {
      case kCM_DeleteOnAge:
         break;
      case kCM_DeleteOnFlush:
         if ((int)pMsg->itemId == g_HndInUse)
            do_delete=FALSE;
         break;
   }
   if (do_delete&&g_pKillCallback)
      do_delete=(*g_pKillCallback)((int)pMsg->itemId, pMsg->pItem);
   return do_delete?S_OK:S_FALSE;
}

///////////
// init/term of cache

// pass in a size?
void lrCache_Init(lr_callback toast_it, int size)
{
    AutoAppIPtr(SharedCache);

    if (pSharedCache)
    {
        sCacheClientDesc cacheClientDesc;

        cacheClientDesc.pID         = &GUID_lrCache;
        cacheClientDesc.pContext    = NULL;
        cacheClientDesc.pfnCallback = _lrCache_CallbackFunc;
        cacheClientDesc.nMaxBytes   = (ulong) size;
        cacheClientDesc.nMaxItems   = (ulong) -1;
        cacheClientDesc.flags       = 0;

        pSharedCache->AddClient(&cacheClientDesc, &g_pLRCache);

        g_pKillCallback = toast_it;
    }
    else
        CriticalMsg("Expected shared cache");
}

void lrCache_Term(void)
{
    if (g_pLRCache)
    {
        g_pLRCache->FlushAll();
        SafeRelease(g_pLRCache);
        g_pLRCache=NULL;
    }
}

////////////
// lock/free behavior

// there are two mutator calls to the cache
// StoreFreedData and RetrieveFreedData
// as well as the callback installed in Init on Cache delete

BOOL lrCache_StoreFreedData(int hnd, void *pData, int size)
{
   lrCache_VerifyState();
   g_HndInUse=hnd;
   lrCacheTalk(("Storing %d at %x size %x\n",hnd,pData,size));
   VerifyMsg1(g_pLRCache->Add((tCacheItemID)hnd, pData, size) == S_OK,
              "hnd %i already in cache!\n", hnd );
   g_HndInUse=0;
   return TRUE;
}

BOOL lrCache_TakeBackData(int hnd)
{
   void *pData;    // returns TRUE if S_OK, ie. if we were in the cache
   BOOL rv;
   lrCache_VerifyState();
   rv=(g_pLRCache->Remove((tCacheItemID)hnd, &pData) == S_OK);
   lrCacheTalk(("Took back %d (rv %d) (data %x)\n",hnd,rv,pData));
   return rv;
}

////////////
// flush control

BOOL lrCache_FlushAll(void)
{
   lrCache_VerifyState();
#ifdef LR_TALK
   {
      sCacheState state;
      lrCacheRun(g_pLRCache->GetState(&state));
      lrCacheRun(kill_count=0);
      lrCacheTalk(("nItems %d..",state.nItems));
   }
#endif
   g_pLRCache->FlushAll();
   lrCacheTalk(("kill count %d..",kill_count));
   return TRUE;
}

BOOL lrCache_FlushHandle(int hnd)
{
   lrCache_VerifyState();
   if (g_HndInUse != hnd)
       g_pLRCache->Flush(hnd);
   return TRUE;
}

/////////////
// utilities to mainpulate shared cache

// idiotic!!! i should just return a struct! kill me now!!

int lrCache_GetSize(int *maxBytes)
{
   sCacheState state;
   lrCache_VerifyState();
   g_pLRCache->GetState(&state);
   if (maxBytes) *maxBytes=state.nMaxBytes;
   return state.nBytes;
}

int lrCache_GetGlobalSize(int *maxBytes)
{
   sCacheState state;
   AutoAppIPtr(SharedCache);
   pSharedCache->GetState(&state);
   if (maxBytes) *maxBytes=state.nMaxBytes;
   return state.nBytes;
}
