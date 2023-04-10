///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/membase.h $
// $Author: TOML $
// $Date: 1998/06/10 13:58:03 $
// $Revision: 1.4 $
//

#ifndef __MEMBASE_H
#define __MEMBASE_H

#include <comtools.h>
#include <allocapi.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMallocBase
//
// Provides core allocator chaining functionality, some basic implementations
//

class cMallocBase : public IAllocator
{
public:
    ///////////////////////////////////
    //
    // Constructor and destructor
    //

    cMallocBase();
    ~cMallocBase();

    ///////////////////////////////////
    //
    // Set/Get next in chain
    //
    void SetNext(IAllocator *);
    IAllocator * GetNext();

    ///////////////////////////////////
    //
    // IUnknown methods: we do nothing with these!
    //
    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    ///////////////////////////////////
    //
    // IMalloc functions
    //
    STDMETHOD_(void *, Alloc)   (ULONG cb) = 0;
    STDMETHOD_(void *, Realloc) (void * pv, ULONG cb) = 0;
    STDMETHOD_(void,   Free)    (void * pv) = 0;
    STDMETHOD_(ULONG,  GetSize) (void * pv);
    STDMETHOD_(int,    DidAlloc)(void * pv);
    STDMETHOD_(void,   HeapMinimize)();

#ifndef SHIP
    ///////////////////////////////////
    //
    // IDebugMalloc extensions to IMalloc
    //
    STDMETHOD_(void *, AllocEx)  (ULONG cb, const char * pszFile, int line);
    STDMETHOD_(void *, ReallocEx)(void * pv, ULONG cb, const char * pszFile, int line);
    STDMETHOD_(void,   FreeEx)   (void * pv, const char * pszFile, int line);

    STDMETHOD (VerifyAlloc)(void * pv);
    STDMETHOD (VerifyHeap)();

    STDMETHOD_(void, DumpHeapInfo)();

    STDMETHOD_(void, DumpStats)();
    STDMETHOD_(void, DumpBlocks)();
    STDMETHOD_(void, DumpModules)();

    STDMETHOD_(void, PushCredit)(const char * pszFile, int line);
    STDMETHOD_(void, PopCredit)();
#endif

protected:
    IAllocator * m_pNext;

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMallocBase, inline functions
//

inline cMallocBase::cMallocBase()
   : m_pNext(0)
{

}

///////////////////////////////////////

inline cMallocBase::~cMallocBase()
{
}

///////////////////////////////////////

inline void cMallocBase::SetNext(IAllocator * pNext)
{
    m_pNext = pNext;
}

///////////////////////////////////////

inline IAllocator * cMallocBase::GetNext()
{
    return m_pNext;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__MEMBASE_H */
