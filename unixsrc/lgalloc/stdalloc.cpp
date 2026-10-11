#include <stdalloc.h>

#include <malloc.h>
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
    return memory ? malloc_usable_size(memory) : 0;
}

STDMETHODIMP_(void) cStdAlloc::HeapMinimize()
{
    malloc_trim(0);
}

#ifndef SHIP
STDMETHODIMP cStdAlloc::VerifyHeap()
{
    return S_OK;
}
#endif
