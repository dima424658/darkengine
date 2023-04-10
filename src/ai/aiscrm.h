/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiscrm.h,v 1.7 2000/01/29 12:45:51 adurant Exp $
//
//
//
#pragma once

#ifndef __AISCRM_H
#define __AISCRM_H

#include <scrptmsg.h>
#include <aiscrt.h>
#include <aitype.h>

///////////////////////////////////////////////////////////////////////////////
//
// MESSAGES
//


///////////////////////////////////////
//
// MESSAGE: "SignalAI"
//

#define kAISignalMsgVer 1

struct sAISignalMsg : public sScrMsg
{
   string signal;

   sAISignalMsg()
   {
   }

   sAISignalMsg(ObjID to_whom, const char * pszSignal)
    : sScrMsg(to_whom, "SignalAI"),
      signal(pszSignal)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};


///////////////////////////////////////
//
// MESSAGE: "PatrolPoint"
//

#define kAIPatrolPointMsgVer 1

struct sAIPatrolPointMsg : public sScrMsg
{
   ObjID patrolObj;

   sAIPatrolPointMsg()
    : patrolObj(0)
   {
   }

   sAIPatrolPointMsg(ObjID to_whom, ObjID point)
    : sScrMsg(to_whom, "PatrolPoint"),
      patrolObj(point)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};


///////////////////////////////////////
//
// MESSAGE: "Alertness"
//

#define kAIAlertnessMsgVer 1

struct sAIAlertnessMsg : public sScrMsg
{
   eAIScriptAlertLevel level;
   eAIScriptAlertLevel oldLevel;

   sAIAlertnessMsg()
    : level(kNoAlert)
   {
   }

   sAIAlertnessMsg(ObjID to_whom, eAIScriptAlertLevel level, eAIScriptAlertLevel oldLevel)
    : sScrMsg(to_whom, "Alertness"),
      level(level),
      oldLevel(oldLevel)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};



#define kAIHighAlertMsgVer 1

struct sAIHighAlertMsg : public sScrMsg
{
   eAIScriptAlertLevel level;
   eAIScriptAlertLevel oldLevel;

   sAIHighAlertMsg()
    : level(kNoAlert)
   {
   }

   sAIHighAlertMsg(ObjID to_whom, eAIScriptAlertLevel level, eAIScriptAlertLevel oldLevel)
    : sScrMsg(to_whom, "HighAlert"),
      level(level),
      oldLevel(oldLevel)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};

///////////////////////////////////////
//
// MESSAGE: "AIModeChange" (AI)
//

#define kAIModeChangeMsgVer 1

struct sAIModeChangeMsg : public sScrMsg
{
   eAIMode mode;
   eAIMode previous_mode;

   sAIModeChangeMsg()
    : mode(kAIM_Asleep),previous_mode(kAIM_Asleep)
   {
   }

   sAIModeChangeMsg(ObjID to_whom, eAIMode m, eAIMode p )
    : sScrMsg(to_whom, "AIModeChange"),
      mode(m),
      previous_mode(p)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};

///////////////////////////////////////
//
// STRUCT: sAIResultMsg
//

enum eAIActionResult
{
   kActionDone,
   kActionFailed,
   kActionNotAttempted,

   kActionResultSizer = 0xffffffff
};

enum eAIAction
{
   kAINoAction,

   kAIGoto,
   kAIFrob,
   kAIManeuver,

   kActionResult = 0xffffffff
};

#define kAIResultMsgVer 1

struct sAIResultMsg : public sScrMsg
{
   eAIAction        action;
   eAIActionResult  result;
   cMultiParm       data;

   sAIResultMsg()
    : result(kActionNotAttempted)
   {
   }

   sAIResultMsg(ObjID to_whom, eAIAction action, const char * pszMessage, eAIActionResult fResult, const cMultiParm & dataToSendOnResult)
    : sScrMsg(to_whom, pszMessage),
      action(action),
      result(fResult),
      data(dataToSendOnResult)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};



///////////////////////////////////////
//
// MESSAGE: "ObjActResult" (AI)
//

#define kAIObjActResultMsgVer 1

struct sAIObjActResultMsg : public sAIResultMsg
{
   ObjID  target;

   sAIObjActResultMsg()
    : target(0)
   {
   }

   sAIObjActResultMsg(ObjID to_whom, eAIAction action, ObjID targetObject, eAIActionResult fResult, const cMultiParm & dataToSendOnReach)
    : sAIResultMsg(to_whom, action, "ObjActResult", fResult, dataToSendOnReach),
      target(targetObject)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};






///////////////////////////////////////
//
// MESSAGE: "ManeuverResult" (AI)
//

#if 0

#define kAIMnvrResultMsgVer 1

struct sAIMnvrResultMsg : public sAIResultMsg
{
   string maneuver;

   sAIMnvrResultMsg()
   {
   }

   sAIMnvrResultMsg(boolean didSucceed, const string & attemptedManeuver, eAIActionResult fResult, const cMultiParm & dataToSendOnComplete)
    : sAIResultMsg(to, "ManeuverResult", fResult, dataToSendOnComplete),
      maneuver(attemptedManeuver)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};
#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AISCRM_H */
