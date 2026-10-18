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
#if !defined(__IMalloc_INTERFACE_DEFINED__) || !defined(__IMallocSpy_INTERFACE_DEFINED__)
#define DEFINE_BASE_ALLOC_INTERFACES
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef DEFINE_BASE_ALLOC_INTERFACES
#ifndef __IMalloc_FWD_DEFINED__
#define __IMalloc_FWD_DEFINED__
struct IMalloc;
typedef struct IMalloc IMalloc;
EXTERN_C const GUID CDECL FAR IID_IMalloc;
#endif

#ifndef __IMallocSpy_FWD_DEFINED__
#define __IMallocSpy_FWD_DEFINED__
struct IMallocSpy;
typedef struct IMallocSpy IMallocSpy;
EXTERN_C const GUID CDECL FAR IID_IMallocSpy;
#endif
#endif

typedef IMalloc *LPMALLOC;
typedef IMallocSpy *LPMALLOCSPY;

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
    size_t totalAlloc;
    size_t allocCap;
    size_t initAllocCap;

#ifndef SHIP
    size_t peakAlloc;
#endif
};

typedef struct sAllocLimits sAllocLimits;

///////////////////////////////////////

typedef size_t (LGAPI * tAllocatorPageFunc)(size_t needed, sAllocLimits * pLimits);

EXTERN BOOL LGAPI  AllocSetPageFunc(tAllocatorPageFunc);
EXTERN void LGAPI  AllocGetLimits(sAllocLimits *);
EXTERN size_t LGAPI AllocSetAllocCap(size_t);
EXTERN ulong LGAPI AllocPickAllocCap();

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACES: IMalloc, IMallocSpy (standard interfaces)
//

#ifdef DEFINE_BASE_ALLOC_INTERFACES
#ifndef __IMalloc_INTERFACE_DEFINED__
#define __IMalloc_INTERFACE_DEFINED__
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

    STDMETHOD_(void *, Alloc)   (THIS_ SIZE_T cb) PURE;
    STDMETHOD_(void *, Realloc) (THIS_ void * pv, SIZE_T cb) PURE;
    STDMETHOD_(void,   Free)    (THIS_ void * pv) PURE;
    STDMETHOD_(SIZE_T, GetSize) (THIS_ void * pv) PURE;
    STDMETHOD_(int,    DidAlloc)(THIS_ void * pv) PURE;
    STDMETHOD_(void,   HeapMinimize)(THIS) PURE;
};
#endif

#ifndef __IMallocSpy_INTERFACE_DEFINED__
#define __IMallocSpy_INTERFACE_DEFINED__
#undef INTERFACE
#define INTERFACE IMallocSpy

DECLARE_INTERFACE_(IMallocSpy, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)  (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD_(SIZE_T, PreAlloc)        (SIZE_T cbRequest) PURE;
    STDMETHOD_(void *, PostAlloc)       (void * Actual) PURE;
    STDMETHOD_(void *, PreFree)         (void * Request, BOOL fSpyed) PURE;
    STDMETHOD_(void,   PostFree)        (BOOL fSpyed) PURE;
    STDMETHOD_(SIZE_T, PreRealloc)      (void * Request, SIZE_T cbRequest,void ** pNewRequest, BOOL fSpyed) PURE;
    STDMETHOD_(void *, PostRealloc)     (void * Actual, BOOL fSpyed) PURE;
    STDMETHOD_(void *, PreGetSize)      (void * Request, BOOL fSpyed) PURE;
    STDMETHOD_(SIZE_T, PostGetSize)     (SIZE_T cbActual, BOOL fSpyed) PURE;
    STDMETHOD_(void *, PreDidAlloc)     (void * Request, BOOL fSpyed) PURE;
    STDMETHOD_(int,    PostDidAlloc)    (void * Request, BOOL fSpyed, int fActual) PURE;
    STDMETHOD_(void,   PreHeapMinimize) (void) PURE;
    STDMETHOD_(void,   PostHeapMinimize)(void) PURE;

};
#endif
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
    STDMETHOD_(void *, Alloc)   (THIS_ SIZE_T cb) PURE;
    STDMETHOD_(void *, Realloc) (THIS_ void * pv, SIZE_T cb) PURE;
    STDMETHOD_(void,   Free)    (THIS_ void * pv) PURE;
    STDMETHOD_(SIZE_T, GetSize) (THIS_ void * pv) PURE;
    STDMETHOD_(int,    DidAlloc)(THIS_ void * pv) PURE;
    STDMETHOD_(void,   HeapMinimize)(THIS) PURE;

    //
    // Debugging allocation functions
    //
    STDMETHOD_(void *, AllocEx)  (THIS_ SIZE_T cb, const char * pszFile, int line) PURE;
    STDMETHOD_(void *, ReallocEx)(THIS_ void * pv, SIZE_T cb, const char * pszFile, int line) PURE;
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
            if (g_pMalloc)
                g_pMalloc->PushCredit(pszFile, line);
         }
         
         ~__cAutoAllocCredit()
         {
            if (g_pMalloc)
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
