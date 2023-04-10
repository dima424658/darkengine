/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/namedres/RCS/resapi.h 1.15 1999/04/13 12:09:00 Justin Exp $
//
// -------------------
// Resource System API
// -------------------
//
// Description of interfaces:
// 
// IResMan 
//    
//    The heart of the resource system. The resource manager allows streams
//    on disk to be bound to in-memory resources, and provides more or less
//    invisible memory management on those resources, keeping them in an
//    LRU queue for later re-use. It keeps track of the resource types that
//    have been installed, and provides a convenient interface for
//    translating disk formats into memory types. It provides a mask over
//    the storage management, so that the application doesn't have to worry
//    about all the grody details.
// 
//  IRes
// 
//    The memory representation of disk data. Each resource is associated
//    with a stream, and provides convenient methods for reading that
//    stream in and translating it into the appropriate memory format.
//    Works hand-in-hand with the resource manager to deal with issues
//    like memory management. Lets the app access the resource's data in
//    several different ways, including asynchronous loading. Derives from
//    the generic IDataSource interface.
//
//  IResControl
//
//    A side-interface of IRes; these are methods mainly used in controlling
//    and managing the resource, and are primarily of interest to the
//    resource manager.
// 
//  IResType
//
//    Defines a specific "type" of data. You create an IRes from an
//    IResType, and the type remains associated with the IRes.
//
//  IResMemOverride
//    
//    An optional interface that can be attached to an IRes aggregate 
//    when IResFactory creates it. Provides special memory handling 
//    abilities.
//

 
#ifndef __RESAPI_H
#define __RESAPI_H

#include <lg.h>
#include <comtools.h>

// For IDataSource:
#include <datasrc.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IResMan);
F_DECLARE_INTERFACE(IResStats);
F_DECLARE_INTERFACE(IRes);
F_DECLARE_INTERFACE(IResControl);
F_DECLARE_INTERFACE(IResType);
F_DECLARE_INTERFACE(IResMemOverride);
F_DECLARE_INTERFACE(IStore);
F_DECLARE_INTERFACE(IStoreHierarchy);
F_DECLARE_INTERFACE(IStoreFactory);
F_DECLARE_INTERFACE(IStoreManager);
F_DECLARE_INTERFACE(IStoreStream);
F_DECLARE_INTERFACE(ISearchPath);

// Sorry, can't mix old 16-bit Windows API and this API
#undef UnlockResource

///////////////////////////////////////////////////////////////////////////////
//
// Debugging tools
//

#ifndef SHIP
EXTERN BOOL g_fResPrintAccesses;
EXTERN BOOL g_fResPrintDrops;
#define ResSetPrintAccesses(b) g_fResPrintAccesses = (b)
#define ResSetPrintDrops(b)    g_fResPrintDrops = (b)
#else
#define ResSetPrintAccesses(b)
#define ResSetPrintDrops(b)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Create the resource system and add it to the global app-object
//

#define Res2Create() \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _Res2Create(IID_TO_REFIID(IID_IResMan), NULL, pAppUnknown); \
    COMRelease(pAppUnknown); \
}

//
// Creates a Resource Manager, aggregating it with specfied pOuter,
//
EXTERN tResult LGAPI
_Res2Create(REFIID, IResMan ** ppResMan, IUnknown * pOuter);


///////////////////////////////////////////////////////////////////////////////
//
// Callback types
//

///////////////////////////////////////
//
// Block extraction callback
//
// pResource is NOT AddRef'd. Like IStream, returns buffer, bytes, and block index
//

typedef long (*tResBlockCallback)(IRes * pResource, 
                                  void * pBuf, 
                                  long   nNumBytes, 
                                  long   nIx, 
                                  void * pClientData);

///////////////////////////////////////
//
// Resource from a storage callback.
//
// This is called by IResMan::PrepAllResources; it is called once for
// each resource found in the given storage. It is up to this callback
// to decide whether it is interested in the resource, and to Lock it
// if so.
//
typedef void (*tStoredResCallback)(IRes *pRes,
                                   IStore *pStore,
                                   void *pClientData);

///////////////////////////////////////
//
// The callback from IResType::EnumerateExts().
//
typedef void (*tResEnumExtsCallback)(const char *pExt, 
                                     IResType *,
                                     void *pClientData);

///////////////////////////////////////

//
// Flags for MakeResource
//
#define RES_IS_STATIC 0x01

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IResMan
//
// Weakness: Association between IRes implemenations and installed types.
//
// Multiple calls to "Open" with the same file will create new 
// instances of an IStore - its up to the IStoreFactory 
// implementation to ensure this. There is transient data 
// associated with a StructuredStorage in the form of the storage 
// name and location.
//

#undef INTERFACE
#define INTERFACE IResMan

DECLARE_INTERFACE_(IResMan, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Get a new ISearchPath, which can be set up to look for
   // resources along. If pPath is non-NULL, it should contain an initial
   // path for this search path, in the same form as you would pass to
   // ISearchPath::AddPath().
   //
   STDMETHOD_(ISearchPath *, NewSearchPath)
      (THIS_ const char *pPath DEFAULT_TO(NULL)) PURE;

   //
   // Set the default search path to be used for opening storages.
   // This can be ignored if you always pass in explicit paths to
   // the appropriate methods. This will make a *reference* to the
   // given path; changes to the path will be immediately reflected.
   // Clear the default path by calling this with NULL.
   //
   STDMETHOD_(void, SetDefaultPath)(THIS_ ISearchPath *pPath) PURE;

   //
   // Set the default context path for new search paths. This should
   // generally be used to set a "global context path" -- the roots that
   // everything lives underneath. All new search paths will have this
   // as their context from then on. (You may use ISearchPath::SetContext
   // to change that on a case-by-case basis.) This should usually be
   // called very early on, generally before creating any other paths.
   //
   STDMETHOD_(void, SetGlobalContext)(THIS_ ISearchPath *pPath) PURE;

   //
   // Set the default variants for new search paths. This is usually
   // used for internationalization -- if, for instance, this is the
   // German version of a game, you might set up a default variant of
   // "german;." for all paths.
   //
   STDMETHOD_(void, SetDefaultVariants)(THIS_ ISearchPath *pPath) PURE;

   //
   // Given the name and type of a resource on disk, this readies that
   // resource for use. Note that you do not have to give the extension
   // of the file on disk; that will be derived from the type. (This is
   // useful so you can ignore, eg, whether an Image is actually a GIF
   // or a BMP.)  If a search path is specified, that will be searched
   // for the resource; otherwise, the default will be used.
   //
   // Binding a resource involves two steps: getting the storage that it
   // is contained within, and binding the resource within that storage.
   // If you need finer-grained control over the process, use the GetStore
   // and BindSpecific methods below. After the resource is bound, you may
   // Lock it when you actually want to get at its data.
   //
   // Note that the disk image is generally not loaded as part of a Bind
   // operation. We load from disk when we need it, generally during a
   // Lock.
   //
   // Bind can take flags controlling its behaviour. Currently,
   // the only flag allowed is RES_IS_STATIC. If this is set, then this
   // resource will be fixed to the given storage; if not, the resource
   // could be rebound to a different storage later, if GlobalRebind is
   // called.
   //
   // You may specify a relative path for the resource, which is relative
   // to the given search path. This may either be specified separately in
   // pRelPath (which defaults to NULL in C++), or as part of pName
   // (that is, pName could be "foo\bar\blotz"). Some apps will not need
   // to use relative paths like this, but the capability is there if
   // needed. It is not legal for both pName and pRelPath to contain a
   // relative path.
   //
   // Returns the IRes if it could be found and loaded, or NULL if not.
   //
   STDMETHOD_(IRes *, Bind) (THIS_
                             const char *pName,
                             const char *pTypeName,
                             ISearchPath *pPath,
                             const char *pRelPath DEFAULT_TO(NULL),
                             const uint fBindFlags DEFAULT_TO(0)) PURE;

   //
   // Bind all of the resources whose names match the given pattern,
   // found along the given path, with the given type. If the pattern
   // is NULL, then prep all resources of that type.
   //
   // For each resource found, the given callback will be called. It is
   // up to that callback to AddRef the resource if it is interested in it;
   // no guarantees are made that the storage or resource will be kept
   // around afterward otherwise. Note that the resource will not necessarily
   // be loaded into memory at this point; it will be just as if Bind
   // had been called on it.
   //
   // If pRelPath is specified, then that specifies the relative path of
   // a storage *under* each storage in this path to look in. So if the
   // given path is the global context path, and pRelPath is "fam\dungeon",
   // then the contents of each "fam\dungeon" directory will be bound,
   // under each node of the path.
   //
   // pClientData is a pointer to arbitrary memory, and can be used by
   // the caller to pass information through to the callback.
   //
   STDMETHOD_(void, BindAll) (THIS_
                              const char *pPattern,
                              const char *pTypeName,
                              ISearchPath *pPath,
                              tStoredResCallback,
                              void *pClientData,
                              const char *pRelPath DEFAULT_TO(NULL),
                              const uint fBindFlags DEFAULT_TO(0)) PURE;

   //
   // The underlying mechanism, which binds a specific stream to a resource.
   //
   // This takes a particular storage, and an unqualified name. Assuming
   // that the stream indicated by "pStore/pName" exists, this will
   // create a resource for it. Note that the name is the name of a real
   // stream on disk -- in general, it must have the file extension. (As
   // opposed to the usual case in Bind, where you don't bother with the
   // file extension, instead inferring it from the type.)
   //
   // The canonical store of the resource will be whatever you specify, and
   // does not affect the actual binding; it is there for later lookups.
   // This is the storage used to compute the canonical name of the resource,
   // and may be different from the real storage, especially if the resource
   // actually comes from a variant.
   //
   // This is a low-level method, intended for cases where the app really
   // wants to bind a resource to a specific stream on disk. Usually, you
   // should use Bind instead.
   //
   STDMETHOD_(IRes *, BindSpecific) (THIS_
                                     const char *pName,
                                     const char *pTypeName,
                                     IStore *pStore,
                                     IStore *pCanonStore,
                                     const uint fBindFlags) PURE;

   //
   // This takes an existing IRes, and creates a new one with the same
   // underlying stream, but the specified type.
   //
   STDMETHOD_(IRes *, Retype) (THIS_
                               IRes *pOldRes,
                               const char *pTypeName,
                               const uint fBindFlags) PURE;

   //
   // If a resource with the given name is known (that is, it has been
   // found by a variant of Bind), then this returns a pointer to
   // that resource. 
   //
   // You may specify the canonical path for this resource, either in
   // pCanonPath or as part of pName, just as Bind allows for the relative
   // path. Note that this canonical path is generally the path from the
   // context root that this resource was found under, and may not be the
   // same as the relative path passed into Bind. If the canonical path is
   // omitted, the name is potentially ambiguous, and Lookup will return
   // an arbitrary resource with the given name and type. (Which is fine
   // if you only have one resource by that name and type.)
   //
   // Note that you can always obtain the IRes by calling Bind instead;
   // we internally guarantee that you will always get an existing resource
   // if there is one. Bind is sometimes easier to use if you have the
   // ISearchPath that this thing came from, and don't want to worry about
   // getting the canonical path right.
   //
   STDMETHOD_(IRes *, Lookup) (THIS_ 
                               const char *pName,
                               const char *pTypeName,
                               const char *pCanonPath DEFAULT_TO(NULL)) PURE;

   // 
   // Tries to get the named storage. To get a storage along a search path,
   // use ISearchPath::Find instead. Returns NULL if this storage can't be
   // found.
   //
   STDMETHOD_(IStore *, GetStore) (THIS_ 
                                   const char *pPathName) PURE;

   ////////////////////////////////////
   //
   // IStores are created through an IStoreFactory which is registered
   // here; it decides which Factory to use based on the extension of the
   // file. Similarly, IRes are created through an IResType, which have
   // associated extensions (but whose extensions may overlap).
   //
   // By default, RESTYPE_BINARY is pre-registered; applications don't need
   // to register it. Also, the basic directory/file storage factory is
   // pre-registered.
   //
   // Returns TRUE iff the given factory is invalid or unuseable; otherwise
   // returns FALSE.
   //
   STDMETHOD_(BOOL, RegisterResType) (THIS_ IResType *) PURE;
   STDMETHOD_(BOOL, RegisterStoreFactory) (THIS_ IStoreFactory *) PURE;

   //
   // Unregister a factory. Note that storage factories cannot currently
   // be unregistered.
   //
   STDMETHOD_(void, UnregisterResType) (THIS_ IResType *) PURE;

   //////////
   //
   // Refresh methods
   //
   // These methods are intended for times when you have reason to believe
   // that the outside world has changed. These take effect on a resource
   // the next time that resource qualifies for a refresh, which is when
   // it is fully Unlocked, and then a Lock is requested. The Resource
   // Manager will first check the binding of the resource, and see if this
   // resource has appeared at a new, earlier location on the context path.
   // (It will be at the same canonical path and variant on the context path,
   // though.) If it is still in the same location, it will check the
   // timestamp of the underlying stream. If there is reason to believe that
   // this resource has changed on disk, it will be reloaded.
   //
   // Note that this does *not* force a reload; it does it if one
   // appears to be appropriate. To force a resource to be reloaded, make
   // sure it is fully Unlocked, then Drop it. To force it to be rebound,
   // Release the IRes, and then call Bind again. Usually, Refresh is good
   // enough, though.
   //

   //
   // Refresh a single resource.
   //
   STDMETHOD_(void, MarkForRefresh) (THIS_ IRes *pRes) PURE;

   //
   // Mark all the resources in the system to refresh. Note that this
   // will only actually change the resources that appear to have changed
   // on disk. It is intended to provide an easy way for the application to
   // implement a "recheck the world" button.
   //
   STDMETHOD_(void, GlobalRefresh) (THIS) PURE;
};

//////////////////////////////////////////////////////////////////////
//
// INTERFACE: IResStats
//
// This interface is optionally available from IResMan; if available, it
// provides various statistics about the Resource Manager.
//

#undef INTERFACE
#define INTERFACE IResStats

//
// Modes that can be turned on and off in IResStats::SetMode.
//
enum eResStatMode
{
   // Turn all statistics on or off
   kResStatMode_All,
   // Report just the top-level summary
   kResStatMode_Summary,
   // Report on usage by type
   kResStatMode_Types,
   // Report on usage by canonical path
   kResStatMode_CanonPath,
   // Report on every individual resource (requires lots of memory)
   kResStatMode_Res,
   // Report on the most-used resources (requires lots of memory)
   kResStatMode_TopHits,
   kResStatMode_NumModes,
   
   kResStats_IntMax = 0xffffffff                 // ensure 32 bits
};

typedef enum eResStatMode eResStatMode;

DECLARE_INTERFACE_(IResStats, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Enable stat tracking.
   //
   STDMETHOD_(void, SetMode) (THIS_ 
                              const eResStatMode,
                              BOOL bTurnOn) PURE;

   //
   // Dump stats into given file, or monolog if none.
   //
   STDMETHOD_(void, Dump) (THIS_ const char *pFile) PURE;

   //
   // Dump a table of information about all extant resources into the
   // given file, or monolog if none. Table contains canonical pathnames,
   // type, and both external and internal locks. (The resource is not
   // in memory if both lock counts are zero.)
   //
   STDMETHOD_(void, DumpSnapshot) (THIS_ const char *pFile) PURE;
};

//////////////////////////////////////////////////////////////////////
//
// INTERFACE: IResMem
//
// This interface controls the memory-management policies of the
// Resource Manager.
//

#undef INTERFACE
#define INTERFACE IResMem

DECLARE_INTERFACE_(IResMem, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Enable paging.
   //
   STDMETHOD_(void, EnablePaging) (THIS_ BOOL) PURE;

   //
   // Flush the cache, cleanup the heap.
   //
   STDMETHOD_(void, Compact) (THIS) PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IRes
//
// This is primarily the methods that are likely to be interesting to
// applications. The methods for building and manipulating resources are
// principally under IResControl.
//

#undef INTERFACE
#define INTERFACE IRes

#define NO_RES_APP_DATA -1

DECLARE_INTERFACE_(IRes, IDataSource)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //////////
   //
   // IDataSource Methods
   //
   // These need to come first, so our class signature matches IDataSource
   // correctly.
   //

   //
   // Will look in LRU or will load. Allocates buff.
   // Returns NULL if for some reason it can't lock this resource. (Eg, it
   // doesn't exist in storage.)
   //
   STDMETHOD_(void *, Lock)(THIS) PURE;

   //
   // Decrement lock count
   //
   STDMETHOD_(void, Unlock)(THIS) PURE;

   //////////
   //
   // Distinctly IRes Methods
   //

   //
   // Get the lock count
   //
   STDMETHOD_(unsigned, GetLockCount)(THIS) PURE;

   //
   // Returns the type for this resource; use this to get the type name,
   // and translators.
   //
   STDMETHOD_(IResType *, GetType) (THIS) PURE;

   //
   // Returns the "name" of this resource. This is usually the name of the
   // underlying stream, without the file extension.
   //
   STDMETHOD_(const char *, GetName)(THIS) PURE;

   //
   // Returns the canonical path of this resource. This is a partial path
   // that helps disambiguate this resource. It is usually the relative path
   // for this resource, under the context root that it was found under.
   // It is the responsibility of the caller to Free the returned string.
   //
   STDMETHOD_(void, GetCanonPath)(THIS_ char **ppPath) PURE;

   //
   // Convenience method: gets the name of the resource, concatenated
   // onto the canonical path. ppPathname should be the address of a
   // string pointer; it will be filled with a Malloc'ed string, and it
   // is the caller's responsibility to Free it. It will be left as NULL
   // iff this resource has not been properly initialized.
   //
   STDMETHOD_(void, GetCanonPathName)(THIS_ char **ppPathname) PURE;

   //
   // The canonical storage for this resource. This is the storage that
   // corresponds to the canonical path.
   //
   STDMETHOD_(IStore *, GetCanonStore)(THIS) PURE;

   //
   // Get the variant of this resource that was actually loaded. This is
   // generally used for things like internationalization, where we may
   // have several variants of the same resource, but treat them largely
   // the same. It is the responsibility of the caller to Free the
   // returned string.
   //
   STDMETHOD_(void, GetVariant)(THIS_ char **ppVariantPath) PURE;

   //
   // Get the storage that this resource came from. This is mainly useful
   // if you are dealing with multiple resources -- say, if you found one
   // resource, and need to load another related one from the same storage.
   // In this case, you would GetStore() from the original, and then use
   // BindSpecific() to bind the related one.
   //
   STDMETHOD_(IStore *, GetStore)(THIS) PURE;

   //
   // Gets the name of the underlying stream for this resource.
   // Semantics are variable if this resource does not correspond to a single
   // stream on disk. ppPathname should be the address of a string pointer;
   // it will be filled with a Malloc'ed string, and it is the caller's
   // responsibility to Free it. It will be left as NULL iff this resource
   // has not been properly initialized. If bFullpath is TRUE, then the
   // returned value will be the absolute path of the stream; otherwise,
   // it will just be the name.
   //
   STDMETHOD_(void, GetStreamName)(THIS_ 
                                   BOOL bFullpath,
                                   char **ppPathname) PURE;

   ////////////////////////////////////
   //
   // Raw data operations
   //

   //
   // Get the data for this resource, without loading or locking.
   //
   // *If* this resource is currently loaded into memory, this returns a
   // pointer to that resource's data. This pointer can only be considered
   // valid as long as the resource is Locked; it will cease to be valid at
   // an unpredictable time after that. Do not use unless you really know
   // what you are doing! (This method is particularly dangerous in a
   // multithreaded context: if the resource is Unlocked in another thread,
   // the data could go away in the middle of an operation. In general, this
   // should only be used for brief peeks at the data of a resource which is
   // Locked in the long term.)
   //
   // If the resource's data is not currently loaded in memory, this will
   // return NULL.
   //
   STDMETHOD_(void *, DataPeek)(THIS) PURE;

   //
   // Remove from memory. Blow LRU for this. TRUE if success.
   // FALSE if resource is locked.
   //
   STDMETHOD_(BOOL, Drop)(THIS) PURE;

   //
   // Query the size
   //
   STDMETHOD_(long, GetSize)(THIS) PURE;

   //
   // Extract into memory.
   //
   STDMETHOD_(void *, Extract)(THIS_ void * pBuf) PURE;

   //
   // Get data into memory (LRU). If already there, makes data 
   // most-recently-used.
   //
   STDMETHOD_(void *, PreLoad)(THIS) PURE;

   //
   // Grab data from file. If data already loaded for this resource, 
   // no file ops are done. Else, if associated stream is not already 
   // opened, then the stream is opened, data read, then stream 
   // closed. If stream is already open via 'OpenStream', then that 
   // stream is used, and not closed until explicitly closed via 
   // 'CloseStream'. Returns number of bytes extracted
   // 
   STDMETHOD_(int, ExtractPartial)(THIS_ 
                                   const long nStart, 
                                   const long nEnd, 
                                   void *pBuf)
      PURE;

   //
   // Extract this resource, one block at a time, passing each block to
   // the specified callback method. The blocks are placed in the given
   // buffer; it is the caller's responsibility to manage that buffer.
   //
   // @param pBuf A buffer in which to place each block, which will be
   //   passed to the callback.
   // @param nSize The size of that buffer, in bytes.
   // @param callback A routine to call each time a block is loaded into
   //   the buffer.
   // @param pCallbackData Some arbitrary data which will be passed to the
   //   callback, for the caller to use as it sees fit.
   //
   STDMETHOD_(void, ExtractBlocks)(THIS_ 
                                   void *pBuf, 
                                   const long nSize, 
                                   tResBlockCallback, 
                                   void *pCallbackData) 
      PURE;

   ////////////////////////////////////
   //
   // Async operations
   //

   //
   // Lock (that is, read in and get the data for) the given resource 
   // asynchronously.
   //
   // Priority is a 'prikind.h' priority.
   //
   STDMETHOD_(BOOL, AsyncLock)(THIS_ const int nPriority) PURE;
   //
   // Similar to AsyncLock, but place the data into the given buffer.
   // Memory management on this buffer is up to the caller.
   //
   STDMETHOD_(BOOL, AsyncExtract)(THIS_ 
                                  const int nPriority, 
                                  void *pBuf, 
                                  const long bufSize) PURE;
   //
   // Similar to AsyncLock, but doesn't actually lock the Resource, just
   // loads it in on the assumption that it'll get locked Real Soon Now.
   // Assumed to be low-priority. Note that IsAsyncFulfilled and
   // GetAsyncResult are not meaningful for AsyncPreload, and should
   // not be used.
   //
   STDMETHOD_(BOOL, AsyncPreload)(THIS) PURE;
   //
   // Returns TRUE if async request has been fulfilled.
   //
   STDMETHOD_(BOOL, IsAsyncFulfilled)(THIS) PURE;
   //
   // Kills async request associated with this resource.
   //
   STDMETHOD(AsyncKill)(THIS) PURE;
   // 
   // Forces synchronous completion of async request. ppResult is as
   // passed back from the ARQ fulfiller.
   //
   STDMETHOD(GetAsyncResult)(THIS_ void **ppResult) PURE;

   //
   // Application Data methods
   //
   // Sometimes, the application wants to attach a bit of data to its
   // resources; usually, but not always, this is a pointer of some sort.
   // These methods allow you to do this, giving a DWORD that will be
   // associated with this resource until further notice. (To unset this
   // data, just call SetAppData again with NO_RES_APP_DATA.) 
   //
   // Note that the resource system absolutely ignores this data. This
   // is significant, because if the IRes is completely Released, it will
   // delete itself without performing any cleanup on this data. If the
   // the data is a pointer to a structure, it is the responsibility of
   // the application to clean it up *before* completely releasing the
   // resource. (This *may* get enhanced in the future, but no promises.)
   //
   // The "empty" AppData has the value NO_RES_APP_DATA -- that is, this is
   // what GetAppData will return if SetAppData has not been called on
   // this resource.
   //
   STDMETHOD_(void, SetAppData)(THIS_ DWORD AppData) PURE;
   STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
   
};

//////////////////////////////////////////////////////////////////////
//
// INTERFACE: IResControl
//
// This is a side-interface from IRes; it collects the low-level functionality
// which is really more interesting to the Resource Manager than to the
// applications.
//

#undef INTERFACE
#define INTERFACE IResControl

DECLARE_INTERFACE_(IResControl, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Set the structured storage 
   //
   STDMETHOD_(BOOL, SetStore)(THIS_ IStore *) PURE;

   //
   // Set the name of the resource under its storage
   //
   STDMETHOD_(void, SetName)(THIS_ const char *pName) PURE;

   //
   // If FALSE, only the first call to SetStore will stick. May be useful
   // if you don't want this resource getting looked up again if the state
   // of the disk changes.
   //
   STDMETHOD_(void, AllowStorageReset)(THIS_ BOOL) PURE;

   //
   // Set the canonical storage for this resource. This is the storage
   // that this resource "officially" comes from, not counting variants.
   // It will be used to compute the canonical path, and the Variant is
   // computed as the difference between the real storage and this.
   //
   STDMETHOD_(void, SetCanonStore)(THIS_ IStore *pCanonStore) PURE;

   //
   // Grab data, allocate, rework as needed. Will get used by resource
   // manager to load raw data. Timestamp is the last modification time
   // of the data. pSize and pTimestamp may be NULL.
   //
   STDMETHOD_(void *, LoadData)(THIS_ 
                                ulong * pSize,
                                ulong * pTimestamp,
                                IResMemOverride * pResMem) PURE;

   //
   // Free the data. This will be called by the resource system when it
   // wants this data for the resource to the removed. It is essentially
   // the inverse of LoadData. Returns TRUE iff the data was freed.
   //
   STDMETHOD_(BOOL, FreeData)(THIS_ 
                              void *pData,
                              ulong nSize,
                              IResMemOverride *pResMem) PURE;

   //  
   // Opens a stream using the IStore and resource name that were
   // established already.
   //
   STDMETHOD_(IStoreStream *, OpenStream)(THIS) PURE;

   ////////////////////////////////////
   //
   // Translation methods
   //
   // A resource may be able to obtain its data from other, already-loaded
   // resources based on the same underlying stream. The issue is whether
   // this type knows how to translate the data from another existing look
   // at this data, in some other type.

   //
   // Returns an array of strings, with the names of types that this one
   // knows how to translate from. Places the size of the array in
   // pnTypes.
   //
   // @NOTE: This used to be GetProxiedTypes, with a different signature.
   //
   STDMETHOD_(char **, GetTranslatableTypes)(THIS_
                                             /* OUT */ int *pnTypes) PURE;

   //
   // Load this resource from the given data block, which is from another
   // resource. Analogous to LoadData in many ways. The parameters are:
   //
   //   pOldData -- An existing block of data
   //   nOldSize -- The size of that old data block
   //   pOldTypeName -- The name of the type of that old data
   //   pAllocated -- Will return TRUE iff we allocated fresh memory for
   //       this resource, FALSE iff we are simply pointing to the old,
   //       given data block
   //   pSize -- Will return the size of the new data for this resource
   //   pResMem -- Will be used to allocate the new memory space, if
   //       any allocation is needed
   //
   // The method returns a pointer to the new data block for this resource.
   // It will return NULL if something goes wrong. (For instance, if we don't
   // know how to translate from the given data type.)
   //
   // @NOTE: This used to be LoadProxiedData, with a different signature.
   //
   STDMETHOD_(void *, LoadTranslation)(THIS_
                                       void *pOldData,
                                       long nOldSize,
                                       const char *pOldTypeName,
                                       /* OUT */ BOOL *pAllocated,
                                       /* OUT */ ulong *pSize,
                                       IResMemOverride *pResMem) PURE;

   //
   // Resource Manager Data methods
   //
   // The resource manager may assign some arbitrary data to this resource,
   // for efficiency reasons. Functionally identical to Set/GetAppData.
   //
   // In general, applications shouldn't touch this.
   //
   STDMETHOD_(void, SetManData)(THIS_ DWORD ManData) PURE;
   STDMETHOD_(DWORD, GetManData)(THIS) PURE;

   //
   // @TBD (justin 5-7-98): We will probably eventually need a bit
   // more interface here for the output side of the equation, so a
   // tool can create a resource and add it to a storage...
   //
};

//////////////////////////////////////////////////////////////////////
//
// INTERFACE: IResType
//
// A "type" encapsulates a particular family of resources. It serves as
// a factory for that kind of resource -- you call CreateRes () to get
// a new resource of this type, or CreateResFromRes () to create a new
// one based on the data of an existing one.
//

#undef INTERFACE
#define INTERFACE IResType

DECLARE_INTERFACE_(IResType, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Get the name of this type. This is an arbitrary static string,
   // which should ideally be unique.
   //
   STDMETHOD_(const char *, GetName) (THIS) PURE;

   //
   // Get the extensions that this type supports. This will call the callback
   // once for each supported extension.
   //
   STDMETHOD_(void, EnumerateExts) (THIS_ 
                                    tResEnumExtsCallback callback,
                                    void *pClientData) PURE;

   //
   // Return TRUE iff the given extension is legal for this type. Arguably
   // redundant with EnumerateExts, but it is useful for efficiency reasons
   // to have both.
   //
   STDMETHOD_(BOOL, IsLegalExt) (THIS_ const char *pExt) PURE;

   //
   // Create a new resource of this type from the specified IStore, with
   // the given name. If ppResMem is non-null, it gives the memory
   // manager for this data. Returns NULL iff it can't create the IRes
   // for some reason. It should make sure that the resource knows its
   // name and storage, from the params.
   //
   // Note that this does not actually read any data in, or even open
   // up the stream in the IStore; it just sets things up so that later
   // operations can do so.
   //
   STDMETHOD_(IRes *, CreateRes) (THIS_
                                  IStore *pStore,
                                  const char *pName,
                                  const char *pExt,
                                  IResMemOverride **ppResMem) PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IResMemOverride
//

#undef INTERFACE
#define INTERFACE IResMemOverride

DECLARE_INTERFACE_(IResMemOverride, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   // @TBD (justin, 5-1-98): Is there any good reason to use special names
   // for these methods? It seems more sensible to me to just call them
   // Malloc and Free...

   //
   //
   //
   STDMETHOD_(void *, ResMalloc) (THIS_ ulong nNumBytes) PURE;

   //
   //
   //
   STDMETHOD_(void, ResFree) (THIS_ void *) PURE;

   //
   //
   //
   STDMETHOD_(ulong, GetSize) (THIS_ void *) PURE;
};

//////////////////////////////////////////////////////////////////////////
//
// C Accessor macros
//

#define IResMan_NewSearchPath(p, a)            COMCall1(p, NewSearchPath, a)
#define IResMan_SetDefaultPath(p, a)           COMCall1(p, SetDefaultPath, a)
#define IResMan_SetGlobalContext(p, a)         COMCall1(p, SetGlobalContext, a)
#define IResMan_SetDefaultVariants(p, a)       COMCall1(p, SetDefaultVariants, a)
#define IResMan_Bind(p, a, b, c, d, e)         COMCall5(p, Bind, a, b, c, d, e)
#define IResMan_BindAll(p, a, b, c, d, e, f, g) COMCall7(p, BindAll, a, b, c, d, e, f, g)
#define IResMan_BindSpecific(p, a, b, c, d, e) COMCall5(p, BindSpecific, a, b, c, d, e)
#define IResMan_Retype(p, a, b, c)             COMCall3(p, Retype, a, b, c)
#define IResMan_Lookup(p, a, b, c)             COMCall3(p, Lookup, a, b, c)
#define IResMan_GetStore(p, a)                 COMCall1(p, GetStore, a)
#define IResMan_RegisterResType(p, a)          COMCall1(p, RegisterResType, a)
#define IResMan_RegisterStoreFactory(p, a)     COMCall1(p, RegisterStoreFactory, a)
#define IResMan_UnregisterResType(p, a)        COMCall1(p, UnregisterResType, a)
#define IResMan_MarkForRefresh(p, a)           COMCall1(p, MarkForRefresh, a)
#define IResMan_GlobalRefresh(p)               COMCall0(p, GlobalRefresh)

//////////

#define IResStats_SetMode(p, a, b)             COMCall2(p, SetMode, a, b)
#define IResStats_Dump(p, a)                   COMCall1(p, Dump, a)

//////////

#define IResMem_EnablePaging(p, a)             COMCall1(p, EnablePaging, a)
#define IResMem_Compact(p)                     COMCall0(p, Compact)

///////////////////////////////////////

#define IRes_Lock(p)                       COMCall0(p, Lock)
#define IRes_Unlock(p)                     COMCall0(p, Unlock)
#define IRes_GetLockCount(p)               COMCall0(p, GetLockCount)
#define IRes_GetType(p)                    COMCall0(p, GetType)
#define IRes_GetName(p)                    COMCall0(p, GetName)
#define IRes_GetCanonPath(p, a)            COMCall1(p, GetCanonPath, a)
#define IRes_GetCanonPathName(p, a)        COMCall1(p, GetCanonPathName, a)
#define IRes_GetCanonStore(p)              COMCall0(p, GetCanonStore)
#define IRes_GetVariant(p, a)              COMCall1(p, GetVariant, a)
#define IRes_GetStore(p)                   COMCall0(p, GetStore)
#define IRes_GetStreamName(p, a, b)        COMCall2(p, GetStreamName, a, b)
#define IRes_DataPeek(p)                   COMCall0(p, DataPeek)
#define IRes_Drop(p)                       COMCall0(p, Drop)
#define IRes_GetSize(p)                    COMCall0(p, GetSize)
#define IRes_Extract(p, a)                 COMCall1(p, Extract, a)
#define IRes_PreLoad(p)                    COMCall0(p, PreLoad)
#define IRes_ExtractPartial(p, a, b, c)    COMCall3(p, ExtractPartial a, b, c)
#define IRes_ExtractBlocks(p, a, b, c, d)  COMCall4(p, ExtractBlocks, a, b, c, d)
#define IRes_AsyncLock(p, a, b, c)         COMCall3(p, AsyncLock, a, b, c)
#define IRes_AsyncExtract(p, a, b, c, d, e) COMCall5(p, AsyncExtract, a, b, c, d, e)
#define IRes_AsyncPreload(p, a, b)         COMCall2(p, AsyncPreload, a, b)
#define IRes_IsAsyncFulfilled(p)           COMCall0(p, IsAsyncFulfilled)
#define IRes_AsyncKill(p)                  COMCall0(p, AsyncKill)
#define IRes_GetAsyncResult(p, a)          COMCall1(p, GetAsyncResult, a)
#define IRes_SetAppData(p, a)              COMCall1(p, SetAppData, a)
#define IRes_GetAppData(p)                 COMCall0(p, GetAppData)

//////////

#define IResControl_SetStore(p, a)         COMCall1(p, SetStore, a)
#define IResControl_SetName(p, a)          COMCall1(p, SetName, a)
#define IResControl_AllowStorageReset(p, a) COMCall1(p, AllowStorageReset, a)
#define IResControl_SetCanonStore(p, a)    COMCall1(p, SetCanonStore, a)
#define IResControl_LoadData(p, a, b)      COMCall2(p, LoadData, a, b)
#define IResControl_FreeData(p, a, b, c)   COMCall3(p, FreeData, a, b, c)
#define IResControl_OpenStream(p)          COMCall0(p, OpenStream)
#define IResControl_GetTranslatableTypes(p, a) COMCall1(p, GetTranslatableTypes, a)
#define IResControl_LoadTranslation(p, a, b, c, d, e, f) COMCall6(p, LoadTranslation, a, b, c, d, e, f)
#define IResControl_SetManData(p, a)       COMCall1(p, SetManData, a)
#define IResControl_GetManData(p)          COMCall0(p, GetManData)

///////////////////////////////////////

#define IResType_GetName(p)                COMCall0(p, GetName)
#define IResType_EnumerateExts(p, a, b)    COMCall2(p, EnumerateExts, a, b)
#define IResType_IsLegalExt(p, a)          COMCall1(p, IsLegalExt, a)
#define IResType_CreateRes(p, a, b, c, d)  COMCall4(p, CreateRes, a, b, c, d)

///////////////////////////////////////

#define IResMemOverride_ResMalloc(p, a)  COMCall1(p, ResMalloc, a)
#define IResMemOverride_ResFree(p, a)    COMCall1(p, ResFree, a)
#define IResMemOverride_ResGetSize(p, a) COMCall1(p, ResGetSize, a)

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__RESAPI_H */
