///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/aggmemb.h $
// $Author: JAEMZ $
// $Date: 1999/01/06 17:49:19 $
// $Revision: 1.12 $
//

#ifndef __AGGMEMB_H
#define __AGGMEMB_H

#include <comtools.h>
#include <objcguid.h>
#include <prikind.h>
#include <constrid.h>

///////////////////////////////////////////////////////////////////////////////
//
// Macros to add and init CT-template/MI object components to aggregate, doing aggregate control
//

#define MI_INIT_AGGREGATION_PRIVATE(pOuterUnknown, name, priority) \
    \
    InitDelegation(pOuterUnknown); \
    MI_ADD_TO_AGGREGATE_PRIVATE(pOuterUnknown, name, priority)

#define MI_INIT_AGGREGATION_PRIVATE_EX(pOuterUnknown, id, priority, pConstraints) \
    \
    InitDelegation(pOuterUnknown); \
    MI_ADD_TO_AGGREGATE_PRIVATE_EX(pOuterUnknown, id, priority, pConstraints)

#define MI_INIT_AGGREGATION_1(pOuterUnknown, INTERFACE1, priority, pConstraints) \
    \
    cCTDelegating<INTERFACE1>::InitDelegation(pOuterUnknown); \
    MI_ADD_TO_AGGREGATE_1(pOuterUnknown, INTERFACE1, priority, pConstraints)

#define MI_INIT_AGGREGATION_SPLIT(pOuterUnknown, GUID1, INTERFACE1, priority, pConstraints) \
    \
    cCTDelegating<INTERFACE1>::InitDelegation(pOuterUnknown); \
    MI_ADD_TO_AGGREGATE_SPLIT(pOuterUnknown, GUID1,INTERFACE1, priority, pConstraints)


#define MI_INIT_AGGREGATION_2(pOuterUnknown, INTERFACE1, INTERFACE2, priority, pConstraints) \
    \
    cCTDelegating<INTERFACE1>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE2>::InitDelegation(pOuterUnknown); \
    MI_ADD_TO_AGGREGATE_2(pOuterUnknown, INTERFACE1, INTERFACE2, priority, pConstraints)

#define MI_INIT_AGGREGATION_3(pOuterUnknown, INTERFACE1, INTERFACE2, INTERFACE3, priority, pConstraints) \
    \
    cCTDelegating<INTERFACE1>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE2>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE3>::InitDelegation(pOuterUnknown); \
    MI_ADD_TO_AGGREGATE_3(pOuterUnknown, INTERFACE1, INTERFACE2, INTERFACE3, priority, pConstraints)

#define MI_INIT_AGGREGATION_4(pOuterUnknown, INTERFACE1, INTERFACE2, INTERFACE3, INTERFACE4, priority, pConstraints) \
    \
    cCTDelegating<INTERFACE1>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE2>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE3>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE4>::InitDelegation(pOuterUnknown); \
    MI_ADD_TO_AGGREGATE_4(pOuterUnknown, INTERFACE1, INTERFACE2, INTERFACE3, INTERFACE4, priority, pConstraints)

#define MI_INIT_AGGREGATION_5(pOuterUnknown, INTERFACE1, INTERFACE2, INTERFACE3, INTERFACE4, INTERFACE5, priority, pConstraints) \
    \
    cCTDelegating<INTERFACE1>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE2>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE3>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE4>::InitDelegation(pOuterUnknown); \
    cCTDelegating<INTERFACE5>::InitDelegation(pOuterUnknown); \
    MI_ADD_TO_AGGREGATE_5(pOuterUnknown, INTERFACE1, INTERFACE2, INTERFACE3, INTERFACE4, INTERFACE5, priority, pConstraints)

///////////////////////////////////////////////////////////////////////////////
//
// Common aggregate member macros.  If all you want is to become part
// of the aggregate as a C++ client, this is what you want.
//
// "Normal":    You need to to implement Init() & End()
// "Simple":    You don't need to implement anything
// "Complex":   You need to to implement Connect(), PostConnect(), Init(), End() & Disconnect()
//
// Simple case example:
//
//  class cMyInterface : public IMyInterface
//  {
//  public:
//      cMyInterface(IUnknown * pAggregate);
//
//      DECLARE_SIMPLE_AGGREGATION(cMyInterface);
//
//      /* ... */
//  };
//
//  IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cMyInterface);
//
//  cMyInterface::cMyInterface(IUnknown * pOuterUnknown)
//  {
//      INIT_AGGREGATION_1(pOuterUnknown, IID_IMyInterface, this, kPriorityNormal, NULL);
//  }
//


//
// DECLARE_..._AGGREGATION()
//
#define DECLARE_AGGREGATION(CLASS) \
    \
    HRESULT Connect()       { return 0; } \
    HRESULT PostConnect()   { return 0; } \
    HRESULT Disconnect()    { return 0; } \
    DECLARE_COMPLEX_AGGREGATION(CLASS)

#define DECLARE_SIMPLE_AGGREGATION(CLASS) \
    \
    DECLARE_DELEGATION(); \
    DECLARE_SIMPLE_AGGREGATE_CONTROL(CLASS)

#define DECLARE_COMPLEX_AGGREGATION(CLASS) \
    \
    DECLARE_DELEGATION(); \
    DECLARE_COMPLEX_AGGREGATE_CONTROL(CLASS)


//
// IMPLEMENT_..._AGGREGATION()
//
#define IMPLEMENT_AGGREGATION(CLASS) \
    \
    IMPLEMENT_COMPLEX_AGGREGATION(CLASS)

#define IMPLEMENT_AGGREGATION_SELF_DELETE(CLASS) \
    \
    IMPLEMENT_COMPLEX_AGGREGATION_SELF_DELETE(CLASS)

#define IMPLEMENT_SIMPLE_AGGREGATION(CLASS) \
    \
    IMPLEMENT_DELEGATION(CLASS); \
    IMPLEMENT_SIMPLE_AGGREGATE_CONTROL(CLASS)

#define IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(CLASS) \
    \
    IMPLEMENT_DELEGATION(CLASS); \
    IMPLEMENT_SIMPLE_AGGREGATE_CONTROL_DELETE_CLIENT(CLASS)

#define IMPLEMENT_COMPLEX_AGGREGATION(CLASS) \
    \
    IMPLEMENT_DELEGATION(CLASS); \
    IMPLEMENT_COMPLEX_AGGREGATE_CONTROL(CLASS)

#define IMPLEMENT_COMPLEX_AGGREGATION_SELF_DELETE(CLASS) \
    \
    IMPLEMENT_DELEGATION(CLASS); \
    IMPLEMENT_COMPLEX_AGGREGATE_CONTROL_DELETE_CLIENT(CLASS)

//
// INIT_AGGREGATION_...()
//
#define INIT_AGGREGATION_PRIVATE(pOuterUnknown, name, priority) \
    \
    __INIT_AGGREGATION_BASE(pOuterUnknown); \
    ADD_TO_AGGREGATE_PRIVATE(pOuterUnknown, name, priority)

#define INIT_AGGREGATION_PRIVATE_EX(pOuterUnknown, id, priority, pConstraints) \
    \
    __INIT_AGGREGATION_BASE(pOuterUnknown); \
    ADD_TO_AGGREGATE_PRIVATE_EX(pOuterUnknown, id, priority, pConstraints)

#define INIT_AGGREGATION_1(pOuterUnknown, id1, p1, priority, pConstraints) \
    \
    __INIT_AGGREGATION_BASE(pOuterUnknown); \
    ADD_TO_AGGREGATE_1(pOuterUnknown, id1, p1, priority, pConstraints)

#define INIT_AGGREGATION_2(pOuterUnknown, id1, p1, id2, p2, priority, pConstraints) \
    \
    __INIT_AGGREGATION_BASE(pOuterUnknown); \
    ADD_TO_AGGREGATE_2(pOuterUnknown, id1, p1, id2, p2, priority, pConstraints)

#define INIT_AGGREGATION_3(pOuterUnknown, id1, p1, id2, p2, id3, p3, priority, pConstraints) \
    \
    __INIT_AGGREGATION_BASE(pOuterUnknown); \
    ADD_TO_AGGREGATE_3(pOuterUnknown, id1, p1, id2, p2, id3, p3, priority, pConstraints)

#define INIT_AGGREGATION_4(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, priority, pConstraints) \
    \
    __INIT_AGGREGATION_BASE(pOuterUnknown); \
    ADD_TO_AGGREGATE_4(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, priority, pConstraints)

#define INIT_AGGREGATION_5(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, id5, p5, priority, pConstraints) \
    \
    __INIT_AGGREGATION_BASE(pOuterUnknown); \
    ADD_TO_AGGREGATE_5(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, id5, p5, priority, pConstraints)

#define __INIT_AGGREGATION_BASE(pOuterUnknown) \
    INIT_DELEGATION(pOuterUnknown)


///////////////////////////////////////////////////////////////////////////////
//
// STRUCTS: sAggAddInfo
//

//
// Aggregate add info
//
// Aggregated component is destoyed through release of pControl
// pControl will be queried for optional IAggregateMemberControl support. pControl may be NULL
// Note only the first IUnknown becomes part of the aggregate
//
struct sAggAddInfo
   {
   // The unique identifier of the component
   const GUID *            pID;

   // The human-readable name of the component (optional)
   const char *            pszName;

   // A pointer to the interface to expose when pID is requested.  Can
   // be NULL if the added component is considered "private"
   // i.e.,  lives with aggregate, but not available through QI
   IUnknown *              pAggregated;

   // A pointer to the implicit, or controlling, IUnknown.  Can
   // be NULL if pAggregated is not NULL and the client knows
   // the new aggregate member's is already accounted for by
   // a previous call to Add()
   IUnknown *              pControl;

   // The initialization prioity of the component.  Lower numbers are
   // initialized first.
   int                     controlPriority;

   // The initializarion ordering constraints (to resolve
   // conflicts within priorities).  This is a NULL-terminated
   // array, or NULL for none
   sRelativeConstraint *   pControlConstraints;
   };

typedef struct sAggAddInfo sAggAddInfo;


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAggregateMemberControl
//

#undef INTERFACE
#define INTERFACE IAggregateMemberControl

DECLARE_INTERFACE_(IAggregateMemberControl, IUnknown)
   {
   //
   // IUnkown members
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Connect to other interfaces in the aggregate.  These should only be released during Disconnect()
   //
   STDMETHOD (Connect)(THIS) PURE;

   //
   // Called to allow custom access prior to global initialization (config/state-loading often done here)
   //
   STDMETHOD (PostConnect)(THIS) PURE;

   //
   // Initialize
   //
   STDMETHOD (Init)(THIS) PURE;

   //
   // End (shut down, but remain existant)
   //
   STDMETHOD (End)(THIS) PURE;

   //
   // Disconnect from other interfaces in the aggregate
   //
   STDMETHOD (Disconnect)(THIS) PURE;

   };


///////////////////////////////////////////////////////////////////////////////
//
// Macros to add CT-template/MI object components to aggregate, doing aggregate control
//

#define MI_ADD_TO_AGGREGATE_PRIVATE(pOuterUnknown, name, priority) \
    \
    AddPrivateToAggregate(pOuterUnknown, name, THIS_AS(IAggregateMemberControl), priority); \
    AggregateMemberControlRelease()

#define MI_ADD_TO_AGGREGATE_PRIVATE_EX(pOuterUnknown, id, priority, pConstraints) \
    \
    AddPrivateToAggregateEx(pOuterUnknown, id, THIS_AS(IAggregateMemberControl), priority, pConstraints); \
    AggregateMemberControlRelease()

#define MI_ADD_TO_AGGREGATE_1(pOuterUnknown, INTERFACE1, priority, pConstraints) \
    \
    AddToAggregate1(pOuterUnknown, IID_ ## INTERFACE1, THIS_AS(INTERFACE1), THIS_AS(IAggregateMemberControl), priority, pConstraints); \
    AggregateMemberControlRelease()

#define MI_ADD_TO_AGGREGATE_SPLIT(pOuterUnknown, GUID1, INTERFACE1, priority, pConstraints) \
    \
    AddToAggregate1(pOuterUnknown, IID_ ## GUID1, THIS_AS(INTERFACE1), THIS_AS(IAggregateMemberControl), priority, pConstraints); \
    AggregateMemberControlRelease()

#define MI_ADD_TO_AGGREGATE_2(pOuterUnknown, INTERFACE1, INTERFACE2, priority, pConstraints) \
    \
    AddToAggregate2(pOuterUnknown, IID_ ## INTERFACE1, THIS_AS(INTERFACE1), IID_ ## INTERFACE2, THIS_AS(INTERFACE2), THIS_AS(IAggregateMemberControl), priority, pConstraints); \
    AggregateMemberControlRelease()

#define MI_ADD_TO_AGGREGATE_3(pOuterUnknown, INTERFACE1, INTERFACE2, INTERFACE3, priority, pConstraints) \
    \
    AddToAggregate3(pOuterUnknown, IID_ ## INTERFACE1, THIS_AS(INTERFACE1), IID_ ## INTERFACE2, THIS_AS(INTERFACE2), IID_ ## INTERFACE3, THIS_AS(INTERFACE3), THIS_AS(IAggregateMemberControl), priority, pConstraints); \
    AggregateMemberControlRelease()

#define MI_ADD_TO_AGGREGATE_4(pOuterUnknown, INTERFACE1, INTERFACE2, INTERFACE3, INTERFACE4, priority, pConstraints) \
    \
    AddToAggregate4(pOuterUnknown, IID_ ## INTERFACE1, THIS_AS(INTERFACE1), IID_ ## INTERFACE2, THIS_AS(INTERFACE2), IID_ ## INTERFACE3, THIS_AS(INTERFACE3), IID_ ## INTERFACE4, THIS_AS(INTERFACE4), THIS_AS(IAggregateMemberControl), priority, pConstraints); \
    AggregateMemberControlRelease()

#define MI_ADD_TO_AGGREGATE_5(pOuterUnknown, INTERFACE1, INTERFACE2, INTERFACE3, INTERFACE4, INTERFACE5, priority, pConstraints) \
    \
    AddToAggregate5(pOuterUnknown, IID_ ## INTERFACE1, THIS_AS(INTERFACE1), IID_ ## INTERFACE2, THIS_AS(INTERFACE2), IID_ ## INTERFACE3, THIS_AS(INTERFACE3), IID_ ## INTERFACE4, THIS_AS(INTERFACE4), IID_ ## INTERFACE5, THIS_AS(INTERFACE5), THIS_AS(IAggregateMemberControl), priority, pConstraints); \
    AggregateMemberControlRelease()

#define THIS_AS(INTERFACE)  ((INTERFACE *)this)


///////////////////////////////////////////////////////////////////////////////
//
// Generic C++ implementation of aggregation control
//
// These macros allow quick generation of the controlling
// IUnknown or IAggregateMember interfaces. Macros are used
// instead of templates to simplify symbol generation and
// virtual IUnknown member generation
//

#ifdef __cplusplus

//
// INIT_AGGREGATE_CONTROL()
//
#define INIT_AGGREGATE_CONTROL(pAggregateMember) \
    __m_AggregateControl.InitControl(pAggregateMember)

//
// Simple aggregate control allows for plain controlling IUnknown
// behavior.  Client function OnFinalRelease() is called when
// controller reference count reaches zero.
//
#define DECLARE_SIMPLE_AGGREGATE_CONTROL(CLASS) \
   \
   void OnFinalRelease(); \
   class cSimpleAggregateControl : public IUnknown \
   { \
      cSimpleAggregateControl() \
         : m_pAggregateMember(NULL) {} \
      \
      DECLARE_UNAGGREGATABLE(); \
      void InitControl(CLASS * p) { m_pAggregateMember = p; } \
      CLASS * m_pAggregateMember; \
      friend class CLASS; \
   } __m_AggregateControl; \
   friend class cSimpleAggregateControl


#define IMPLEMENT_SIMPLE_AGGREGATE_CONTROL(CLASS) \
   \
   inline void CLASS::cSimpleAggregateControl::OnFinalRelease() \
   { \
      if (m_pAggregateMember) \
         m_pAggregateMember->OnFinalRelease(); \
   } \
   \
   IMPLEMENT_UNAGGREGATABLE(CLASS::cSimpleAggregateControl, IUnknown)


#define IMPLEMENT_SIMPLE_AGGREGATE_CONTROL_DELETE_CLIENT(CLASS) \
   \
   inline void CLASS::OnFinalRelease() \
   { \
      delete this; \
   } \
   IMPLEMENT_SIMPLE_AGGREGATE_CONTROL(CLASS)


//
// Complex aggregate control provides IAggregateMemberControl
// support.  All IAggregateMemberControl functions are delegated
// to the outer class. Client function OnFinalRelease() is called when
// controller reference count reaches zero.
//
#define DECLARE_COMPLEX_AGGREGATE_CONTROL(CLASS) \
        \
        void OnFinalRelease(); \
        class cComplexAggregateControl : public IAggregateMemberControl \
        { \
            cComplexAggregateControl() \
                 : m_pAggregateMember(NULL) {} \
            \
            DECLARE_UNAGGREGATABLE(); \
            void InitControl(CLASS * p) { m_pAggregateMember = p; } \
            STDMETHOD (Connect)(); \
            STDMETHOD (PostConnect)(); \
            STDMETHOD (Init)(); \
            STDMETHOD (End)(); \
            STDMETHOD (Disconnect)(); \
            CLASS * m_pAggregateMember; \
            friend class CLASS; \
        } __m_AggregateControl; \
        friend class cComplexAggregateControl


#define IMPLEMENT_COMPLEX_AGGREGATE_CONTROL(CLASS) \
    \
    STDMETHODIMP CLASS::cComplexAggregateControl::Connect() \
    { \
        if (m_pAggregateMember) \
           return m_pAggregateMember->Connect(); \
        return 0; \
    } \
    STDMETHODIMP CLASS::cComplexAggregateControl::PostConnect() \
    { \
        if (m_pAggregateMember) \
           return m_pAggregateMember->PostConnect(); \
        return 0; \
    } \
    STDMETHODIMP CLASS::cComplexAggregateControl::Init() \
    { \
        if (m_pAggregateMember) \
           return m_pAggregateMember->Init(); \
        return 0; \
    } \
    STDMETHODIMP CLASS::cComplexAggregateControl::End() \
    { \
        if (m_pAggregateMember) \
           return m_pAggregateMember->End(); \
        return 0; \
    } \
    STDMETHODIMP CLASS::cComplexAggregateControl::Disconnect() \
    { \
        if (m_pAggregateMember) \
           return m_pAggregateMember->Disconnect(); \
        return 0; \
    } \
    inline void CLASS::cComplexAggregateControl::OnFinalRelease() \
    { \
        if (m_pAggregateMember) \
           m_pAggregateMember->OnFinalRelease(); \
    } \
    IMPLEMENT_UNAGGREGATABLE(CLASS::cComplexAggregateControl, IAggregateMemberControl)


#define IMPLEMENT_COMPLEX_AGGREGATE_CONTROL_DELETE_CLIENT(CLASS) \
    \
    inline void CLASS::OnFinalRelease() \
    { \
        delete this; \
    } \
    IMPLEMENT_COMPLEX_AGGREGATE_CONTROL(CLASS)

#endif

///////////////////////////////////////
//
// Macros to add object components to aggregate, doing aggregate control
//
// NOTE: If using template-based tools, use their equivalent macros
//

#define ADD_TO_AGGREGATE_PRIVATE(pOuterUnknown, name, priority) \
    \
    INIT_AGGREGATE_CONTROL(this); \
    AddPrivateToAggregate(pOuterUnknown, name, &__m_AggregateControl, priority); \
    __m_AggregateControl.Release()

#define ADD_TO_AGGREGATE_PRIVATE_EX(pOuterUnknown, id, priority, pConstraints) \
    \
    INIT_AGGREGATE_CONTROL(this); \
    AddPrivateToAggregateEx(pOuterUnknown, id, &__m_AggregateControl, priority, pConstraints); \
    __m_AggregateControl.Release()

#define ADD_TO_AGGREGATE_1(pOuterUnknown, id1, p1, priority, pConstraints) \
    \
    INIT_AGGREGATE_CONTROL(this); \
    AddToAggregate1(pOuterUnknown, id1, p1, &__m_AggregateControl, priority, pConstraints); \
    __m_AggregateControl.Release()

#define ADD_TO_AGGREGATE_2(pOuterUnknown, id1, p1, id2, p2, priority, pConstraints) \
    \
    INIT_AGGREGATE_CONTROL(this); \
    AddToAggregate2(pOuterUnknown, id1, p1, id2, p2, &__m_AggregateControl, priority, pConstraints); \
    __m_AggregateControl.Release()

#define ADD_TO_AGGREGATE_3(pOuterUnknown, id1, p1, id2, p2, id3, p3, priority, pConstraints) \
    \
    INIT_AGGREGATE_CONTROL(this); \
    AddToAggregate3(pOuterUnknown, id1, p1, id2, p2, id3, p3, &__m_AggregateControl, priority, pConstraints); \
    __m_AggregateControl.Release()

#define ADD_TO_AGGREGATE_4(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, priority, pConstraints) \
    \
    INIT_AGGREGATE_CONTROL(this); \
    AddToAggregate4(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, &__m_AggregateControl, priority, pConstraints); \
    __m_AggregateControl.Release()

#define ADD_TO_AGGREGATE_5(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, id5, p5, priority, pConstraints) \
    \
    INIT_AGGREGATE_CONTROL(this); \
    AddToAggregate5(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, id5, p5, &__m_AggregateControl, priority, pConstraints); \
    __m_AggregateControl.Release()


///////////////////////////////////////////////////////////////////////////////
//
// Generic C implementation of aggregation control
//

//
// Simple aggregate control allows for plain controlling IUnknown
// behavior.  Client function OnFinalRelease() is called when
// controller reference count reaches zero.
//

EXTERN IUnknown * LGAPI _CreateSimpleAggregateControl(IUnknown * pAggregateMember, tGenericOnFinalReleaseFunc);

#define DECLARE_C_SIMPLE_AGGREGATE_CONTROL() \
    IUnknown * _pAggregateControl


#define INIT_C_SIMPLE_AGGREGATE_CONTROL(interface_implementation_instance, fnOnFinalRelease) \
    interface_implementation_instance._pAggregateControl = _CreateSimpleAggregateControl((IUnknown *) (&(interface_implementation_instance)), (tGenericOnFinalReleaseFunc)fnOnFinalRelease)


//
// Complex aggregate control provides IAggregateMemberControl
// support.  All IAggregateMemberControl functions are delegated
// to the outer class. Client function OnFinalRelease() is called when
// controller reference count reaches zero.
//

typedef HRESULT (STDMETHODCALLTYPE * tGenericAggregateMemberControlFunc)(IUnknown *);
EXTERN STDMETHODIMP NullAggregateMemberControlFunc(IUnknown *);


EXTERN IAggregateMemberControl * LGAPI
_CreateComplexAggregateControl( IUnknown * pAggregateMember,
                                tGenericOnFinalReleaseFunc,
                                tGenericAggregateMemberControlFunc pfnConnect,
                                tGenericAggregateMemberControlFunc pfnPostConnect,
                                tGenericAggregateMemberControlFunc pfnInit,
                                tGenericAggregateMemberControlFunc pfnEnd,
                                tGenericAggregateMemberControlFunc pfnDisconnect);

#define DECLARE_C_COMPLEX_AGGREGATE_CONTROL() \
    IUnknown * _pAggregateControl

#define INIT_C_COMPLEX_AGGREGATE_CONTROL(interface_implementation_instance, fnOnFinalRelease, fnConnect, fnPostConnect, fnInit, fnEnd, fnDisconnect) \
    interface_implementation_instance._pAggregateControl = (IUnknown*)_CreateComplexAggregateControl((IUnknown *) (&(interface_implementation_instance)), (tGenericOnFinalReleaseFunc)fnOnFinalRelease, (tGenericAggregateMemberControlFunc)fnConnect, (tGenericAggregateMemberControlFunc)fnPostConnect, (tGenericAggregateMemberControlFunc)fnInit, (tGenericAggregateMemberControlFunc)fnEnd, (tGenericAggregateMemberControlFunc)fnDisconnect)


///////////////////////////////////////////////////////////////////////////////
//
// Accessory macros to assist in aggregate addition
//
// Multiple adds must all be controlled by the same pControl
//

EXTERN void LGAPI _AddToAggregate(IUnknown *, const sAggAddInfo *, unsigned nAdd);

#define AddPrivateToAggregate(pOuterUnknown, name, pControl, priority) \
   \
   __BeginAddToAgg(pOuterUnknown, 1); \
       __AddToAgg(NULL, name, NULL, (pControl), priority, NULL); \
   __EndAddToAgg(pOuterUnknown)


#define AddPrivateToAggregateEx(pOuterUnknown, id, pControl, priority, pConstraints) \
   \
   __BeginAddToAgg(pOuterUnknown, 1); \
       __AddToAgg((&(id)), #id, NULL, (pControl), priority, pConstraints); \
   __EndAddToAgg(pOuterUnknown)


#define AddToAggregate1(pOuterUnknown, id1, p1, pControl, priority, pConstraints) \
   \
   __BeginAddToAgg(pOuterUnknown, 1); \
       __AddToAgg((&(id1)), #id1, p1, (pControl), priority, pConstraints); \
   __EndAddToAgg(pOuterUnknown)


#define AddToAggregate2(pOuterUnknown, id1, p1, id2, p2, pControl, priority, pConstraints) \
   \
   __BeginAddToAgg(pOuterUnknown, 2); \
       __AddToAgg((&(id1)), #id1, p1, (pControl), priority, pConstraints); \
       __AddToAgg((&(id2)), #id2, p2, 0, 0, 0); \
   __EndAddToAgg(pOuterUnknown)


#define AddToAggregate3(pOuterUnknown, id1, p1, id2, p2, id3, p3, pControl, priority, pConstraints) \
   \
   __BeginAddToAgg(pOuterUnknown, 3); \
       __AddToAgg((&(id1)), #id1, p1, (pControl), priority, pConstraints); \
       __AddToAgg((&(id2)), #id2, p2, 0, 0, 0); \
       __AddToAgg((&(id3)), #id3, p3, 0, 0, 0); \
   __EndAddToAgg(pOuterUnknown)


#define AddToAggregate4(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, pControl, priority, pConstraints) \
   \
   __BeginAddToAgg(pOuterUnknown, 4); \
       __AddToAgg((&(id1)), #id1, p1, (pControl), priority, pConstraints); \
       __AddToAgg((&(id2)), #id2, p2, 0, 0, 0); \
       __AddToAgg((&(id3)), #id3, p3, 0, 0, 0); \
       __AddToAgg((&(id4)), #id4, p4, 0, 0, 0); \
   __EndAddToAgg(pOuterUnknown)


#define AddToAggregate5(pOuterUnknown, id1, p1, id2, p2, id3, p3, id4, p4, id5, p5, pControl, priority, pConstraints) \
   \
   __BeginAddToAgg(pOuterUnknown, 5); \
       __AddToAgg((&(id1)), #id1, p1, (pControl), priority, pConstraints); \
       __AddToAgg((&(id2)), #id2, p2, 0, 0, 0); \
       __AddToAgg((&(id3)), #id3, p3, 0, 0, 0); \
       __AddToAgg((&(id4)), #id4, p4, 0, 0, 0); \
       __AddToAgg((&(id5)), #id5, p5, 0, 0, 0); \
   __EndAddToAgg(pOuterUnknown)

#define __AddToAgg(id, name, p, pCtl, priority, pConstraints) \
    aAggAddInfo[_i].pID                 = id; \
    aAggAddInfo[_i].pszName             = name; \
    aAggAddInfo[_i].pAggregated         = (IUnknown *)(p); \
    aAggAddInfo[_i].pControl            = (IUnknown *)(pCtl); \
    aAggAddInfo[_i].controlPriority     = priority; \
    aAggAddInfo[_i].pControlConstraints = pConstraints; \
    _i++

#define __BeginAddToAgg(pOuterUnknown, num) \
    if (!(pOuterUnknown)) \
        ; \
    else { \
        sAggAddInfo aAggAddInfo[num]; \
        int _i = 0

#define __EndAddToAgg(pOuterUnknown) \
      _AddToAggregate(pOuterUnknown, aAggAddInfo, sizeof(aAggAddInfo) / sizeof(sAggAddInfo)); \
    }

///////////////////////////////////////////////////////////////////////////////
//
// "New" template-based aggregate member tools
//

#undef INTERFACE

#ifdef __cplusplus

template <const int FLAGS>
class cCTAggregateMemberControl : public cCTUnaggregated<IAggregateMemberControl, &IID_IAggregateMemberControl, FLAGS>
{
public:
   virtual ~cCTAggregateMemberControl()
   {
   }

   void AggregateMemberControlRelease()
   {
      Release();
   }

   STDMETHOD (Connect)()
   {
      return S_OK;
   }

   STDMETHOD (PostConnect)()
   {
      return S_OK;
   }

   STDMETHOD (Init)()
   {
      return S_OK;
   }

   STDMETHOD (End)()
   {
      return S_OK;
   }

   STDMETHOD (Disconnect)()
   {
      return S_OK;
   }
};

#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AGGMEMB_H */
