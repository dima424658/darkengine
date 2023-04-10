///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/memcore.cpp $
// $Author: TOML $
// $Date: 1998/06/10 13:58:04 $
// $Revision: 1.6 $
//
// @TBD (toml 07-14-97): should cram all functions in this library into the same codeseg

#ifdef _WIN32
#include <windows.h>
#endif

#include <allocapi.h>

#ifdef _WIN32
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
#if OS_MUTEX
CRITICAL_SECTION g_AllocMutex;
#endif

///////////////////////////////////////////////////////////////////////////////

extern void AllocMutexInit(void)
{
#if OS_MUTEX
   InitializeCriticalSection(&g_AllocMutex);
#endif
}

///////////////////////////////////////

extern void AllocMutexTerm(void)
{
#if OS_MUTEX
   DeleteCriticalSection(&g_AllocMutex);
#endif
}

///////////////////////////////////////

#if OS_MUTEX
extern void AllocThreadLock(void)
{
    EnterCriticalSection(&g_AllocMutex);
}

///////////////////////////////////////

extern void AllocThreadUnlock(void)
{
    LeaveCriticalSection(&g_AllocMutex);
}
#endif

///////////////////////////////////////////////////////////////////////////////

IAllocator *        g_pMalloc;

#ifndef SHIP
sAllocLimits *      g_pAllocLimits;

cHeapDebug *        g_pHeapDebug;
BOOL                g_bAllocDumpLeaks;
cMemAllocTimer *    g_pMemAllocTimer;
#endif

static const char * pszExternalAllocatorDLL = "lgallocx.dll";

///////////////////////////////////////

#if 0
#ifndef SHIP
static const BOOL g_fUseMultiPool =
#ifdef _WIN32
    GetPrivateProfileInt(
        "Allocator",
        "MultiPool",
        TRUE,
        "lg.ini");
#else
    TRUE;
#endif
#endif
#endif

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

        if (GetPrivateProfileInt("Allocator", "UseExternal", FALSE, "lg.ini"))
        {
            // First, load the primal allocator from lgalloc.dll...

            // @Note (toml 07-10-97): Right now, this is the MS allocator.
            // @TBD (toml 07-10-97): should performance test Watcom vs MS and pick best

            IMalloc * (__stdcall *pfnGetMalloc)() = NULL;

            m_hLgAllocDll = LoadLibrary(pszExternalAllocatorDLL);
            if (m_hLgAllocDll)
                pfnGetMalloc = (IMalloc * (__stdcall *)()) GetProcAddress(m_hLgAllocDll, "_GetMalloc@0");
            else
            {
                MessageBox(NULL, "Failed to load \"lgalloc.dll\"", NULL, MB_OK);
                ExitProcess(1);
            }

            if (!pfnGetMalloc)
            {
                MessageBox(NULL, "Failed to locate \"GetMalloc()\" in \"lgalloc.dll\"", NULL, MB_OK);
                FreeLibrary(m_hLgAllocDll);
                ExitProcess(1);
            }
#ifndef SHIP
            IMalloc * pStdMalloc;
            pStdMalloc = (*pfnGetMalloc)();
            pStdMalloc->QueryInterface(IID_IDebugMalloc, (void **) &pNext);
            pStdMalloc->Release();
#else
            pNext = (*pfnGetMalloc)();
#endif
            if (!pNext)
            {
                MessageBox(NULL, "Failed to load allocator from in \"lgalloc.dll\"", NULL, MB_OK);
                FreeLibrary(m_hLgAllocDll);
                ExitProcess(1);
            }
        }
        else
        {
            m_StdAlloc.Init();
            pNext = &m_StdAlloc;
        }

        ///////////////////////////////

        if (GetPrivateProfileInt("Allocator", "Heap", TRUE, "lg.ini"))
        {
            m_Heap.SetNext(pNext);
            pNext = &m_Heap;
        }

        if (GetPrivateProfileInt("Allocator", "MultiPool", TRUE, "lg.ini"))
        {
            m_MultiPool.SetNext(pNext);
            m_MultiPool.Init();
            pNext = &m_MultiPool;
        }

#ifndef SHIP
        if (GetPrivateProfileInt("Allocator", "Timings", FALSE, "lg.ini"))
        {
            m_MemAllocTimer.SetNext(pNext);
            pNext = g_pMemAllocTimer = &m_MemAllocTimer;
        }

        BOOL fDebugAlloc;
        fDebugAlloc = GetPrivateProfileInt("Allocator", "Debug", 2, "lg.ini");

        if (fDebugAlloc == 2)
        {
        #ifdef DEBUG
            fDebugAlloc = TRUE;
        #else
            fDebugAlloc = FALSE;
        #endif
        }

        if (fDebugAlloc)
        {
            g_bAllocDumpLeaks = GetPrivateProfileInt("Allocator", "DumpUnfreed", FALSE, "lg.ini");
            m_HeapDebug.SetNext(pNext);
            pNext = g_pHeapDebug = &m_HeapDebug;
        }

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

    HINSTANCE      m_hLgAllocDll;

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
#ifdef _WIN32
    if (LGAllocOverride())
    {
        g_MemCore.m_PrimaryMalloc.SetPageFunc(pfnPage);
        return TRUE;
    }
#endif
    return FALSE;
}

///////////////////////////////////////

void LGAPI AllocGetLimits(sAllocLimits * pLimits)
{
#ifdef _WIN32
    memcpy(pLimits, (void *)((sAllocLimits *)&(g_MemCore.m_PrimaryMalloc)), sizeof(sAllocLimits));
#endif
}

///////////////////////////////////////

ulong LGAPI AllocSetAllocCap(ulong cap)
{
#ifdef _WIN32
   ulong old = g_MemCore.m_PrimaryMalloc.allocCap;
   g_MemCore.m_PrimaryMalloc.initAllocCap = g_MemCore.m_PrimaryMalloc.allocCap = cap;
   return old;
#else
   return cap;
#endif
}

///////////////////////////////////////

ulong LGAPI AllocPickAllocCap()
{
#ifdef _WIN32
   MEMORYSTATUS memoryStatus;
   memoryStatus.dwLength = sizeof(memoryStatus);
   GlobalMemoryStatus(&memoryStatus);

   const ulong kTargetCapNum   = 1;
   const ulong kTargetCapDenom = 2;
   const ulong kMinCap         = 0x0800000;      //  8 mb
   const ulong kMaxCap         = 0x2000000;      // 32 mb

         ulong targetCap       = (memoryStatus.dwTotalPhys * kTargetCapNum) / kTargetCapDenom;
         ulong iniCap;

   targetCap = max(kMinCap, min(kMaxCap, targetCap));
   iniCap = GetPrivateProfileInt("Allocator", "MemoryCap", 0, "lg.ini");

   if (iniCap)
      targetCap = iniCap;

   return targetCap;
#else
   return 0;
#endif
}


