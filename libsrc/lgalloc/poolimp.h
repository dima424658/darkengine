///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/poolimp.h $
// $Author: TOML $
// $Date: 1998/09/22 13:02:32 $
// $Revision: 1.7 $
//
// Implementation details of allocation pools.  Most clients should only
// concern themselves with pool.h
//

#ifndef __POOLIMP_H
#define __POOLIMP_H

#include <malloc.h>
#include <heaptool.h>

#undef Free

///////////////////////////////////////////////////////////////////////////////
//
// Macros to specify how pools should get thier memory.
//

#ifndef _WIN32
#define PoolCoreAlloc(n)  malloc(n)
#define PoolCoreFree(p)   free(p)
#else
#define PoolCoreAlloc(n)  cPoolCore::Alloc(n)
#define PoolCoreFree(p)   cPoolCore::Free(p)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPoolAllocator
//
// Pooled Allocator to be used by operator new, operator delete overrides and
// by the Multi-heap
//
// cPoolAllocator will keep a freelist of items of the same size.
// When the freelist is empty it will alloc them in multiple blocks
//
// Instances of cPoolAllocator *must* be staticly allocated,
// typically as static instance variables of the client class,
// as there is no destructor and thus there would be a memory leak if NOT static.
//

struct sPoolBlock;

class cPoolAllocator
{
public:

    cPoolAllocator();
    cPoolAllocator(size_t elemSize); // How big will each item be, and howMany per alloc()'d Bucket

    void Init(size_t elemSize);

    void * Alloc();
    void Free(void *);

    void DumpAllocs();

    static void DumpPools();

    unsigned long GetElemSize() { return m_nElementSize; }

    #ifdef ALLOC_STATS
    // For debugging/optimization:
    unsigned long GetBlockNum();
    unsigned long GetTakes();
    unsigned long GetInUse();
    unsigned long GetFrees();
    unsigned long GetMaxTakes();
    #endif

private:
    void ThreadNewBlock();

    sPoolBlock *    m_pFreeList;
    size_t          m_nElementSize;
    unsigned        m_nBlockingFactor;

    cPoolAllocator * m_pNextPool;

    #ifdef ALLOC_STATS
    unsigned long m_nBlocks;
    unsigned long m_nInUse;
    unsigned long m_nAllocs;
    unsigned long m_nFrees;
    unsigned long m_nMaxTakes;
    sPoolBlock *  m_pAllocList;
    #endif

    static cPoolAllocator * m_pPools;
};

///////////////////////////////////////

inline cPoolAllocator::cPoolAllocator()
{

}

///////////////////////////////////////

inline cPoolAllocator::cPoolAllocator(size_t elemSize)
{
    Init(elemSize);
}

///////////////////////////////////////

#ifdef ALLOC_STATS
inline unsigned long cPoolAllocator::GetBlockNum()
{
    return m_nBlocks;
}

///////////////////////////////////////

inline unsigned long cPoolAllocator::GetTakes()
{
    return m_nAllocs;
}

///////////////////////////////////////

inline unsigned long cPoolAllocator::GetInUse()
{
   return m_nInUse;
}

///////////////////////////////////////

inline unsigned long cPoolAllocator::GetFrees()
{
    return m_nFrees;
}

///////////////////////////////////////

inline unsigned long cPoolAllocator::GetMaxTakes()
{
    return m_nMaxTakes;
}
#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__POOLIMP_H */
