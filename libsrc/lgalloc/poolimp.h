///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/poolimp.h $
// $Author: TOML $
// $Date: 1997/08/14 12:22:16 $
// $Revision: 1.6 $
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
#define PoolCoreAllocPage()   malloc(kPageSize)
#define PoolCoreFreePage(p)   free(p)
#else
#define PoolCoreAllocPage()   cPoolCore::AllocPage()
#define PoolCoreFreePage(p)   cPoolCore::FreePage(p)
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

    #ifdef TRACK_ALLOCS
    // For debugging/optimization:
    unsigned long GetBlockNum();
    unsigned long GetTakes();
    unsigned long GetBlockSize();
    unsigned long GetFrees();
    unsigned long GetMaxTakes();
    #endif

private:
    void ThreadNewBlock();

    sPoolBlock *    m_pFreeList;
    size_t          m_nElementSize;
    unsigned        m_nBlockingFactor;

    cPoolAllocator * m_pNextPool;

    #ifdef TRACK_ALLOCS
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

#ifdef TRACK_ALLOCS
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
