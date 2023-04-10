///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/cacheapi.h $
// $Author: JUSTIN $
// $Date: 1998/08/24 13:13:41 $
// $Revision: 1.3 $
//
// Interfaces for simple resource caching.  The application object will
// have a shared cache for cooperative system-wide resource sharing
//

#ifndef __CACHEAPI_H
#define __CACHEAPI_H

#include <comtools.h>
#include <cachguid.h>

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(ICache);
F_DECLARE_INTERFACE(ISharedCache);

typedef struct  sCacheState sCacheState;
typedef int                 eCacheMsg;
typedef struct  sCacheMsg   sCacheMsg;

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sCacheState
//

struct sCacheState
{
    // Current size, limit of the cache
    ulong   nBytes;
    ulong   nMaxBytes;

    // Number of items in the cache
    ulong   nItems;
    ulong   nMaxItems;

    // Statistics
#ifndef SHIP
    ulong   nRequests;
    ulong   nHits;
#endif
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: ICache
//

typedef ulong tCacheItemID;

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE ICache

DECLARE_INTERFACE_(ICache, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Query the state of the cache
    //
    STDMETHOD_(void, GetState)(THIS_ sCacheState *) PURE;

    //
    // Add an item to the cache
    //
    STDMETHOD (Add)(THIS_ tCacheItemID id, void * pItem, ulong size) PURE;

    //
    // Retrieve an item from the cache.
    //
    //  - Returns S_FALSE if item is not in the cache.
    //  - Touches the item
    //
    STDMETHOD (Retrieve)(THIS_ tCacheItemID id, void ** ppResult) PURE;

    //
    // Retrieve and remove item from the cache.
    //
    STDMETHOD (Remove)(THIS_ tCacheItemID id, void ** ppResult) PURE;

    //
    // Explicitly mark an item as "recently used"
    //
    STDMETHOD (Touch)(THIS_ tCacheItemID id) PURE;

    //
    // Explicitly mark an item as "least recently used"
    //
    STDMETHOD (Age)(THIS_ tCacheItemID id) PURE;

    //
    // Remove an item from the cache
    //
    STDMETHOD (Flush)(THIS_ tCacheItemID id) PURE;

    //
    // Empty the cache
    //
    STDMETHOD (FlushAll)(THIS) PURE;

    //
    // Dump the oldest items to free the specified amount of memory
    //
    STDMETHOD_(ulong, Purge)(THIS_ ulong nBytes) PURE;

};

///////////////////////////////////////

#define ICache_QueryInterface(p, a, b)      COMQueryInterface(p, a, b)
#define ICache_AddRef(p)                    COMAddRef(p)
#define ICache_Release(p)                   COMRelease(p)

///////////////////////////////////////////////////////////////////////////////
//
// Shared Cache API
//

enum eCacheMsgEnum
{
    // The item is falling of the LRU and client should delete.  Return S_FALSE to block
    kCM_DeleteOnAge,

    // The item is being explicitly flushed and client should delete.
    kCM_DeleteOnFlush,

    // The item should be dumped to the mono screen. Return S_FALSE for default handling
    kCM_MonoDump,
};

struct sCacheMsg
{
    eCacheMsg    message;
    void *       pClientContext;
    tCacheItemID itemId;
    void *       pItem;
    ulong        itemSize;
};

typedef HRESULT (LGAPI * tCacheCallbackFunc)(const sCacheMsg *);

///////////////////////////////////////
//
// STRUCT: sCacheClientDesc.  Used to describe cache clients.
//

struct sCacheClientDesc
{
    // The unique id of the client
    const GUID *       pID;

    // A client-defined context value
    void *             pContext;

    // Client's cache handler callback
    tCacheCallbackFunc pfnCallback;

    // Clients self-proscribed limits.
    ulong              nMaxBytes;                // -1 for no limit
    ulong              nMaxItems;                // -1 for no limit

    // Flags, unused but _must_ be set to zero
    unsigned           flags;
};

///////////////////////////////////////
//
// STRUCT: sCacheStats. Used to obtain information about the cache usage.
//

struct sCacheStats
{
    ulong reserved;

    ulong memoryLoad;    // percent of memory in use
    ulong totalPhys;     // bytes of physical memory
    ulong availPhys;     // free physical memory bytes
    ulong totalPageFile; // bytes of paging file
    ulong availPageFile; // free bytes of paging file
    ulong totalVirtual;  // user bytes of address space
    ulong availVirtual;  // free user bytes

    ulong allocCap;
    ulong totalMalloc;
    ulong lockedMalloc;
    ulong cachedMalloc;
};

///////////////////////////////////////
//
// INTERFACE: ISharedCache
//

#undef INTERFACE
#define INTERFACE ISharedCache

DECLARE_INTERFACE_(ISharedCache, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Query the state of the cache
    //
    STDMETHOD_(void, GetState)(THIS_ sCacheState *) PURE;

    //
    // Add a client to the caching system.  Pointer argument is not stored.
    //
    STDMETHOD (AddClient)(THIS_ const sCacheClientDesc * pClientDesc, ICache ** ppResult) PURE;

    //
    // Find a client in the caching system.
    //
    STDMETHOD (FindClient)(THIS_ const GUID * pID, ICache ** ppResult) PURE;

    //
    // Mark an whole client set as "recently used"
    //
    STDMETHOD (Touch)(THIS_ const GUID * pID) PURE;

    //
    // Mark an whole client set as "least recently used"
    //
    STDMETHOD (Age)(THIS_ const GUID * pID) PURE;

    //
    // Flush a whole client set
    //
    STDMETHOD (Flush)(THIS_ const GUID * pID) PURE;

    //
    // Empty the cache
    //
    STDMETHOD (FlushAll)(THIS) PURE;

    //
    // Dump the oldest items to free the specified amount of memory
    //
    STDMETHOD_(ulong, Purge)(THIS_ ulong nBytes) PURE;

    //
    // Get information about the cache usage
    //
    STDMETHOD_(void, GetStats)(THIS_ sCacheStats *pStats) PURE;

};

///////////////////////////////////////

#define ISharedCache_QueryInterface(p, a, b)    COMQueryInterface(p, a, b)
#define ISharedCache_AddRef(p)                  COMAddRef(p)
#define ISharedCache_Release(p)                 COMRelease(p)

///////////////////////////////////////////////////////////////////////////////

#endif /* !__CACHEAPI_H */
