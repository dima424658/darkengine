///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/res/RCS/rescache.h $
// $Author: TOML $
// $Date: 1997/02/28 22:16:27 $
// $Revision: 1.2 $
//

#ifndef __RESCACHE_H
#define __RESCACHE_H

#include <lg.h>
#include <comtools.h>
#include <aggmemb.h>
#include <cacheapi.h>

#include <hashfns.h>
#include <dynarray.h>

///////////////////////////////////////////////////////////////////////////////

class cResCache;
class cResSharedCache;

typedef cDynArray<cResCache *> tRCSCacheArray;

struct sRSCItem;
class cRSCItemList;
class cRSCItemTable;

///////////////////////////////////////

extern cResSharedCache * g_pResSharedCache;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResCache
//

class cResCache : public ICache, private sCacheClientDesc
{
public:
    cResCache(const sCacheClientDesc * pDesc);
    virtual ~cResCache() {};

    // IUnknown methods
    DECLARE_UNAGGREGATABLE();

    // Query the state of the cache
    STDMETHOD_(void, GetState)(sCacheState *);

    // Add an item to the cache
    STDMETHOD (Add)(tCacheItemID id, void * pItem, ulong size);

    // Retrieve an item from the cache.
    STDMETHOD (Retrieve)(tCacheItemID id, void ** ppResult);

    // Retrieve and remove item from the cache.
    STDMETHOD (Remove)(tCacheItemID id, void ** ppResult);

    // Explicitly mark an item as "recently used"
    STDMETHOD (Touch)(tCacheItemID id);

    // Explicitly mark an item as "least recently used"
    STDMETHOD (Age)(tCacheItemID id);

    // Remove an item from the cache
    STDMETHOD (Flush)(tCacheItemID id);

    // Empty the cache
    STDMETHOD (FlushAll)();

    // Dump the oldest items to free the specified amount of memory
    STDMETHOD_(ulong, Purge)(ulong nBytes);

    const sCacheClientDesc * GetDesc() const;
    ulong GetHashValue() const;

    static void StaticInit(cResSharedCache * pSharedCache);
    static ulong SharedPurge(ulong nBytes);

private:

    void RemoveItem(sRSCItem *);

    static cResSharedCache * gm_pSharedCache;
    static cRSCItemList *    gm_pItemsList;
    static cRSCItemTable *   gm_pItemsTable;

    ulong   m_dwHashValue;

    ulong   m_nBytes;
    ulong   m_nItems;

#ifndef SHIP
    ulong   m_nRequests;
    ulong   m_nHits;
#endif
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cResSharedCache
//

class cResSharedCache : public ISharedCache
{
public:
    cResSharedCache();
    virtual ~cResSharedCache() {};

    // IUnknown methods
    DECLARE_SIMPLE_AGGREGATION(cResSharedCache);

    // Add the cache to the application aggregate
    void ExposeToAggregate(IUnknown * pOuter);

    // Query the state of the cache
    STDMETHOD_(void, GetState)(sCacheState *);

    // Add a client to the caching system.  Pointer argument is not stored.
    STDMETHOD (AddClient)(const sCacheClientDesc * pClientDesc, ICache ** ppResult);

    // Find a client in the caching system.
    STDMETHOD (FindClient)(const GUID * pID, ICache ** ppResult);

    // Mark an whole client set as "recently used"
    STDMETHOD (Touch)(const GUID * pID);

    // Mark an whole client set as "least recently used"
    STDMETHOD (Age)(const GUID * pID);

    // Flush a whole client set
    STDMETHOD (Flush)(const GUID * pID);

    // Empty the cache
    STDMETHOD (FlushAll)();

    // Dump the oldest items to free the specified amount of memory
    STDMETHOD_(ulong, Purge)(ulong nBytes);

    // Get information about the cache usage
    STDMETHOD_(void, GetStats)(sCacheStats* pStats);

    // Find a client without ref counting
    cResCache * DoFindClient(const GUID * pID);

    // Remove a client from the caching system.
    void RemoveCache(cResCache *);

    void NoteAddItem(ulong size);
    void NoteRemoveItem(ulong size);

private:
    tRCSCacheArray          m_SubCaches;

    ulong m_nBytes;
    ulong m_nItems;

#ifndef SHIP
    ulong m_nRequests;
    ulong m_nHits;
#endif
};

///////////////////////////////////////////////////////////////////////////////
//
// Inline functions
//

inline cResCache::cResCache(const sCacheClientDesc * pDesc)
  : m_nBytes(0),
    m_nItems(0)
#ifndef SHIP
    ,
    m_nRequests(0),
    m_nHits(0)
#endif
{
    memcpy(((sCacheClientDesc *)this), pDesc, sizeof(sCacheClientDesc));
    m_dwHashValue = HashGUID(pID);
}

///////////////////////////////////////

inline const sCacheClientDesc * cResCache::GetDesc() const
{
    return this;
}

///////////////////////////////////////

inline ulong cResCache::GetHashValue() const
{
    return m_dwHashValue;
}

///////////////////////////////////////

inline cResCache * cResSharedCache::DoFindClient(const GUID * pID)
{
    for (int i = 0; i < m_SubCaches.Size(); i++)
        if (*(m_SubCaches[i]->GetDesc()->pID) == *pID)
            return m_SubCaches[i];
    return NULL;
}

///////////////////////////////////////

inline void cResSharedCache::NoteAddItem(ulong size)
{
    m_nBytes += size;
    m_nItems++;
}

///////////////////////////////////////

inline void cResSharedCache::NoteRemoveItem(ulong size)
{
    m_nBytes -= size;
    m_nItems--;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__RESCACHE_H */
