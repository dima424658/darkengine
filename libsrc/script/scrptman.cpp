/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptman.cpp,v 1.42 2000/02/22 19:49:46 toml Exp $
//
// @TBD (toml 12-01-97): time will tell if we need to track inheritance
// relationships and conflicts more closely. let's see what the designers
// actually do.


#include <windows.h>    // for DebugBreak
#include <filespec.h>
#include <scrptman.h>
#include <scrptsrv.h>
#include <nullsrv.h>
#include <appagg.h>

#include <initguid.h>
#include <scrguid.h>
#include <scrptapi.h>
#include <scrptnet.h>

#include <hshsttem.h>
#include <mprintf.h>

#include <dbmem.h>

#if defined(__WATCOMC__)
#pragma initialize library
#else
#pragma init_seg (lib)
#endif

///////////////////////////////////////////////////////////////////////////////

cScrStr NULL_STRING;

///////////////////////////////////////////////////////////////////////////////

cScriptMan * g_pScriptMan;

///////////////////////////////////////

EXTERN tResult LGAPI
_ScriptManCreate(REFIID, IScriptMan ** /*ppScriptMan*/, IUnknown * pOuter, 
                 tScrTimeFunc pfnTime, tScriptPrintFunc pfnPrint)
{
   if (pOuter
    && (g_pScriptMan = new cScriptMan(pOuter, pfnTime, pfnPrint)) != NULL)
      return S_OK;
   return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
template cHashSet <cScrObj *, int, cHashFunctions>;
template cHashSet <sScrClassDesc *, const char *, cCaselessStringHashFuncs>;
template cHashSet <sServiceTableEntry *, const GUID *, cHashFunctions>;
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrServiceTable
//

cDynArray<sScrSrvRegData *> g_ScrSrvRegData;

cScrServiceTable::~cScrServiceTable()
{
}

///////////////////////////////////////

tHashSetKey cScrServiceTable::GetKey(tHashSetNode p) const
{
   return (tHashSetKey)(((sServiceTableEntry *)p)->pGuid);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScriptMan
//

cScriptMan::cScriptMan(IUnknown * pOuter, tScrTimeFunc pfnTime, 
                       tScriptPrintFunc pfnPrint)
   : m_pfnTime(pfnTime),
     m_pfnPrint(pfnPrint),
     m_iTraceLineMask(1),
     m_fSendBeginMessages(FALSE),
     m_fInPump(FALSE),
     m_NextTimerID(1),
     m_pScriptNet(NULL)
{
   MI_INIT_AGGREGATION_1(pOuter, IScriptMan, kPriorityLibrary, NULL);
   DatapathClear(&m_Datapath);
}

///////////////////////////////////////

cScriptMan::~cScriptMan()
{
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::GameInit()
{
   for (int i = 0; i < g_ScrSrvRegData.Size(); i++)
   {
      ExposeService(g_ScrSrvRegData[i]->pService, g_ScrSrvRegData[i]->pGuid);
      g_ScrSrvRegData[i]->pService->Init();
      g_ScrSrvRegData[i]->pService->Release();
   }

   // If the application has defined a networking interface, go get it:
   m_pScriptNet = AppGetObj(IScriptNet);
   
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::GameEnd()
{
   for (int i = 0; i < g_ScrSrvRegData.Size(); i++)
   {
      g_ScrSrvRegData[i]->pService->End();
      g_ScrSrvRegData[i]->pService->Release();
   }

   SafeRelease(m_pScriptNet);
   
   m_Services.DestroyAll();
   ClearModules();
   m_ActiveObjs.DestroyAll();
   DatapathFree(&m_Datapath);

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::SetModuleDatapath(const Datapath * pDatapath)
{
   DatapathCopy(&m_Datapath, pDatapath);
   return 0;
}


///////////////////////////////////////

void cScriptMan::LoadClasses()
{
   tScrIter              iter;
   const sScrClassDesc * pDesc;

   for (int i = 0; i < m_Modules.Size(); i++)
   {
      IScriptModule * pModule = m_Modules[i].pModule;

      pDesc = pModule->GetFirstClass(&iter);
      while (pDesc)
      {
         m_ClassDescs.RemoveByKey(pDesc->pszClass);
         m_ClassDescs.Insert(pDesc);
         pDesc = pModule->GetNextClass(&iter);
      }
   }
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::AddModule(const char * pszModule)
{
   AssertMsg(!m_sendingTo.Size(), "Cannot add script modules from within a script");

   BEGIN_DEBUG_MSG1("cScriptMan::AddModule(%s)", pszModule);
   
   if (!pszModule || !*pszModule)
      return E_FAIL;

   // Brute force:  Remove all possible references to superceded classes and modules
   HRESULT result = S_OK;

   m_ActiveObjs.DisconnectAll();
   m_ClassDescs.SetEmpty();

   cFileSpec      fsModule(pszModule);
   sScrModuleInfo moduleInfo;
   
   fsModule.SetFileExtension("osm");

   if (m_Modules.Remove(fsModule.GetFileName(), &moduleInfo))
      FreeModule(&moduleInfo);

   // Load the new module
   char buf[kScrMaxModuleName+1];

   if (DatapathFind(&m_Datapath, fsModule.GetName(), buf, 
                    kScrMaxModuleName + 1))
      fsModule.SetRelativePath(buf);

   fsModule.MakeFullPath();
   DebugMsg1("Attempting to load %s...", fsModule.GetName());

   LoadModule(fsModule, &moduleInfo);

   if (moduleInfo.pModule)
      m_Modules.Insert(moduleInfo);
   else
      result = E_FAIL;

   // Reload our classes
   LoadClasses();

   // Reconnect scripts
   m_ActiveObjs.ConnectAll();
   if (m_fSendBeginMessages)
   {
      if (m_fFilterBeginMessages)
         m_ActiveObjs.DispatchBeginScriptsFiltered(&m_NeedBeginMessage);
      else
         m_ActiveObjs.DispatchBeginScriptsAll();
   }
   ExecuteDeferredActions();

   return result;

   END_DEBUG;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::RemoveModule(const char * pszModule)
{
   AssertMsg(!m_sendingTo.Size(), "Cannot remove script modules from within a script");

   cFileSpec fsModule(pszModule);
   fsModule.SetFileExtension("osm");

   sScrModuleInfo moduleInfo;
   if (m_Modules.Remove(fsModule.GetFileName(), &moduleInfo))
   {
      m_ActiveObjs.DisconnectAll();
      m_ClassDescs.SetEmpty();

      FreeModule(&moduleInfo);

      LoadClasses();
      m_ActiveObjs.ConnectAll();
      
      return S_OK;
   }
   return S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::ClearModules()
{
   AssertMsg(!m_sendingTo.Size(), "Cannot remove script modules from within a script");

   m_ActiveObjs.DisconnectAll();

   m_ClassDescs.SetEmpty();

   for (int i = 0; i < m_Modules.Size(); i++)
      FreeModule(&m_Modules[i]);

   m_Modules.SetSize(0);

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::ExposeService(IUnknown * pService, const GUID * pGuid)
{
   AssertMsg(!m_Services.Search(pGuid), "Duplicate service provided to script manager");

   sServiceTableEntry * pEntry = new sServiceTableEntry;

   pEntry->pService = pService;
   pEntry->pGuid = pGuid;
   m_Services.Insert(pEntry);

   pService->AddRef();

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(IUnknown *) cScriptMan::GetService(const GUID * pGuid)
{
   sServiceTableEntry * pEntry = m_Services.Search(pGuid);

   if (pEntry)
   {
      pEntry->pService->AddRef();
      return pEntry->pService;
   }

   if (*pGuid != IID_INullScriptService)
      return GetService(&IID_INullScriptService);

   return NULL;
}

///////////////////////////////////////

STDMETHODIMP_(const sScrClassDesc *) cScriptMan::GetFirstClass(tScrIter * pIter)
{
   tHashSetHandle * pHandle = new tHashSetHandle;
   *pIter = (tScrIter)pHandle;
   return m_ClassDescs.GetFirst(*pHandle);
}

///////////////////////////////////////

STDMETHODIMP_(const sScrClassDesc *) cScriptMan::GetNextClass(tScrIter * pIter)
{
   return m_ClassDescs.GetNext(*((tHashSetHandle *)(*pIter)));
}

///////////////////////////////////////

STDMETHODIMP_(void) cScriptMan::EndClassIter(tScrIter * pIter)
{
   delete ((tHashSetHandle *)(*pIter));
   *pIter = NULL;
}

///////////////////////////////////////

STDMETHODIMP_(const sScrClassDesc *) cScriptMan::GetClass(const char * pszClass)
{
   return m_ClassDescs.Search(pszClass);
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::SetObjScripts(ObjID objId, const char ** ppszClasses, unsigned n)
{
   if (m_sendingTo.Size())
      for (int i = 0; i < m_sendingTo.Size(); i++)
         if (m_sendingTo[i] == objId)
         {
            DeferAction(kSDA_SetObjScripts, objId, ppszClasses, n);
            return S_OK;
         }  

   HRESULT               result = S_OK;
   cScrObj *             pScrObj;
   cScrObj *             pOldScrObj;

   // Grab any former script list
   if ((pOldScrObj = m_ActiveObjs.Search(objId)) != NULL)
   {
      pOldScrObj = m_ActiveObjs.Remove(pOldScrObj);
   }

   // Create and connect the new script list...
   pScrObj = m_ActiveObjs.Insert(new cScrObj(objId));

   while (n--)
      pScrObj->Prepend(new cScrScriptInfo(ppszClasses[n]));

   result = pScrObj->Connect();

   // Decide who's new...
   cScrScriptInfo * pInfo;
   cScrScriptInfo * pOldInfo;

   if (pOldScrObj)
   {
      pInfo = pScrObj->GetFirst();
      while (pInfo)
      {
         if ((pOldInfo = pOldScrObj->Search(pInfo->className)) != NULL)
            pInfo->flags = pOldInfo->flags;
         pInfo = pInfo->GetNext();
      }
   }

   if (m_fSendBeginMessages)
   {
      if (m_fFilterBeginMessages)
         m_ActiveObjs.DispatchBeginScriptsFiltered(&m_NeedBeginMessage);
      else
         m_ActiveObjs.DispatchBeginScriptsAll();
   }

   // ... and who's old...
   sScrMsg * pMsg = new sScrMsg(objId, "EndScript");
   pMsg->time = GetTime();

   if (pOldScrObj)
   {
      pOldInfo = pOldScrObj->GetFirst();
      while (pOldInfo)
      {
         if (pOldInfo->pScript && !pScrObj->Search(pOldInfo->className))
         {
            if (m_fSendBeginMessages)
               DoSendDirect(pOldInfo->pScript, pMsg, NULL);
            ClearScriptData(objId, pOldInfo->className);
         }
         pOldInfo = pOldInfo->GetNext();
      }

      pOldScrObj->Disconnect();
      delete pOldScrObj;
   }

   pMsg->Release();


   ExecuteDeferredActions();

   return (pScrObj) ? result : E_FAIL;
}

///////////////////////////////////////

HRESULT cScriptMan::DoForgetObj(cScrObj * pScrObj)
{

   // Remove object, if any
   if (pScrObj)
   {
      int       i;

      if (m_fSendBeginMessages)
      {
         sScrMsg * pMsg = new sScrMsg(pScrObj->GetObjID(), "EndScript");

         pMsg->time = GetTime();

         DoSendMessage(pScrObj, pMsg, NULL);

         pMsg->Release();
      }

      // Flush any messages pending for object
      for (i = m_TimedMsgs.Size() - 1; i >= 0; i--)
      {
         if (m_TimedMsgs[i]->pMsg->to == pScrObj->GetObjID())
         {
            m_TimedMsgs[i]->pMsg->Release();
            m_TimedMsgs[i]->Release();
            m_TimedMsgs.FastDeleteItem(i);
         }
      }

      for (i = 0; i < m_PostedMsgs.Size(); i++)
      {
         if (m_PostedMsgs[i].pMsg->to == pScrObj->GetObjID())
         {
            m_PostedMsgs[i].pMsg->Release();
            m_PostedMsgs.DeleteItem(i);
         }
      }

      // Remove references from script data table
      ClearScriptData(pScrObj->GetObjID());

      pScrObj->Disconnect();
      delete m_ActiveObjs.Remove(pScrObj);

      ExecuteDeferredActions();

      return S_OK;
   }

   return S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::ForgetObj(ObjID objId)
{
   if (m_sendingTo.Size())
      for (int i = 0; i < m_sendingTo.Size(); i++)
         if (m_sendingTo[i] == objId)
         {
            DeferAction(kSDA_ForgetObj, objId);
            return S_OK;
         }

   return DoForgetObj(m_ActiveObjs.Search(objId));
}

///////////////////////////////////////
//
// Begin/End Scripts
//
STDMETHODIMP cScriptMan::BeginScripts()
{
   if (m_fSendBeginMessages)
      return S_FALSE; 
   m_fSendBeginMessages = TRUE; 
   m_ActiveObjs.DispatchBeginScriptsAll(); 
   return S_OK; 
}

STDMETHODIMP cScriptMan::EndScripts()
{
   if (!m_fSendBeginMessages)
      return S_FALSE; 
   m_fSendBeginMessages = FALSE; 
   m_ActiveObjs.DispatchEndScriptsAll(); 
   return S_OK; 
}


////////////////////////////////////////


STDMETHODIMP cScriptMan::ForgetAllObjs()
{
   if (m_sendingTo.Size())
   {
      DeferAction(kSDA_ForgetObj, kScrObjIDAll);
      return S_OK;
   }

   tHashSetHandle iter;

   cScrObj * pScrObj = m_ActiveObjs.GetFirst(iter);

   while (pScrObj)
   {
      DoForgetObj(pScrObj);
      pScrObj = m_ActiveObjs.GetNext(iter);
   }

   // recycle timer ids 
   m_NextTimerID = 1; 

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cScriptMan::WantsMessage(ObjID objId, const char * /*pszMessage*/)
{
   return (m_ActiveObjs.Search(objId) != NULL);
}

///////////////////////////////////////

HRESULT cScriptMan::DoSendMessage(cScrObj * pScrObj, sScrMsg * pMsg, sMultiParm * pReply)
{
   cScrScriptInfo * pInfo  = pScrObj->GetFirst();
   HRESULT          result = S_OK;

   while (pInfo)
   {
      if (pInfo->pScript)
      {
         if (DoSendDirect(pInfo->pScript, pMsg, pReply) != S_OK)
            result = E_FAIL;
         if (pMsg->flags & kSMF_MsgBlock)
            break;
      }
      pInfo = pInfo->GetNext();
   }

   return result;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::SendMessage(sScrMsg * pMsg, sMultiParm * pReply)
{
   HRESULT result = S_OK;

   if (!m_ActiveObjs.IsEmpty())
   {
      cScrObj * pScrObj;

      pMsg->time = GetTime();
      
      // See if this message should be delivered:
      BOOL bSendMsg = TRUE;
      if (!(pMsg->flags & kSMF_MsgSendToProxy))
      {
         if (m_pScriptNet && m_pScriptNet->ObjIsProxy(pMsg->to)) 
         {
            // Okay, the object is a proxy and we're not sending this
            // message to proxies, so we're not dealing with it locally:
            bSendMsg = FALSE;
            if (pMsg->flags & kSMF_MsgPostToOwner)
            {
               // But we've been told to make sure it does go to the
               // owner, so send it along. Note that we aren't going to
               // get any reply, so the caller had better not be
               // expecting one.
               result = m_pScriptNet->PostToOwner(pMsg->from, 
                                                  pMsg->to, 
                                                  pMsg->message,
                                                  pMsg->data,
                                                  pMsg->data2,
                                                  pMsg->data3);
            }
         }
      }

      if (bSendMsg) {
         if ((pScrObj = m_ActiveObjs.Search(pMsg->to)) != NULL)
         {
            result = DoSendMessage(pScrObj, pMsg, pReply);
         }
         else if (pMsg->to == kScrObjIDAll)
         {
            tHashSetHandle iter;

            pScrObj = m_ActiveObjs.GetFirst(iter);

            while (pScrObj)
            {
               if (DoSendMessage(pScrObj, pMsg, pReply) != S_OK)
                  result = E_FAIL;
               pScrObj = m_ActiveObjs.GetNext(iter);
            }
         }
      }
      ExecuteDeferredActions();
   }

   pMsg->flags |= kSMF_MsgSent;
   return result;
}

///////////////////////////////////////

STDMETHODIMP_(void) cScriptMan::PostMessage(sScrMsg * pMsg)
{
   sScrPostedMsg postedMsg;

   AssertMsg1(PersistLookupReg(pMsg->GetName()),"Posting script message '%s' with no registered factory\n",pMsg->message); 

   pMsg->AddRef();
   
   postedMsg.pMsg        = pMsg;

   m_PostedMsgs.Append(postedMsg);

   ExecuteDeferredActions();
}

///////////////////////////////////////

STDMETHODIMP_(cMultiParm) cScriptMan::SendMessage2(ObjID from,
                                                   ObjID to, 
                                                   const char * pszMessage,
                                                   const cMultiParm & data, 
                                                   const cMultiParm & data2, 
                                                   const cMultiParm & data3)
{
   sScrMsg * pMsg = new sScrMsg(from, to, pszMessage, data, data2, data3);
   cMultiParm reply;

   SendMessage(pMsg, &reply);

   pMsg->Release();

   return reply;
}

///////////////////////////////////////

STDMETHODIMP_(void) cScriptMan::PostMessage2(ObjID from, 
                                             ObjID to, 
                                             const char * pszMessage,
                                             const cMultiParm & data, 
                                             const cMultiParm & data2, 
                                             const cMultiParm & data3,
                                             ulong flags)
{
   sScrMsg * pMsg = new sScrMsg(from, to, pszMessage, data, data2, data3);
   pMsg->flags |= flags; 
   PostMessage(pMsg);
   pMsg->Release();
}

///////////////////////////////////////

STDMETHODIMP_(tScrTimer) cScriptMan::SetTimedMessage(sScrMsg * pMsg,
                                                     ulong time,
                                                     eScrTimedMsgKind kind)
{
   sScrTimedMsg * pTimedMsg = new sScrTimedMsg;

   pMsg->AddRef();

   pTimedMsg->pMsg        = pMsg;

   pTimedMsg->nextTime    = GetTime() + time;
   pTimedMsg->period      = (kind == kSTM_Periodic) ? time : 0;
   // We could theoretically run out of these...
   pTimedMsg->id          = m_NextTimerID++; 

   m_TimedMsgs.Append(pTimedMsg);

   return (tScrTimer)pTimedMsg->id;
}

///////////////////////////////////////

STDMETHODIMP_(tScrTimer) cScriptMan::SetTimedMessage2(ObjID to, 
                                                      const char * pszMessage,
                                                      ulong time,
                                                      eScrTimedMsgKind kind,
                                                      const cMultiParm &data)
{
   sScrMsg * pMsg = new sScrTimerMsg(to, pszMessage, data);
   tScrTimer result = (tScrTimer) SetTimedMessage(pMsg, time, kind);
   pMsg->Release();
   return result;
}

///////////////////////////////////////

STDMETHODIMP_(void) cScriptMan::KillTimedMessage(tScrTimer timer)
{
   if (!timer)
      return;

   for (int i = 0; i < m_TimedMsgs.Size(); i++)
   {
      if (!m_TimedMsgs[i])
         continue; 
      if (m_TimedMsgs[i]->id == (ulong)timer)
      {
         m_TimedMsgs[i]->pMsg->Release();
         m_TimedMsgs[i]->Release();
         m_TimedMsgs.FastDeleteItem(i);
         return;
      }
   }
}

///////////////////////////////////////

STDMETHODIMP_(int) cScriptMan::PumpMessages()
{
   int             i;
   cMultiParm      reply;
   sScrTimedMsg *  pTimedMsg;
   ulong           time = GetTime();
   
   ExecuteDeferredActions();

   if (m_fInPump)
      return 0;

   m_fInPump = TRUE;
   
   // Dispatch timer messages
   cDynArray<sScrTimedMsg *> TimedMsgs(m_TimedMsgs);
   
   for (i = 0; i < TimedMsgs.Size(); i++)
   {
      TimedMsgs[i]->AddRef();
      TimedMsgs[i]->pMsg->AddRef();
   }
   
   for (i = TimedMsgs.Size() - 1; i >= 0; i--)
   {
      pTimedMsg = TimedMsgs[i];
      if (pTimedMsg->nextTime <= time)
      {
         // Update first, in case script results in killing the message
         if (pTimedMsg->period)
         {
            pTimedMsg->nextTime = time + pTimedMsg->period;
         }
         else
         {
            KillTimedMessage((tScrTimer)pTimedMsg->id);
         }
         
         SendMessage(pTimedMsg->pMsg, &reply);
      }
   }

   for (i = 0; i < TimedMsgs.Size(); i++)
   {
      TimedMsgs[i]->pMsg->Release();
      TimedMsgs[i]->Release();
   }
   
   TimedMsgs.SetSize(0);
   
   // Now dispatch queued messages
   const int             nPosted     = m_PostedMsgs.Size();
   sScrPostedMsg * const pPostedMsgs = m_PostedMsgs.Detach();
   
   for (i = 0; i < nPosted; i++)
   {
      SendMessage(pPostedMsgs[i].pMsg, &reply);
      pPostedMsgs[i].pMsg->Release();
   }
   
   free(pPostedMsgs);
   m_fInPump = FALSE;

   return 0;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cScriptMan::IsScriptDataSet(const sScrDatumTag * pTag)
{
   return (m_ScriptsData.Search(pTag) != NULL);
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::GetScriptData(const sScrDatumTag * pTag, sMultiParm * pParm)
{
   sScrDatum * pDatum = m_ScriptsData.Search(pTag);

   if (pDatum)
   {
      CopyParm(pParm, &pDatum->value);
      return S_OK;
   }

   return S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::SetScriptData(const sScrDatumTag * pTag, 
                                       const sMultiParm * pParm)
{
   sScrDatum * pDatum = m_ScriptsData.Search(pTag);

   if (!pDatum)
      pDatum = m_ScriptsData.Insert(new sScrDatum(pTag));

   pDatum->value = *pParm;
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::ClearScriptData(const sScrDatumTag * pTag,
                                         sMultiParm * pParm)
{
   sScrDatum * pDatum = m_ScriptsData.Search(pTag);

   if (pDatum)
   {
      CopyParm(pParm, &pDatum->value);
      delete m_ScriptsData.Remove(pDatum);
      return S_OK;
   }

   return S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::AddTrace(ObjID Object, char *pszMessage, 
                                  eScrTraceAction ScrTraceAction,
                                  int iTraceLine)
{
   cScrTraceHashKey TempKey(pszMessage, Object);

   if (m_TracePointHash.Search(TempKey.m_Combo)) {
      Warning(("cScriptMan::AddTrace: Already tracing %s on object %d\n",
               pszMessage, Object));
      return S_FALSE;
   }

   m_TracePointHash.Insert(new cScrTrace(pszMessage, Object, 
                                         ScrTraceAction, iTraceLine));

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cScriptMan::RemoveTrace(ObjID Object, char *pszMessage)
{
   cScrTraceHashKey TempKey(pszMessage, Object);
   cScrTrace *pTrace = m_TracePointHash.Search(TempKey.m_Combo);

   if (pTrace) {
      delete m_TracePointHash.Remove(pTrace);
      return S_OK;
   }

   Warning(("cScriptMan::RemoveTrace: Not tracing %s on object %d\n",
            pszMessage, Object));
   return S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cScriptMan::GetTraceLine(int iTraceLine)
{
   if (m_iTraceLineMask & (1 << iTraceLine))
      return TRUE;
   else
      return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cScriptMan::SetTraceLine(int iTraceLine, BOOL bStatus)
{
   if (bStatus) 
      m_iTraceLineMask |= (1 << iTraceLine);
   else
      m_iTraceLineMask &= ~(1 << iTraceLine);
}

///////////////////////////////////////

STDMETHODIMP_(int) cScriptMan::GetTraceLineMask()
{
   return m_iTraceLineMask;
}

///////////////////////////////////////

STDMETHODIMP_(void) cScriptMan::SetTraceLineMask(int iTraceLineMask)
{
   m_iTraceLineMask = iTraceLineMask;
}

///////////////////////////////////////

STDMETHODIMP_(const cScrTrace *) cScriptMan::GetFirstTrace(tScrIter *pIter)
{
   tHashSetHandle * pHandle = new tHashSetHandle;
   *pIter = (tScrIter)pHandle;
   return m_TracePointHash.GetFirst(*pHandle);
}

///////////////////////////////////////

STDMETHODIMP_(const cScrTrace *) cScriptMan::GetNextTrace(tScrIter *pIter)
{
   return m_TracePointHash.GetNext(*((tHashSetHandle *)(*pIter)));
}

///////////////////////////////////////

STDMETHODIMP_(void) cScriptMan::EndTraceIter(tScrIter *pIter)
{
   delete ((tHashSetHandle *)(*pIter));
   *pIter = NULL;
}

///////////////////////////////////////

// This is gonna slow us down substantially when any trace lines are
// on.  But, well, we'll live.
eScrTraceAction cScriptMan::FindDebugFlags(sScrMsg *pMessage)
{
   if (!m_iTraceLineMask)
      return kNoAction;

   cScrTraceHashKey TempKey(pMessage->message, pMessage->to);
   cScrTrace *pTrace = m_TracePointHash.Search(TempKey.m_Combo);
   if (!pTrace || (((1 << pTrace->m_iTraceLine) & m_iTraceLineMask) == 0))
      return kNoAction;

   return pTrace->m_TraceAction;
}

///////////////////////////////////////

HRESULT cScriptMan::ClearScriptData(ObjID objId, const char * pszClass)
{
   sScrDatum * pDatum;
   tHashSetHandle iter;

   pDatum = m_ScriptsData.GetFirst(iter);

   while (pDatum)
   {
      if (pDatum->objId == objId
       && (!pszClass || strcmp(pszClass, pDatum->pszClass) == 0))
      {
         delete m_ScriptsData.Remove(pDatum);
      }
      pDatum = m_ScriptsData.GetNext(iter);
   }
   return S_OK;
}

///////////////////////////////////////

// We save script data, current message queues, timers, and a record
// of which script objects have received BeginScript messages.  If we
// are loading, we expect the states of all of these things to be
// clear before we get here.

// The one thing which should be set up at this point is the script
// objects, since the host app knows which objects have them and what
// their names are.  All that's left for us to do is set those flags.
HRESULT cScriptMan::SaveLoad(tPersistIOFunc pfnIO, 
                                  void * pContextIO, 
                                  BOOL fLoading)
{
   int i, iNumElements;
   tHashSetHandle iter;

   if (fLoading)
      ForgetAllObjs();

   // First, we handle posted messages.
   iNumElements = m_PostedMsgs.Size();
   pfnIO(pContextIO, &iNumElements, sizeof(iNumElements));

   if (fLoading)
      m_PostedMsgs.SetSize(iNumElements);
   for (i = 0; i < iNumElements; ++i)
      m_PostedMsgs[i].pMsg
        = (sScrMsg *) PersistReadWrite(m_PostedMsgs[i].pMsg,
                                       pfnIO, pContextIO, fLoading);

   // The timed messages have a little more data, but it's not in the
   // message structure itself so we load and save it here.
   iNumElements = m_TimedMsgs.Size();
   pfnIO(pContextIO, &iNumElements, sizeof(iNumElements));

   if (fLoading)
      m_TimedMsgs.SetSize(iNumElements);
   for (i = 0; i < iNumElements; ++i)
   {
      if (fLoading)
         m_TimedMsgs[i] = new sScrTimedMsg;

      pfnIO(pContextIO, &m_TimedMsgs[i]->nextTime, 
            sizeof(m_TimedMsgs[i]->nextTime));
      pfnIO(pContextIO, &m_TimedMsgs[i]->period, 
            sizeof(m_TimedMsgs[i]->period));
      m_TimedMsgs[i]->pMsg
        = (sScrMsg *) PersistReadWrite(m_TimedMsgs[i]->pMsg,
                                       pfnIO, pContextIO, fLoading);

      pfnIO(pContextIO, &m_TimedMsgs[i]->id, 
            sizeof(m_TimedMsgs[i]->id)); 

      if (fLoading && m_TimedMsgs[i]->id >= m_NextTimerID)
         m_NextTimerID = m_TimedMsgs[i]->id + 1; 
   }

   // Script data is simple since all the elements are the same size.
   // But reading and writing require different code since it's stored
   // in a hash table rather than an array.
   iNumElements = m_ScriptsData.GetCount();
   pfnIO(pContextIO, &iNumElements, sizeof(iNumElements));
   sScrDatum *pDatum;
   sScrDatumTag tempTag = { 0, "", "" };

   if (fLoading)
   {
      for (i = 0; i < iNumElements; ++i)
      {
         pDatum = new sScrDatum(&tempTag);
         pDatum->ReadWrite(pfnIO, pContextIO, fLoading);
         m_ScriptsData.Insert(pDatum);
      }
   }
   else 
   {
      pDatum = m_ScriptsData.GetFirst(iter);

      while (pDatum)
      {
         pDatum->ReadWrite(pfnIO, pContextIO, fLoading);
         pDatum = m_ScriptsData.GetNext(iter);
      }
   }

#define SAVE_BEGIN_SCRIPTS
#ifdef SAVE_BEGIN_SCRIPTS

   // Finally, for each script instance we need to store whether it's
   // received its BeginScript message.  Each instance is identified
   // by its ObjID and its name.

   // Note that what we're actually tracking is those objects which
   // have not gotten their messages, since we expect there to be
   // fewer of them.  So the first thing we do is flag all script
   // objects as having gotten BeginScript.
   if (!fLoading)
      iNumElements = m_ActiveObjs.CountUninitializedAll();
   pfnIO(pContextIO, &iNumElements, sizeof(iNumElements));

   if (fLoading) 
   {
      m_ActiveObjs.SetInfoFlagsAll(kSIF_SentBegin);
      m_fFilterBeginMessages = TRUE;

      for (i = 0; i < iNumElements; ++i)
      {
         char aClassName[kScrMaxClassName + 1];
         ObjID obj;

         pfnIO(pContextIO, aClassName, sizeof(aClassName));
         pfnIO(pContextIO, &obj, sizeof(obj));

         {
            sScrObjFilterTag filterTag(aClassName, obj);
         }
      }
      m_fSendBeginMessages = FALSE; // default value 
   }
   else
      m_ActiveObjs.SaveScrInfo(pfnIO, pContextIO);

   pfnIO(pContextIO, &m_fSendBeginMessages, sizeof(m_fSendBeginMessages)); 

#endif // SAVE_BEGIN_SCRIPTS

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(void) cScriptMan::PostLoad()
{
   m_fFilterBeginMessages = FALSE;
   m_NeedBeginMessage.Clear();
}

///////////////////////////////////////

void cScriptMan::DeferAction(int action, ObjID objId, 
                             const char ** ppszClasses, unsigned n)
{
   m_DeferredActions.Append(new sScrDeferredAction(action, objId, 
                                                   ppszClasses, n));
}

///////////////////////////////////////

void cScriptMan::ExecuteDeferredActions()
{
   static BOOL fExecuting = FALSE;

   if (m_sendingTo.Size() || fExecuting)
      return;

   fExecuting = TRUE;

   // Note that the deferred actions array may actually grow while
   // this loop is running, should executing any deferred actions
   // introduce new deferred actions. This makes life easy on one
   // hand, but may be shown to be problematic. (toml 12-28-97)
   for (int i = 0; i < m_DeferredActions.Size(); i++)
   {
      sScrDeferredAction * pAction = m_DeferredActions[i];
      switch (pAction->action)
      {
         case kSDA_SetObjScripts:
            SetObjScripts(pAction->objId, pAction->ppszClasses, 
                          pAction->nClasses);
            break;

         case kSDA_ForgetObj:
            if (pAction->objId == kScrObjIDAll)
               ForgetAllObjs();
            else
               ForgetObj(pAction->objId);
            break;

         default:
            CriticalMsg("Unknown case");
      }
      delete pAction;
   }
   m_DeferredActions.SetSize(0);

   fExecuting = FALSE;
}

///////////////////////////////////////////////////////////////////////////////
