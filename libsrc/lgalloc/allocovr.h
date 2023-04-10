///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/allocovr.h $
// $Author: TOML $
// $Date: 1998/06/10 13:58:05 $
// $Revision: 1.5 $
//
// Include this file once in some C++ file in the executable to overload allocs
//

#ifdef _WIN32

#ifndef __cplusplus
#error Allocator override must be used with a C++ source
#endif

// #pragma message("Overriding standard allocator")
#pragma code_seg("lgalloc")

#if defined(_MSC_VER)
#undef bool
#endif

#include <stdlib.h>
#include <malloc.h>
#include <new.h>
#include <string.h>

#if defined(_MSC_VER)
#define bool sbool
#endif

#include <comtools.h>
#include <allocapi.h>
#include <mallocdb.h>

EXTERN BOOL LGAllocOverride()
{
   return TRUE;
}

#if defined(_MSC_VER)
EXTERN int LGAPI HeapInit();
EXTERN void LGAPI HeapTerm();
#endif

extern "C"
{

void * malloc(size_t size)
{
    return g_pMalloc->Alloc(size);
}

void free(void * p)
{
    g_pMalloc->Free(p);
}

void * realloc(void * p, size_t size)
{
    return g_pMalloc->Realloc(p, size);
}

void * calloc(size_t n, size_t sizeEach)
{
    void * p = g_pMalloc->Alloc(sizeEach * n);
    memset(p, 0, sizeEach * n);
    return p;
}

int _heapchk()
{
#ifndef SHIP
    return g_pMalloc->VerifyHeap();
#else
    return 0;
#endif
}

int _heapmin()
{
    g_pMalloc->HeapMinimize();
    return 1;
}

size_t _msize(void * p)
{
    return g_pMalloc->GetSize(p);
}

size_t msize(void * p)
{
    return g_pMalloc->GetSize(p);
}


#ifndef SHIP
void * malloc_db(size_t size, const char * file, int line)
{
    return g_pMalloc->AllocEx(size, file, line);
}

void free_db(void * p, const char * file, int line)
{
    g_pMalloc->FreeEx(p, file, line);
}

void * realloc_db(void * p, size_t size, const char * file, int line)
{
    return g_pMalloc->ReallocEx(p, size, file, line);
}
#endif

#if defined(__WATCOMC__)

void * _nmalloc(size_t size)
{
    return g_pMalloc->Alloc(size);
}

void _nfree(void * p)
{
    g_pMalloc->Free(p);
}

void * _nrealloc(void * p, size_t size)
{
    return g_pMalloc->Realloc(p, size);
}

void * _ncalloc(size_t n, size_t sizeEach)
{
    void * p = g_pMalloc->Alloc(sizeEach * n);
    memset(p, sizeEach * n, 0);
    return p;
}

int _nheapchk()
{
#ifndef SHIP
    return g_pMalloc->VerifyHeap();
#else
    return 0;
#endif
}

int _nheapmin()
{
    g_pMalloc->HeapMinimize();
    return 1;
}

size_t _nmsize(void * p)
{
    return g_pMalloc->GetSize(p);
}

size_t nmsize(void * p)
{
    return g_pMalloc->GetSize(p);
}

}

void * operator new(size_t size)
{
    return g_pMalloc->Alloc(size);
}

void * operator new(size_t, void * p)
{
    return p;
}

void * operator new [](size_t size)
{
    return g_pMalloc->Alloc(size);
}

void * operator new[](size_t, void * p)
{
    return p;
}

void operator delete(void * p)
{
    g_pMalloc->Free(p);
}

void operator delete[](void * p)
{
    g_pMalloc->Free(p);
}

extern "C"
{
#else

int __cdecl _heap_init()
{
    return HeapInit();
}

void __cdecl _heap_term()
{
    HeapTerm();
}

void * __cdecl _heap_alloc(size_t size)
{
    return g_pMalloc->Alloc(size);
}

void * __cdecl _nh_malloc(size_t size, int)
{
    return g_pMalloc->Alloc(size);
}

#endif

}

#endif