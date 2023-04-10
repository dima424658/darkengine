///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/memtimer.h $
// $Author: TOML $
// $Date: 1997/07/15 21:43:20 $
// $Revision: 1.2 $
//

#ifndef __MEMTIMER_H
#define __MEMTIMER_H

#ifndef SHIP

#include <membase.h>
#include <timings.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMemAllocTimer
//

class cMemAllocTimer : public cMallocBase
{
public:
    ///////////////////////////////////
    //
    // Constructor, destructor and architectural hooks
    //

    cMemAllocTimer();
    ~cMemAllocTimer();

    void DumpTimes()
    {
        m_AllocTimer.SetName("Average Alloc");
        m_ReallocTimer.SetName("Average Realloc");
        m_FreeTimer.SetName("Average Free");
        m_AllocTimer.OutputResult();
        m_ReallocTimer.OutputResult();
        m_FreeTimer.OutputResult();
        m_AllocTimer.Clear();
        m_ReallocTimer.Clear();
        m_FreeTimer.Clear();
    }

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

#ifndef SHIP
    ///////////////////////////////////
    //
    // IDebugMalloc extensions to IMalloc
    //
    STDMETHOD_(void *, AllocEx)  (ULONG cb, const char * pszFile, int line);
    STDMETHOD_(void *, ReallocEx)(void * pv, ULONG cb, const char * pszFile, int line);
    STDMETHOD_(void,   FreeEx)   (void * pv, const char * pszFile, int line);

    STDMETHOD (VerifyAlloc)(void * pv);
    STDMETHOD (VerifyHeap)();
#endif

private:
    cAverageTimer   m_AllocTimer;
    cAverageTimer   m_ReallocTimer;
    cAverageTimer   m_FreeTimer;

};

///////////////////////////////////////////////////////////////////////////////

#endif

#endif /* !__MEMTIMER_H */
