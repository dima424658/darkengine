#include <stdalloc.h>
#include <stdlib.h>

#ifdef _WIN32
#include <malloc.h>
#pragma code_seg("lgalloc")
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__linux__)
#include <malloc.h>
#endif

cStdAlloc::cStdAlloc()
    = default;

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
#if defined(_WIN32)
    return static_cast<SIZE_T>(_msize(memory));
#elif defined(__APPLE__)
    return malloc_size(memory);
#else
    return malloc_usable_size(memory);
#endif
}

STDMETHODIMP_(void) cStdAlloc::HeapMinimize()
{
#if defined(_WIN32)
    _heapmin();
#elif defined(__linux__) && !defined(__ANDROID__)
    malloc_trim(0);
#endif
}

#ifndef SHIP
STDMETHODIMP cStdAlloc::VerifyHeap()
{
#if defined(_WIN32)
    return _heapchk() == _HEAPOK ? S_OK : E_FAIL;
#else
    return S_OK;
#endif
}
#endif
