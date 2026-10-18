///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/stdalloc.h $
// $Author: TOML $
// $Date: 1997/07/15 21:28:19 $
// $Revision: 1.1 $
//

#ifndef __STDALLOC_H
#define __STDALLOC_H

#include <membase.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cStdAlloc
//

class cStdAlloc : public cMallocBase
{
public:
    ///////////////////////////////////
    //
    // Constructor, destructor and architectural hooks
    //

    cStdAlloc();
    ~cStdAlloc();

    void Init();

    ///////////////////////////////////
    //
    // IMalloc functions
    //
    STDMETHOD_(void *, Alloc)   (SIZE_T cb);
    STDMETHOD_(void *, Realloc) (void * pv, SIZE_T cb);
    STDMETHOD_(void,   Free)    (void * pv);
    STDMETHOD_(SIZE_T, GetSize) (void * pv);
    STDMETHOD_(void,   HeapMinimize)();

#ifndef SHIP
    ///////////////////////////////////
    //
    // IDebugMalloc extensions to IMalloc
    //
    STDMETHOD (VerifyHeap)();
#endif

private:
    void *m_hHeap;

};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__STDALLOC_H */
