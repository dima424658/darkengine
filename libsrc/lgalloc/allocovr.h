///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/allocovr.h $
// $Author: ccarollo $
// $Date: 1999/02/11 11:47:55 $
// $Revision: 1.6 $
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

void *  __cdecl _expand(void *, size_t)
{
   return NULL;
}


}

#endif
