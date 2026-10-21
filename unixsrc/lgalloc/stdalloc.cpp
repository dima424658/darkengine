#include <stdalloc.h>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif
#include <stdlib.h>

cStdAlloc::cStdAlloc()
    : m_hHeap(nullptr)
{
}

cStdAlloc::~cStdAlloc() = default;

void cStdAlloc::Init()
{
}

STDMETHODIMP_(void *) cStdAlloc::Alloc(SIZE_T size)
{
    return malloc(size);
}

STDMETHODIMP_(void *) cStdAlloc::Realloc(void *memory, SIZE_T size)
{
    return realloc(memory, size);
}

STDMETHODIMP_(void) cStdAlloc::Free(void *memory)
{
    free(memory);
}

STDMETHODIMP_(SIZE_T) cStdAlloc::GetSize(void *memory)
{
    if (!memory)
        return 0;
#if defined(__APPLE__)
    return malloc_size(memory);
#else
    return malloc_usable_size(memory);
#endif
}

STDMETHODIMP_(void) cStdAlloc::HeapMinimize()
{
#if defined(__linux__) && !defined(__ANDROID__)
    malloc_trim(0);
#endif
}

#ifndef SHIP
STDMETHODIMP cStdAlloc::VerifyHeap()
{
    return S_OK;
}
#endif
