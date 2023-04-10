///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/stdalloc.cpp $
// $Author: TOML $
// $Date: 1997/07/15 21:27:38 $
// $Revision: 1.1 $
//

#ifdef _WIN32

#include <windows.h>
#include <stdalloc.h>

#pragma code_seg("lgalloc")

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cStdAlloc, members
//

cStdAlloc::cStdAlloc()
{
    m_hHeap = 0;
}

///////////////////////////////////////

cStdAlloc::~cStdAlloc()
{
#if 0
    if (m_hHeap)
        HeapDestroy(m_hHeap);
#endif
}

///////////////////////////////////////

void cStdAlloc::Init()
{
    m_hHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x40000, 0);
}

///////////////////////////////////////

STDMETHODIMP_(void *) cStdAlloc::Alloc(ULONG cb)
{
    return HeapAlloc(m_hHeap, 0, cb);
}

///////////////////////////////////////

STDMETHODIMP_(void *) cStdAlloc::Realloc(void * pv, ULONG cb)
{
    return HeapReAlloc(m_hHeap, 0, pv, cb);
}

///////////////////////////////////////

STDMETHODIMP_(void) cStdAlloc::Free(void * pv)
{
    HeapFree(m_hHeap, 0, pv);
}

///////////////////////////////////////

STDMETHODIMP_(ULONG) cStdAlloc::GetSize(void * pv)
{
    return HeapSize(m_hHeap, 0, pv);
}

///////////////////////////////////////

STDMETHODIMP_(void) cStdAlloc::HeapMinimize()
{
    HeapCompact(m_hHeap, 0);
}

///////////////////////////////////////
#ifndef SHIP
STDMETHODIMP cStdAlloc::VerifyHeap()
{
    return (HeapValidate(m_hHeap, 0, 0)) ? S_OK : E_FAIL;
}
#endif
///////////////////////////////////////////////////////////////////////////////
#endif
