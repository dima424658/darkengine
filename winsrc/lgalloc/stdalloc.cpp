///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/stdalloc.cpp $
// $Author: TOML $
// $Date: 1997/07/15 21:27:38 $
// $Revision: 1.1 $
//

#ifdef _WIN32

#include <win32_platform.h>
#include <stdlib.h>
#include <malloc.h>
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
#ifdef DARKENGINE_MODERN_CRT_ALLOCATOR
    m_hHeap = 0;
#else
    m_hHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x40000, 0);
#endif
}

///////////////////////////////////////

STDMETHODIMP_(void *) cStdAlloc::Alloc(SIZE_T cb)
{
#ifdef DARKENGINE_MODERN_CRT_ALLOCATOR
    return malloc(cb);
#else
    return HeapAlloc(m_hHeap, 0, cb);
#endif
}

///////////////////////////////////////

STDMETHODIMP_(void *) cStdAlloc::Realloc(void * pv, SIZE_T cb)
{
#ifdef DARKENGINE_MODERN_CRT_ALLOCATOR
    return realloc(pv, cb);
#else
    return HeapReAlloc(m_hHeap, 0, pv, cb);
#endif
}

///////////////////////////////////////

STDMETHODIMP_(void) cStdAlloc::Free(void * pv)
{
#ifdef DARKENGINE_MODERN_CRT_ALLOCATOR
    free(pv);
#else
    HeapFree(m_hHeap, 0, pv);
#endif
}

///////////////////////////////////////

STDMETHODIMP_(SIZE_T) cStdAlloc::GetSize(void * pv)
{
#ifdef DARKENGINE_MODERN_CRT_ALLOCATOR
    return pv ? (ULONG)_msize(pv) : 0;
#else
    return HeapSize(m_hHeap, 0, pv);
#endif
}

///////////////////////////////////////

STDMETHODIMP_(void) cStdAlloc::HeapMinimize()
{
#ifdef DARKENGINE_MODERN_CRT_ALLOCATOR
    _heapmin();
#else
    HeapCompact(m_hHeap, 0);
#endif
}

///////////////////////////////////////
#ifndef SHIP
STDMETHODIMP cStdAlloc::VerifyHeap()
{
#ifdef DARKENGINE_MODERN_CRT_ALLOCATOR
    return (_heapchk() == _HEAPOK) ? S_OK : E_FAIL;
#else
    return (HeapValidate(m_hHeap, 0, 0)) ? S_OK : E_FAIL;
#endif
}
#endif
///////////////////////////////////////////////////////////////////////////////
#endif
