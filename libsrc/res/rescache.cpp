///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/res/RCS/rescache.cpp $
// $Author: dc $
// $Date: 1997/10/30 16:42:44 $
// $Revision: 1.12 $
//
// @TBD (toml 02-21-97): The internal statistics are not correct right now! Will fix...
//

#include <lg.h>
#include <resthred.h>
#include <rescache.h>

#include <pool.h>
#include <hashset.h>
#include <dlist.h>

#include <mprintf.h>

#include <allocapi.h>

///////////////////////////////////////////////////////////////////////////////

static cResSharedCache g_ResSharedCache;
cResSharedCache *      g_pResSharedCache = &g_ResSharedCache;

///////////////////////////////////////////////////////////////////////////////

EXTERN
HRESULT LGAPI _ResSharedCacheCreate(REFIID, ISharedCache ** /*ppSharedCache*/, IUnknown * pOuter)
{
    g_ResSharedCache.ExposeToAggregate(pOuter);
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sRSCItem
//

struct sRSCItem;
typedef cDListNode<sRSCItem, 1> sRSCItemNode;

struct sRSCItemTag
{
    tCacheItemID    id;
    cResCache *     pCache;
};

struct sRSCItem : public sRSCItemTag, public sRSCItemNode
{
    sRSCItem(const sRSCItemTag & tag, void * pItemData_, ulong size_)
    {
        id        = tag.id;
        pCache    = tag.pCache;
        pItemData = pItemData_;
        size      = size_;
    }

    void *       pItemData;
    ulong        size;

    DECLARE_POOL();
};

IMPLEMENT_POOL(sRSCItem);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cRSCItemList
//

class cRSCItemList : public cDList<sRSCItem, 1>
{

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cRSCItemTable
//

class cRSCItemHashFunc
{
public:
   static unsigned Hash(const sRSCItemTag *p)
   {
      return (HashIntegerValue(p->id) ^ p->pCache->GetHashValue());
   }

   static BOOL IsEqual(const sRSCItemTag *p1, const sRSCItemTag *p2)
   {
      return (p1->id == p2->id) && (p1->pCache == p2->pCache);
   }
};

class cRSCItemTable : public cHashSet<sRSCItem *, sRSCItemTag *, cRSCItemHashFunc>
{
    virtual tHashSetKey GetKey(tHashSetNode n) const
    {
        return (tHashSetKey)(((sRSCItemTag *)((sRSCItem *)n)));
    }
};

#include <hshsttem.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResCache
//

void cResCache::StaticInit(cResSharedCache * pSharedCache)
{
    gm_pSharedCache = pSharedCache;
}

///////////////////////////////////////

inline void cResCache::OnFinalRelease()
{
    gm_pSharedCache->RemoveCache(this);
    delete this;
}

IMPLEMENT_UNAGGREGATABLE(cResCache, ICache);

///////////////////////////////////////

static cRSCItemList  _g_ItemsList;
static cRSCItemTable _g_ItemsTable;

cResSharedCache * cResCache::gm_pSharedCache;
cRSCItemList *    cResCache::gm_pItemsList = &_g_ItemsList;
cRSCItemTable *   cResCache::gm_pItemsTable = &_g_ItemsTable;

///////////////////////////////////////

inline void cResCache::RemoveItem(sRSCItem * pItem)
{
    m_nBytes -= pItem->size;
    m_nItems--;
    gm_pSharedCache->NoteRemoveItem(pItem->size);
    gm_pItemsList->Remove(pItem);
    gm_pItemsTable->Remove(pItem);
    delete pItem;
}

///////////////////////////////////////
//
// Query the state of the cache
//

STDMETHODIMP_(void) cResCache::GetState(sCacheState * pState)
{
    cAutoResThreadLock lock;

    pState->nBytes    = m_nBytes;
    pState->nMaxBytes = nMaxBytes;
    pState->nItems    = m_nItems;
    pState->nMaxItems = nMaxItems;
#ifndef SHIP
    pState->nRequests = m_nRequests;
    pState->nHits     = m_nHits;
#endif
}

///////////////////////////////////////
//
// Add an item to the cache
//

STDMETHODIMP cResCache::Add(tCacheItemID id, void * pItemData, ulong size)
{
    cAutoResThreadLock lock;
    HRESULT result;

    sRSCItem *  pItem;
    sRSCItemTag tag;

    tag.id     = id;
    tag.pCache = this;

    pItem = gm_pItemsTable->Search(&tag);

    if (pItem)
    {
        gm_pItemsList->Remove(pItem);
        result = S_FALSE;
    }
    else
    {
        unsigned nNewByteCount = m_nBytes + size;
        if (nNewByteCount > nMaxBytes)
            Purge(nNewByteCount - nMaxBytes);

        pItem = new sRSCItem(tag, pItemData, size);
        gm_pItemsTable->Insert(pItem);

        m_nBytes += size;
        m_nItems++;
        gm_pSharedCache->NoteAddItem(size);

        result = S_OK;
    }

    gm_pItemsList->Prepend(pItem);

    return result;
}

///////////////////////////////////////
//
// Retrieve an item from the cache.
//

STDMETHODIMP cResCache::Retrieve(tCacheItemID id, void ** ppResult)
{
    cAutoResThreadLock lock;

    sRSCItem *  pItem;
    sRSCItemTag tag;

    tag.id     = id;
    tag.pCache = this;

    pItem = gm_pItemsTable->Search(&tag);

#ifndef SHIP
    m_nRequests++;
#endif

    if (pItem)
    {
        *ppResult = pItem->pItemData;

        gm_pItemsList->Remove(pItem);
        gm_pItemsList->Prepend(pItem);

#ifndef SHIP
        m_nHits++;
#endif
        return S_OK;
    }

    *ppResult = NULL;
    return S_FALSE;
}

///////////////////////////////////////
//
// Retrieve and remove item from the cache.
//

STDMETHODIMP cResCache::Remove(tCacheItemID id, void ** ppResult)
{
    cAutoResThreadLock lock;

    sRSCItem *  pItem;
    sRSCItemTag tag;

    tag.id     = id;
    tag.pCache = this;

    pItem = gm_pItemsTable->Search(&tag);

#ifndef SHIP
    m_nRequests++;
#endif

    if (pItem)
    {
#ifndef SHIP
        m_nHits++;
#endif
        *ppResult = pItem->pItemData;
        RemoveItem(pItem);
        return S_OK;
    }

    *ppResult = NULL;
    return S_FALSE;
}

///////////////////////////////////////
//
// Explicitly mark an item as "recently used"
//

STDMETHODIMP cResCache::Touch(tCacheItemID id)
{
    cAutoResThreadLock lock;
    sRSCItem *  pItem;
    sRSCItemTag tag;

    tag.id     = id;
    tag.pCache = this;

    pItem = gm_pItemsTable->Search(&tag);

    if (pItem)
    {
        gm_pItemsList->Remove(pItem);
        gm_pItemsList->Prepend(pItem);
        return S_OK;
    }

    return E_FAIL;
}

///////////////////////////////////////
//
// Explicitly mark an item as "least recently used"
//

STDMETHODIMP cResCache::Age(tCacheItemID id)
{
    cAutoResThreadLock lock;
    sRSCItem *  pItem;
    sRSCItemTag tag;

    tag.id     = id;
    tag.pCache = this;

    pItem = gm_pItemsTable->Search(&tag);

    if (pItem)
    {
        gm_pItemsList->Remove(pItem);
        gm_pItemsList->Append(pItem);
        return S_OK;
    }

    return E_FAIL;
}

///////////////////////////////////////
//
// Remove an item from the cache
//

STDMETHODIMP cResCache::Flush(tCacheItemID id)
{
    cAutoResThreadLock lock;

    sRSCItem *  pItem;
    sRSCItemTag tag;

    tag.id     = id;
    tag.pCache = this;

    pItem = gm_pItemsTable->Search(&tag);

    if (pItem)
    {
        sCacheMsg cacheMsg;

        cacheMsg.pClientContext = GetDesc()->pContext;
        cacheMsg.itemId         = pItem->id;
        cacheMsg.pItem          = pItem->pItemData;
        cacheMsg.itemSize       = pItem->size;
        cacheMsg.message        = kCM_DeleteOnFlush;

        if ((*(GetDesc()->pfnCallback))(&cacheMsg) == S_OK)
        {
            RemoveItem(pItem);
            return S_OK;
        }
        return S_FALSE;
    }
    return E_FAIL;
}

///////////////////////////////////////
//
// Empty the cache
//

STDMETHODIMP cResCache::FlushAll()
{
    cAutoResThreadLock lock;
    ulong                    nBytesPurged = 0;
    sCacheMsg                cacheMsg;
    sRSCItem *               pItem = gm_pItemsList->GetFirst();
    sRSCItem *               pNextItem;

    cacheMsg.message = kCM_DeleteOnFlush;

    while (pItem)
    {
        pNextItem = pItem->GetNext();
        if (pItem->pCache == this)
        {
            cacheMsg.pClientContext = GetDesc()->pContext;
            cacheMsg.itemId         = pItem->id;
            cacheMsg.pItem          = pItem->pItemData;
            cacheMsg.itemSize       = pItem->size;

            if ((*(GetDesc()->pfnCallback))(&cacheMsg) == S_OK)
            {   // override pNext in case user was a goof and deleted some items
                pNextItem = pItem->GetNext();
                RemoveItem(pItem);
            }
        }
        pItem = pNextItem;
    }

    return S_OK;
}

///////////////////////////////////////
//
// Dump the oldest items to free the specified amount of memory
//

STDMETHODIMP_(ulong) cResCache::Purge(ulong nBytesToPurge)
{
    cAutoResThreadLock lock;
    ulong                    nBytesPurged = 0;
    sCacheMsg                cacheMsg;
    sRSCItem *               pItem = gm_pItemsList->GetLast();
    sRSCItem *               pPrevItem;

    cacheMsg.message = kCM_DeleteOnAge;

    while (pItem && m_nItems && nBytesPurged < nBytesToPurge)
    {
        pPrevItem = pItem->GetPrevious();
        if (pItem->pCache == this)
        {
            cacheMsg.pClientContext = GetDesc()->pContext;
            cacheMsg.itemId         = pItem->id;
            cacheMsg.pItem          = pItem->pItemData;
            cacheMsg.itemSize       = pItem->size;

            if ((*GetDesc()->pfnCallback)(&cacheMsg) == S_OK)
            {   // update PrevItem correctly
                pPrevItem = pItem->GetPrevious();
                nBytesPurged += pItem->size;
                RemoveItem(pItem);
            }
        }
        pItem = pPrevItem;
    }

    return nBytesPurged;
}

///////////////////////////////////////
//
// Dump the oldest items to free the specified amount of memory
//

ulong cResCache::SharedPurge(ulong nBytesToPurge)
{
    ulong                    nBytesPurged = 0;
    sCacheMsg                cacheMsg;
    const sCacheClientDesc * pClientDesc;
    sRSCItem *               pItem = gm_pItemsList->GetLast();
    sRSCItem *               pPrevItem;

    cacheMsg.message = kCM_DeleteOnAge;

    while (pItem && nBytesPurged < nBytesToPurge)
    {
        pPrevItem   = pItem->GetPrevious();
        pClientDesc = pItem->pCache->GetDesc();

        cacheMsg.pClientContext = pClientDesc->pContext;
        cacheMsg.itemId         = pItem->id;
        cacheMsg.pItem          = pItem->pItemData;
        cacheMsg.itemSize       = pItem->size;

        if ((*pClientDesc->pfnCallback)(&cacheMsg) == S_OK)
        {   // update PrevItem correctly
            pPrevItem = pItem->GetPrevious();
            nBytesPurged += pItem->size;
            pItem->pCache->RemoveItem(pItem);
        }
        pItem = pPrevItem;
    }

    return nBytesPurged;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResCache
//

inline void cResSharedCache::OnFinalRelease() {} // Do nothing!

IMPLEMENT_SIMPLE_AGGREGATION(cResSharedCache);

///////////////////////////////////////

cResSharedCache::cResSharedCache()
  : m_nBytes(0),
    m_nItems(0)
#ifndef SHIP
    ,
    m_nRequests(0),
    m_nHits(0)
#endif
{
    cResCache::StaticInit(this);
}

///////////////////////////////////////
//
// Add the cache to the application aggregate
//

void cResSharedCache::ExposeToAggregate(IUnknown * pOuter)
{
    INIT_AGGREGATION_1(pOuter, IID_ISharedCache, this, 0, NULL);
}

///////////////////////////////////////
//
// Query the state of the cache
//

STDMETHODIMP_(void) cResSharedCache::GetState(sCacheState * pState)
{
    cAutoResThreadLock lock;

    pState->nBytes    = m_nBytes;
    pState->nMaxBytes = (unsigned)-1;
    pState->nItems    = m_nItems;
    pState->nMaxItems = (unsigned)-1;
#ifndef SHIP
    pState->nRequests = m_nRequests;
    pState->nHits     = m_nHits;
#endif
}

///////////////////////////////////////
//
// Add a client to the caching system.  Pointer argument is not stored.
//

STDMETHODIMP cResSharedCache::AddClient(const sCacheClientDesc * pClientDesc, ICache ** ppResult)
{
    cAutoResThreadLock lock;

//    AssertMsg(!DoFindClient(pClientDesc->pID), "Improperly adding a cache client a second time");

    cResCache * pResult = new cResCache(pClientDesc);
    m_SubCaches.Append(pResult);
    
    // @Note (toml 10-28-97): Be aware that the shared cache does not hold a reference
    // to the sub cache, as the subcache is really part of the same aggregate object. 
    // The subcache removes itself on final release.

    *ppResult = pResult;

    return S_OK;
}

///////////////////////////////////////
//
// Remove a client from the caching system.
//

void cResSharedCache::RemoveCache(cResCache * pCache)
{
    cAutoResThreadLock lock;

    for (int i = 0; i < m_SubCaches.Size(); i++)
        if (m_SubCaches[i] == pCache)
        {
            m_SubCaches.DeleteItem(i);
            break;
        }
}

///////////////////////////////////////
//
// Find a client in the caching system.
//

STDMETHODIMP cResSharedCache::FindClient(const GUID * pID, ICache ** ppResult)
{
    cAutoResThreadLock lock;
    *ppResult = DoFindClient(pID);
    if (*ppResult)
    {
        (*ppResult)->AddRef();
        return S_OK;
    }
    return S_FALSE;
}

///////////////////////////////////////
//
// Mark an whole client set as "recently used"
//

STDMETHODIMP cResSharedCache::Touch(const GUID * /*pID*/)
{
    cAutoResThreadLock lock;
    CriticalMsg("Unimplemented functionality");
    return E_FAIL;
}

///////////////////////////////////////
//
// Mark an whole client set as "least recently used"
//

STDMETHODIMP cResSharedCache::Age(const GUID * /*pID*/)
{
    cAutoResThreadLock lock;
    CriticalMsg("Unimplemented functionality");
    return E_FAIL;
}

///////////////////////////////////////
//
// Flush a whole client set
//

STDMETHODIMP cResSharedCache::Flush(const GUID * /*pID*/)
{
    cAutoResThreadLock lock;
    CriticalMsg("Unimplemented functionality");
    return E_FAIL;
}

///////////////////////////////////////
//
// Empty the cache
//

STDMETHODIMP cResSharedCache::FlushAll()
{
    cAutoResThreadLock lock;
    for (int i = 0; i < m_SubCaches.Size(); i++)
        m_SubCaches[i]->FlushAll();

    return S_OK;
}

///////////////////////////////////////
//
// Dump the oldest items to free the specified amount of memory
//

STDMETHODIMP_(ulong) cResSharedCache::Purge(ulong nBytesToPurge)
{
    cAutoResThreadLock lock;
    return cResCache::SharedPurge(nBytesToPurge);
}

STDMETHODIMP_(void) cResSharedCache::GetStats(sCacheStats* pStats)
{
    // TODO
    MEMORYSTATUS Buffer;
    Buffer.dwLength = 32;
    GlobalMemoryStatus(&Buffer);
    memcpy(pStats, &Buffer, sizeof(Buffer));

    sAllocLimits limits;
    AllocGetLimits(&limits);

    sCacheState state;
    g_pResSharedCache->GetState(&state);

    pStats->allocCap = limits.allocCap;
    pStats->totalMalloc = limits.totalAlloc;
    pStats->lockedMalloc = limits.totalAlloc - state.nBytes;
    pStats->cachedMalloc = state.nBytes;
}

///////////////////////////////////////////////////////////////////////////////
