/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptman.h,v 1.21 1999/06/29 19:11:24 mahk Exp $
//
//
//

#ifndef __SCRPTMAN_H
#define __SCRPTMAN_H

#include <lg.h>
#include <allocapi.h>
#include <datapath.h>
#include <str.h>
#include <dynarray.h>
#include <hashset.h>
#include <aggmemb.h>

#include <scrptapi.h>
#include <scrptnet.h>

#include <scrobj.h>
#include <smodinfo.h>

#undef LoadModule
#undef FreeModule

class cFileSpec;
class cScriptMan;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrServiceTable
//

struct sServiceTableEntry
{
   IUnknown *   pService;
   const GUID * pGuid;
};

class cScrServiceTable : public cGuidHashSet<sServiceTableEntry *>
{
public:
   ~cScrServiceTable();
   virtual tHashSetKey GetKey(tHashSetNode p) const;
};

///////////////////////////////////////////////////////////////////////////////
//
// Stored messages
//

///////////////////////////////////////
//
// STRUCT: sScrPostedMsg
//

struct sScrPostedMsg
{
   sScrMsg *       pMsg;
};

///////////////////////////////////////
//
// STRUCT: sScrTimedMsg
//

struct sScrTimedMsg : public cCTUnaggregated<IUnknown, &IID_IUnknown, kCTU_Default>
{
   sScrMsg *       pMsg;
   ulong           nextTime;
   ulong           period;
   ulong           id; 
   
protected:
   ~sScrTimedMsg() 
   {
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// Deferred actions
//

enum eScrDeferredActions
{
   kSDA_SetObjScripts,
   kSDA_ForgetObj
};

struct sScrDeferredAction
{
   sScrDeferredAction(int a, ObjID o, const char ** pp, unsigned n)
    : action(a),
      objId(o),
      nClasses(n)
   {
      if (nClasses)
      {
         ppszClasses = new const char *[nClasses];
         for (int i = 0; i < nClasses; i++)
            ppszClasses[i] = strdup(pp[i]);
      }
      else
         ppszClasses = NULL;
   }

   ~sScrDeferredAction()
   {
      if (nClasses)
      {
         for (int i = 0; i < nClasses; i++)
            free((void *)ppszClasses[i]);

         delete [] ppszClasses;
      }
   }

   int           action;
   ObjID         objId;
   const char ** ppszClasses;
   unsigned      nClasses;
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScriptMan
//

extern cScriptMan * g_pScriptMan;

///////////////////////////////////////

class cScriptMan : public cCTDelegating<IScriptMan>,
                   public cCTAggregateMemberControl<kCTU_Default>
{
public:
   cScriptMan(IUnknown * pOuter, tScrTimeFunc pfnTime, 
              tScriptPrintFunc pfnPrint);
   virtual ~cScriptMan();

   // Explicit game control of initialization
   STDMETHOD (GameInit)();
   STDMETHOD (GameEnd)();
   
   // Set the script module path
   STDMETHOD (SetModuleDatapath)(const Datapath *);

   // Add and remove modules
   STDMETHOD (AddModule)(const char *);
   STDMETHOD (RemoveModule)(const char *);
   STDMETHOD (ClearModules)();

   // Script services
   STDMETHOD (ExposeService)(IUnknown *, const GUID *);
   STDMETHOD_(IUnknown *, GetService)(const GUID *);

   // Iterate over known script classes
   STDMETHOD_(const sScrClassDesc *, GetFirstClass)(tScrIter *);
   STDMETHOD_(const sScrClassDesc *, GetNextClass)(tScrIter *);
   STDMETHOD_(void, EndClassIter)(tScrIter *);

   STDMETHOD_(const sScrClassDesc *, GetClass)(const char *);

   // Instance control
   STDMETHOD (SetObjScripts)(ObjID, const char **, unsigned n);
   STDMETHOD (ForgetObj)(ObjID);
   STDMETHOD (ForgetAllObjs)();
   HRESULT DoForgetObj(cScrObj * pScrObj);

   // Messaging
   STDMETHOD_(BOOL, WantsMessage)(ObjID, const char * pszMessage);

   HRESULT DoSendDirect(IScript * pScript, sScrMsg * pMsg, 
                        sMultiParm * pReply);
   HRESULT DoSendMessage(cScrObj * pScrObj, sScrMsg * pMsg,
                         sMultiParm * pReply);

   STDMETHOD (SendMessage)(sScrMsg *, sMultiParm *);

   STDMETHOD_(void, PostMessage)(sScrMsg *);

   STDMETHOD_(tScrTimer, SetTimedMessage)(sScrMsg *pMessage,
                                          ulong time,
                                          eScrTimedMsgKind kind);

   STDMETHOD_ (cMultiParm, SendMessage2)(ObjID from,
                                         ObjID to,
                                         const char * pszMessage,
                                         const cMultiParm & data, 
                                         const cMultiParm & data2, 
                                         const cMultiParm & data3);

   STDMETHOD_(void, PostMessage2)(ObjID from, 
                                  ObjID to, 
                                  const char * pszMessage,
                                  const cMultiParm & data, 
                                  const cMultiParm & data2, 
                                  const cMultiParm & data3,
                                  ulong flags);

   STDMETHOD_(tScrTimer, SetTimedMessage2)(ObjID to,
                                           const char *pszName,
                                           ulong time,
                                           eScrTimedMsgKind kind,
                                           const cMultiParm & data);

   STDMETHOD_(void, KillTimedMessage)(tScrTimer);

   STDMETHOD_(int, PumpMessages)();

   // Script data
   STDMETHOD_(BOOL, IsScriptDataSet)(const sScrDatumTag * pTag);
   STDMETHOD (GetScriptData)(const sScrDatumTag * pTag, sMultiParm *);
   STDMETHOD (SetScriptData)(const sScrDatumTag * pTag, const sMultiParm *);
   STDMETHOD (ClearScriptData)(const sScrDatumTag * pTag, sMultiParm *);
   HRESULT ClearScriptData(ObjID objId, const char * pszClass = NULL);

   // Debugging
   STDMETHOD (AddTrace)(ObjID Object, char *pszMessage, 
                        eScrTraceAction ScrTraceAction, int iTraceLine);
   STDMETHOD (RemoveTrace)(ObjID Object, char *pszMessage);
   STDMETHOD_ (BOOL, GetTraceLine)(int iTraceLine);
   STDMETHOD_ (void, SetTraceLine)(int iTraceLine, BOOL bStatus);
   STDMETHOD_ (int, GetTraceLineMask)();
   STDMETHOD_ (void, SetTraceLineMask)(int iTraceLineMask);

   STDMETHOD_(const cScrTrace *, GetFirstTrace)(tScrIter *);
   STDMETHOD_(const cScrTrace *, GetNextTrace)(tScrIter *);
   STDMETHOD_(void, EndTraceIter)(tScrIter *);

   // Save/Load
   STDMETHOD (SaveLoad)(tPersistIOFunc pfnIO, void * pContextIO,
                        BOOL fLoading);

   // clear internal state which distinguishes objects attached on
   // load from those created in-game
   STDMETHOD_(void, PostLoad)();

   STDMETHOD(BeginScripts)(); 
   STDMETHOD(EndScripts)(); 

   // Get the game time
   ulong GetTime()
   {
      return (*m_pfnTime)();
   }

private:

   BOOL LoadModule(const cFileSpec &, sScrModuleInfo *);
   void FreeModule(sScrModuleInfo *);
   void LoadClasses();

   void DeferAction(int action, ObjID objId, const char ** = NULL, 
                    unsigned = 0);
   void ExecuteDeferredActions();

   eScrTraceAction FindDebugFlags(sScrMsg *pMsg);

   SaveLoad();

   ////////////////////////////////////

   // who are we sending to
   cDynArray<int>            m_sendingTo; 

   // The active script aggregates
   cScrObjsTable             m_ActiveObjs;

   // Are we sending BeginScript messages at all? 
   BOOL                      m_fSendBeginMessages; 

   // When we load, we only send OnBeginScript to these script objects.
   cScrObjFilter             m_NeedBeginMessage;

   // This tells us whether to use m_NeedBeginMessage.
   BOOL                      m_fFilterBeginMessages;
   
   // Check for currently pumping
   BOOL                      m_fInPump;
   
   // The available services
   cScrServiceTable          m_Services;
   
   // The hash table of script state data
   cScriptsData              m_ScriptsData;
   
   // Where to find modules
   Datapath                  m_Datapath;
   
   // The currently loaded modules
   cScrModuleInfoTable       m_Modules;
   
   // The currently available classes
   cScrClassDescTable        m_ClassDescs;

   // Messaging
   cDynArray<sScrPostedMsg>  m_PostedMsgs;
   cDynArray<sScrTimedMsg *> m_TimedMsgs;
   tScrTimeFunc              m_pfnTime;
   ulong                     m_NextTimerID; 

   // Deferred actions
   cDynArray<sScrDeferredAction *> m_DeferredActions;

   // Text output for debugging
   tScriptPrintFunc          m_pfnPrint;

   // Debugging
   cScrTracePointHash        m_TracePointHash;
   int                       m_iTraceLineMask;

   // The interface for dealing with network capabilities
   IScriptNet *              m_pScriptNet;
};

///////////////////////////////////////

inline HRESULT cScriptMan::DoSendDirect(IScript * pScript, sScrMsg * pMsg, 
                                        sMultiParm * pReply)
{
   
   HRESULT result;
      
   m_sendingTo.Append(pMsg->to); 


   // LGALLOC_PUSH_CREDIT();
   
   // Here's where we intercept messages for breaks and mono spew.
   result = pScript->ReceiveMessage(pMsg, pReply, FindDebugFlags(pMsg));

   // LGALLOC_POP_CREDIT();

   m_sendingTo.Shrink(); 
   return result;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__SCRPTMAN_H */

