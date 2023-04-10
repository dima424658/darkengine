///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/objcoll.h $
// $Author: TOML $
// $Date: 1996/10/21 09:34:24 $
// $Revision: 1.19 $
//
// General interfaces for component object collections
//

#ifndef __OBJCOLL_H
#define __OBJCOLL_H

#include <comtools.h>
#include <objcguid.h>

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAggregate);
F_DECLARE_INTERFACE(IAggregateMemberControl);
F_DECLARE_INTERFACE(IInterfaceSet);
typedef struct sAggAddInfo sAggAddInfo;
typedef void tISetHandle;

///////////////////////////////////////////////////////////////////////////////
//
// Create an instance of the default implementation of collections
//

enum eInterfaceCollectionCreateFlags
   {
   //
   // The table will contain LG GUIDs (DEFINE_LG_GUID)
   //
   kISetKeyLGGUIDs     = 0x01,

   //
   // The table will contain non-LG GUIDs
   //
   kISetKeyNonLGGUIDs  = 0x02

   };

EXTERN IAggregate *    LGAPI CreateGenericAggregate(const char * pszName, int fCreateFlags);
EXTERN IInterfaceSet * LGAPI CreateInterfaceSet(const char * pszName, int fCreateFlags);


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAggregate
//
// IAggregate holds a set of interfaces, associated by interface Ids (IIDs).
// IAggregateMemberControl provides a mechanism of more complex
// initialization and clean-up.
//
// Only one object supporting a given interface may be aggregated in a given set
//
// In debug builds IAggregate implementors should randomize initialization order
// of libraries with equal priority
//

#undef INTERFACE
#define INTERFACE IAggregate

DECLARE_INTERFACE_(IAggregate, IUnknown)
   {
   //
   // IUnkown members
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Add component(s)
   //
   STDMETHOD (Add)(THIS_ const sAggAddInfo *, unsigned nAdd) PURE;

   //
   // Initialize
   //
   STDMETHOD (Init)(THIS) PURE;

   //
   // End (shut down, but remain existant)
   //
   STDMETHOD (End)(THIS) PURE;

   //
   // Because managing an arbitrary aggregate often is more complex than simple
   // reference counting allows, IAggregate presents an explicit function
   // to signal full release. Returns the reference count before the call, which
   // should be 1 in a normal termination case.
   //
   STDMETHOD_(int, ReleaseAll)(THIS) PURE;

   //
   // Hand control of self-reference inhibition.  Done automatically in
   // Connect()/Disconnect() phases
   //
   STDMETHOD_(void, BeginSelfReference)(THIS) PURE;
   STDMETHOD_(void, EndSelfReference)(THIS) PURE;

   };

///////////////////////////////////////

#define IAggregate_Add(p, a, b, c, d, e)    COMCall2(p, Add, a, b)
#define IAggregate_Init(p)                  COMCall0(p, Init)
#define IAggregate_End(p)                   COMCall0(p, End)
#define IAggregate_ReleaseAll(p)            COMCall0(p, ReleaseAll)
#define IAggregate_BeginSelfReference(p)    COMCall0(p, BeginSelfReference)
#define IAggregate_EndSelfReference(p)      COMCall0(p, EndSelfReference)

///////////////////////////////////////////////////////////////////////////////
//
// IInterfaceSet
//
// IInterfaceSet is a generic collection of IUnknown *'s using a GUID key
//
// Because this is generic storage interface for use by other interface
// implementors, AddRef() & Release() on set members are never called, except
// in the case of ReleaseAll()
//
//

enum eISetOrder
   {
   kISetFIFO,
   kISetLIFO,
   kISetAnyOrder
   };

typedef eISetOrder eISetOrder;

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IInterfaceSet

DECLARE_INTERFACE_(IInterfaceSet, IUnknown)
   {
   //
   // IUnkown members
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Insertion and removal.
   //
   STDMETHOD (Add)(THIS_ REFIID, IUnknown * pUnknown) PURE;
   STDMETHOD (Remove)(THIS_ REFIID, IUnknown * pOptionalUnknown) PURE;

   //
   // Lookup
   //
   STDMETHOD (Search)(THIS_ REFIID, IUnknown **) PURE;

   //
   // Iteration
   //
   // Iteration protocol: Given an optional GUID, set passes back
   // the first item, plus a cookie.  This cookie may be used to
   // iterate until FindNext returns NULL.  An Add() or Remove()
   // will NOT invalidate the cookie
   //
   // The client can only declare pointers to the cookie, not the
   // cookie itself
   //
   STDMETHOD_(IUnknown *, FindFirst)(THIS_ eISetOrder, const GUID *, tISetHandle **) PURE;
   STDMETHOD_(IUnknown *, FindNext)(THIS_ tISetHandle *) PURE;
   STDMETHOD_(void, FindEnd)(THIS_ tISetHandle *) PURE;

   //
   // Call Release() on all interfaces in the set, then empty the set
   //
   STDMETHOD (ReleaseAll)(THIS_ eISetOrder) PURE;

   //
   // Empty the set without calling release
   //
   STDMETHOD (Empty)(THIS) PURE;

   };

///////////////////////////////////////////////////////////////////////////////
//
// Accessory macros to assist in aggregate querying
//
// _AutoAggIPtr()
// _AutoAggIPtr_()
// _ConAggIPtr()
// _ConAggIPtr_()
//


//
// Macro for connection to other parts of same aggregate, using m_pInterface
// naming convention
//
#ifndef NO_SELF_CONNECT
#define SelfConnectOptional(root)     QueryInterface(IID_I ## root, (void **) (&m_p ## root))
#define SelfConnectRequired(root)     { QueryInterface(IID_I ## root, (void **) (&m_p ## root)); AssertMsg((m_p ## root), "Expected instance of I" #root " in aggregate"); }
#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__OBJCOLL_H */

