/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiscrpt.cpp,v 1.17 2000/02/19 12:45:34 toml Exp $
//
//
//

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>

#include <link.h>
#include <relation.h>

#include <aiapiiai.h>
#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptmsg.h>

#include <aialert.h>
#include <aiapi.h>
#include <aiaware.h>
#include <aiprcore.h>
#include <aiscrabl.h>
#include <aisignal.h>
#include <aisound.h>
#include <speech.h>

#include <objscrt.h>
#include <aiscrpt.h>

#include <netmsg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// Messages
//

IMPLEMENT_SCRMSG_PERSISTENT(sAISignalMsg)
{
   PersistenceHeader(sScrMsg, kAISignalMsgVer);
   PersistentEnum(signal);

   return TRUE;
}

IMPLEMENT_SCRMSG_PERSISTENT(sAIPatrolPointMsg)
{
   PersistenceHeader(sScrMsg, kAIPatrolPointMsgVer);
   PersistentEnum(patrolObj);

   return TRUE;
}

IMPLEMENT_SCRMSG_PERSISTENT(sAIModeChangeMsg)
{
   PersistenceHeader(sScrMsg, kAIModeChangeMsgVer);
   PersistentEnum(mode);
   PersistentEnum(previous_mode);

   return TRUE;
}


IMPLEMENT_SCRMSG_PERSISTENT(sAIAlertnessMsg)
{
   PersistenceHeader(sScrMsg, kAIAlertnessMsgVer);
   PersistentEnum(level);
   PersistentEnum(oldLevel);

   return TRUE;
}


IMPLEMENT_SCRMSG_PERSISTENT(sAIHighAlertMsg)
{
   PersistenceHeader(sScrMsg, kAIHighAlertMsgVer);
   PersistentEnum(level);
   PersistentEnum(oldLevel);

   return TRUE;
}

IMPLEMENT_SCRMSG_PERSISTENT(sAIResultMsg)
{
   PersistenceHeader(sScrMsg, kAIResultMsgVer);
   PersistentEnum(action);
   PersistentEnum(result);
   Persistent(data);

   return TRUE;
}

IMPLEMENT_SCRMSG_PERSISTENT(sAIObjActResultMsg)
{
   PersistenceHeader(sAIResultMsg, kAIObjActResultMsgVer);
   Persistent(target);

   return TRUE;
}

#if 0
IMPLEMENT_SCRMSG_PERSISTENT(sAIMnvrResultMsg)
{
   PersistenceHeader(sAIResultMsg, kAIMnvrResultMsgVer);
   Persistent(maneuver);

   return TRUE;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Distributed services
//
// These are the implementations for script service methods that may need
// to be sent to AIs on other machines.
//
// @TBD (justin 2/12/99): Many more of the services should be distributed
// like this. Essentially, anything that may get invoked by a script on an
// object *other* than the AI itself should be treated like this. For the
// time being, AI.Signal() is the high-priority, since it's usually called
// by other objects.
//

//////////
//
// AI.Signal()
//
static void doSignal(ObjID objIdAI, const char *signalName)
{
   cAIBasicScript * pBasicScript = cAIBasicScript::AccessBasicScript(objIdAI);

   if (pBasicScript)
   {
      IInternalAI * pAI = pBasicScript->AccessOuterAI();
      sAISignal signal;
      signal.name = signalName;
      pAI->NotifySignal(&signal);
   }
}

static cNetMsg *g_pAISignalMsg = NULL;

// Message contains an ObjID and a string, and will be sent to the object's
// owner. If that is this machine, it will loopback.
static sNetMsgDesc g_pAISignalDesc =
{
   kNMF_SendToObjOwner,
   "AISignal",
   "AI.Signal Script Service",
   NULL,
   doSignal,
   {{kNMPT_ReceiverObjID},
    {kNMPT_String},
    {kNMPT_End}}
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIScrSrv
//

eAIPriority g_AIScriptPriorityToAIPriority[] =
{
   kAIP_Low,                                     // kLowPriorityAction
   kAIP_Normal,                                  // kNormalPriorityAction
   kAIP_VeryHigh                                 // kHighPriorityAction
};

eAISpeed g_AIScriptSpeedToAISpeed[] =
{
   kAIS_Slow,                                    // kSlow
   kAIS_Normal,                                  // kNormalSpeed
   kAIS_Fast                                     // kFast
};

#define ScrPriToAIPri(pri) g_AIScriptPriorityToAIPriority[pri]
// @TBD (toml 05-25-98): remove this test after the next skup
#define ScrSpdToAISpd(spd)  \
   (((spd) > kAIS_Fast) ?  \
      (((spd) == 100) ? kAIS_Fast : kAIS_Normal) : \
      g_AIScriptSpeedToAISpeed[(spd)])

DECLARE_SCRIPT_SERVICE_IMPL(cAIScrSrv, AI)
{
public:

   STDMETHOD_(void, Init)()
   {
      g_pAISignalMsg = new cNetMsg(&g_pAISignalDesc);
   }

   STDMETHOD_(void, End)()
   {
      if (g_pAISignalMsg)
      {
         delete g_pAISignalMsg;
         g_pAISignalMsg = NULL;
      }
   }

   STDMETHOD_(boolean, MakeGotoObjLoc)(ObjID objIdAI, const object &objIdTarget, eAIScriptSpeed speed, eAIActionPriority priority, const cMultiParm& dataToSendOnReach)
   {
      cAIBasicScript * pBasicScript = cAIBasicScript::AccessBasicScript(objIdAI);
      if (pBasicScript)
      {
         // @TBD (toml 05-25-98): casts bad
         return pBasicScript->GotoObjLoc(ScriptObjID(objIdTarget),
                                         ScrSpdToAISpd(speed),
                                         ScrPriToAIPri(priority),
                                         dataToSendOnReach);
      }
      return FALSE;
   }

#if 0
   STDMETHOD_(boolean, MakeDoManeuver)(ObjID objIdAI, const string & Maneuver, eAIActionPriority priority, const cMultiParm & dataToSendOnCompletion)
   {
      cAI * pAI;
      g_pAIPool->GetObjAI(objIdAI, pAI);
      if (pAI)
         return pAI->GetScriptCtx()->DoManeuver(Maneuver, priority, dataToSendOnCompletion);

      return FALSE;
   }
#endif

   STDMETHOD_(boolean, MakeFrobObj)(ObjID objIdAI, const object &objIdTarget, eAIActionPriority priority, const cMultiParm& dataToSendOnReach)
   {
      cAIBasicScript * pBasicScript = cAIBasicScript::AccessBasicScript(objIdAI);
      if (pBasicScript)
      {
         // @TBD (toml 05-25-98): casts bad
         return pBasicScript->FrobObj(ScriptObjID(objIdTarget),
                                      OBJ_NULL,
                                      ScrPriToAIPri(priority),
                                      dataToSendOnReach);
      }
      return FALSE;
   }

   STDMETHOD_(boolean, MakeFrobObj)(ObjID objIdAI, const object &objIdTarget, const object &objWith, eAIActionPriority priority, const cMultiParm& dataToSendOnReach)
   {
      cAIBasicScript * pBasicScript = cAIBasicScript::AccessBasicScript(objIdAI);
      if (pBasicScript)
      {
         // @TBD (toml 05-25-98): casts bad
         return pBasicScript->FrobObj(ScriptObjID(objIdTarget),
                                      ScriptObjID(objWith),
                                      ScrPriToAIPri(priority),
                                      dataToSendOnReach);
      }
      return FALSE;
   }

   STDMETHOD_(eAIScriptAlertLevel, GetAlertLevel)(ObjID objIdAI)
   {
      cAIBasicScript * pBasicScript = cAIBasicScript::AccessBasicScript(objIdAI);
      if (pBasicScript)
      {
         return (eAIScriptAlertLevel)pBasicScript->AccessOuterAI()->GetState()->GetAlertness();
      }
      return kNoAlert;
   }

   STDMETHOD_(void, SetMinimumAlert)(ObjID objIdAI, eAIScriptAlertLevel level)
   {
      cAIBasicScript * pBasicScript = cAIBasicScript::AccessBasicScript(objIdAI);
      if (pBasicScript)
      {
         sAIAlertCap defCap;
         sAIAlertCap * pAlertCap;
         if (!g_pAIAlertCapProperty->Get(objIdAI, &pAlertCap))
            pAlertCap = &defCap;

         if (pAlertCap->minLevel != (eAIAwareLevel)level)
         {
            pAlertCap->minLevel = (eAIAwareLevel)level;
            g_pAIAlertCapProperty->Set(objIdAI, pAlertCap);
         }
      }
   }

   STDMETHOD_(void, ClearGoals)(ObjID objIdAI)
   {
      // I'm not exactly sure if this is meaningful in the new AI
   }

   // @HACK.  The whole thing.  Also, this only supports stopping
   // speech, despite the existance of a matching flag to stop motions.
   STDMETHOD_(void, SetScriptFlags)(ObjID objIdAI, int fFlags)
   {
      cAIBasicScript * pBasicScript
         = cAIBasicScript::AccessBasicScript(objIdAI);

      if (pBasicScript)
      {
         IInternalAI *pAI = pBasicScript->AccessOuterAI();
         cAISoundEnactor *pEnactor
            = (cAISoundEnactor *) pAI->AccessSoundEnactor();

         if (!pEnactor)
            return;

         if (fFlags & kSpeechOff)
         {
            pEnactor->m_fSpeechStop = TRUE;
            SpeechHalt(objIdAI);
         }
         else
            pEnactor->m_fSpeechStop = FALSE;
      }
   }

   STDMETHOD_(void, ClearAlertness)(ObjID objIdAI)
   {
      cAIBasicScript * pBasicScript = cAIBasicScript::AccessBasicScript(objIdAI);
      if (pBasicScript)
      {
         sAIAlertness defaultAlertness;
         sAIAlertness * pAlertness;
         if (!g_pAIAlertnessProperty->Get(objIdAI, &pAlertness))
            pAlertness = &defaultAlertness;
         pAlertness->level = kAIAL_Lowest;
         g_pAIAlertnessProperty->Set(objIdAI, pAlertness);
         LinkDestroyMany(objIdAI, LINKOBJ_WILDCARD, g_pAIAwarenessLinks->GetID());
      }
   }

   STDMETHOD_(void, Signal)(ObjID objIdAI, const string& signalName)
   {
      // This will go to doSignal() on the AI's host machine:
      g_pAISignalMsg->Send(OBJ_NULL, objIdAI, signalName);
   }

   STDMETHOD_(boolean, StartConversation)(ObjID conversationID)
   {
      AutoAppIPtr(AIManager);
      return pAIManager->StartConversation(conversationID);
   }

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cAIScrSrv, AI);

///////////////////////////////////////////////////////////////////////////////

