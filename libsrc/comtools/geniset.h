///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/geniset.h $
// $Author: TOML $
// $Date: 1996/12/13 16:00:27 $
// $Revision: 1.10 $
//
// Generic COM-interface-based IUnknown colection
//

#ifndef __GENISET_H
#define __GENISET_H

#include <interset.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGenericInterfaceSet
//

class cGenericInterfaceSet : public IInterfaceSet
    {
    //
    // IUnkown members
    //
    DECLARE_UNAGGREGATABLE();

public:
    cGenericInterfaceSet(int fCreateFlags);
    virtual ~cGenericInterfaceSet();

    //
    // Insertion and removal.  Because this is generic storage interface, add
    // and remove don't AddRef() or Release()
    //
    STDMETHOD (Add)(REFIID, IUnknown * pUnknown);
    STDMETHOD (Remove)(REFIID, IUnknown * pOptionalUnknown);

    //
    // Lookup
    //
    STDMETHOD (Search)(REFIID, IUnknown **);

    //
    // Iteration
    //
    STDMETHOD_(IUnknown *, FindFirst)(eISetOrder, const GUID *, tISetHandle **);
    STDMETHOD_(IUnknown *, FindNext)(tISetHandle *);
    STDMETHOD_(void, FindEnd)(tISetHandle *);

    //
    // Call Release() on all interfaces in the set
    //
    STDMETHOD (ReleaseAll)(eISetOrder);

    //
    // Empty the set without calling release
    //
    STDMETHOD (Empty)();

private:

    //
    // Set record keeping
    //
    cInterfaceTable m_LookupTable;
    cInterfaceList  m_InfoList;

    //
    // Iteration
    //
    struct sIterInfo
        {
        eISetOrder Order;
        const GUID * pIID;
        cInterfaceInfo * pNext;
        };
    sIterInfo m_IterInfo;
    };

///////////////////////////////////////////////////////////////////////////////

#endif /* !__GENISET_H */
