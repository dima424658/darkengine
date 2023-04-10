///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/genagg.h $
// $Author: TOML $
// $Date: 1996/06/26 12:16:57 $
// $Revision: 1.11 $
//

#ifndef __GENAGG_H
#define __GENAGG_H

#include <comtools.h>
#include <objcoll.h>
#include <interset.h>
#include <pintarr.h>

///////////////////////////////////////

class cGenericAggregate : public IAggregate
    {
public:
    cGenericAggregate(const char * pszName, int fCreateFlags);
    virtual ~cGenericAggregate();

    DECLARE_UNKNOWN();

    STDMETHOD (Add)(THIS_ const sAggAddInfo *, unsigned nAdd);
    STDMETHOD (Init)();
    STDMETHOD (End)();
    STDMETHOD_(int, ReleaseAll)();
    STDMETHOD_(void, BeginSelfReference)();
    STDMETHOD_(void, EndSelfReference)();

private:

    BOOL AddToAggregate(const sAggAddInfo &);
    BOOL AddToControlList(const sAggAddInfo &);

    unsigned long   m_ulRefs;
    int             m_fSelfReferencing;
    unsigned long   m_ulSelfRefs;       // For diagnostics presently (toml 05-02-96)

    enum eStateFlags
        {
        kConnecting     = 0x01,
        kInitialized    = 0x02,
        kDisconnecting  = 0x04,
        kEnded          = 0x08
        };

    int m_fState;

    cInterfaceTable                 m_AggregateMembers;
    cPriIntArray<IUnknown>          m_ControllingUnknowns;
    cDynArray<sAbsoluteConstraint>  m_ControlConstraints;
    cStr                            m_AggregateNameStr;
    };

///////////////////////////////////////


#endif /* !__GENAGG_H */
