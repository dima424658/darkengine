/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimplyr.cpp,v 1.3 2000/02/11 18:27:53 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>

#include <playrobj.h>

#include <aiman.h>
#include <aiactmot.h>
#include <aiactwt.h>
#include <aigoal.h>
#include <aimplyr.h>
#include <label.h>

// Must be last header
#include <dbmem.h>

EXTERN Label g_TestMotionName= { "" };


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITestMotionPlayer
//


STDMETHODIMP_(const char *) cAITestMotionPlayer::GetName()
{
   return "Test motion player ability";
}

///////////////////////////////////////
//
// Scheduling calls
//

STDMETHODIMP_(void) cAITestMotionPlayer::ConnectAbility(unsigned ** ppSignals)
{
   cAIAbility::ConnectAbility(ppSignals);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAITestMotionPlayer::SetControl(BOOL fInControl)
{
   cAIAbility::SetControl(fInControl);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAITestMotionPlayer::InControl()
{
   return cAIAbility::InControl();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAITestMotionPlayer::Init()
{
   SetNotifications(kAICN_ActionProgress);
   cAIAbility::Init();
}

///////////////////////////////////////

DECLARE_TIMER(cAITestMotionPlayer_OnActionProgress, Average);

STDMETHODIMP_(void) cAITestMotionPlayer::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAITestMotionPlayer_OnActionProgress);

   if (!pAction->InProgress())
      SignalAction();
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAITestMotionPlayer::UpdateGoal(cAIGoal *)
{
   return kAIR_NoResult;
}

///////////////////////////////////////

STDMETHODIMP cAITestMotionPlayer::SuggestMode(sAIModeSuggestion * pModeSuggestion)
{
   pModeSuggestion->mode     = kAIM_Normal;
   pModeSuggestion->priority = kAIP_Normal;
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAITestMotionPlayer::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
{
   cAIGoal * pGoal;
   pGoal = new cAICustomGoal(this);
   pGoal->priority = kAIP_Absolute;

   SignalAction();
   *ppNew = pGoal;

   return S_OK;
}

///////////////////////////////////////
//
// Default action suggestor
//
// The default behavior is to move towards the goal
//

STDMETHODIMP cAITestMotionPlayer::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAIAction * pAction = NULL;

   if ((!previous.Size() || previous[0]->GetType() == kAIAT_Wait)&&strlen(g_TestMotionName.text)>0)
   {
      // motion
      mprintf("Playing \"%s\"\n", g_TestMotionName.text);
      cAIMotionAction * pMotion = CreateMotionAction();
      pMotion->SetByName(g_TestMotionName.text);

      pAction = pMotion;
   }
   else
   {
      if(strlen(g_TestMotionName.text)>0)
         mprintf("Done \"%s\"\n", g_TestMotionName.text);

      // wait
      cAIWaitAction * pWait = CreateWaitAction();
      pWait->Set(700);
      pAction = pWait;
   }

   pNew->Append(pAction);

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
