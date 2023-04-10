///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/geniset.cpp $
// $Author: TOML $
// $Date: 1997/10/16 13:19:26 $
// $Revision: 1.14 $
//
// @Note (toml 04-29-96): Right now, this is a crude implementation for
// expedience.
//
// The hash table, because it "knows" exactly how many interfaces there are,
// and that we're only supporting LG interfaces, sizes and hashes accordingly.
// However, we'll need to dynamicaly pick different hash tables based on
// the create flags when we support non LG GUIDs, and the size will have to be
// less fixed.
//
// Given this, the size of every table is enough to fill every LG interface.  This
// will clearly have to change as we start using more GUIDs


#include <lg.h>
#include <comtools.h>
#include <objcoll.h>
#include <geniset.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////////////////////////////////////////////

IInterfaceSet * CreateInterfaceSet(int fCreateFlags)
    {
    return new cGenericInterfaceSet(fCreateFlags);
    }

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGenericInterfaceSet
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cGenericInterfaceSet, IInterfaceSet);

///////////////////////////////////////

cGenericInterfaceSet::cGenericInterfaceSet(int fCreateFlags)
    {
    DebugMsg1("cGenericInterfaceSet::cGenericInterfaceSet(%x)", fCreateFlags);
    // @Note (toml 04-29-96): This crude implementation only does LG interfaces
    AssertMsg(!(fCreateFlags & kISetKeyNonLGGUIDs), "cGenericInterfaceSet implementation restricted to LG GUIDs right now!");
    }

///////////////////////////////////////

cGenericInterfaceSet::~cGenericInterfaceSet()
    {
    m_InfoList.Empty();
    DebugMsg("cGenericInterfaceSet::~cGenericInterfaceSet()");
    }

///////////////////////////////////////

STDMETHODIMP cGenericInterfaceSet::Add(REFIID id, IUnknown * pUnknown)
    {
    cInterfaceInfo *pMemberInfo = new cInterfaceInfo(&id, pUnknown);

    m_LookupTable.Insert(pMemberInfo);
    m_InfoList.Prepend(pMemberInfo);

    return NOERROR;
    }

///////////////////////////////////////

STDMETHODIMP cGenericInterfaceSet::Remove(REFIID id, IUnknown * pOptionalUnknown)
    {
    cInterfaceInfo *pMemberInfo = m_LookupTable.Search(&id);

    while (pMemberInfo)
        {
        if (!pOptionalUnknown || pMemberInfo->pUnknown == pOptionalUnknown)
            {
            m_LookupTable.Remove(pMemberInfo);
            delete m_InfoList.Remove(pMemberInfo);
            return NOERROR;
            }
        else
            // @TBD (toml 05-02-96): Should move on to next in table (when have time to code)
            CriticalMsg("cGenericInterfaceSet not yet fully compatible with multiple entries of same GUID");
        }

    return S_FALSE;
    }

///////////////////////////////////////

STDMETHODIMP cGenericInterfaceSet::Search(REFIID id, IUnknown ** ppReturn)
    {
    cInterfaceInfo *pMemberInfo = m_LookupTable.Search(&id);
    if (!pMemberInfo)
        {
        *ppReturn  = 0;
        return E_NOINTERFACE;
        }
    *ppReturn = pMemberInfo->pUnknown;
    return NOERROR;
    }

///////////////////////////////////////

STDMETHODIMP_(IUnknown *) cGenericInterfaceSet::FindFirst(eISetOrder, const GUID * /*pGUID*/, tISetHandle ** /*ppHandle*/)
    {
    CriticalMsg("Unimplemented call");
    return 0;
    }

///////////////////////////////////////

STDMETHODIMP_(IUnknown *) cGenericInterfaceSet::FindNext(tISetHandle * /*pHandle*/)
    {
    CriticalMsg("Unimplemented call");
    return 0;
    }

///////////////////////////////////////

STDMETHODIMP_(void) cGenericInterfaceSet::FindEnd(tISetHandle * /*pHandle*/)
    {
    CriticalMsg("Unimplemented call");
    }

///////////////////////////////////////

STDMETHODIMP cGenericInterfaceSet::ReleaseAll(eISetOrder order)
    {
    m_LookupTable.SetEmpty();
    return m_InfoList.ReleaseAll(order);
    }

///////////////////////////////////////

STDMETHODIMP cGenericInterfaceSet::Empty()
    {
    m_LookupTable.SetEmpty();
    return m_InfoList.Empty();
    }

///////////////////////////////////////

