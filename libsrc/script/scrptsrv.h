/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptsrv.h,v 1.5 1997/12/31 12:33:53 TOML Exp $
//
//
//

#ifndef __SCRPTSRV_H
#define __SCRPTSRV_H

#include <dynarray.h>

///////////////////////////////////////////////////////////////////////////////
//
// Macros to build a script service
//
// Steps to defining your own script service:
//
// A script interface is similar to a COM interface, with the following
// differences
//
//    1) it cannot only be called from a script
//    2) arguments and return values must be of one of the base types in scrptbas.h,
//       or a cMultiParm
//    3) Script functions by convention are always mixed-case style
//    4) In declarations of service methods, omit "THIS"
//    5) Implementations must derive from interface & implement using macros
//
// Note: because passive linking is used here, services in libraries may
// not get linked in if the service is the only thing in the module.

#undef INTERFACE

DECLARE_INTERFACE_(IAutoScriptService, IUnknown)
{
   STDMETHOD_(void, Init)() PURE;
   STDMETHOD_(void, End)() PURE;
};

#define DECLARE_SCRIPT_SERVICE_(iface_root) \
   DECLARE_INTERFACE_( I##iface_root##ScriptService, IAutoScriptService)

#ifdef SCRIPT_MAIN_MODULE

   #define DECLARE_SCRIPT_SERVICE(iface_root, lgiid) \
      F_DECLARE_INTERFACE( I##iface_root##ScriptService); \
      DEFINE_LG_GUID_UNCONDITIONAL(IID_I##iface_root##ScriptService, lgiid); \
      extern IScriptMan * g_pScriptMan; \
      I##iface_root##ScriptService & iface_root = *((I##iface_root##ScriptService *)g_pScriptMan->GetService(&IID_I##iface_root##ScriptService)); \
      class c##iface_root##ServiceReleaser \
      { \
      public: \
         c##iface_root##ServiceReleaser() \
         { \
            ((IUnknown *)(&iface_root))->Release(); \
         } \
      } _g_##iface_root##ServiceReleaser; \
      DECLARE_SCRIPT_SERVICE_(iface_root)

#else

   #define DECLARE_SCRIPT_SERVICE(iface_root, lgiid) \
      F_DECLARE_INTERFACE( I##iface_root##ScriptService); \
      const int kIID_I##iface_root##ScriptService = lgiid; \
      DECLARE_SCRIPT_SERVICE_(iface_root)

#endif

template <class INTERFACE, const GUID * pIID_INTERFACE>
class cScriptServiceImplBase : public cCTUnaggregated<INTERFACE, pIID_INTERFACE, kCTU_NoSelfDelete>
{
   STDMETHOD_(void, Init)()
   {
   }
   
   STDMETHOD_(void, End)()
   {
   }
};

#define DECLARE_SCRIPT_SERVICE_IMPL(impl, iface_root) \
   class impl : public cScriptServiceImplBase<I##iface_root##ScriptService, &IID_I##iface_root##ScriptService>

#define IMPLEMENT_SCRIPT_SERVICE_IMPL(impl, iface_root) \
   DEFINE_LG_GUID_UNCONDITIONAL(IID_I##iface_root##ScriptService, kIID_I##iface_root##ScriptService); \
   impl g_##impl##ScriptService; \
   sScrSrvRegData g_##impl##ScrSrvRegData(&g_##impl##ScriptService, &IID_I##iface_root##ScriptService)

struct sScrSrvRegData;

extern cDynArray<sScrSrvRegData *> g_ScrSrvRegData;

struct sScrSrvRegData
{
   sScrSrvRegData(IAutoScriptService * pService, const GUID * pGuid)
    : pService(pService), pGuid(pGuid)
   {
      g_ScrSrvRegData.Append(this);
   }

   IAutoScriptService * pService;
   const GUID *         pGuid;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__SCRPTSRV_H */
