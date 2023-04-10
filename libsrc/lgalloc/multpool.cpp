///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/multpool.cpp $
// $Author: TOML $
// $Date: 1997/08/14 12:22:13 $
// $Revision: 1.3 $
//

#ifdef _WIN32

#include <lg.h>
#include <multpool.h>
#include <string.h>

#ifndef DEBUG_HEAP
#define DEBUG_HEAP 0
#endif

#if DEBUG_STATS
#pragma message ("DEBUG_STATS enabled")
#include <stdio.h>
#endif

#pragma code_seg("lgalloc")

///////////////////////////////////////////////////////////////////////////////
//
// Utility functions and macros
//

///////////////////////////////////////
//
// Allocate something at the specified location
//

inline void *operator new(size_t, cPoolAllocator *p)
{
    return (void *)p;
}

///////////////////////////////////////
//
// Allocate something at the specified location
//

inline void *operator new(size_t, cPoolAllocator **p)
{
    return (void *)p;
}

///////////////////////////////////////

#define OuterHeapAlloc(n)       m_pNext->Alloc(n)
#define OuterHeapFree(p)        m_pNext->Free(p)
#define OuterHeapRealloc(p,n)   m_pNext->Realloc(p, n)
#define OuterHeapGetSize(p)     m_pNext->GetSize(p)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMultiPool
//
// @TBD (toml 03-08-97): make pool arrangements flexible
//

#if defined(__WATCOMC__)
    #define GetMPAllocHeader(p) ((tMPAllocHeader *)(p))
#else
    #define GetMPAllocHeader(p) (reinterpret_cast < tMPAllocHeader * >(p))
#endif

typedef size_t tMPAllocHeader;
#define kHeaderSize sizeof(tMPAllocHeader)

///////////////////////////////////////
//
// Compute an index from an uncompensated size
//

inline size_t SizeFromIndex(int index)
{
    return (index + 1) << 3;
}

///////////////////////////////////////
//
// Compute an uncompensated size from an index
//

inline size_t IndexFromSize(size_t size_val)
{
    return (size_val - 1) >> 3;
}

///////////////////////////////////////
//
// Compute the maximum uncompensated size available  given an uncompensated size
//

inline size_t MaxFit(size_t size_val)
{
    return SizeFromIndex(IndexFromSize(size_val));
}

///////////////////////////////////////
//
// Free a block from the pool
//

inline void cMultiPool::PoolFree(void *pMemInfo, unsigned long cb)
{
    DebugMsgEx(HEAP, "MultiPool Freeing from Pool");
    unsigned int ix = IndexFromSize(cb);

#if DEBUG_STATS
    _pool_outstanding -= *GetMPAllocHeader(pMemInfo) + kHeaderSize;
    pPoolArrayNum[ix]--;
#endif

    m_pPoolArray[ix]->Free(pMemInfo);
}

///////////////////////////////////////
//
// Free a block from the outer allocator
//

inline void cMultiPool::AllocatorFree(void *pMemInfo)
{
#if DEBUG_STATS
    _heap_outstanding -= *GetMPAllocHeader(pMemInfo) + kHeaderSize;
#endif

    DebugMsgEx(HEAP, "MultiPool Freeing from Heap");
    OuterHeapFree(pMemInfo);
}

///////////////////////////////////////
//
// Allocate a block from the pool
//

inline void *cMultiPool::PoolAlloc(unsigned long cb)
{
    unsigned int ix = IndexFromSize(cb);

    DebugMsgEx(HEAP, "MultiPool Allocating from Pool");

#if DEBUG_STATS
    pPoolArrayNum[ix]++;
    _pool_outstanding += (cb + kHeaderSize);
    _pool_peak = _pool_outstanding > _pool_peak ? _pool_outstanding : _pool_peak;
#endif

    return m_pPoolArray[ix]->Alloc();
}

///////////////////////////////////////
//
// Allocate a block from the outer allocator
//

inline void *cMultiPool::AllocatorAlloc(unsigned long cb)
{
#if DEBUG_STATS
    _heap_outstanding += (cb + kHeaderSize);
    _heap_peak = _heap_outstanding > _heap_peak ? _heap_outstanding : _heap_peak;
#endif

    DebugMsgEx(HEAP, "MultiPool Allocating from Heap");
    return OuterHeapAlloc(cb + kHeaderSize);
}

///////////////////////////////////////
//
// Choose the correct allocator
//

inline void *cMultiPool::SelectAlloc(size_t size_val)
{
    return size_val <= kMaxPool ? PoolAlloc(size_val) : AllocatorAlloc(size_val);
}

///////////////////////////////////////
//
// Choose the correct deallocator
//

inline void cMultiPool::SelectFree(void *MemPtr)
{
    unsigned long cb = *GetMPAllocHeader(MemPtr);
    if (cb <= kMaxPool)
        PoolFree(MemPtr, cb);
    else
        AllocatorFree(MemPtr);
}

///////////////////////////////////////
//
// Constructor
//

cMultiPool::cMultiPool()
  : m_pPoolArray(0)
#if DEBUG_STATS
    ,
    pPoolArrayNum(0),
    _heap_peak(0),
    _heap_outstanding(0),
    _pool_peak(0),
    _pool_outstanding(0)
#endif
{
}

///////////////////////////////////////
//
// Debugging destrutor: Let system free memory at app terminate.
//

cMultiPool::~cMultiPool()
{
#ifdef DEBUG_STATS
    DebugMsg("*** Dumping Pool info for MultiPool ***");
    DebugMsg2("       Direct Heap: outstanding=%ldk  peak=%ldk\n", _heap_outstanding / 1024, _heap_peak / 1024);
    DebugMsg2("    Multipool heap: outstanding=%ldk  peak=%ldk\n", _pool_outstanding / 1024, _pool_peak / 1024);

    for (int i = 0; i < kNumPools; i++)
    {
        DebugMsg6("    PoolArray[%d] (%d bytes)  = %ld items outstanding, %ld Blocks (%ld bytes each, %ldk total)",
                  i,
                  (i + 1) * 8,
                  pPoolArrayNum[i],
                  m_pPoolArray[i]->GetBlockNum(),
                  m_pPoolArray[i]->GetBlockSize(),
        m_pPoolArray[i]->GetBlockNum() * (m_pPoolArray[i]->GetBlockSize()) / 1024);

        unsigned long diff = m_pPoolArray[i]->GetTakes() - m_pPoolArray[i]->GetFrees();

        DebugMsg4("            %ld takes - %ld frees = %ld (%ldk) unfreed",
                  m_pPoolArray[i]->GetTakes(),
                  m_pPoolArray[i]->GetFrees(),
                  diff, ((i + 1) * 8 * diff) / 1024);

        DebugMsg2("            %ld maximum takes (%ldk)",
                  m_pPoolArray[i]->GetMaxTakes(),
                  (m_pPoolArray[i]->GetMaxTakes() * (i + 1) * 8) / 1024);
    }
#endif
}

///////////////////////////////////////
//
// Initializtion
//

void cMultiPool::Init()
{

#if DEBUG_STATS
    pPoolArrayNum = (ulong *)OuterHeapAlloc(sizeof(ulong) * kNumPools);
    memset(pPoolArrayNum, 0, sizeof(unsigned long) * kNumPools);
#endif

    m_pPoolArray = (cPoolAllocator **) OuterHeapAlloc(sizeof(cPoolAllocator *) * kNumPools);
//        AssertMsg(m_pPoolArray, "Failed to allocate MultiPool");
//        new(m_pPoolArray) (cPoolAllocator *)[kNumPools];

    int i;
    size_t elemsize = 0;
    cPoolAllocator * pLast;

// @TBD (toml 03-10-97): This brute logic needs to be cleaned up & the client should be able to tune based on usage
    // For 0 - 64, we go by 8
    for (i = 1; i <= kNumPools && i <= 8; i++)
    {
        elemsize += 8;

        m_pPoolArray[i - 1] = (cPoolAllocator *)OuterHeapAlloc(sizeof(cPoolAllocator));
//            AssertMsg(m_pPoolArray[i - 1], "Failed to allocate pool");

        new(m_pPoolArray[i - 1]) cPoolAllocator(elemsize + kHeaderSize);

    }

    // For 65 - 128, we go by 16
    for (; i <= kNumPools && i <= 16; i++)
    {
        if (i % 2 == 1)
        {
            elemsize += 16;
            pLast = m_pPoolArray[i - 1] = (cPoolAllocator *)OuterHeapAlloc(sizeof(cPoolAllocator));
//                AssertMsg(m_pPoolArray[i - 1], "Failed to allocate pool");
            new(m_pPoolArray[i - 1]) cPoolAllocator(elemsize + kHeaderSize);
        }
        else
            m_pPoolArray[i - 1] = pLast;
    }

    // For 129 - 256, we go by 32
    for (; i <= kNumPools && i <= 32; i++)
    {
        if (i % 4 == 1)
        {
            elemsize += 32;
            pLast = m_pPoolArray[i - 1] = (cPoolAllocator *)OuterHeapAlloc(sizeof(cPoolAllocator));
//                AssertMsg(m_pPoolArray[i - 1], "Failed to allocate pool");
            new(m_pPoolArray[i - 1]) cPoolAllocator(elemsize + kHeaderSize);
        }
        else
            m_pPoolArray[i - 1] = pLast;
    }


    // For 257 - 512, we go by 64
    for (; i <= kNumPools && i <= 64; i++)
    {
        if (i % 8 == 1)
        {
            elemsize += 64;
            pLast = m_pPoolArray[i - 1] = (cPoolAllocator *)OuterHeapAlloc(sizeof(cPoolAllocator));
//                AssertMsg(m_pPoolArray[i - 1], "Failed to allocate pool");
            new(m_pPoolArray[i - 1]) cPoolAllocator(elemsize + kHeaderSize);
        }
        else
            m_pPoolArray[i - 1] = pLast;
    }

    // For 513 - 768, we go by 128
    for (; i <= kNumPools && i <= 96; i++)
    {
        if (i % 16 == 1)
        {
            elemsize += 128;
            pLast = m_pPoolArray[i - 1] = (cPoolAllocator *)OuterHeapAlloc(sizeof(cPoolAllocator));
//                AssertMsg(m_pPoolArray[i - 1], "Failed to allocate pool");
            new(m_pPoolArray[i - 1]) cPoolAllocator(elemsize + kHeaderSize);
        }
        else
            m_pPoolArray[i - 1] = pLast;
    }
    
    // For 769 - 1024, we go by 256
    for (; i <= kNumPools && i <= 128; i++)
    {
        if (i % 32 == 1)
        {
            elemsize += 256;
            pLast = m_pPoolArray[i - 1] = (cPoolAllocator *)OuterHeapAlloc(sizeof(cPoolAllocator));
//                AssertMsg(m_pPoolArray[i - 1], "Failed to allocate pool");
            new(m_pPoolArray[i - 1]) cPoolAllocator(elemsize + kHeaderSize);
        }
        else
            m_pPoolArray[i - 1] = pLast;
    }

    AssertMsg(i == kNumPools + 1, "Max multipool size is 1k");

    DebugMsgEx(HEAP, "Allocated MultiPool");
    
}

///////////////////////////////////////
//
// Allocation
//

STDMETHODIMP_(void *) cMultiPool::Alloc(ULONG cb)
{
    void *pMem = SelectAlloc(cb);

    if (!pMem)
        return 0;

    // now store size, and return the actual requested memory
    *GetMPAllocHeader(pMem) = cb;
    return (void *) ((GetMPAllocHeader(pMem)) + 1);
}

///////////////////////////////////////
//
// Reallocation
//

STDMETHODIMP_(void *) cMultiPool::Realloc(void * pMem, ULONG cb)
{
    DebugMsgEx(HEAP, "MultiPool reallocating");

    tMPAllocHeader *pMemInfo = GetMPAllocHeader(pMem) - 1;

    // If we are shrinking...
    if (cb <= *pMemInfo)
        // ...then don't do anything
        return pMem;

    // Growing...

    // If we are in the pool...
    if (*pMemInfo <= kMaxPool)
    {
        // ...and we still want to remain within the same pool
        if (cb <= MaxFit(*pMemInfo))
        {
            // ...then just change the size
            *pMemInfo = cb;
            return pMem;
        }

        // Just alloc, copy, and free.
        void *pNewMem = SelectAlloc(cb);

        if (pNewMem)
        {
            // Set the size (warning: conversion from unsigned long to
            // tMPAllocHeader)
            *GetMPAllocHeader(pNewMem) = cb;

            memcpy(GetMPAllocHeader(pNewMem) + 1, pMem, *pMemInfo);
            SelectFree(pMemInfo);
            return GetMPAllocHeader(pNewMem) + 1;
        }
    }

    // ...else we are not in a pool and we want to get bigger...
    else
    {
        // Let the subordinate realloc do the work
        void *pNewMem = OuterHeapRealloc(pMemInfo, cb + kHeaderSize);

        if (pNewMem)
        {
            // Set the size (warning: conversion from unsigned long to
            // tMPAllocHeader)
            *GetMPAllocHeader(pNewMem) = cb;
            return GetMPAllocHeader(pNewMem) + 1;
        }
    }

    return 0;
}


///////////////////////////////////////
//
// Free
//

STDMETHODIMP_(void) cMultiPool::Free(void * pMem)
{
    tMPAllocHeader *pMemInfo = (GetMPAllocHeader(pMem)) - 1;

    SelectFree(pMemInfo);
}


///////////////////////////////////////
//
//  Get the usable size of a block.
//

unsigned long cMultiPool::GetSize(void *pMem)
{
    DebugMsgEx1(HEAP, "cMultiPool::GetSize %p", pMem);

    tMPAllocHeader *pMemInfo = GetMPAllocHeader(pMem) - 1;

    DebugMsgEx1(HEAP, "...Size is %lu", *pMemInfo);

    // If we are in the pool...
    if (*pMemInfo <= kMaxPool)
        MaxFit(*pMemInfo);
    else
        return OuterHeapGetSize(pMemInfo) - kHeaderSize;

    return (*pMemInfo);
}

///////////////////////////////////////

#ifndef SHIP
STDMETHODIMP cMultiPool::VerifyHeap()
{
    return m_pNext->VerifyHeap();
}
#endif

///////////////////////////////////////////////////////////////////////////////
#endif
