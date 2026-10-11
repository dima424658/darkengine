/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

#ifndef __AISCRPTAPI_H
#define __AISCRPTAPI_H

#include <scrptsrv.h>
#include <scrptbas.h>
#include <aiscrt.h>
#include <objscrt.h>

// Platform-neutral AI script-service interface.  Message declarations stay
// in aiscrpt.h so low-level script bridges need not include the AI type graph.
DECLARE_SCRIPT_SERVICE(AI, 0xe5)
{
   STDMETHOD_(boolean, MakeGotoObjLoc)(ObjID objIdAI, const object &objIdTarget,
      eAIScriptSpeed speed = kNormalSpeed,
      eAIActionPriority priority = kNormalPriorityAction,
      const cMultiParm &dataToSendOnReach = NULL_PARM) PURE;
   STDMETHOD_(boolean, MakeFrobObj)(ObjID objIdAI, const object &objIdTarget,
      eAIActionPriority priority = kNormalPriorityAction,
      const cMultiParm &dataToSendOnReach = NULL_PARM) PURE;
   STDMETHOD_(boolean, MakeFrobObj)(ObjID objIdAI, const object &objIdTarget,
      const object &objWith,
      eAIActionPriority priority = kNormalPriorityAction,
      const cMultiParm &dataToSendOnReach = NULL_PARM) PURE;
   STDMETHOD_(eAIScriptAlertLevel, GetAlertLevel)(ObjID objIdAI) PURE;
   STDMETHOD_(void, SetMinimumAlert)(ObjID objIdAI,
      eAIScriptAlertLevel level) PURE;
   STDMETHOD_(void, ClearGoals)(ObjID objIdAI) PURE;
   STDMETHOD_(void, SetScriptFlags)(ObjID objIdAI, int flags) PURE;
   STDMETHOD_(void, ClearAlertness)(ObjID objIdAI) PURE;
   STDMETHOD_(void, Signal)(ObjID objIdAI, const string &signal) PURE;
   STDMETHOD_(boolean, StartConversation)(ObjID conversationID) PURE;
};

#endif
