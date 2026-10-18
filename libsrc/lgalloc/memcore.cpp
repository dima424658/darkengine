///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/memcore.cpp $
// $Author: TOML $
// $Date: 1998/06/10 13:58:04 $
// $Revision: 1.6 $
//
// @TBD (toml 07-14-97): should cram all functions in this library into the same codeseg

#include <allocapi.h>
#include <platform_services.h>

#include <memcore.h>
#include <multpool.h>
#include <heap.h>
#include <dbgalloc.h>
#include <memtimer.h>
#include <primallc.h>
#include <stdalloc.h>
#include <nullallc.h>

#include <coremutx.h>

#pragma code_seg("lgalloc")

EXTERN BOOL LGAllocOverride();

///////////////////////////////////////////////////////////////////////////////

#ifdef __WATCOMC__
#pragma initialize 0
#else
// under MS, disable normal static construction
#pragma warning(disable:4075)
#pragma init_seg ("lgalloc_custom_init")
#endif

///////////////////////////////////////////////////////////////////////////////
sPlatformMutex g_AllocMutex;

///////////////////////////////////////////////////////////////////////////////

extern void AllocMutexInit(void)
{
   PlatformMutexInit(&g_AllocMutex);
}

///////////////////////////////////////

extern void AllocMutexTerm(void)
{
   PlatformMutexTerm(&g_AllocMutex);
}

///////////////////////////////////////

extern void AllocThreadLock(void)
{
    PlatformMutexLock(&g_AllocMutex);
}

///////////////////////////////////////

extern void AllocThreadUnlock(void)
{
    PlatformMutexUnlock(&g_AllocMutex);
}

///////////////////////////////////////////////////////////////////////////////

IAllocator *        g_pMalloc;

#ifndef SHIP
sAllocLimits *      g_pAllocLimits;

cHeapDebug *        g_pHeapDebug;
BOOL                g_bAllocDumpLeaks;
cMemAllocTimer *    g_pMemAllocTimer;
///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMemCore
//
// This class contains all the allocator implementations, and is
// primarily the focus of initialization.  Under Watcom, initialization is
// done by high-priority C++ static initialization.  Under Microsoft, it is
// accomplished by a direct call from the C-runtime startup code.
//
// Note that under Watcom, the Watcom libraries continue to use the allocator
// even though it's supposed lifetime has passed.  This is due to the fact that
// stdio is actually cleaned up out-of-order relative to start-up.
//
// Also note that under Microsoft, the cleanup functions are only called in
// exceptional cases. The OS will automatically unload the DLL and clean up
// allocated memory
//

#pragma pack(8)
class cMemCore
{
public:
    ///////////////////////////////////
    //
    // Initialization
    //

    cMemCore()
    {
        AllocMutexInit();
        CoreMutexInit();
        IAllocator * pNext = NULL;

        // The legacy MSVC build replaced the CRT allocation entry points with
        // allocovr.h. Modern UCRT does not expose those hooks, so keep the
        // allocator handed to OSM modules on the same CRT heap used by the
        // rest of the executable.
        m_StdAlloc.Init();
        pNext = &m_StdAlloc;

#ifndef SHIP
        g_pAllocLimits = &m_PrimaryMalloc;

#endif

        m_PrimaryMalloc.SetNext(pNext);
        g_pMalloc = &m_PrimaryMalloc;
    }

    ///////////////////////////////////
    //
    // Cleanup: almost always done by OS, by default
    //
    ~cMemCore()
    {
        // We rely on the OS to unload the DLL and cleanup memory.  This just
        // protects us from the (actual) calls into the allocator after formal
        // shutdown under Watcom
        g_pMalloc = (IAllocator *) g_pNullMalloc;
        CoreMutexTerm();
        AllocMutexTerm();
    }

    cStdAlloc      m_StdAlloc;
    cHeap          m_Heap;
    cMultiPool     m_MultiPool;
#ifndef SHIP
    cMemAllocTimer m_MemAllocTimer;
    cHeapDebug     m_HeapDebug;
#endif
    cPrimaryMalloc m_PrimaryMalloc;

};
#pragma pack()

static cMemCore g_MemCore;

///////////////////////////////////////////////////////////////////////////////
//
// Microsoft heap init/term
//

#if defined(_MSC_VER)

///////////////////////////////////////
//
// Heap initialization function called by start-up code
//

EXTERN
int LGAPI HeapInit()
{
    g_MemCore.cMemCore::cMemCore();
//    g_NoOpMallocVtbl.Free = NoOpFree;
//    g_NoOpMalloc.pVtbl = &g_NoOpMallocVtbl;
    return !!g_pMalloc;
}

///////////////////////////////////////
//
// Heap cleanup function called only by start-up in exceptional cases when
// OS is not expected to clean up for us automatically
//

EXTERN
void LGAPI HeapTerm()
{
    g_MemCore.cMemCore::~cMemCore();
}
#endif

///////////////////////////////////////////////////////////////////////////////

#endif

///////////////////////////////////////////////////////////////////////////////

BOOL LGAPI AllocSetPageFunc(tAllocatorPageFunc pfnPage)
{
    if (LGAllocOverride())
    {
        g_MemCore.m_PrimaryMalloc.SetPageFunc(pfnPage);
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////

void LGAPI AllocGetLimits(sAllocLimits * pLimits)
{
    memcpy(pLimits, (void *)((sAllocLimits *)&(g_MemCore.m_PrimaryMalloc)), sizeof(sAllocLimits));
}

///////////////////////////////////////

size_t LGAPI AllocSetAllocCap(size_t cap)
{
   size_t old = g_MemCore.m_PrimaryMalloc.allocCap;
   g_MemCore.m_PrimaryMalloc.initAllocCap = g_MemCore.m_PrimaryMalloc.allocCap = cap;
   return old;
}

///////////////////////////////////////

ulong LGAPI AllocPickAllocCap()
{
   sPlatformMemoryStatus memoryStatus;
   if (!PlatformGetMemoryStatus(&memoryStatus))
      return 0;

   const ulong kTargetCapNum   = 1;
   const ulong kTargetCapDenom = 2;
   const ulong kMinCap         = 0x0800000;      //  8 mb
   const ulong kMaxCap         = 0x2000000;      // 32 mb

   SIZE_T targetCap =
      (memoryStatus.total_physical_bytes * kTargetCapNum) / kTargetCapDenom;

   if (targetCap < kMinCap)
      targetCap = kMinCap;
   if (targetCap > kMaxCap)
      targetCap = kMaxCap;
   return (ulong)targetCap;
}


