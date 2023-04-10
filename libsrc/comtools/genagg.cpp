///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/genagg.cpp $
// $Author: TOML $
// $Date: 1997/10/28 18:55:16 $
// $Revision: 1.24 $
//

#include <lg.h>
#include <comtools.h>
#include <objcoll.h>
#include <aggmemb.h>
#include <genagg.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Create an instance of the generic implementation of IAggregate
//

IAggregate * LGAPI CreateGenericAggregate(const char * pszName, int fCreateFlags)
    {
    return new cGenericAggregate(pszName, fCreateFlags);
    }

///////////////////////////////////////////////////////////////////////////////
//
// Helper function to pass request for add
//

void LGAPI _AddToAggregate(IUnknown * pUnknown, const sAggAddInfo * pAddInfo, unsigned nAdd)
   {
   AutoConIPtr(Aggregate, pUnknown);
   AssertMsg(pAggregate, "Expected but did not find support for IAggregate");

   if (!pAggregate)
      return;

   pAggregate->Add(pAddInfo, nAdd);
   }

///////////////////////////////////////////////////////////////////////////////
//
// Simple wrapper to provide aggregation control to C clients
//

class cSimpleAggregateControlWrapper
    {
public:
    cSimpleAggregateControlWrapper(IUnknown * pAggregateMember, tGenericOnFinalReleaseFunc pfnOnFinalRelease)
      : m_pAggregateMember(pAggregateMember),
        m_pfnOnFinalRelease(pfnOnFinalRelease)
        {
        __m_AggregateControl.InitControl(this);
        }

    DECLARE_SIMPLE_AGGREGATE_CONTROL(cSimpleAggregateControlWrapper);

private:

    IUnknown *                  m_pAggregateMember;
    tGenericOnFinalReleaseFunc  m_pfnOnFinalRelease;
    };

///////////////////////////////////////

inline void cSimpleAggregateControlWrapper::OnFinalRelease()
    {
    (*m_pfnOnFinalRelease)(m_pAggregateMember);
    delete this;
    }

///////////////////////////////////////

IMPLEMENT_SIMPLE_AGGREGATE_CONTROL(cSimpleAggregateControlWrapper);

///////////////////////////////////////

IUnknown * LGAPI _CreateSimpleAggregateControl(IUnknown * pAggregateMember, tGenericOnFinalReleaseFunc pfnOnFinalRelease)
    {
    cSimpleAggregateControlWrapper * pWrapper = new cSimpleAggregateControlWrapper(pAggregateMember, pfnOnFinalRelease);
    if (pWrapper)
        return &pWrapper->__m_AggregateControl;
    else
        return NULL;
    }

///////////////////////////////////////////////////////////////////////////////
//
// Complex wrapper to provide aggregation control to C clients
//

class cComplexAggregateControlWrapper
    {
public:
    cComplexAggregateControlWrapper(IUnknown * pAggregateMember,
                                    tGenericOnFinalReleaseFunc pfnOnFinalRelease,
                                    tGenericAggregateMemberControlFunc pfnConnect,
                                    tGenericAggregateMemberControlFunc pfnPostConnect,
                                    tGenericAggregateMemberControlFunc pfnInit,
                                    tGenericAggregateMemberControlFunc pfnEnd,
                                    tGenericAggregateMemberControlFunc pfnDisconnect)

      : m_pAggregateMember(pAggregateMember),
        m_pfnOnFinalRelease(pfnOnFinalRelease),
        m_pfnConnect(pfnConnect),
        m_pfnPostConnect(pfnPostConnect),
        m_pfnInit(pfnInit),
        m_pfnEnd(pfnEnd),
        m_pfnDisconnect(pfnDisconnect)
        {
        __m_AggregateControl.InitControl(this);
        }

    DECLARE_COMPLEX_AGGREGATE_CONTROL(cComplexAggregateControlWrapper);

private:
    HRESULT Connect();
    HRESULT PostConnect();
    HRESULT Init();
    HRESULT End();
    HRESULT Disconnect();

    IUnknown *                          m_pAggregateMember;
    tGenericOnFinalReleaseFunc          m_pfnOnFinalRelease;
    tGenericAggregateMemberControlFunc  m_pfnConnect;
    tGenericAggregateMemberControlFunc  m_pfnPostConnect;
    tGenericAggregateMemberControlFunc  m_pfnInit;
    tGenericAggregateMemberControlFunc  m_pfnEnd;
    tGenericAggregateMemberControlFunc  m_pfnDisconnect;
    };

///////////////////////////////////////

inline void cComplexAggregateControlWrapper::OnFinalRelease()
    {
    (*m_pfnOnFinalRelease)(m_pAggregateMember);
    delete this;
    }

///////////////////////////////////////

inline HRESULT cComplexAggregateControlWrapper::Connect()
    {
    return (*m_pfnConnect)(m_pAggregateMember);
    }

///////////////////////////////////////

inline HRESULT cComplexAggregateControlWrapper::PostConnect()
    {
    return (*m_pfnPostConnect)(m_pAggregateMember);
    }

///////////////////////////////////////

inline HRESULT cComplexAggregateControlWrapper::Init()
    {
    return (*m_pfnInit)(m_pAggregateMember);
    }

///////////////////////////////////////

inline HRESULT cComplexAggregateControlWrapper::End()
    {
    return (*m_pfnEnd)(m_pAggregateMember);
    }

///////////////////////////////////////

inline HRESULT cComplexAggregateControlWrapper::Disconnect()
    {
    return (*m_pfnDisconnect)(m_pAggregateMember);
    }

///////////////////////////////////////

IMPLEMENT_COMPLEX_AGGREGATE_CONTROL(cComplexAggregateControlWrapper);

///////////////////////////////////////

EXTERN IAggregateMemberControl * LGAPI
_CreateComplexAggregateControl( IUnknown * pAggregateMember,
                                tGenericOnFinalReleaseFunc pfnOnFinalRelease,
                                tGenericAggregateMemberControlFunc pfnConnect,
                                tGenericAggregateMemberControlFunc pfnPostConnect,
                                tGenericAggregateMemberControlFunc pfnInit,
                                tGenericAggregateMemberControlFunc pfnEnd,
                                tGenericAggregateMemberControlFunc pfnDisconnect)
    {
    cComplexAggregateControlWrapper * pWrapper = new cComplexAggregateControlWrapper(pAggregateMember,
                                                                                     pfnOnFinalRelease,
                                                                                     pfnConnect,
                                                                                     pfnPostConnect,
                                                                                     pfnInit,
                                                                                     pfnEnd,
                                                                                     pfnDisconnect);

    if (pWrapper)
        return &pWrapper->__m_AggregateControl;
    else
        return NULL;
    }

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGenericAggregate
//

cGenericAggregate::cGenericAggregate(const char * pszName, int /*fCreateFlags*/)
    : m_ulRefs(1), m_fSelfReferencing(0), m_ulSelfRefs(0), m_fState(0)
    {
    if (pszName)
        m_AggregateNameStr = pszName;
    }

///////////////////////////////////////

cGenericAggregate::~cGenericAggregate()
    {
    AssertMsg(m_AggregateMembers.IsEmpty() && !m_ControllingUnknowns.Size(), "An aggregate was not cleaned-up properly");
    }

///////////////////////////////////////
//
// Request access to a member of the aggregate
//

STDMETHODIMP cGenericAggregate::QueryInterface(REFIID id, void ** ppReturn)
    {
    AssertMsg(!(m_fState & kEnded), "cGenericAggregate use after End()");

    *ppReturn = NULL;

    if (IsEqualOrIUnknownGUID(id, IID_IAggregate))
        *ppReturn = this;
    else
        {
        // Allow QI only for IAggregate or IUnknown until Connect...
        DebugMsgTrue(!((m_fState & kConnecting) || (m_fState & kInitialized)), "Warning: cGenericAggregate use before Connect() -- should be stack based");
        cInterfaceInfo *pMemberInfo = m_AggregateMembers.Search(&id);
        if (pMemberInfo)
            *ppReturn = pMemberInfo->pUnknown;
        }

    if (*ppReturn)
        {
        AddRef();
        return NOERROR;
        }

    return E_NOINTERFACE;
    }

///////////////////////////////////////
//
// Increase the reference count
//

STDMETHODIMP_(ULONG) cGenericAggregate::AddRef()
    {
    AssertMsg(!(m_fState & kEnded), "cGenericAggregate use after End()");

    if (!m_fSelfReferencing)
        return ++m_ulRefs;

    return ++m_ulSelfRefs;
    }

///////////////////////////////////////
//
// Decrease the reference count
//

STDMETHODIMP_(ULONG) cGenericAggregate::Release()
    {
    if (m_fSelfReferencing)
        return --m_ulSelfRefs;

    AssertMsg(m_ulRefs, "cGenericAggregate::Release() past zero");

    if (--m_ulRefs)
        {
        return m_ulRefs;
        }

    m_AggregateMembers.DestroyAll();
    m_AggregateMembers.SetEmpty();

    int size = m_ControllingUnknowns.Size();
    if (size > 0)
    {
        for (int i = size - 1; i >= 0; i--)
        {
            m_ControllingUnknowns[(index_t)i]->pInterface->Release();
            delete m_ControllingUnknowns.Remove(i);
        }
    }

    delete this;
    return 0;
    }

///////////////////////////////////////
//
// Add component(s)
//

STDMETHODIMP cGenericAggregate::Add(const sAggAddInfo * pAddInfo, unsigned nAdd)
   {
   AssertMsg(!(m_fState & (kConnecting | kInitialized)), "cGenericAggregate::Add() after Connect()");

   for (unsigned i = 0; i < nAdd; i++)
      {
      if (pAddInfo[i].pAggregated && !AddToAggregate(pAddInfo[i]))
         return E_FAIL;

      if (pAddInfo[i].pControl && !AddToControlList(pAddInfo[i]))
         return E_FAIL;
      }

   return NOERROR;
   }

///////////////////////////////////////
//
// Initialize
//

STDMETHODIMP cGenericAggregate::Init()
    {
    AssertMsg(!(m_fState & (kConnecting | kInitialized)), "cGenericAggregate double initialize");
    AssertMsg(!(m_fState & kEnded), "cGenericAggregate use after End()");

    BEGIN_DEBUG_MSG("cGenericAggregate::Init()");

    // Sort the control list
    if (m_ControlConstraints.Size())
        m_ControllingUnknowns.Sort(m_ControlConstraints.AsPointer(), m_ControlConstraints.Size());
    else
        m_ControllingUnknowns.Sort();

    //
    // Working from the end of m_ControllingUnknowns to start, we query for
    // IAggregateMemberControl and call each initialization function
    // successively.  Because of the relatively high cost of initialization,
    // we're not too concerned about the cost of the QI (toml 05-04-96)
    //

    IAggregateMemberControl * pAggregateMemberControl;
    const int nControls = m_ControllingUnknowns.Size();
    index_t iCurrentControl;

    // Call Connect() on all members
    m_fState |= kConnecting;
    BeginSelfReference();

    for (iCurrentControl = 0; iCurrentControl < nControls; iCurrentControl++)
        {
        if (m_ControllingUnknowns[iCurrentControl]->pInterface->QueryInterface(IID_IAggregateMemberControl, (void**)&pAggregateMemberControl) == NOERROR)
            {
            pAggregateMemberControl->Connect();
            pAggregateMemberControl->Release();
            }
        }
    EndSelfReference();
    m_fState &= ~kConnecting;

    // Call PostConnect() on all members
    for (iCurrentControl = 0; iCurrentControl < nControls; iCurrentControl++)
        {
        if (m_ControllingUnknowns[iCurrentControl]->pInterface->QueryInterface(IID_IAggregateMemberControl, (void**)&pAggregateMemberControl) == NOERROR)
            {
            pAggregateMemberControl->PostConnect();
            pAggregateMemberControl->Release();
            }
        }

    // Call Init() on all members
    for (iCurrentControl = 0; iCurrentControl < nControls; iCurrentControl++)
        {
        if (m_ControllingUnknowns[iCurrentControl]->pInterface->QueryInterface(IID_IAggregateMemberControl, (void**)&pAggregateMemberControl) == NOERROR)
            {
            pAggregateMemberControl->Init();
            pAggregateMemberControl->Release();
            }
        }

    m_fState |= kInitialized;

    return NOERROR;

    END_DEBUG;
    }

///////////////////////////////////////
//
// End (shut down, but remain existant)
//

STDMETHODIMP cGenericAggregate::End()
    {
    AssertMsg(!(m_fState & (kDisconnecting | kEnded)), "cGenericAggregate double End()");

    BEGIN_DEBUG_MSG("cGenericAggregate::End()");

    IAggregateMemberControl * pAggregateMemberControl;
    const int nControls = m_ControllingUnknowns.Size();
    int iCurrentControl;

    if (nControls)
    {
        // Call End() on all members
        for (iCurrentControl = nControls - 1; iCurrentControl >= 0; iCurrentControl--)
        {
            if (m_ControllingUnknowns[(index_t)iCurrentControl]->pInterface->QueryInterface(IID_IAggregateMemberControl, (void**)&pAggregateMemberControl) == NOERROR)
            {
                pAggregateMemberControl->End();
                pAggregateMemberControl->Release();
            }
        }

        // Call Disconnect() on all members
        m_fState |= kDisconnecting;
        BeginSelfReference();
        for (iCurrentControl = nControls - 1; iCurrentControl >= 0; iCurrentControl--)
        {
            if (m_ControllingUnknowns[(index_t)iCurrentControl]->pInterface->QueryInterface(IID_IAggregateMemberControl, (void**)&pAggregateMemberControl) == NOERROR)
            {
                pAggregateMemberControl->Disconnect();
                pAggregateMemberControl->Release();
            }
        }
        EndSelfReference();
        m_fState &= ~kDisconnecting;
    }

    m_fState |= kEnded;

    AssertMsg(m_ulSelfRefs == 0, "Mismatch of self-references in aggregate");

    return NOERROR;

    END_DEBUG;
    }

///////////////////////////////////////
//
// Force reference count to zero
//

STDMETHODIMP_(int) cGenericAggregate::ReleaseAll()
    {
    DebugMsgTrue(m_ulRefs <= 1, "High reference count on destruction of aggregate");
    ulong retVal = m_ulRefs;
    m_ulRefs = 1;
    Release();
    return retVal;
    }

///////////////////////////////////////
//
// Hand control of self-reference inhibition.  Done automatically in
// Connect()/Disconnect() phases
//

STDMETHODIMP_(void) cGenericAggregate::BeginSelfReference()
    {
    m_fSelfReferencing++;
    }

///////////////////////////////////////
//
// Hand control of self-reference inhibition.  Done automatically in
// Connect()/Disconnect() phases
//

STDMETHODIMP_(void) cGenericAggregate::EndSelfReference()
    {
    m_fSelfReferencing--;
    AssertMsg(m_fSelfReferencing >= 0, "Inconsistent self referencing bracketing");
    }

///////////////////////////////////////
//
// Add a component to the aggregate
//

BOOL cGenericAggregate::AddToAggregate(const sAggAddInfo & addInfo)
    {
    AssertMsg(addInfo.pID, "Must supply a unique identifier if adding exposed interface to aggregate");
    AssertMsg(!addInfo.pID || !m_AggregateMembers.Search(addInfo.pID), "Aggregate already has an implementation of interface: only one is allowed");
    if (addInfo.pID)
        {
        cInterfaceInfo * pAggregateMemberInfo = new cInterfaceInfo(addInfo.pID, addInfo.pAggregated, NULL);
        m_AggregateMembers.Insert(pAggregateMemberInfo);
        return TRUE;
        }
    return FALSE;
    }

///////////////////////////////////////

BOOL cGenericAggregate::AddToControlList(const sAggAddInfo & addInfo)
    {
    if (addInfo.pControl)
        {
        const GUID * const pID = (addInfo.pID) ? addInfo.pID : &IID_IUnknown;
        addInfo.pControl->AddRef();
        sPriIntInfo<IUnknown> * pControlMemberInfo = new sPriIntInfo<IUnknown>(pID, addInfo.pControl, addInfo.pszName, addInfo.controlPriority);
        m_ControllingUnknowns.Append(pControlMemberInfo);

        AssertMsg(!addInfo.pControlConstraints || addInfo.pID, "Must supply a unique identifier if constraining aggregated component, even private ones");
        if (addInfo.pControlConstraints && addInfo.pID)
            {
            sAbsoluteConstraint absoluteConstraint;
            sRelativeConstraint * pRelativeConstraints = addInfo.pControlConstraints;

            while (pRelativeConstraints->kind != kNullConstraint)
                {
                MakeAbsolute(*pRelativeConstraints, addInfo.pID, absoluteConstraint);
                m_ControlConstraints.Append(absoluteConstraint);
                pRelativeConstraints++;
                }
            }
        }
    return TRUE;
    }

///////////////////////////////////////
