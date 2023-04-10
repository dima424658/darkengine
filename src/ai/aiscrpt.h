/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiscrpt.h,v 1.10 2000/01/29 12:45:52 adurant Exp $
//
//
#pragma once

#ifndef __AISCRPT_H
#define __AISCRPT_H

#include <scrptsrv.h>
#include <scrptbas.h>

#include <aiscrt.h>
#include <aiscrm.h>

#include <objscrt.h>

///////////////////////////////////////////////////////////////////////////////
//
// AI Services
//

DECLARE_SCRIPT_SERVICE(AI, 0xe5)
{
   STDMETHOD_(boolean, MakeGotoObjLoc)(ObjID objIdAI, const object &objIdTarget, eAIScriptSpeed speed = kNormalSpeed, eAIActionPriority = kNormalPriorityAction, const cMultiParm ref dataToSendOnReach = NULL_PARM) PURE;
#if 0
   STDMETHOD_(boolean, MakeDoManeuver)(ObjID objIdAI, const string ref Maneuver, eAIActionPriority = kNormalPriorityAction, const cMultiParm ref dataToSendOnCompletion = NULL_PARM) PURE;
#endif
   STDMETHOD_(boolean, MakeFrobObj)(ObjID objIdAI, const object &objIdTarget, eAIActionPriority = kNormalPriorityAction, const cMultiParm ref dataToSendOnReach = NULL_PARM) PURE;
   STDMETHOD_(boolean, MakeFrobObj)(ObjID objIdAI, const object &objIdTarget, const object &objWith, eAIActionPriority = kNormalPriorityAction, const cMultiParm ref dataToSendOnReach = NULL_PARM) PURE;
   STDMETHOD_(eAIScriptAlertLevel, GetAlertLevel)(ObjID objIdAI) PURE;
   STDMETHOD_(void, SetMinimumAlert)(ObjID objIdAI, eAIScriptAlertLevel level) PURE;
   STDMETHOD_(void, ClearGoals)(ObjID objIdAI) PURE;
   STDMETHOD_(void, SetScriptFlags)(ObjID objIdAI, int iFlags) PURE;
   STDMETHOD_(void, ClearAlertness)(ObjID objIdAI) PURE;
   STDMETHOD_(void, Signal)(ObjID objIdAI, const string ref signal) PURE;
   STDMETHOD_(boolean, StartConversation)(ObjID conversationID) PURE;
};


///////////////////////////////////////////////////////////////////////////////
//
// Base scripts
//

#ifdef SCRIPT

#define SuccessfulGoto(s) \
      (message.action == kAIGoto && \
       message.result == kActionDone && \
       message.target == (int)(object(s)))

#define SetState(s) SetData("State", (s))
#define GetState()  GetData("State")
#define StateIs(s) (GetData("State") == (s))
#define States      enum

#define OnSignalAI()                  SCRIPT_MESSAGE_HANDLER_SIGNATURE_(SignalAI, sAISignalMsg)
#define DefaultOnSignalAI()           SCRIPT_CALL_BASE(SignalAI)

#define OnPatrolPoint()               SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PatrolPoint, sAIPatrolPointMsg)
#define DefaultOnPatrolPoint()        SCRIPT_CALL_BASE(PatrolPoint)

#define OnAlertness()                 SCRIPT_MESSAGE_HANDLER_SIGNATURE_(Alertness, sAIAlertnessMsg)
#define DefaultOnAlertness()          SCRIPT_CALL_BASE(Alertness)

#define OnHighAlert()                 SCRIPT_MESSAGE_HANDLER_SIGNATURE_(HighAlert, sAIHighAlertMsg)
#define DefaultOnHighAlert()          SCRIPT_CALL_BASE(HighAlert)

#define OnObjActResult()              SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ObjActResult, sAIObjActResultMsg)
#define DefaultOnObjActResult()       SCRIPT_CALL_BASE(ObjActResult)

#define OnManeuverResult()            SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ManeuverResult, sAIMnvrResultMsg)
#define DefaultOnManeuverResult()     SCRIPT_CALL_BASE(ManeuverResult)

#define OnAIModeChange()              SCRIPT_MESSAGE_HANDLER_SIGNATURE_(AIModeChange, sAIModeChangeMsg)
#define DefaultOnAIModeChange()       SCRIPT_CALL_BASE(AIModeChange)


///////////////////////////////////////
//
// SCRIPT: AI
//

BEGIN_SCRIPT(AI, RootScript)

   MESSAGE_DEFS:

      SCRIPT_DEFINE_MESSAGE_(SignalAI, sAISignalMsg)
      {
      }

      SCRIPT_DEFINE_MESSAGE_(PatrolPoint, sAIPatrolPointMsg)
      {
      }

      SCRIPT_DEFINE_MESSAGE_(AIModeChange, sAIModeChangeMsg)
      {
      }


      SCRIPT_DEFINE_MESSAGE_(Alertness, sAIAlertnessMsg)
      {
      }

      SCRIPT_DEFINE_MESSAGE_(HighAlert, sAIHighAlertMsg)
      {
      }

      SCRIPT_DEFINE_MESSAGE_(ObjActResult, sAIObjActResultMsg)
      {
      }

#if 0
      SCRIPT_DEFINE_MESSAGE_(ManeuverResult, sAIMnvrResultMsg)
      {
      }
#endif

      SCRIPT_BEGIN_MESSAGE_MAP()
         SCRIPT_MSG_MAP_ENTRY(SignalAI)
         SCRIPT_MSG_MAP_ENTRY(PatrolPoint)
         SCRIPT_MSG_MAP_ENTRY(AIModeChange)
         SCRIPT_MSG_MAP_ENTRY(Alertness)
         SCRIPT_MSG_MAP_ENTRY(HighAlert)
         SCRIPT_MSG_MAP_ENTRY(ObjActResult)
#if 0
         SCRIPT_MSG_MAP_ENTRY(ManeuverResult)
#endif
      SCRIPT_END_MESSAGE_MAP()

   METHODS:

      METHOD boolean GotoObjLoc(const object &objIdTarget, eAIScriptSpeed speed = kNormalSpeed, eAIActionPriority priority = kNormalPriorityAction, const cMultiParm ref dataToSendOnReach = NULL_PARM)
      {
         return AI.MakeGotoObjLoc(self, objIdTarget, speed, priority, dataToSendOnReach);
      }

#if 0
      METHOD boolean DoManeuver(const string ref Maneuver, eAIActionPriority priority = kNormalPriorityAction, const cMultiParm ref dataToSendOnCompletion = NULL_PARM)
      {
         return AI.MakeDoManeuver(self, Maneuver, priority, dataToSendOnCompletion);
      }
#endif

      METHOD boolean FrobObj(const object &objIdTarget, eAIActionPriority priority = kNormalPriorityAction, const cMultiParm ref dataToSendOnReach = NULL_PARM)
      {
         return AI.MakeFrobObj(self, objIdTarget, priority, dataToSendOnReach);
      }

      METHOD boolean FrobObj(const object &objIdTarget, const object &objWith, eAIActionPriority priority = kNormalPriorityAction, const cMultiParm ref dataToSendOnReach = NULL_PARM)
      {
         return AI.MakeFrobObj(self, objIdTarget, objWith, priority, dataToSendOnReach);
      }

      METHOD eAIScriptAlertLevel GetAlertLevel()
      {
         return AI.GetAlertLevel(self);
      }

      METHOD void SetMinimumAlert(eAIScriptAlertLevel level)
      {
         AI.SetMinimumAlert(self, level);
      }

      METHOD void ClearGoals()
      {
         AI.ClearGoals(self);
      }

      METHOD void SetScriptFlags(int iFlags)
      {
         AI.SetScriptFlags(self, iFlags);
      }

      METHOD void ClearAlertness()
      {
         AI.ClearAlertness(self);
      }

      METHOD void Signal(const string ref signal)
      {
         AI.Signal(self, signal);
      }

END_SCRIPT(AI)

#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AISCRPT_H */
