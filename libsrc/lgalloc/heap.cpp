///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/heap.cpp $
// $Author: TOML $
// $Date: 1997/08/14 12:22:10 $
// $Revision: 1.2 $
//
// This is a nice heap, originating as freeware, this is based on the source
// code publicly available in the (late) Symantec C++ product.
//
// It's a good algorithm, and runs well despite being all C.  Inner
// loops could probably stand being redone in inline assembler.
//
// It's particularly suited for allocations between 1k and 64k, which is
// what we use it for.  In this range, allocation is twice as fast as Watcom,
// free is ten times as fast.
//
// An improvement might be to have a first level list-of-lists where
// blocks on the same page are kept together to reduce paging
// while traversing on alloc. (toml 7-23-96)
//

#include <lg.h>
#include <heap.h>
#include <heaptool.h>

#include <limits.h>
#include <string.h>
#include <stdio.h>

#include <mprintf.h>

#undef Free
#undef Malloc
#undef Realloc

#pragma code_seg("lgalloc")

///////////////////////////////////////////////////////////////////////////////
//
// Debugging
// These are very important if you modify the logic in this file in any way,
// even minor.  Almost nothing sucks worse than a buggy heap (toml 07-25-96)
//
// DEBUG: basic checking
// HEAP_FILL: fill heap blocks (slow)
// CHECK_BLOCKS: check blocks (slow)
// CHECK_ALL: check heap after each alloc (very slow)
//

#ifdef DEBUG_HEAP
#define HeapAssertMsg(t, s) AssertMsg(t, s)
#define HeapAssertMsg2(t, s, p1, p2) AssertMsg2(t, s, p1, p2)
#else
#define HeapAssertMsg(t, s)
#define HeapAssertMsg2(t, s, p1, p2)
#endif

#ifdef STRONG_DEBUG
#undef STRONG_DEBUG
#define STRONG_DEBUG 1
#pragma message ("Defaulting to strong debugging")
#else
#define STRONG_DEBUG 0
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Constants
//
// Effort is placed on ensuring things line up on 16 byte boundaries
// for optimal performance under all conditions.  This is really inefficient
// for small blocks, but this heap is not intended for use on small blocks.
//

// Minimum requested size of allocation from OS
const unsigned kMinCoreSize           = 0x22000;  /* 128k + 2 pages worth of headers or slack */ 

// Size of blocks greater than which to immediately return to OS
//const unsigned kSizeImmediateCoreFree = kMinCoreSize + 1;

// 
const unsigned kMaxFix = 0x8000;

const unsigned kMinFreeCoreBlocksRetain = 2;

// Size of header
const unsigned kHeaderSize      = SizeAlign(sizeof(cHeapBlockHeader), kDoubleQuadWordShift);

// Required increment of alloc size
const unsigned kUnitSize        = SizeAlign(sizeof(cHeapBlock), kDoubleQuadWordShift);
const unsigned kUnitSizeShift   = kDoubleQuadWordShift;

// Minimum size of an allocation
const unsigned kMinAllocSize         = kUnitSize;
const unsigned kMinAllocRethreadSize = 0x400; // kUnitSize;

// Maximum size of an allocation
const unsigned kMaxAllocSize    = UINT_MAX - kUnitSize;

// Helper macros
#define AllocToBlock(p) ((cHeapBlock *)(((BYTE *) p) - kHeaderSize))
#define BlockToAlloc(p) ((void *)(((BYTE *) p) + kHeaderSize))

///////////////////////////////////////////////////////////////////////////////
//
// Placement operator new
//

inline void * operator new(size_t, void *p)
{
    return p;
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cHeapBlockHeader
//

inline cHeapBlockHeader::cHeapBlockHeader(size_t size)
  : pBack(0),
    size(size),
    fStatus(kHeapFree)
{
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cHeapBlock
//

inline cHeapBlock::cHeapBlock()
  : cHeapBlockHeader(0)
{
    // This constructor is used exclusively for free chain list heads
    pFreeNext = this;
    pFreePrev = this;
}

///////////////////////////////////////

inline cHeapBlock::cHeapBlock(cHeapBlock & insertAfter, size_t size)
  : cHeapBlockHeader(size)
{
    pFreeNext = insertAfter.pFreeNext;
    pFreePrev = &insertAfter;
    insertAfter.pFreeNext = this;
    pFreeNext->pFreePrev = this;
}

///////////////////////////////////////

inline int cHeapBlock::IsFree()
{
    return fStatus == kHeapFree;
}

///////////////////////////////////////

inline int cHeapBlock::IsClaimed()
{
    return fStatus == kHeapClaimed;
}

///////////////////////////////////////
//
// Get the next sorted block after this block
//

inline cHeapBlock & cHeapBlock::GetNextInGroup()
{
    return *(cHeapBlock *) ((char *) this + size);
}

///////////////////////////////////////

inline int cHeapBlock::IsWhole()
{
    return !(pBack || GetNextInGroup().size);
}


///////////////////////////////////////
//
// Check if block is valid
//

#ifdef DEBUG_HEAP
void cHeapBlock::AssertValid()
{
    AssertMsg2(fStatus == kHeapFree || fStatus == kHeapClaimed,
               "Invalid heap block 0x%x %x", this, fStatus);

    if (pBack)
    {
        AssertMsg(pBack->fStatus == kHeapFree || pBack->fStatus == kHeapClaimed,
                  "Invalid backward heap block");
        AssertMsg(&pBack->GetNextInGroup() == this, "Invalid back pointer");
    }

    cHeapBlock & Forward = GetNextInGroup();
    if (Forward.size)
    {
        AssertMsg(Forward.fStatus == kHeapFree || Forward.fStatus == kHeapClaimed,
                  "Invalid forward heap block");
        AssertMsg(Forward.pBack == this, "Invalid back pointer in next block");
    }
}
#endif


///////////////////////////////////////
//
// Insert this on the free list
//

inline void cHeapBlock::InsertAfter(cHeapBlock & insertAfter)
{
    pFreeNext = insertAfter.pFreeNext;
    pFreePrev = &insertAfter;
    insertAfter.pFreeNext = this;
    pFreeNext->pFreePrev = this;
}

///////////////////////////////////////

inline void cHeapBlock::InsertBefore(cHeapBlock & insertBefore)
{
    pFreeNext = &insertBefore;
    pFreePrev = insertBefore.pFreePrev;
    insertBefore.pFreePrev = this;
    pFreePrev->pFreeNext = this;
}

///////////////////////////////////////
//
// Remove this block from the free chain
//

#ifndef DEBUG
inline
#endif
void cHeapBlock::RemoveFromFreeChain()
{
#ifdef DEBUG_HEAP
    HeapAssertMsg(pFreeNext != this, "Can't delete last free block");
    HeapAssertMsg(pFreePrev != this, "Invalid links detect during RemoveFromFreeChain");
    HeapAssertMsg(IsFree(), "Can't RemoveFromFreeChain a block which isn't on the free chain");

    pFreePrev->AssertValid();
    HeapAssertMsg(pFreePrev->IsFree(), "Claimed block on free chain");

    pFreeNext->AssertValid();
    HeapAssertMsg(pFreeNext->IsFree(), "Claimed block on free chain");
#endif

    pFreeNext->pFreePrev = pFreePrev;
    pFreePrev->pFreeNext = pFreeNext;

#ifdef DEBUG_HEAP
    pFreePrev = pFreeNext = NULL;
#endif
}


///////////////////////////////////////
//
// Merge this block with the previous block if possible
//

cHeapBlock::eMergeResult cHeapBlock::MergeBackward()
{
    register cHeapBlock * p = pBack;
    if (!p)
        return kAlreadyMerged;

    if (p->IsClaimed())
        return kNoMerge;

    cHeapBlock & next = GetNextInGroup();

#ifdef DEBUG_HEAP
    p->AssertValid();

    if (next.size)
        next.AssertValid();
#endif

    const size_t oldSize = p->size;

    p->size += size;

    if (next.size)
    {
        next.pBack = p;
#ifdef DEBUG_HEAP
        next.AssertValid();
#endif
    }

#ifdef DEBUG_HEAP
    p->AssertValid();
#endif

    return kMerged;
}


///////////////////////////////////////
//
// Merge this block with the next block if possible
//

cHeapBlock::eMergeResult cHeapBlock::MergeForward()
{
    cHeapBlock & next = GetNextInGroup();

    if (!next.size)
        return kAlreadyMerged;

    if (next.IsClaimed())
        return kNoMerge;

#ifdef DEBUG_HEAP
    next.AssertValid();
#endif

    cHeapBlock & NextNext = next.GetNextInGroup();

#ifdef DEBUG_HEAP
    if (NextNext.size)
        NextNext.AssertValid();
#endif

    next.RemoveFromFreeChain();

    size += next.size;

    if (NextNext.size)
    {
        NextNext.pBack = this;
#ifdef DEBUG_HEAP
        NextNext.AssertValid();
#endif
    }

#ifdef DEBUG_HEAP
    AssertValid();
#endif

    return kMerged;
}


///////////////////////////////////////
//
// Claim the given number of bytes of this block
//

inline
cHeapBlock::eClaimResult cHeapBlock::Claim(size_t needed)
{
#ifdef DEBUG_HEAP
    HeapAssertMsg(needed <= size, "Block too small to claim");
    AssertValid();
    HeapAssertMsg(IsFree(), "Can't claim a claimed block");
#endif

    cHeapBlock & insertFreedAfter = *pFreePrev;
    RemoveFromFreeChain();
    fStatus = kHeapClaimed;

#ifdef DEBUG_HEAP
    AssertValid();
#endif

    return Reclaim(needed, insertFreedAfter);
}


///////////////////////////////////////
//
// Reclaim the given number of bytes of this block
//

cHeapBlock::eClaimResult cHeapBlock::Reclaim(size_t needed, cHeapBlock & insertFreedAfter)
{
#ifdef DEBUG_HEAP
    HeapAssertMsg(needed <= size, "Block too small to reclaim");
    AssertValid();
    HeapAssertMsg(IsClaimed(), "Can't reclaim a freed block");
#endif

    // Compute what our new size would be
    size_t newSize = size - needed;
    if (newSize <= kMinAllocRethreadSize)
        return kClaimedAll;

    size = needed;

    // Create a new block at the tail of this block
    // Use placement new to make a block out of it
    cHeapBlock * pNewBlock = new((void *) &GetNextInGroup()) cHeapBlock(insertFreedAfter, newSize);
    pNewBlock->pBack = this;

    // Fix the back pointer of the block after the new block
    cHeapBlock & nextBlock = pNewBlock->GetNextInGroup();
    if (nextBlock.size)
    {
        nextBlock.pBack = pNewBlock;
#ifdef DEBUG_HEAP
        nextBlock.AssertValid();
#endif
    }

#ifdef DEBUG_HEAP
    pNewBlock->AssertValid();

    AssertValid();
#endif

    return kClaimedSome;
}


///////////////////////////////////////
//
// Validate the group of blocks associated with this block
//

#ifdef DEBUG_HEAP
void cHeapBlock::CheckGroup()
{
    AssertValid();

    for (cHeapBlock * p = pBack; p; p = p->pBack)
        p->AssertValid();

    for (p = &this->GetNextInGroup(); p->size; p = &p->GetNextInGroup())
        p->AssertValid();
}
#endif


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cHeap
//

#ifdef DEBUG_HEAP
ulong cHeap::gm_nTotalCoreBytes;
#endif

///////////////////////////////////////

cHeap::cHeap()
  : m_nWholeCoreBlocks(0)
#ifdef DEBUG_HEAP
  , m_nFree(0),
    m_nAlloced(0)
#endif
{
}


///////////////////////////////////////

cHeap::~cHeap()
{
}

///////////////////////////////////////
//
// Fix the size of the request block
//

inline size_t cHeap::FixSize(size_t size)
{
    HeapAssertMsg(size <= kMaxAllocSize, "Block too large");

    if (size < kMaxFix)
    {
        // Allocate 1/16th more to reduce fragmentation
        size += size >> 4;
    }

    // Make room for block header
    size += kHeaderSize;
        
    // If we're not asking for less than a heap block...
    if (size > kMinAllocSize)
        // .. then ensure size is divisible by free block size
        return SizeAlign(size, kUnitSizeShift);
    else
        // ...else provide at least a heap block's worth, which is assumed divisible by unit size
        return  kMinAllocSize;

}


///////////////////////////////////////
//
// Allocate a block from the heap
//

void * cHeap::Alloc(unsigned long size)
{
    DebugMsg1("Allocating %d bytes", size);

#ifdef DEBUG_HEAP
    VerifyHeap();
#endif

    // This loop is very time critical.
    // Some of the explicit use of temporaries and
    // other weird stuff here is to assist the compilier
    // in producing well-paired, penalty-free code.  It
    // could probably be redone in assembler
    register const size_t       fixedSize   = FixSize(size);

    register cHeapBlock *       pBlock  = &m_Base;
    register cHeapBlock *       pEnd    = &m_Base;

#ifdef DEBUG_HEAP
    long nCycles = 0;
    const long nFree = m_nFree;
#endif

    for (;;)
    {
#ifdef DEBUG_HEAP
        HeapAssertMsg(nCycles <= nFree, "Heap free chain is invalid");
        pBlock->AssertValid();
        HeapAssertMsg(pBlock->IsFree(), "Claimed block on free chain");
        DebugMsg2("....checking against 0x%x of size %d", pBlock, pBlock->size);
#endif
        // If the current block is not suitable...
        if (pBlock->size < fixedSize)
        {
            // Advance to the next free block...
            pBlock = pBlock->pFreeNext;
        }
        else
        {
            DebugMsg("....block is suitable");
            break;
        }

        // If we have looped around...
        if (pEnd == pBlock)
        {
            // ...there are no suitable blocks
            DebugMsg("Getting more core");
            pBlock = MoreCore(fixedSize);
            if (!pBlock)
            {
                if (DoMinimize())
                    pBlock = MoreCore(fixedSize);

                if (!pBlock)
                    return NULL;
            }

            break;
        }

#ifdef DEBUG_HEAP
        nCycles++;
#endif
    }

#ifdef DEBUG_HEAP
    HeapAssertMsg(m_nFree >= 0, "Invalid free item count");

    pBlock->AssertValid();
    pBlock->pFreePrev->AssertValid();
    pBlock->pFreeNext->AssertValid();
    HeapAssertMsg(pBlock->IsFree(), "Claimed block on free chain");
    HeapAssertMsg(pBlock->pFreePrev->IsFree(), "Claimed block on free chain");
    HeapAssertMsg(pBlock->pFreeNext->IsFree(), "Claimed block on free chain");
#endif

    if (pBlock->IsWhole())
        m_nWholeCoreBlocks--;

#ifdef DEBUG_HEAP
    if (pBlock->Claim(fixedSize) == cHeapBlock::kClaimedAll)
    {
        m_nFree--;
        HeapAssertMsg(m_nFree >= 0, "Invalid free item count");
    }
    m_nAlloced++;
#else
    pBlock->Claim(fixedSize);
#endif

#ifdef DEBUG_HEAP

    VerifyHeap();
    HeapAssertMsg2(pBlock->size >= fixedSize, "Stored size less than required size (%d, %d)", pBlock->size, fixedSize);

    DebugMsg2("....allocated %d at 0x%x", pBlock->size, pBlock);
#endif

    return BlockToAlloc(pBlock);
}

///////////////////////////////////////
//
// Reallocate a block
//

void * cHeap::Realloc(void * pAlloc, unsigned long size)
{
    DebugMsg2("Realloc 0x%x to %d", pAlloc, size);

    // If we have a NULL pointer...
    if (!pAlloc)
    {
        DebugMsg("Null pointer for realloc");
        // ...then just do an allocation
        return Alloc(size);
    }

#ifdef DEBUG_HEAP
    VerifyHeap();
#endif

    cHeapBlock *    pBlock  = AllocToBlock(pAlloc);
    size_t          oldSize = pBlock->size - kHeaderSize;

    // If the current block is too small...
    if (oldSize < size)
    {
        DebugMsg1("....current block too small %d", oldSize);

        size_t fixedSize = FixSize(size);
        DebugMsg1("Fixed size = %d", fixedSize);

        // If we can merge with the next block...
        if (pBlock->MergeForward() == cHeapBlock::kMerged)
        {
#ifdef DEBUG_HEAP
            m_nFree--;
            HeapAssertMsg(m_nFree >= 0, "Invalid free item count");
#endif
            DebugMsg1("....merged forward and now block is %d", pBlock->size);

            // If the block is now big enough...
            if (pBlock->size >= fixedSize)
            {
                DebugMsg("....using merged block");

#ifdef DEBUG_HEAP
                // If we didn't reclaim the whole block...
                if (pBlock->Reclaim(fixedSize, m_Base) == cHeapBlock::kClaimedSome)
                {
                    // ...then we have a new free item
                    DebugMsg("....didn't use all of it");
                    m_nFree++;
                }

                VerifyHeap();
#else
                pBlock->Reclaim(fixedSize, m_Base);
#endif

                DebugMsg1("....returing 0x%x", pAlloc);

                return pAlloc;
            }
        }

        // Allocate a new block
        void * pNewAlloc = Alloc(size);

        // Copy the data portion of the old block
        if (pNewAlloc)
            memcpy(pNewAlloc, pAlloc, oldSize);

        // Now free the old block
        Free(pAlloc);

        return pNewAlloc;
    }
    // If we're shrinking a lot, we should move it into a smaller
    // block, or reclaim part of the allocation

    return pAlloc;
}


///////////////////////////////////////
//
// Allocate another chunk of memory from core
//

cHeapBlock * cHeap::MoreCore(size_t size)
{
    // End of core block marked with zero sized marker block
     size += sizeof(size_t);
    
    // If the request is for less than the core minimum,
    // set it to the minimum. The size already
    // accounts for the header.
    if (size < kMinCoreSize)
        size = kMinCoreSize;
    else
        size = size + (kPageSize - size % kPageSize);

    BYTE * cp = (BYTE *) m_pNext->Alloc(size + sizeof(size));

    if (!cp)
        return NULL;

    // Mark the end of the block with a zero size
    *((size_t *) (cp + (size - sizeof(size_t)))) = 0;

    // Use placement new to make a block out of it
    cHeapBlock * up = new((void *) cp) cHeapBlock(m_Base, size - sizeof(size_t));

#ifdef DEBUG_HEAP
    up->AssertValid();
    m_nFree++;
    gm_nTotalCoreBytes += size;
#endif

    m_nWholeCoreBlocks++;
    
    return up;
}

///////////////////////////////////////
//
// Offer a block to the core
//

inline BOOL cHeap::OfferToCore(cHeapBlock * pBlock)
{
    if (pBlock->IsWhole() && pBlock->size > 0x40000)
    {
        DebugMsg2("Returning block to OS (%u @ 0x%x)", pBlock->size, pBlock);
#ifdef DEBUG_HEAP
        gm_nTotalCoreBytes -= pBlock->size;
#endif
        pBlock->RemoveFromFreeChain();
        m_pNext->Free(pBlock);
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////
//
// Free the given block
//

void cHeap::Free(void * pAlloc)
{
    if (pAlloc)
    {
        cHeapBlock * pBlock = AllocToBlock(pAlloc);

#ifdef DEBUG_HEAP
        size_t freeSize = pBlock->size;
        VerifyHeap();
        pBlock->AssertValid();
        HeapAssertMsg(pBlock->IsClaimed(), "Block already free");
        pBlock->AssertValid();
#endif

#ifdef DEBUG_HEAP
        cHeapBlock & next = pBlock->GetNextInGroup();
        if (pBlock->MergeForward() == cHeapBlock::kMerged)
        {
            m_nFree--;
            HeapAssertMsg(m_nFree >= 0, "Invalid free item count");
        }
#else
        pBlock->MergeForward();
#endif

        cHeapBlock * const pBackBlock = pBlock->pBack;

        if (pBlock->MergeBackward() == cHeapBlock::kMerged)
        {
            pBlock = pBackBlock;
            pBlock->RemoveFromFreeChain();
#ifdef DEBUG_HEAP
            m_nFree--;
#endif
        }

        // If this is not a fully freed block or we're below our desired quota of big, whole blocks
        const BOOL fBlockIsWhole = pBlock->IsWhole();
        if (!fBlockIsWhole || m_nWholeCoreBlocks < kMinFreeCoreBlocksRetain)
        {
            pBlock->fStatus = kHeapFree;
            if (!fBlockIsWhole)
            {
               pBlock->InsertAfter(m_Base);
            }
            else
            {
               pBlock->InsertBefore(m_Base);
               m_nWholeCoreBlocks++;
            }
#ifdef DEBUG_HEAP
            m_nFree++;
#endif
        }
        else
        {
            HeapAssertMsg(!pBlock->GetNextInGroup().size, "A block of this size can't have been suballocated");
            DebugMsg2("Returning block to OS (%u @ 0x%x)", pBlock->size, pBlock);
#ifdef DEBUG_HEAP
            gm_nTotalCoreBytes -= pBlock->size;
#endif
            m_pNext->Free(pBlock);
        }

#ifdef DEBUG_HEAP
        VerifyHeap();
        m_nAlloced--;
#endif
    }
}


////////////////////////////////////////////////////////////

BOOL cHeap::DoMinimize()
{
    BOOL                        fResult = FALSE;
    register cHeapBlock *       pBlock  = m_Base.pFreeNext;
    cHeapBlock *                pTemp;
    const cHeapBlock * const    pEnd    = &m_Base;

    // If there's anything on the list...
    for (;;)
    {
        // If the current block is not cut up...
        if (pBlock->size && pBlock->IsWhole())
        {
            DebugMsg2("Returning block to OS (%u @ 0x%x)", pBlock->size, pBlock);
            pTemp = pBlock->pFreeNext;
#ifdef DEBUG_HEAP
            gm_nTotalCoreBytes -= pBlock->size;
#endif
            pBlock->RemoveFromFreeChain();
            m_pNext->Free(pBlock);
#ifdef DEBUG_HEAP
            m_nFree--;
#endif

            // Advance to the next free block...
            pBlock = pTemp;
            fResult = TRUE;
        }
        else
            pBlock = pBlock->pFreeNext;

        // If we have looped around...
        if (pBlock == pEnd)
        {
            break;
        }
    }

#ifdef DEBUG_HEAP
    VerifyHeap();
#endif

    m_pNext->HeapMinimize();

    return fResult;
}

////////////////////////////////////////////////////////////

STDMETHODIMP_(void) cHeap::HeapMinimize()
{
    DoMinimize();
}

///////////////////////////////////////
//
//  Get the usable size of a block.
//

unsigned long cHeap::GetSize(void *pAlloc)
{
    DebugMsg1("GetSize 0x%x", pAlloc);

    // If we have a NULL pointer...
    if (!pAlloc)
    {
        DebugMsg("Null pointer for GetSize");
        return (unsigned)-1;
    }

#ifdef DEBUG_HEAP
    VerifyHeap();
#endif

    cHeapBlock *p = (cHeapBlock *) (cHeapBlockFree *) pAlloc;
    size_t size = p->size - sizeof(cHeapBlockHeader);
    DebugMsg2("... GetSize 0x%x is %lu", pAlloc, size);
    return size;
}

///////////////////////////////////////
//
// Check the heap
//
#ifndef SHIP
STDMETHODIMP cHeap::VerifyHeap()
{
#ifdef DEBUG_HEAP
    m_Base.pFreeNext->CheckGroup();

    long                        nCycles = 0;
    register cHeapBlock *       p       = &m_Base;
    const cHeapBlock * const    pEnd    = &m_Base;

    for (;;)
    {
        AssertMsg(nCycles <= m_nFree, "Heap free chain is invalid");

        p->AssertValid();
        AssertMsg(p->IsFree(), "Claimed block on free chain");

        // Advance to the next free block
        p = p->pFreeNext;

        // If we have looped around...
        if (p == pEnd)
            break;

        nCycles++;
    }

    AssertMsg(nCycles == m_nFree, "Invalid free item count");
#endif
    return S_OK;
}
#endif

///////////////////////////////////////////////////////////////////////////////

