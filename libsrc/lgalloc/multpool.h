///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/multpool.h $
// $Author: TOML $
// $Date: 1997/08/14 12:22:15 $
// $Revision: 1.3 $
//

#ifndef __MULTPOOL_H
#define __MULTPOOL_H

#include <membase.h>
#include <poolimp.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMultiPool
//
// Fast allocator for blocks < 1k
//

class cMultiPool : public cMallocBase
{
public:
    ///////////////////////////////////
    //
    // Constructor, destructor and architectural hooks
    //

    cMultiPool(); 
    ~cMultiPool();

    void Init();

    ///////////////////////////////////
    //
    // Overriden IMalloc functions
    //
    STDMETHOD_(void *, Alloc)   (ULONG cb);
    STDMETHOD_(void *, Realloc) (void * pv, ULONG cb);
    STDMETHOD_(void,   Free)    (void * pv);
    STDMETHOD_(ULONG,  GetSize) (void * pv);

    ///////////////////////////////////
    //
    // Overridden IDebugMalloc extensions to IMalloc
    //
#ifndef SHIP
    STDMETHOD (VerifyHeap)();
#endif

private:
    void * PoolAlloc(unsigned long);
    void * AllocatorAlloc(unsigned long);
    void   PoolFree(void *, unsigned long);
    void   AllocatorFree(void *);
    void * SelectAlloc(size_t);
    void   SelectFree(void *);

    cPoolAllocator ** m_pPoolArray;

    enum eConstants
    {
        kMaxPool = 1024L,
        kNumPools = 128L, // kMaxPool / 8
       
        kConstantsEnumWindener = 0xffffffff
    };

#ifdef DEBUG_STATS
    unsigned long _heap_peak;
    unsigned long _heap_outstanding;
    unsigned long _pool_peak;
    unsigned long _pool_outstanding;
    unsigned long *pPoolArrayNum;                // For debugging/optimizing
#endif
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__MULTPOOL_H */
