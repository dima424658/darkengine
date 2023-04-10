/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrptbas.h,v 1.21 1999/06/29 19:11:21 mahk Exp $
//
//

#ifndef __SCRPTBAS_H
#define __SCRPTBAS_H

#include <scrpttyp.h>

#ifdef SCRIPT

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScript
//
// Class from which project derives its root script class
//

extern IScriptMan * g_pScriptMan;
extern tScriptPrintFunc g_pPrint;


class cScript : public cCTUnaggregated <IScript, &IID_IScript, kCTU_Default>
{
public:
   cScript(ObjID objId)
    : self(objId)
   {
   }

   STDMETHOD_(const char *, GetClassName)() = 0;

   STDMETHOD (ReceiveMessage)(sScrMsg * pMsg, sMultiParm * pReply,
                              eScrTraceAction DebugAction = kNoAction)
   {
      sScrMsgMapKey key = { GetClassName(), pMsg->message };
      sScrMsgMapEntry * pMapEntry = gm_MsgMap.Search(&key);

      m_time = pMsg->time;

      switch (DebugAction)
      {
         case kBreak:
         {
            (*g_pPrint)("Breaking: script %s on object %d getting %s.\n",
                        GetClassName(), pMsg->to, pMsg->message);
            __asm { int 3 }
            break;
         }

         case kSpew:
         {
            (*g_pPrint)("Script %s on object %d getting %s.\n", 
                        GetClassName(), pMsg->to, pMsg->message);
            break;
         }
      }

      if (pMapEntry)
         (this->*(pMapEntry->pfnRouter))(pMsg, (cMultiParm *)pReply);
      else
         OnMessage_(*pMsg, (cMultiParm *)pReply);

      return S_OK;
   }

   ////////////////////////////////////

   virtual void OnMessage_(sScrMsg & message, cMultiParm * pReply)
   {
   }

   ////////////////////////////////////

   void Init()
   {
      if (!gm_Mapped.Search(GetClassName()))
      {
         DoAddMsgMaps();
         gm_Mapped.Insert(strdup(GetClassName()));
      }
   }

   ////////////////////////////////////
   //
   // Script methods
   //

   cMultiParm SendMessage(ObjID to, const char * pszMessage, 
                          const cMultiParm & data = NULL_PARM, 
                          const cMultiParm & data2 = NULL_PARM, 
                          const cMultiParm & data3 = NULL_PARM)
   {
      return g_pScriptMan->SendMessage2(self, to, pszMessage, 
                                        data, data2, data3);
   }

   void PostMessage(ObjID to, const char * pszMessage,
                    const cMultiParm & data = NULL_PARM, 
                    const cMultiParm & data2 = NULL_PARM, 
                    const cMultiParm & data3 = NULL_PARM)
   {
      g_pScriptMan->PostMessage2(self, to, pszMessage, data, data2, data3, kSMF_MsgPostToOwner);
   }

   ////////////////////////////////////

   timer_handle SetOneShotTimer(ObjID to, 
                                const string & name, 
                                float time, 
                                const cMultiParm & data = NULL_PARM)
   {
      return (timer_handle) g_pScriptMan->SetTimedMessage2(to, name,
                                                          (ulong)(time * 1000),
                                                          kSTM_OneShot, data);
   }

   void KillTimer(timer_handle hTimer)
   {
      g_pScriptMan->KillTimedMessage((tScrTimer)hTimer);
   }

   ////////////////////////////////////

#if 0
   cMultiParm SendMessage(const char * pszMessage, 
                          const cMultiParm & data = NULL_PARM, const cMultiParm & data2 = NULL_PARM, const cMultiParm & data3 = NULL_PARM)
   {
      return SendMessage(self, pszMessage, data, data2, data3);
   }

   void PostMessage(const char * pszMessage,
                    const cMultiParm & data = NULL_PARM, const cMultiParm & data2 = NULL_PARM, const cMultiParm & data3 = NULL_PARM)
   {
      PostMessage(self, pszMessage, data2, data3);
   }
#endif

   ////////////////////////////////////

   timer_handle SetOneShotTimer(const string & name, float time,
                                const cMultiParm & data = NULL_PARM)
   {
      return SetOneShotTimer(self, name, time, data);
   }

   float GetTime()
   {
      return (float)m_time / 1000.0;
   }

protected:
   ////////////////////////////////////
   //
   // Script state data
   //
   boolean IsDataSet(const char * pszDataName)
   {
      sScrDatumTag tag = { self, GetClassName(), pszDataName };
      return g_pScriptMan->IsScriptDataSet(&tag);
   }

   cMultiParm GetData(const char * pszDataName)
   {
      sScrDatumTag tag = { self, GetClassName(), pszDataName };
      cMultiParm result;
      g_pScriptMan->GetScriptData(&tag, &result);
      return result;
   }

   cMultiParm SetData(const char * pszDataName, const cMultiParm & data)
   {
      sScrDatumTag tag = { self, GetClassName(), pszDataName };
      g_pScriptMan->SetScriptData(&tag, &data);
      return data;
   }

   cMultiParm ClearData(const char * pszDataName)
   {
      sScrDatumTag tag = { self, GetClassName(), pszDataName };
      cMultiParm result;
      g_pScriptMan->ClearScriptData(&tag, &result);
      return result;
   }

   ////////////////////////////////////
   //
   // Message routing
   //

   typedef void (cScript::*tRouterFunc)(sScrMsg *, cMultiParm * pReply);

   struct sScrMsgMapKey
   {
      const char * pszClass;
      const char * pszMessage;
   };

   struct sScrMsgMapEntry : public sScrMsgMapKey
   {
      tRouterFunc  pfnRouter;
   };

   ////////////////////////////////////

   void AddMsgMapEntry(const char * pszMsg, tRouterFunc pfnRouter)
   {
      sScrMsgMapKey key = { GetClassName(), pszMsg };
      sScrMsgMapEntry * pMapEntry = gm_MsgMap.Search(&key);

      if (!pMapEntry)
      {
         pMapEntry = new sScrMsgMapEntry;
         pMapEntry->pszClass = GetClassName();
         pMapEntry->pszMessage = strdup(pszMsg);
         gm_MsgMap.Insert(pMapEntry);
      }

      pMapEntry->pfnRouter = pfnRouter;
   }

   ////////////////////////////////////
   //
   // Base messages
   //

   #define OnBeginScript()          SCRIPT_MESSAGE_HANDLER_SIGNATURE(BeginScript)
   #define OnEndScript()            SCRIPT_MESSAGE_HANDLER_SIGNATURE(EndScript)
   #define OnTimer()                SCRIPT_MESSAGE_HANDLER_SIGNATURE_(Timer, sScrTimerMsg)
   #define DefaultOnBeginScript()   SCRIPT_CALL_BASE(BeginScript)
   #define DefaultOnEndScript()     SCRIPT_CALL_BASE(EndScript)
   #define DefaultOnTimer()         SCRIPT_CALL_BASE(Timer)

   SCRIPT_DEFINE_MESSAGE_DEFAULT(BeginScript)
   SCRIPT_DEFINE_MESSAGE_DEFAULT(EndScript)
   SCRIPT_DEFINE_MESSAGE_DEFAULT_(Timer, sScrTimerMsg)

   virtual void DoAddMsgMaps()
   {
      SCRIPT_MSG_MAP_ENTRY(BeginScript);
      SCRIPT_MSG_MAP_ENTRY(EndScript);
      SCRIPT_MSG_MAP_ENTRY(Timer);
   }

   ////////////////////////////////////

   #define kScrDefMsgMapSize 401

   class cScrMsgMapHash
   {
   public:
      static unsigned Hash(const sScrMsgMapKey * p)
      {
         return (HashString(p->pszClass) ^ HashStringCaseless(p->pszMessage));
      }

      static BOOL IsEqual(const sScrMsgMapKey * p1, const sScrMsgMapKey * p2)
      {
         return !strcmp(p1->pszClass, p2->pszClass) && !stricmp(p1->pszMessage, p2->pszMessage);
      }
   };

   class cScrMsgMap : public cHashSet<sScrMsgMapEntry *, const sScrMsgMapKey *, cScrMsgMapHash>
   {
   public:
      cScrMsgMap()
       : cHashSet<sScrMsgMapEntry *, const sScrMsgMapKey *, cScrMsgMapHash>(kScrDefMsgMapSize)
      {
      }

      ~cScrMsgMap()
      {
         DestroyAll();
      }

      virtual tHashSetKey GetKey(tHashSetNode p) const
      {
         return (tHashSetKey)(p);
      }

      virtual void DestroyAll()
      {
         tHashSetHandle handle;

         sScrMsgMapEntry * p = GetFirst(handle);
         sScrMsgMapEntry * pNext;

         while (p)
         {
            pNext = GetNext(handle);
            delete Remove(p);
            p = pNext;
         }
      }

   };

   ////////////////////////////////////

   class cScrMappedClasses : public cStrHashSet<char *>
   {
   public:
      ~cScrMappedClasses()
      {
         DestroyAll();
      }

      virtual tHashSetKey GetKey(tHashSetNode p) const
      {
         return (tHashSetKey)(p);
      }

      virtual void DestroyAll()
      {
         tHashSetHandle handle;

         char * p = GetFirst(handle);
         char * pNext;

         while (p)
         {
            pNext = GetNext(handle);
            free(Remove(p));
            p = pNext;
         }
      }
   };

   ////////////////////////////////////

   ObjID             self;

   int               m_time;

   static cScrMappedClasses gm_Mapped;
   static cScrMsgMap        gm_MsgMap;
};

///////////////////////////////////////

cScript::cScrMappedClasses cScript::gm_Mapped;
cScript::cScrMsgMap        cScript::gm_MsgMap;


///////////////////////////////////////////////////////////////////////////////
//
// Module management
//

class cScriptModule : public cCTUnaggregated <IScriptModule, &IID_IScriptModule, kCTU_NoSelfDelete>
{
public:

   cScriptModule()
   {
   }

   STDMETHOD_(const char *, GetName)()
   {
      return m_szModuleName;
   }

   STDMETHOD_(const sScrClassDesc *, GetFirstClass)(tScrIter * pIter)
   {
      if (m_ClassDescs.Size())
      {
         *pIter = (tScrIter)0;
         return &m_ClassDescs[0];
      }
      return NULL;
   }

   STDMETHOD_(const sScrClassDesc *, GetNextClass)(tScrIter * pIter)
   {
      *pIter = (tScrIter)(((int)*pIter) + 1);
      if (m_ClassDescs.Size() > ((int)*pIter))
      {
         return &m_ClassDescs[((int)*pIter)];
      }
      return NULL;
   }

   STDMETHOD_(void, EndClassIter)(tScrIter * pIter)
   {
   }

   void SetName(const char * pszName)
   {
      strncpy(m_szModuleName, pszName, kScrMaxModuleName);
      m_szModuleName[kScrMaxModuleName] = 0;
   }
   
   void AddClass(const char * pszClass, const char * pszBaseClass, tScriptFactory pfnFactory)
   {
      sScrClassDesc desc;

      desc.pszModule    = m_szModuleName;
      desc.pszClass     = pszClass;
      desc.pszBaseClass = pszBaseClass;
      desc.pfnFactory   = pfnFactory;

      m_ClassDescs.Append(desc);
   }

private:
   cDynArray<sScrClassDesc> m_ClassDescs;
   char                     m_szModuleName[kScrMaxModuleName + 1];
};

extern cScriptModule g_ScriptModule;

class cScriptClassInstaller
{
public:
   cScriptClassInstaller(const char * pszClass, const char * pszBaseClass, tScriptFactory pfnFactory)
   {
      g_ScriptModule.AddClass(pszClass, pszBaseClass, pfnFactory);
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// Macros for defining actual scripts
//

#define BEGIN_ROOT_SCRIPT(ScriptName) \
   class cScr_##ScriptName : public cScript  \
   { \
   public: \
      cScr_##ScriptName(ObjID objId) \
       : cScript(objId) \
      { \
      } \
      \
      typedef cScript Base;


#define BEGIN_SCRIPT(ScriptName, BaseScriptName) \
   \
   IScript * Make##ScriptName(const char *, ObjID objId); \
   cScriptClassInstaller g_##ScriptName##Installer( #ScriptName, #BaseScriptName, Make##ScriptName ); \
   \
   class cScr_##ScriptName : public cScr_##BaseScriptName \
   { \
   public: \
      cScr_##ScriptName(ObjID objId) \
       : cScr_##BaseScriptName(objId) \
      { \
      } \
      \
      typedef cScr_##BaseScriptName Base;

#define MESSAGES     public
#define DATA         public
#define METHODS      public
#define METHOD virtual

#define END_SCRIPT(ScriptName) \
      STDMETHOD_(const char *, GetClassName)() \
      { \
         return #ScriptName; \
      } \
   }; \
   \
   IScript * Make##ScriptName(const char *, ObjID objId) \
   { \
      cScr_##ScriptName * pScript = new cScr_##ScriptName(objId); \
      pScript->Init(); \
      return pScript; \
   }

///////////////////////////////////////

#define OnMessage()        virtual void OnMessage_(sScrMsg & message, cMultiParm * pReply)
#define DefaultOnMessage() Base::OnMessage_(message, pReply)
#define BlockMessage() message.flags |= kSMF_MsgBlock
#define Reply(reply)   do { if (pReply) *(pReply) = (reply); } while (0)
#define MessageIs(s)   (stricmp(message.message, (s)) == 0)

///////////////////////////////////////////////////////////////////////////////
#endif

#endif /* !__SCRPTBAS_H */
