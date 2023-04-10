///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/memtimer.cpp $
// $Author: TOML $
// $Date: 1997/07/15 21:45:16 $
// $Revision: 1.2 $
//

#if defined(_WIN32) && !defined(SHIP)

#include <memtimer.h>
#include <memcore.h>
#include <timings.h>

#pragma code_seg("lgalloc")

///////////////////////////////////////////////////////////////////////////////

#ifdef __WATCOMC__
#pragma initialize library
#else
#pragma init_seg (lib)
#endif

class cMemAllocTimerReporter
{
public:
    cMemAllocTimerReporter()
    {

    }

    ~cMemAllocTimerReporter()
    {
        if (g_pMemAllocTimer)
            g_pMemAllocTimer->DumpTimes();
    }
};

static cMemAllocTimerReporter g_MemAllocTimerReporter;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMemAllocTimer, members
//

cMemAllocTimer::cMemAllocTimer()
{

}

///////////////////////////////////////

cMemAllocTimer::~cMemAllocTimer()
{

}

///////////////////////////////////////

STDMETHODIMP_(void *) cMemAllocTimer::Alloc(ULONG cb)
{
    m_AllocTimer.Start();
    void * p = m_pNext->Alloc(cb);
    m_AllocTimer.MarkStop();
    return p;
}

///////////////////////////////////////

STDMETHODIMP_(void *) cMemAllocTimer::Realloc(void * pv, ULONG cb)
{
    m_ReallocTimer.Start();
    void * p = m_pNext->Realloc(pv, cb);
    m_ReallocTimer.MarkStop();
    return p;
}

///////////////////////////////////////

STDMETHODIMP_(void) cMemAllocTimer::Free(void * pv)
{
    m_FreeTimer.Start();
    m_pNext->Free(pv);
    m_FreeTimer.MarkStop();
}

///////////////////////////////////////

STDMETHODIMP_(ULONG) cMemAllocTimer::GetSize(void * pv)
{
    return m_pNext->GetSize(pv);
}

///////////////////////////////////////

STDMETHODIMP_(int) cMemAllocTimer::DidAlloc(void * pv)
{
    return m_pNext->DidAlloc(pv);
}

///////////////////////////////////////

STDMETHODIMP_(void) cMemAllocTimer::HeapMinimize()
{
    m_pNext->HeapMinimize();
}

///////////////////////////////////////
#ifndef SHIP

STDMETHODIMP_(void *) cMemAllocTimer::AllocEx(ULONG cb, const char * pszFile, int line)
{
    m_AllocTimer.Start();
    void * p = m_pNext->AllocEx(cb, pszFile, line);
    m_AllocTimer.MarkStop();
    return p;
}

///////////////////////////////////////

STDMETHODIMP_(void *) cMemAllocTimer::ReallocEx(void * pv, ULONG cb, const char * pszFile, int line)
{
    m_ReallocTimer.Start();
    void * p = m_pNext->ReallocEx(pv, cb, pszFile, line);
    m_ReallocTimer.MarkStop();
    return p;
}

///////////////////////////////////////

STDMETHODIMP_(void) cMemAllocTimer::FreeEx(void * pv, const char * pszFile, int line)
{
    m_FreeTimer.Start();
    m_pNext->FreeEx(pv, pszFile, line);
    m_FreeTimer.MarkStop();
}

///////////////////////////////////////

STDMETHODIMP cMemAllocTimer::VerifyAlloc(void * pv)
{
    return m_pNext->VerifyAlloc(pv);
}

///////////////////////////////////////

STDMETHODIMP cMemAllocTimer::VerifyHeap()
{
    return m_pNext->VerifyHeap();
}

#endif
///////////////////////////////////////////////////////////////////////////////
#endif
