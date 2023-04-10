/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptapi.h,v 1.14 1999/06/29 19:11:09 mahk Exp $
//
// ObjScript API
//

#ifndef __SCRPTAPI_H
#define __SCRPTAPI_H

#include <lg.h>
#include <comtools.h>
#include <multparm.h>

#include <scrpttyp.h>
#include <scrguid.h>
#include <scrptmsg.h>
#include <hashset.h>


///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IScriptMan);
F_DECLARE_INTERFACE(IScript);
F_DECLARE_INTERFACE(IScriptModule);
typedef struct Datapath Datapath;
typedef struct sScrMsg sScrMsg;
typedef int ObjID;

///////////////////////////////////////////////////////////////////////////////
//
// Create a recorder and add it to the global app-object
//

typedef ulong (*tScrTimeFunc)(void);

#define ScriptManCreate(pfnTime, pfnPrint) \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _ScriptManCreate(IID_TO_REFIID(IID_IScriptMan), NULL, pAppUnknown, \
                     pfnTime, pfnPrint); \
    COMRelease(pAppUnknown); \
}

//
// Creates a script manager, aggregating it with specfied pOuter,
// use IAggregate protocol if ppScriptMan is NULL, else self-init
//
EXTERN tResult LGAPI
_ScriptManCreate(REFIID, IScriptMan ** ppScriptMan, IUnknown * pOuter, 
                 tScrTimeFunc, tScriptPrintFunc);


///////////////////////////////////////////////////////////////////////////////
//
// Constants, handles and structures
//

// Special name for root of all scripts
#define kScrRootScript "Object"

// Special message recipient to dispatch a global message
#define kScrObjIDAll 0x7fffffff

// Maximum length of script class names, module names
#define kScrMaxClassName  31
#define kScrMaxModuleName 255

// Iteration handle
DECLARE_HANDLE(tScrIter);

// External representation of a script factory
typedef IScript * (*tScriptFactory)(const char * pszClass, ObjID);

// Script module entry point
typedef BOOL (LGAPI * tScriptModuleInitFunc)(const char * pszName, 
                                             IScriptMan * pScriptMan,
                                             tScriptPrintFunc pPrintFunc,
                                             IUnknown * pMalloc, 
                                             IScriptModule ** ppModule);

///////////////////////////////////////
//
// Script class descriptor
//

struct sScrClassDesc
{
   const char *   pszModule;
   const char *   pszClass;
   const char *   pszBaseClass;
   tScriptFactory pfnFactory;
};

typedef struct sScrClassDesc sScrClassDesc;

///////////////////////////////////////
//
// Timed script messages
//

enum eScrTimedMsgKind
{
   kSTM_OneShot,
   kSTM_Periodic,

   kSTM_Max = 0xffffffff                         // Make sure it's an int
};

typedef enum eScrTimedMsgKind eScrTimedMsgKind;

// Timer handle
DECLARE_HANDLE(tScrTimer);

// Timer callback
typedef void (*tScrMsgCallback)(const sScrMsg *, const sMultiParm *, void *);


///////////////////////////////////////
//
// tracing message sending
// @TODO: Once we have all the debugging tools we want, we should
// take them out of SHIP builds.
//

enum eScrTraceAction
{
   kNoAction,
   kBreak,
   kSpew,

   kPad = 65536
};


class cScrTraceHashKey
{
public:
   char m_Combo[kScrMsgNameMax + 9];

   cScrTraceHashKey() {}
   cScrTraceHashKey(const char *pszName, ObjID HostObjID)
   {
      sprintf(m_Combo, "%s:%d", pszName, HostObjID);
   }
};


class cScrTrace
{
public:
   ObjID m_HostObjID;
   eScrTraceAction m_TraceAction;
   int m_iTraceLine;
   cScrTraceHashKey m_TraceHashKey;

   cScrTrace() {}
   cScrTrace(const char *pszMessage, ObjID HostObjID, 
             eScrTraceAction TraceAction, int iTraceLine)
      : m_HostObjID(HostObjID),
        m_TraceAction(TraceAction),
        m_TraceHashKey(pszMessage, HostObjID),
        m_iTraceLine(iTraceLine)
   {
      AssertMsg1(iTraceLine >= 0 && iTraceLine <= 31,
                 "cScrTrace: iTraceLine out of range: %d\n", iTraceLine);
   }

   // This actually copies the string.
   // @TODO: Should this really be inlined?
   void MessageNameToString(char *pszDest) const
   {
      for (int i = 0; m_TraceHashKey.m_Combo[i] != ':'; ++i)
         pszDest[i] = m_TraceHashKey.m_Combo[i];
      pszDest[i] = 0;
   }
};


class cScrTracePointHash : public cStrIHashSet<cScrTrace *>
{
public:
   tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey) ((cScrTrace *)p)->m_TraceHashKey.m_Combo;
   }
};


///////////////////////////////////////
//
// Script state data
//

struct sScrDatumTag
{
   ObjID        objId;
   const char * pszClass;
   const char * pszName;
};

typedef struct sScrDatumTag sScrDatumTag;

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IScriptMan
//

#undef INTERFACE
#define INTERFACE IScriptMan

DECLARE_INTERFACE_(IScriptMan, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Explicit game control of initialization
   //
   STDMETHOD (GameInit)(THIS) PURE;
   STDMETHOD (GameEnd)(THIS) PURE;

   //
   // Explicitly begin/end scripts
   // 
   STDMETHOD(BeginScripts)(THIS) PURE; 
   STDMETHOD(EndScripts)(THIS) PURE; 
   
   //
   // Set the script module path
   //
   STDMETHOD (SetModuleDatapath)(THIS_ const Datapath *) PURE;

   //
   // Add and remove modules
   //
   STDMETHOD (AddModule)(THIS_ const char *) PURE;
   STDMETHOD (RemoveModule)(THIS_ const char *) PURE;
   STDMETHOD (ClearModules)(THIS) PURE;

   //
   // Script services
   //
   STDMETHOD (ExposeService)(THIS_ IUnknown *, const GUID *) PURE;
   STDMETHOD_(IUnknown *, GetService)(THIS_ const GUID *) PURE;

   //
   // Iterate over known script classes
   //
   STDMETHOD_(const sScrClassDesc *, GetFirstClass)(THIS_ tScrIter *) PURE;
   STDMETHOD_(const sScrClassDesc *, GetNextClass)(THIS_ tScrIter *) PURE;
   STDMETHOD_(void, EndClassIter)(THIS_ tScrIter *) PURE;

   STDMETHOD_(const sScrClassDesc *, GetClass)(THIS_ const char *) PURE;

   //
   // Set the scripts associated with an object
   //
   STDMETHOD (SetObjScripts)(THIS_ ObjID, const char **, unsigned n) PURE;

   //
   // Remove all scripts, messages and data related to an object
   //
   STDMETHOD (ForgetObj)(THIS_ ObjID) PURE;
   STDMETHOD (ForgetAllObjs)(THIS) PURE;

   //
   // Messaging
   //
   STDMETHOD_(BOOL, WantsMessage)(THIS_ ObjID, const char * pszMessage) PURE;

   STDMETHOD (SendMessage)(THIS_ sScrMsg *,
                           sMultiParm * DEFAULT_TO(NULL)) PURE;

   STDMETHOD_(void, KillTimedMessage)(THIS_ tScrTimer) PURE;

   STDMETHOD_(int, PumpMessages)(THIS) PURE;

   STDMETHOD_(void, PostMessage)(THIS_ sScrMsg *) PURE;

   STDMETHOD_(tScrTimer, SetTimedMessage)(THIS_ sScrMsg *,
                                          ulong time,
                                          eScrTimedMsgKind kind) PURE;

   // These next three are used by scripts which want to send
   // messages.  This lets us use the constructors on the app side,
   // which have our persistence fields.
   STDMETHOD_ (cMultiParm, SendMessage2)(ObjID, 
                                         ObjID, 
                                         const char *,
                                         const cMultiParm &, 
                                         const cMultiParm &, 
                                         const cMultiParm &) PURE;

   STDMETHOD_(void, PostMessage2)(ObjID, 
                                  ObjID, 
                                  const char *,
                                  const cMultiParm &, 
                                  const cMultiParm &, 
                                  const cMultiParm &,
                                  ulong flags DEFAULT_TO(0)) PURE;

   STDMETHOD_(tScrTimer, SetTimedMessage2)(ObjID to,
                                           const char *,
                                           ulong,
                                           eScrTimedMsgKind,
                                           const cMultiParm &) PURE;

   //
   // Script data
   //
   STDMETHOD_(BOOL, IsScriptDataSet)(THIS_ const sScrDatumTag * pTag) PURE;
   STDMETHOD (GetScriptData)(THIS_ const sScrDatumTag * pTag, sMultiParm *) PURE;
   STDMETHOD (SetScriptData)(THIS_ const sScrDatumTag * pTag, const sMultiParm *) PURE;
   STDMETHOD (ClearScriptData)(THIS_ const sScrDatumTag * pTag, sMultiParm *) PURE;

   //
   // Debugging
   //
   STDMETHOD (AddTrace)(ObjID Object, char *pszMessage, 
                        eScrTraceAction ScrTraceAction, int iTraceLine) PURE;
   STDMETHOD (RemoveTrace)(ObjID Object, char *pszMessage) PURE;

   // bitmask to control which traces are active
   STDMETHOD_ (BOOL, GetTraceLine)(int iTraceLine) PURE;
   STDMETHOD_ (void, SetTraceLine)(int iTraceLine, BOOL bStatus) PURE;
   STDMETHOD_ (int, GetTraceLineMask)() PURE;
   STDMETHOD_ (void, SetTraceLineMask)(int iTraceLineMask) PURE;

   // iteration over traces
   STDMETHOD_(const cScrTrace *, GetFirstTrace)(tScrIter *) PURE;
   STDMETHOD_(const cScrTrace *, GetNextTrace)(tScrIter *) PURE;
   STDMETHOD_(void, EndTraceIter)(tScrIter *) PURE;

   //
   // Save/Load
   //
   STDMETHOD (SaveLoad)(THIS_ tPersistIOFunc pfnIO, 
                        void * pContextIO, BOOL fLoading) PURE;

   STDMETHOD_(void, PostLoad)() PURE;
};

#define IScriptMan_PumpMessages(p) COMCall0(p, PumpMessages)

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IScript
//

#undef INTERFACE
#define INTERFACE IScript

DECLARE_INTERFACE_(IScript, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   // Query the type
   //
   STDMETHOD_(const char *, GetClassName)(THIS) PURE;

   //
   // Messaging
   //
   STDMETHOD (ReceiveMessage)(THIS_ sScrMsg *pMsg, sMultiParm *pReply,
                              eScrTraceAction DebugAction = kNoAction) PURE;

};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IScriptModule
//

#undef INTERFACE
#define INTERFACE IScript

DECLARE_INTERFACE_(IScriptModule, IUnknown)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   //
   //
   //
   STDMETHOD_(const char *, GetName)(THIS) PURE;

   //
   //
   //
   STDMETHOD_(const sScrClassDesc *, GetFirstClass)(THIS_ tScrIter *) PURE;
   STDMETHOD_(const sScrClassDesc *, GetNextClass)(THIS_ tScrIter *) PURE;
   STDMETHOD_(void, EndClassIter)(THIS_ tScrIter *) PURE;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__SCRPTAPI_H */
