///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/dbgalloc.h $
// $Author: TOML $
// $Date: 1998/06/10 13:58:02 $
// $Revision: 1.5 $
//

#ifndef __DBGALLOC_H
#define __DBGALLOC_H

#ifndef SHIP
#include <membase.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cHeapDebug
//
// Comprehensive heap error detection
//

struct sHDSentinel
{
    DWORD highPart;
    DWORD lowPart;
};

struct sHDTraceInfo;
struct sHDModuleInfo;

class cHeapDebug : public cMallocBase
{
public:
    ///////////////////////////////////
    //
    // Constructor, destructor and architectural hooks
    //

    cHeapDebug();
    ~cHeapDebug();

    void LeakTrackStart();
    void DumpLeaks();

    ///////////////////////////////////
    //
    // IMalloc functions
    //
    STDMETHOD_(void *, Alloc)   (ULONG cb);
    STDMETHOD_(void *, Realloc) (void * pv, ULONG cb);
    STDMETHOD_(void,   Free)    (void * pv);
    STDMETHOD_(ULONG,  GetSize) (void * pv);
    STDMETHOD_(int,    DidAlloc)(void * pv);
    STDMETHOD_(void,   HeapMinimize)();

    ///////////////////////////////////
    //
    // IDebugMalloc extensions to IMalloc
    //
    STDMETHOD_(void *, AllocEx)  (ULONG cb, const char * pszFile, int line);
    STDMETHOD_(void *, ReallocEx)(void * pv, ULONG cb, const char * pszFile, int line);
    STDMETHOD_(void,   FreeEx)   (void * pv, const char * pszFile, int line);

    STDMETHOD (VerifyAlloc)(void * pv);
    STDMETHOD (VerifyHeap)();

    STDMETHOD_(void, DumpHeapInfo)();

    STDMETHOD_(void, DumpStats)();
    STDMETHOD_(void, DumpBlocks)();
    STDMETHOD_(void, DumpModules)();

    STDMETHOD_(void, PushCredit)(const char * pszFile, int line);
    STDMETHOD_(void, PopCredit)();

private:

    //
    // Debug sentinel set
    //
    void SetDebug(void * p, size_t clientSize);

    //
    // Detailed allocation tracing
    //
    void TraceMalloc(void * p, size_t clientSize, const char * pszFile, int line);
    void TraceFree(void * p, const char * pszFile, int line);

    void           TraceInsert(sHDTraceInfo *);
    sHDTraceInfo * TraceSearch(void * p);
    sHDTraceInfo * TraceRemove(void * p);

    ulong m_nAllocs;
    ulong m_nBytesAlloced;
    ulong m_nMaxBytesAlloced;
    ulong m_nRealBytesAlloced;
    ulong m_nMaxRealBytesAlloced;

    //
    // monitoring source modules--each handles one filename
    //
    sHDModuleInfo *ModuleInfoGet(const char *pszFileName);

    //
    // Sentinel bytes used to guard head and tail of allocations
    //
    sHDSentinel m_Sentinel;
    sHDSentinel m_FreedSentinel;

    ulong m_TimeTraceStart;
    ulong m_TimeLeakTrackStart;
    ulong m_HeapTestInterval;

    #define kTraceTableSize 0x9e81
    sHDTraceInfo * m_TraceTable[kTraceTableSize];

    #define kModuleTableSize 257
    sHDModuleInfo *m_ModuleTable[kModuleTableSize];
    
    #define kCreditStackSize 32
    struct sHDCreditInfo
    {
       const char * pszFile;
       int line;
    };
    
    int           m_iCreditStack;
    sHDCreditInfo m_CreditStack[kCreditStackSize];
};

///////////////////////////////////////////////////////////////////////////////
#endif /* !SHIP */
#endif /* !__DBGALLOC_H */
