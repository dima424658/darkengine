///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/membase.cpp $
// $Author: TOML $
// $Date: 1998/06/10 13:58:02 $
// $Revision: 1.6 $
//

#ifdef _WIN32
#include <membase.h>

#pragma code_seg("lgalloc")

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMallocBase
//
// Provides core allocator chaining functionality
//

STDMETHODIMP cMallocBase::QueryInterface(REFIID id, void ** ppI)
{
    if (id == IID_IMalloc)
    {
        *ppI = this;
        AddRef();
        return 0;
    }

#ifndef SHIP
    if (id == IID_IDebugMalloc)
    {
        *ppI = this;
        AddRef();
        return 0;
    }
#endif

    *ppI = NULL;
    return E_NOINTERFACE;
}

///////////////////////////////////////

STDMETHODIMP_(ULONG) cMallocBase::AddRef()
{
    return 1;
}

///////////////////////////////////////

STDMETHODIMP_(ULONG) cMallocBase::Release()
{
    return 1;
}

///////////////////////////////////////

STDMETHODIMP_(ULONG) cMallocBase::GetSize(void * )
{
    return (ULONG)-1;
}

///////////////////////////////////////

STDMETHODIMP_(int) cMallocBase::DidAlloc(void * )
{
    return -1;
}

///////////////////////////////////////

STDMETHODIMP_(void) cMallocBase::HeapMinimize()
{
   if (m_pNext)
      m_pNext->HeapMinimize();
}

///////////////////////////////////////
#ifndef SHIP

STDMETHODIMP_(void *) cMallocBase::AllocEx(ULONG cb, const char *, int)
{
    return Alloc(cb);
}

///////////////////////////////////////

STDMETHODIMP_(void *) cMallocBase::ReallocEx(void * pv, ULONG cb, const char *, int)
{
    return Realloc(pv, cb);
}

///////////////////////////////////////

STDMETHODIMP_(void) cMallocBase::FreeEx(void * pv, const char *, int)
{
    Free(pv);
}

///////////////////////////////////////

STDMETHODIMP cMallocBase::VerifyAlloc(void *)
{
    return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cMallocBase::VerifyHeap()
{
   if (m_pNext)
      return m_pNext->VerifyHeap();
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(void) cMallocBase::DumpHeapInfo()
{
   if (m_pNext)
      m_pNext->DumpHeapInfo();
}

///////////////////////////////////////

STDMETHODIMP_(void) cMallocBase::DumpStats()
{
   if (m_pNext)
      m_pNext->DumpStats();
}

///////////////////////////////////////

STDMETHODIMP_(void) cMallocBase::DumpBlocks()
{
   if (m_pNext)
      m_pNext->DumpBlocks();
}

///////////////////////////////////////

STDMETHODIMP_(void) cMallocBase::DumpModules()
{
   if (m_pNext)
      m_pNext->DumpModules();
}

///////////////////////////////////////

STDMETHODIMP_(void) cMallocBase::PushCredit(const char * pszFile, int line)
{
   if (m_pNext)
      m_pNext->PushCredit(pszFile, line);
}

///////////////////////////////////////

STDMETHODIMP_(void) cMallocBase::PopCredit()
{
   if (m_pNext)
      m_pNext->PopCredit();
}

#endif
///////////////////////////////////////////////////////////////////////////////

#endif

