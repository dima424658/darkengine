///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/dbgalloc.cpp $
// $Author: TOML $
// $Date: 1998/06/10 13:57:58 $
// $Revision: 1.9 $
//

#ifdef _WIN32
#ifndef SHIP

#include <lg.h>
#include <dbgalloc.h>
#include <memcore.h>
#include <hashfns.h>
#include <mprintf.h>

#ifdef DBGALLOC_STACKWALK
#include <stktrace.h>
#endif

#pragma code_seg("lgalloc")

EXTERN __declspec(dllimport) BOOL __stdcall IsBadReadPtr(const void *, uint ucb);
EXTERN __declspec(dllimport) unsigned long __stdcall timeGetTime   (void);

///////////////////////////////////////////////////////////////////////////////

#ifdef __WATCOMC__
#pragma initialize library
#else
#pragma init_seg (lib)
#endif

class cHeapDebugLeakReporter
{
public:
    cHeapDebugLeakReporter()
    {
        if (g_pHeapDebug)
            g_pHeapDebug->LeakTrackStart();

    }

    ~cHeapDebugLeakReporter()
    {
        if (g_pHeapDebug)
        {
            g_pHeapDebug->VerifyHeap();
            g_pHeapDebug->DumpLeaks();
        }
    }
};

static cHeapDebugLeakReporter g_HeapDebugLeakReporter;

///////////////////////////////////////////////////////////////////////////////
//
// Debug format is:
//
//  - int         clientSize
//  - sHDSentinel headSentinel
//  - (client space)
//  - sHDSentinel tailSentinel
//

struct sHDHeader
{
    int         clientSize;
    sHDSentinel headSentinel;
};

#define SizeWithDebug(clientSize)   ((clientSize) + kSizeofMemDebug)
#define SizeLessDebug(allocSize)    ((allocSize) - kSizeofMemDebug)
#define GetDebugInfo(p)             ((sHDInfo *)((BYTE *)(p) + (m_pNext->GetSize(p) - sizeof(sHDInfo))))

#define ClientToDebug(p)            (((sHDHeader *)(p)) - 1)
#define DebugToClient(p)            (((sHDHeader *)(p)) + 1)

#define GetHeader(p)                (((sHDHeader *)(p)) - 1)
#define GetHeadSentinel(p)          (&(GetHeader(p)->headSentinel))
#define GetTailSentinel(p)          ((sHDSentinel *)(((BYTE *)(p)) + GetHeader(p)->clientSize))

///////////////////////////////////////////////////////////////////////////////

#define    kSizeofMemDebug  (sizeof(sHDSentinel) * 2 + sizeof(int))
const BYTE kMallocFill = 0xcd;
const BYTE kFreeFill   = 0xdd;

///////////////////////////////////////////////////////////////////////////////

// Because this is only debugging code, we're not interested in
// super-efficiency


#define kStackWalkDepth 4

struct sHDTraceInfo
{
    sHDTraceInfo * pNext;
    void *         p;
    size_t         size;

    const char *   pszFile;
    int            line;
    DWORD          time;

#ifdef DBGALLOC_STACKWALK
    void *         callStack[kStackWalkDepth];
#endif

};


///////////////////////////////////////////////////////////////////////////////
//
// We have one of these for every unique filename we see, storing them
// in a hand-build hash table with linked list buckets.  We update
// them at every allocation and deallocation.
//

struct sHDModuleInfo
{
   sHDModuleInfo * pNext;
   size_t          maxSize;        // over lifetime of app
   size_t          currentSize;
   size_t          maxRealSize;
   size_t          realSize;
   unsigned        nAllocs;
   const char   *  pszFileName;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cHeapDebug, members
//
// This does the basics:
//
// - Mallocs come out non-zero
// - Frees are blasted out
// - Blocks have trailing sentinel.  Previous sentinels are not used because
//   there exists code that secretly peeks at the Watcom alloc block header
// - Realloc always returns a different block
//
// For more advanced use, there's a complete block tracking system to detect
// bad frees and test all outstanding blocks
//

cHeapDebug::cHeapDebug()
{
    m_nAllocs = 0;
    m_nBytesAlloced = 0;
    m_nMaxBytesAlloced = 0;
    m_nRealBytesAlloced = 0;
    m_nMaxRealBytesAlloced = 0;
    
    m_HeapTestInterval = 0;
    m_TimeTraceStart = timeGetTime();
    m_TimeLeakTrackStart = 0;
    memset(m_TraceTable, 0, sizeof(m_TraceTable));
    memset(m_ModuleTable, 0, sizeof(m_ModuleTable));

    m_Sentinel.highPart      = 0xbd122969; // my birthday
    m_Sentinel.lowPart       = 0xf8675309; // jennys number
    m_FreedSentinel.highPart = 0xdddddddd;
    m_FreedSentinel.lowPart  = 0xdddddddd;

    m_iCreditStack = -1;
}

///////////////////////////////////////

cHeapDebug::~cHeapDebug()
{
   
}

///////////////////////////////////////

inline void cHeapDebug::SetDebug(void * p, size_t clientSize)
{
    GetHeader(p)->clientSize = clientSize;
    memcpy(GetHeadSentinel(p), &m_Sentinel, sizeof(m_Sentinel));
    memcpy(GetTailSentinel(p), &m_Sentinel, sizeof(m_Sentinel));
}

///////////////////////////////////////

STDMETHODIMP_(void *) cHeapDebug::Alloc(ULONG size)
{
    return AllocEx(size, NULL, 0);
}

///////////////////////////////////////

STDMETHODIMP_(void *) cHeapDebug::Realloc(void * pOld, ULONG newClientSize)
{
    return ReallocEx(pOld, newClientSize, NULL, 0);
}

///////////////////////////////////////

STDMETHODIMP_(void) cHeapDebug::Free(void * p)
{
    FreeEx(p, NULL, 0);
}

///////////////////////////////////////

STDMETHODIMP_(ULONG) cHeapDebug::GetSize(void * p)
{
    if (VerifyAlloc(p) == S_OK)
        return SizeLessDebug(m_pNext->GetSize(ClientToDebug(p)));
    return 0;
}

///////////////////////////////////////

STDMETHODIMP_(int) cHeapDebug::DidAlloc(void * p)
{
    return m_pNext->DidAlloc(ClientToDebug(p));
}

///////////////////////////////////////

STDMETHODIMP_(void) cHeapDebug::HeapMinimize()
{
    m_pNext->HeapMinimize();
}

///////////////////////////////////////

STDMETHODIMP_(void *) cHeapDebug::AllocEx(ulong size, const char * pszFile, 
                                          int line)
{
    if (!size)
        size = 1;  // For VC compatibility, allow 0 sized allocations.

    void * p;

    p = m_pNext->Alloc(SizeWithDebug(size));

    if (p)
    {
        // Advance pointer to client position, insert sentinels, and blast the block
        p = DebugToClient(p);
        SetDebug(p, size);
        memset(p, kMallocFill, size);
        TraceMalloc(p, size, pszFile, line);
        AssertMsg((ulong)p % 2 == 0, "Odd allocation!");
    }
    
    return p;
}

///////////////////////////////////////

STDMETHODIMP_(void *) cHeapDebug::ReallocEx(void * pOld, ULONG newClientSize, const char * pszFile, int line)
{
    // Handle "exception" behaviors here...
    if (!pOld)
        return AllocEx(newClientSize, pszFile, line);

    if (!newClientSize)
    {
        FreeEx(pOld, pszFile, line);
        return 0;
    }

    // We always return a different block to help trap stray
    // references to the old block
    const int oldClientSize = GetSize(pOld);
    void * pNew;

    pNew = AllocEx(newClientSize, pszFile, line);

    if (pNew)
        memcpy(pNew, pOld, (newClientSize > oldClientSize) ? oldClientSize : newClientSize);

    FreeEx(pOld, pszFile, line);

    return (pNew);
}

///////////////////////////////////////

STDMETHODIMP_(void) cHeapDebug::FreeEx(void * p, const char * pszFile, int line)
{
    if (!p)
        return;

    if (VerifyAlloc(p) == S_OK)
    {
        TraceFree(p, pszFile, line);

        // Move back from client position and blast the block
        p = ClientToDebug(p);
        Assert_(p < (void *) m_TraceTable || p >= (void *) &m_TraceTable[kTraceTableSize]);
        memset(p, kFreeFill, m_pNext->GetSize(p));
        m_pNext->Free(p);
    }
}

///////////////////////////////////////

STDMETHODIMP cHeapDebug::VerifyAlloc(void * p)
{
    if (p)
    {
        sHDTraceInfo * pInfo = TraceSearch(p);
        if (!pInfo)
        {
            // If you've crashed here, you've most likely passed
            // an invalid pointer to free or delete
            g_fQuietAssert = FALSE;
#if 0
            if (memcmp(GetHeadSentinel(p), &m_FreedSentinel, sizeof(sHDSentinel)) == 0)
                CriticalMsg1("Dynamic memory error (0x%x): Pointer not a valid heap pointer, probable double free", p);
            else
#endif
                CriticalMsg1("Dynamic memory error (0x%x): Pointer not a valid heap pointer", p);
            return E_FAIL;
        }

        const char * pszFailureMessage = NULL;

        sHDSentinel * pHeadSentinel   = GetHeadSentinel(p);
        sHDSentinel * pTailSentinel   = GetTailSentinel(p);

        if ((BYTE *)pTailSentinel - (BYTE *)p > m_pNext->GetSize(ClientToDebug(p)) ||
            memcmp(pHeadSentinel, &m_Sentinel, sizeof(sHDSentinel)) != 0)
        {
            pszFailureMessage = "\nPossible: \na) buffer underrun (most likely)\nb) wild pointer\nc) write after free, or\nd) double free (least likely)\n";
        }
        else if (memcmp(pTailSentinel, &m_Sentinel, sizeof(sHDSentinel)) != 0)
        {
            pszFailureMessage = "\nPossible: \na) buffer overrun (most likely)\nb) wild pointer\nc) write after free, or\nd) double free (least likely)\n";
        }

        if (pszFailureMessage)
        {
            g_fQuietAssert = FALSE;

#ifdef DBGALLOC_STACKWALK
            if (pInfo && (pInfo->pszFile || pInfo->callStack[0]))
#else
            if (pInfo && pInfo->pszFile)
#endif
            {
                if (pInfo->pszFile)
                    CriticalMsg4("Dynamic memory error (0x%x): %s \n[%s@%d]", p, pszFailureMessage,
                                 pInfo->pszFile,
                                 pInfo->line);
#ifdef DBGALLOC_STACKWALK
                else
                    CriticalMsg6("Dynamic memory error (0x%x): %s \n[0x%x:0x%x:0x%x:0x%x]", p, pszFailureMessage,
                                 pInfo->callStack[0],
                                 pInfo->callStack[1],
                                 pInfo->callStack[2],
                                 pInfo->callStack[3]);
#endif
            }
            else
                CriticalMsg2("Dynamic memory error (0x%x): %s ", p, pszFailureMessage);

            return E_FAIL;
        }
    }
    return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cHeapDebug::VerifyHeap()
{
    const sHDTraceInfo * pInfo;

    for (int i = 0; i < kTraceTableSize; i++)
    {
        pInfo = m_TraceTable[i];
        while (pInfo)
        {
            if (VerifyAlloc(pInfo->p) != S_OK)
                return E_FAIL;
            pInfo = pInfo->pNext;
        }
    }

    return m_pNext->VerifyHeap();
}

///////////////////////////////////////

STDMETHODIMP_(void) cHeapDebug::DumpHeapInfo()
{
   DumpStats();
   mprint("\n");
   DumpModules();
   mprint("\n");
   DumpBlocks();
   mprint("\n");
   
   m_pNext->DumpHeapInfo();
}

///////////////////////////////////////

STDMETHODIMP_(void) cHeapDebug::DumpStats()
{
   mprintf("Debug heap stats (note: debug allocator can cause unusual overhead):\n");
   mprintf("   Total allocs:     %d\n"
           "   Client bytes:     %dk\n"
           "   Max client bytes: %dk\n"
           "   Real bytes:       %dk\n"
           "   Max real bytes:   %dk\n",
           m_nAllocs,
           m_nBytesAlloced / 1024,
           m_nMaxBytesAlloced / 1024,
           m_nRealBytesAlloced / 1024,
           m_nMaxRealBytesAlloced / 1024);
}

///////////////////////////////////////

STDMETHODIMP_(void) cHeapDebug::DumpBlocks()
{
    BOOL   fFirst = TRUE;
    size_t totalLeak = 0;

    const sHDTraceInfo * pInfo;

    for (int i = 0; i < kTraceTableSize; i++)
    {
        pInfo = m_TraceTable[i];
        while (pInfo)
        {
            if (pInfo->time > m_TimeLeakTrackStart)
            {
                if (fFirst)
                {
                    mprint("The following blocks are in use:\n");
                    fFirst = FALSE;
                }
                mprintf("%7u:%7u @ %#-9x",
                         pInfo->time, pInfo->size, pInfo->p);
                if (pInfo->pszFile)
                {
                    mprintf(" [%s@%d]", pInfo->pszFile, pInfo->line);
                    
                }
#ifdef DBGALLOC_STACKWALK
                else if (pInfo->callStack[0])
                    mprintf(" [0x%x:0x%x:0x%x:0x%x]", pInfo->callStack[0], pInfo->callStack[1], pInfo->callStack[2], pInfo->callStack[3]);
#endif

                mprint("\n");

                totalLeak += pInfo->size;
            }
            pInfo = pInfo->pNext;
        }
    }

    if (totalLeak)
        mprintf("        Total in use %u bytes\n", totalLeak);

   m_pNext->DumpBlocks();
}

///////////////////////////////////////

STDMETHODIMP_(void) cHeapDebug::DumpModules()
{
    mprintf("Memory use by module:\n");
    mprintf("     Bytes    Peak    Num\n");

    for (int i = 0; i < kModuleTableSize; ++i) 
    {
       sHDModuleInfo *pModule;

       pModule = m_ModuleTable[i];
       while (pModule) 
       {
          mprintf("   %7d %7d %6d  %s\n", pModule->currentSize, pModule->maxSize, pModule->nAllocs, pModule->pszFileName);
          pModule = pModule->pNext;
       }
    }

    m_pNext->DumpHeapInfo();
}

///////////////////////////////////////

STDMETHODIMP_(void) cHeapDebug::PushCredit(const char * pszFile, int line)
{
   m_iCreditStack++;
   AssertMsg(m_iCreditStack < kCreditStackSize, "Credit stack overflow");
   m_CreditStack[m_iCreditStack].pszFile = pszFile;
   m_CreditStack[m_iCreditStack].line = line;
}

///////////////////////////////////////

STDMETHODIMP_(void) cHeapDebug::PopCredit()
{
   m_iCreditStack--;
   AssertMsg(m_iCreditStack >= -1, "Credit stack underflow");
}

///////////////////////////////////////

void cHeapDebug::LeakTrackStart()
{
    m_TimeLeakTrackStart = timeGetTime() - m_TimeTraceStart;
}

///////////////////////////////////////

void cHeapDebug::DumpLeaks()
{
    if (g_bAllocDumpLeaks)
        DumpHeapInfo();
}

///////////////////////////////////////

void cHeapDebug::TraceMalloc(void * p, size_t clientSize, const char * pszFile, int line)
{
   sHDTraceInfo * pInfo = TraceSearch(p);
   if (!pInfo)
   {
      if (m_iCreditStack >= 0)
      {
         pszFile = m_CreditStack[m_iCreditStack].pszFile;
         line = m_CreditStack[m_iCreditStack].line;
      }
      
      pInfo = (sHDTraceInfo *) m_pNext->Alloc(sizeof(sHDTraceInfo));
      pInfo->p = p;
      pInfo->size = clientSize;
      pInfo->pszFile = pszFile;
      pInfo->line = line;
      pInfo->time = timeGetTime() - m_TimeTraceStart;

#ifdef DBGALLOC_STACKWALK
      if (!pszFile)
      {
         int depth;
         depth = FillStackArray(6, kStackWalkDepth, pInfo->callStack);

         if (!depth)
            pInfo->callStack[0] = 0;
      }
#endif
       TraceInsert(pInfo);

       // 
       // Update per-module info
       //
       sHDModuleInfo * pModuleInfo = ModuleInfoGet(pszFile);
       int             realSize    = m_pNext->GetSize(ClientToDebug(p)) - kSizeofMemDebug;
       
       pModuleInfo->currentSize += clientSize;
       pModuleInfo->realSize += realSize;
       pModuleInfo->nAllocs++;

       if (pModuleInfo->currentSize > pModuleInfo->maxSize)
          pModuleInfo->maxSize = pModuleInfo->currentSize;

       if (pModuleInfo->realSize > pModuleInfo->maxRealSize)
          pModuleInfo->maxRealSize = pModuleInfo->realSize;

       //
       // Update heap-wide info
       //
       m_nAllocs++;

       m_nBytesAlloced += clientSize;
       m_nRealBytesAlloced += realSize;
       
       if (m_nBytesAlloced > m_nMaxBytesAlloced)
         m_nMaxBytesAlloced = m_nBytesAlloced;
         
       if (m_nRealBytesAlloced > m_nMaxRealBytesAlloced)
         m_nMaxRealBytesAlloced = m_nRealBytesAlloced;
       
       return;
   }

   CriticalMsg3("Malloc returned a pointer 0x%x that previously commited to [%s@%d]",
                p,
                pInfo->pszFile ? pInfo->pszFile : "unknown",
                pInfo->line);
}

///////////////////////////////////////

void cHeapDebug::TraceFree(void * p, const char *, int)
{
   sHDTraceInfo * pInfo = TraceRemove(p);

   if (pInfo)
   {
      int realSize = m_pNext->GetSize(ClientToDebug(p)) - kSizeofMemDebug;
      
      //
      // Update module info
      //
      sHDModuleInfo *pModuleInfo = ModuleInfoGet(pInfo->pszFile);
      pModuleInfo->currentSize -= pInfo->size;
      pModuleInfo->realSize -= realSize;
      pModuleInfo->nAllocs--;

      //
      // Update heap-wide info
      //
      m_nAllocs--;
      m_nBytesAlloced -= pInfo->size;
      m_nRealBytesAlloced -= realSize;

      m_pNext->Free(pInfo);
   }
}

///////////////////////////////////////

void cHeapDebug::TraceInsert(sHDTraceInfo * pInfo)
{
    int i = HashPtr(pInfo->p) % kTraceTableSize;

    pInfo->pNext = NULL;

    if (!m_TraceTable[i])
        m_TraceTable[i] = pInfo;

    else
    {
        sHDTraceInfo * pCurInfo = m_TraceTable[i];

        while (pCurInfo->pNext)
            pCurInfo = pCurInfo->pNext;

        pCurInfo->pNext = pInfo;
    }
}

///////////////////////////////////////

sHDTraceInfo * cHeapDebug::TraceSearch(void * p)
{
    int i = HashPtr(p) % kTraceTableSize;

    sHDTraceInfo * pCurInfo = m_TraceTable[i];

    while (pCurInfo)
    {
        if (pCurInfo->p == p)
            return pCurInfo;
        pCurInfo = pCurInfo->pNext;
    }
    return NULL;
}

///////////////////////////////////////

sHDTraceInfo * cHeapDebug::TraceRemove(void * p)
{
    int i = HashPtr(p) % kTraceTableSize;

    sHDTraceInfo * pReturn = NULL;

    if (m_TraceTable[i])
    {
        if (m_TraceTable[i]->p == p)
        {
            pReturn = m_TraceTable[i];
            m_TraceTable[i] = m_TraceTable[i]->pNext;
        }
        else
        {
            sHDTraceInfo * pCurInfo = m_TraceTable[i];

            while (pCurInfo->pNext)
            {
                if (pCurInfo->pNext->p == p)
                {
                    pReturn = pCurInfo->pNext;
                    pCurInfo->pNext = pCurInfo->pNext->pNext;
                    break;
                }
                pCurInfo = pCurInfo->pNext;
            }
        }
    }

    return pReturn;
}

///////////////////////////////////////

// find or create sHDModuleInfo structure for the given module--
// if we create it we initialize it
sHDModuleInfo *cHeapDebug::ModuleInfoGet(const char *pszFileName)
{
    if (!pszFileName)
       pszFileName = "(Unknown)";

    int i = HashString(pszFileName) % kModuleTableSize;

    // This will be 0 if the table entry has no linked list.
    sHDModuleInfo *pModuleInfo = m_ModuleTable[i];

    while (pModuleInfo) 
    {
       if (strcmp(pszFileName, pModuleInfo->pszFileName) == 0)
          break;
       pModuleInfo = pModuleInfo->pNext;
    }

    if (!pModuleInfo) 
    {
       pModuleInfo = (sHDModuleInfo *) m_pNext->Alloc(sizeof(sHDModuleInfo));
       memset(pModuleInfo, 0, sizeof(*pModuleInfo));

       // hook into linked list
       pModuleInfo->pNext = m_ModuleTable[i];
       m_ModuleTable[i] = pModuleInfo;

       // init remaining fields
       pModuleInfo->pszFileName = pszFileName;
    }

    return pModuleInfo;
}

///////////////////////////////////////////////////////////////////////////////

#endif
#endif
