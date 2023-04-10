/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidbgcmp.cpp,v 1.11 2000/02/19 12:17:29 toml Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>
#include <str.h>

#include <label.h>
#include <speech.h>
#include <tagdbin.h>

#include <aiapiabl.h>
#include <aiapiact.h>
#include <aidbgcmp.h>
#include <aidebug.h>
#include <aibasctm.h>
#include <aigoal.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFlowDebugger
//

STDMETHODIMP_(const char *) cAIFlowDebugger::GetName()
{
   return "Flow debugging component";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFlowDebugger::Init()
{
   SetNotifications(kAICN_ActionProgress | kAICN_GoalProgress | kAICN_ModeChange | kAICN_GoalChange | kAICN_ActionChange);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFlowDebugger::OnActionProgress(IAIAction * pAction)
{
#ifndef SHIP
   // mprintf("***** %d\n", pAction->GetResult());
   if (AIIsWatched(Flow, GetID()))
   {
      if (!pAction->InProgress())
      {
         cStr desc;
         pAction->Describe(&desc);
         AIWatch2(Flow,
                  GetID(),
                  "%s action \"%s\"",
                  (pAction->GetResult() == kAIR_Success) ? "completed" : "failed to complete",
                  desc.operator const char *());
      }
   }
#endif
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFlowDebugger::OnGoalProgress(const cAIGoal * pGoal)
{
#ifndef SHIP
   if (AIIsWatched(Flow, GetID()))
   {
      if (!pGoal->InProgress())
      {
         cStr desc;
         pGoal->Describe(&desc);
         AIWatch2(Flow,
                  GetID(),
                  "%s goal \"%s\"",
                  (pGoal->result == kAIR_Success) ? "satisfied" : "failed to satisfy",
                  desc.operator const char *());
      }
   }
#endif
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFlowDebugger::OnModeChange(eAIMode previous, eAIMode mode)
{
   AIWatch1(Flow,
            GetID(),
            "changed to mode \"%s\"",
            AIGetModeName(mode));
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFlowDebugger::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
// @TBD (toml 06-12-98): this whole class has to be ifndef ship
#ifndef SHIP
   if (AIIsWatched(Flow, GetID()))
   {
      cStr desc;
      const char * pszAbilityName;
      if (pGoal)
      {
         pGoal->Describe(&desc);
         pszAbilityName = pGoal->pOwner->GetName();
      }
      else
      {
         desc = "(No goal)";
         pszAbilityName = "(none)";
      }

      AIWatch2(Flow, GetID(),
               "changed goal to \"%s\", ability \"%s\"", desc.operator const char *(), pszAbilityName);
   }
#endif
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFlowDebugger::OnActionChange(IAIAction * pPrevious, IAIAction * pAction)
{
#ifndef SHIP
   if (AIIsWatched(Flow, GetID()))
   {
      cStr desc;
      const char * pszAbilityName;
      if (pAction)
      {
         pAction->Describe(&desc);
         pszAbilityName = pAction->Access()->pOwner->GetName();
      }
      else
      {
         desc = "(No action)";
         pszAbilityName = "(none)";
      }

      AIWatch2(Flow, GetID(),
               "changed action to \"%s\" from ability \"%s\"", desc.operator const char *(), pszAbilityName);
   }
#endif
}

///////////////////////////////////////////////////////////////////////////////
