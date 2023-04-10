///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/heap.h $
// $Author: TOML $
// $Date: 1997/08/14 12:22:21 $
// $Revision: 1.2 $
//

#ifndef __HEAP_H
#define __HEAP_H

#include <membase.h>
#include <stdlib.h>

#pragma pack(1)

///////////////////////////////////////////////////////////////////////////////

class cHeap;
class cHeapBlock;

///////////////////////////////////////////////////////////////////////////////
//
// Heap block structures
//

enum eBlockStatus
{
    kHeapFree       = 0x2d3f98a1,
    kHeapClaimed    = 0x98a12d3f
};


///////////////////////////////////////
//
// cHeapBlockHeader
//
// This structure should be 16 bytes (always)
//

class cHeapBlockHeader
{
public:

    cHeapBlockHeader(size_t);

    // Note: The Size field must be first because a zero size marks
    // the end of the block

    size_t          size;
    cHeapBlock *    pBack;
    eBlockStatus    fStatus;
    DWORD           dwReserved1;
};

///////////////////////////////////////
//
// This structure should be 16 bytes (always)
//

class cHeapBlockFree
{
public:
    cHeapBlock *    pFreePrev;
    cHeapBlock *    pFreeNext;
    DWORD           dwReserved1;
    DWORD           dwReserved2;
};

///////////////////////////////////////

class cHeapBlock : public cHeapBlockHeader, public cHeapBlockFree
{
public:
    cHeapBlock();
    cHeapBlock(cHeapBlock & insertAfter, size_t);

    enum eMergeResult
    {
        kMerged,
        kNoMerge,
        kAlreadyMerged,

        kMergeResultEnumWidener = 0xffffffff
    };

    eMergeResult MergeForward();
    eMergeResult MergeBackward();

    enum eClaimResult
    {
        kClaimedAll,
        kClaimedSome,

        kClaimResultEnumWidener = 0xffffffff
    };

    eClaimResult Claim(size_t);
    eClaimResult Reclaim(size_t, cHeapBlock & insertFreedAfter);

    void InsertAfter(cHeapBlock &);
    void InsertBefore(cHeapBlock &);
    void RemoveFromFreeChain();

    cHeapBlock & GetNextInGroup();

    int     IsFree();
    int     IsClaimed();

    int     IsWhole();

#ifdef DEBUG_HEAP
    void    AssertValid();
    void    CheckGroup();
#endif

};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cHeap
//
// A a traditional free-list mixed-block-size allocator
//

class cHeap : public cMallocBase
{
public:
    //
    // Initialization and cleanup
    //
    cHeap();
    ~cHeap();

    ///////////////////////////////////
    //
    // IMalloc functions
    //
    STDMETHOD_(void *, Alloc)   (ULONG cb);
    STDMETHOD_(void *, Realloc) (void * pv, ULONG cb);
    STDMETHOD_(void,   Free)    (void * pv);
    STDMETHOD_(ULONG,  GetSize) (void * pv);
    STDMETHOD_(void,   HeapMinimize)();

    ///////////////////////////////////
    //
    // IDebugMalloc extensions to IMalloc
    //
#ifndef SHIP
    STDMETHOD (VerifyHeap)();
#endif

 private:

    friend class cHeapBlock;

    ///////////////////////////////////

#ifdef DEBUG_HEAP
    long            m_nAlloced;
    long            m_nFree;
    static ulong    gm_nTotalCoreBytes;
#endif

    cHeapBlock      m_Base;

    ///////////////////////////////////

    size_t          FixSize(size_t);
    cHeapBlock *    MoreCore(size_t nu);
    BOOL            OfferToCore(cHeapBlock *);
    BOOL            DoMinimize();

    unsigned        m_nWholeCoreBlocks;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__HEAP_H */
