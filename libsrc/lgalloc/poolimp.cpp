///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/poolimp.cpp $
// $Author: TOML $
// $Date: 1997/08/14 12:22:17 $
// $Revision: 1.8 $
//
// Implementation of pools
//

#include <lg.h>
#include <pool.h>
#include <poolimp.h>

#include <stktrace.h>

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#pragma code_seg("lgalloc")

///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#include <virtmem.h>

#define kPoolCoreGrowSize (1024 * 64)
#define kPoolCoreMaxSize  (1024 * 1024 * 64)

class cPoolCore
{
public:
    static void * AllocPage();
    static void   FreePage(void *);

private:
    static void * gm_pCoreStack;
    static void * gm_pCoreStackLimit;

#ifdef DEBUG
    static ulong gm_nTotalCoreBytes;
#endif

};

void * cPoolCore::gm_pCoreStack;
void * cPoolCore::gm_pCoreStackLimit;
#ifdef DEBUG
ulong cPoolCore::gm_nTotalCoreBytes;
#endif

void * cPoolCore::AllocPage()
{
//    if (size % 2 != 0)
//        MessageBox(NULL, "Only even allocations?!", NULL, MB_OK);

    if (!gm_pCoreStack)
    {
        gm_pCoreStack = VirtualAlloc(NULL, kPoolCoreMaxSize, MEM_RESERVE, PAGE_READWRITE);
        AssertMsg(gm_pCoreStack, "VirtualAlloc failed!");
        VirtualAlloc(gm_pCoreStack, kPoolCoreGrowSize, MEM_COMMIT, PAGE_READWRITE);
        gm_pCoreStackLimit = (uchar *)gm_pCoreStack + kPoolCoreGrowSize;
    }

    void * pReturn = gm_pCoreStack;
    gm_pCoreStack = (uchar *)gm_pCoreStack + kPageSize;

    while (gm_pCoreStack > gm_pCoreStackLimit)
    {
        if (VirtualAlloc(gm_pCoreStackLimit, kPoolCoreGrowSize, MEM_COMMIT, PAGE_READWRITE))
        {
            gm_pCoreStackLimit = (uchar *)gm_pCoreStackLimit + kPoolCoreGrowSize;
        }
        else
        {
            CriticalMsg("VirtualAlloc failed!");
            return NULL;
        }
    }

#ifdef DEBUG
    gm_nTotalCoreBytes += kPageSize;
#endif

    return pReturn;
}

#endif
///////////////////////////////////////////////////////////////////////////////

struct sPoolBlock;

struct sFreePoolPart
{
    sPoolBlock * pNextFree;
};

#ifdef TRACK_ALLOCS

const nStackLevels = 8;

struct sAllocPoolPart
{
    void *Stack[nStackLevels];
    sPoolBlock *pNextAlloc;
    sPoolBlock *pPrevAlloc;
};

struct sPoolBlock : sAllocPoolPart,
 sFreePoolPart
#else
struct sPoolBlock : sFreePoolPart
#endif
{
};

#ifdef TRACK_ALLOCS
#define RealElemSize(size) ((size) + sizeof(sAllocPoolPart))
#else
#define RealElemSize(size) size
#endif

// Note: Side casts
#define PoolItemToClient(p) ((sFreePoolPart *)(p))
#define ClientToPoolItem(p) (((sPoolBlock *)((sFreePoolPart *)(p))))

#ifdef DUMP_POOLS
struct sPoolDumper
{
    ~sPoolDumper();
};

sPoolDumper::~sPoolDumper()
{
    cPoolAllocator::DumpPools();
}

sPoolDumper PoolDumper;
#endif

///////////////////////////////////////////////////////////////////////////////

cPoolAllocator * cPoolAllocator::m_pPools = 0;

///////////////////////////////////////
//
// Initializer
//

void    cPoolAllocator::Init(size_t elemSize)
{
    m_nElementSize  = elemSize;
    m_pFreeList     = 0;

#ifdef TRACK_ALLOCS
 // Debug Support:
    m_nBlocks       = 0;
    m_nInUse        = 0;
    m_nAllocs       = 0;
    m_nFrees        = 0;
    m_nMaxTakes     = 0;
    m_pAllocList    = 0;
#endif

    if (m_nElementSize < sizeof(sFreePoolPart))
        m_nElementSize = sizeof(sFreePoolPart);

    m_nBlockingFactor = kPageSize / RealElemSize(m_nElementSize);

    m_pNextPool = m_pPools;
    m_pPools = this;
}

///////////////////////////////////////
//
// Refill an empty freelist
//

void cPoolAllocator::ThreadNewBlock()
{
    DebugMsg1("Getting new block of %d", kPageSize);

    AssertMsg(!m_pFreeList, "ThreadNew called when not empty");

    // First get a new batch ...
    m_pFreeList = (sPoolBlock *)(PoolCoreAllocPage());

    if (!m_pFreeList)
        return;

    DebugMsg2("Threading New block: BlockSize = %u, elemSize = %u", kPageSize, RealElemSize(m_nElementSize));

#ifdef TRACK_ALLOCS
    m_nBlocks++;
#endif

    // ... Then start threading it, starting with the last element ...
    sPoolBlock *p = (sPoolBlock *)((char *)(m_pFreeList) + (m_nBlockingFactor - 1) * RealElemSize(m_nElementSize));

    sPoolBlock *pPrev = 0;
    for (;;)
    {
        p->pNextFree = pPrev;

        // ... and work back to the first ...
        if (p == m_pFreeList)
            break;

        pPrev = p;
        p = (sPoolBlock *)((char *)(p) - RealElemSize(m_nElementSize));
    }
}

///////////////////////////////////////
//
// Allocate one item from the pool
//

void *cPoolAllocator::Alloc()
{
    // Refill the free list if needed
    if (!m_pFreeList)
    {
        ThreadNewBlock();

        // Check for out-of-memory
        if (!m_pFreeList)
            return 0;
    }

    sPoolBlock *p = m_pFreeList;
    DebugMsg1("Alloc'd item @%#p", p);

    m_pFreeList = m_pFreeList->pNextFree;

#ifdef HEAP_CHECK
    // Check that the new node isn't still on the list somehow.
    int i = 0;
    for (sPoolBlock * p1 = m_pFreeList; p1; p1 = p1->pNextFree)
    {
        i++;
        Assert_((void *)(p1->pNextFree) != (void *)0xcdcdcdcd);
        AssertMsg(p1 != p, "Rethreading already freed");
    }
#endif

#ifdef TRACK_ALLOCS
     m_nInUse++;

    m_nAllocs++;
    if (m_nAllocs - m_nFrees > m_nMaxTakes)
        m_nMaxTakes = m_nAllocs - m_nFrees;

    // cross link to next element
    if (m_pAllocList)
        m_pAllocList->pPrevAlloc = p;

    p->pNextAlloc = m_pAllocList;

    // cross link to head
    p->pPrevAlloc = 0;
    m_pAllocList = p;

//    FillStackArray(3, nStackLevels, p->Stack);
#endif

    return PoolItemToClient(p);
}

///////////////////////////////////////
//
// Put memory back on the free chain
//

void cPoolAllocator::Free(void *p)
{
    DebugMsg1("Returning item 0x%x to freelist", p);

#if defined(TRACK_ALLOCS) || defined(HEAP_CHECK)
    sPoolBlock *fp = ClientToPoolItem(p);

#ifdef TRACK_ALLOCS
    // Cross link next element to previous element
    if (fp->pNextAlloc)
        fp->pNextAlloc->pPrevAlloc = fp->pPrevAlloc;

    if (fp->pPrevAlloc)
        fp->pPrevAlloc->pNextAlloc = fp->pNextAlloc;
    else
        m_pAllocList = fp->pNextAlloc;

    fp->pPrevAlloc = fp->pNextAlloc = (sPoolBlock *)((void *) -1);

    AssertMsg(m_nInUse, "Freeing once more than Alloc'd");
    m_nInUse--;

    m_nFrees++;
#endif

#ifdef HEAP_CHECK
    // Prevent Circular free list (and resulting memleak & corruption)
    for (sPoolBlock * p1 = m_pFreeList; p1; p1 = p1->pNextFree)
        AssertMsg1(p1 != fp, "Rethreading already freed 0x%x", this);
#endif

#endif

    ClientToPoolItem(p)->pNextFree = m_pFreeList;

    m_pFreeList = ClientToPoolItem(p);
}

///////////////////////////////////////
//
// Dump allocated blocks
//

void cPoolAllocator::DumpAllocs()
{
#ifdef TRACK_ALLOCS
    if (!m_pAllocList)
    {
        DebugMsg("No outstanding allocs");
        return;
    }

    for (sPoolBlock * p = m_pAllocList; p; p = p->pNextAlloc)
    {
        DebugMsg7("[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]", p->Stack[0],
               p->Stack[1], p->Stack[2],
               p->Stack[3], p->Stack[4],
               p->Stack[5], p->Stack[6]);
    }
#endif
}

///////////////////////////////////////
//
// Dump out all pools
//

void cPoolAllocator::DumpPools()
{
#ifdef DUMP_POOLS
    DebugMsg("DumpPools()");

    for (cPoolAllocator * p = m_pPools; p; p = p->m_pNextPool)
    {
        DebugMsg5("Pool: ES=%d BF=%d Bs=%lu A=%lu F=%lu",
               p->m_nElementSize,
               p->m_nBlockingFactor,
               p->m_nBlocks,
               p->m_nAllocs,
               p->m_nFrees);
#ifdef TRACK_ALLOCS
        p->DumpAllocs();
#endif
    }

#endif
}

///////////////////////////////////////////////////////////////////////////////
