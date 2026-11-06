#ifdef _WIN32
#include <win32_platform.h>
#endif

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
    : m_hHeap(nullptr)
{
}

cStdAlloc::~cStdAlloc() = default;

void cStdAlloc::Init()
{
#if defined(_WIN32) && !defined(DARKENGINE_MODERN_CRT_ALLOCATOR)
    m_hHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x40000, 0);
#endif
}

STDMETHODIMP_(void *) cStdAlloc::Alloc(SIZE_T size)
{
#if defined(_WIN32) && !defined(DARKENGINE_MODERN_CRT_ALLOCATOR)
    return HeapAlloc(m_hHeap, 0, size);
#else
    return malloc(size);
#endif
}

STDMETHODIMP_(void *) cStdAlloc::Realloc(void *memory, SIZE_T size)
{
#if defined(_WIN32) && !defined(DARKENGINE_MODERN_CRT_ALLOCATOR)
    return HeapReAlloc(m_hHeap, 0, memory, size);
#else
    return realloc(memory, size);
#endif
}

STDMETHODIMP_(void) cStdAlloc::Free(void *memory)
{
#if defined(_WIN32) && !defined(DARKENGINE_MODERN_CRT_ALLOCATOR)
    HeapFree(m_hHeap, 0, memory);
#else
    free(memory);
#endif
}

STDMETHODIMP_(SIZE_T) cStdAlloc::GetSize(void *memory)
{
    if (!memory)
        return 0;
#if defined(_WIN32) && !defined(DARKENGINE_MODERN_CRT_ALLOCATOR)
    return HeapSize(m_hHeap, 0, memory);
#elif defined(_WIN32)
    return static_cast<SIZE_T>(_msize(memory));
#elif defined(__APPLE__)
    return malloc_size(memory);
#else
    return malloc_usable_size(memory);
#endif
}

STDMETHODIMP_(void) cStdAlloc::HeapMinimize()
{
#if defined(_WIN32) && !defined(DARKENGINE_MODERN_CRT_ALLOCATOR)
    HeapCompact(m_hHeap, 0);
#elif defined(_WIN32)
    _heapmin();
#elif defined(__linux__) && !defined(__ANDROID__)
    malloc_trim(0);
#endif
}

#ifndef SHIP
STDMETHODIMP cStdAlloc::VerifyHeap()
{
#if defined(_WIN32) && !defined(DARKENGINE_MODERN_CRT_ALLOCATOR)
    return HeapValidate(m_hHeap, 0, 0) ? S_OK : E_FAIL;
#elif defined(_WIN32)
    return _heapchk() == _HEAPOK ? S_OK : E_FAIL;
#else
    return S_OK;
#endif
}
#endif
