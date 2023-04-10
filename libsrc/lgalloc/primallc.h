///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/primallc.h $
// $Author: TOML $
// $Date: 1998/02/11 09:28:14 $
// $Revision: 1.2 $
//

#ifndef __PRIMALLC_H
#define __PRIMALLC_H

#include <membase.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPrimaryMalloc
//
// A proxy class that serves as primary entrance to the allocation system
//

class cPrimaryMalloc : public cMallocBase, public sAllocLimits
{
public:
    ///////////////////////////////////
    //
    // Constructor, destructor and architectural hooks
    //

    cPrimaryMalloc();
    ~cPrimaryMalloc();

    void Init();
    void SetPageFunc(tAllocatorPageFunc);

    ///////////////////////////////////
    //
    // IMalloc functions
    //
    STDMETHOD_(void *, Alloc)   (ULONG cb);
    STDMETHOD_(void *, Realloc) (void * pv, ULONG cb);
    STDMETHOD_(void,   Free)    (void * pv);
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
#endif

private:

    tAllocatorPageFunc m_pfnPage;
    static ulong LGAPI DefPageFunc(ulong needed, sAllocLimits * pLimits);
};

///////////////////////////////////////

inline void cPrimaryMalloc::SetPageFunc(tAllocatorPageFunc pfnPage)
{
    m_pfnPage = pfnPage;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PRIMALLC_H */
