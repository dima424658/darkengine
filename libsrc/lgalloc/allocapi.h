///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/allocapi.h $
// $Author: ccarollo $
// $Date: 1999/02/11 11:48:14 $
// $Revision: 1.10 $
//
// Allocator APIs
//

#ifndef __ALLOCAPI_H
#define __ALLOCAPI_H

#include <comtools.h>

#undef Malloc
#undef Free
#undef Realloc

// If the source file didn't include objidl.h, we need to declare
// base allocator interfaces here
#ifndef __objidl_h__
#define DEFINE_BASE_ALLOC_INTERFACES
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef DEFINE_BASE_ALLOC_INTERFACES
struct IMalloc;
typedef struct IMalloc IMalloc;
EXTERN_C const GUID CDECL FAR IID_IMalloc;

struct IMallocSpy;
typedef struct IMallocSpy IMallocSpy;
EXTERN_C const GUID CDECL FAR IID_IMallocSpy;
#endif

struct IDebugMalloc;
typedef struct IDebugMalloc IDebugMalloc;
EXTERN_C const GUID CDECL FAR IID_IDebugMalloc;

///////////////////////////////////////////////////////////////////////////////
//
// Access the allocator directly
//

#ifndef SHIP
   #define IAllocator     IDebugMalloc
   #define IID_IAllocator IID_IDebugMalloc
#else
   #define IAllocator     IMalloc
   #define IID_IAllocator IID_IMalloc
#endif

EXTERN IAllocator * g_pMalloc;

///////////////////////////////////////////////////////////////////////////////

struct sAllocLimits
{
    ulong totalAlloc;
    ulong allocCap;
    ulong initAllocCap;

#ifndef SHIP
    ulong peakAlloc;
#endif
};

typedef struct sAllocLimits sAllocLimits;

///////////////////////////////////////

typedef ulong (LGAPI * tAllocatorPageFunc)(ulong needed, sAllocLimits * pLimits);

EXTERN BOOL LGAPI  AllocSetPageFunc(tAllocatorPageFunc);
EXTERN void LGAPI  AllocGetLimits(sAllocLimits *);
EXTERN ulong LGAPI AllocSetAllocCap(ulong);
EXTERN ulong LGAPI AllocPickAllocCap();

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACES: IMalloc, IMallocSpy (standard interfaces)
//

#ifdef DEFINE_BASE_ALLOC_INTERFACES
#undef INTERFACE
#define INTERFACE IMalloc

DECLARE_INTERFACE_(IMalloc, IUnknown)
{
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD_(void *, Alloc)   (THIS_ ULONG cb) PURE;
    STDMETHOD_(void *, Realloc) (THIS_ void * pv, ULONG cb) PURE;
    STDMETHOD_(void,   Free)    (THIS_ void * pv) PURE;
    STDMETHOD_(ULONG,  GetSize) (THIS_ void * pv) PURE;
    STDMETHOD_(int,    DidAlloc)(THIS_ void * pv) PURE;
    STDMETHOD_(void,   HeapMinimize)(THIS) PURE;
};

#undef INTERFACE
#define INTERFACE IMallocSpy

DECLARE_INTERFACE_(IMallocSpy, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD_(ULONG,  PreAlloc)        (ULONG cbRequest) PURE;
    STDMETHOD_(void *, PostAlloc)       (void * Actual) PURE;
    STDMETHOD_(void *, PreFree)         (void * Request, BOOL fSpyed) PURE;
    STDMETHOD_(void,   PostFree)        (BOOL fSpyed) PURE;
    STDMETHOD_(ULONG,  PreRealloc)      (void * Request, ULONG cbRequest,void ** pNewRequest, BOOL fSpyed) PURE;
    STDMETHOD_(void *, PostRealloc)     (void * Actual, BOOL fSpyed) PURE;
    STDMETHOD_(void *, PreGetSize)      (void * Request, BOOL fSpyed) PURE;
    STDMETHOD_(ULONG,  PostGetSize)     (ULONG cbActual, BOOL fSpyed) PURE;
    STDMETHOD_(void *, PreDidAlloc)     (void * Request, BOOL fSpyed) PURE;
    STDMETHOD_(int,    PostDidAlloc)    (void * Request, BOOL fSpyed, int fActual) PURE;
    STDMETHOD_(void,   PreHeapMinimize) (void) PURE;
    STDMETHOD_(void,   PostHeapMinimize)(void) PURE;

};
#endif

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IDebugMalloc
//

#ifndef SHIP
#undef INTERFACE
#define INTERFACE IDebugMalloc

DECLARE_INTERFACE_(IDebugMalloc, IMalloc)
{
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    //
    // IMalloc methods
    //
    STDMETHOD_(void *, Alloc)   (THIS_ ULONG cb) PURE;
    STDMETHOD_(void *, Realloc) (THIS_ void * pv, ULONG cb) PURE;
    STDMETHOD_(void,   Free)    (THIS_ void * pv) PURE;
    STDMETHOD_(ULONG,  GetSize) (THIS_ void * pv) PURE;
    STDMETHOD_(int,    DidAlloc)(THIS_ void * pv) PURE;
    STDMETHOD_(void,   HeapMinimize)(THIS) PURE;

    //
    // Debugging allocation functions
    //
    STDMETHOD_(void *, AllocEx)  (THIS_ ULONG cb, const char * pszFile, int line) PURE;
    STDMETHOD_(void *, ReallocEx)(THIS_ void * pv, ULONG cb, const char * pszFile, int line) PURE;
    STDMETHOD_(void,   FreeEx)   (THIS_ void * pv, const char * pszFile, int line) PURE;

    STDMETHOD (VerifyAlloc)(THIS_ void * pv) PURE;
    STDMETHOD (VerifyHeap)(THIS) PURE;

    STDMETHOD_(void, DumpHeapInfo)(THIS) PURE;

    STDMETHOD_(void, DumpStats)(THIS) PURE;
    STDMETHOD_(void, DumpBlocks)(THIS) PURE;
    STDMETHOD_(void, DumpModules)(THIS) PURE;

    // Force credit for allocations
    STDMETHOD_(void, PushCredit)(THIS_ const char * pszFile, int line) PURE;
    STDMETHOD_(void, PopCredit)(THIS) PURE;

};

#endif

#ifdef DEBUG
   #define LGALLOC_PUSH_CREDIT() do { if (g_pMalloc) g_pMalloc->PushCredit(__FILE__, __LINE__); } while (FALSE)
   #define LGALLOC_POP_CREDIT()  do { if (g_pMalloc) g_pMalloc->PopCredit(); } while (FALSE)
   #ifdef __cplusplus
      struct __cAutoAllocCredit
      {
         __cAutoAllocCredit(const char * pszFile, int line)
         {
            g_pMalloc->PushCredit(pszFile, line);
         }
         
         ~__cAutoAllocCredit()
         {
            g_pMalloc->PopCredit();
         }
      };
      #define LGALLOC_AUTO_CREDIT() __cAutoAllocCredit _AAC_ ## __LINE__ (__FILE__, __LINE__)
   #endif
#else
   #define LGALLOC_PUSH_CREDIT() 
   #define LGALLOC_POP_CREDIT()
   #define LGALLOC_AUTO_CREDIT()
#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__ALLOCAPI_H */