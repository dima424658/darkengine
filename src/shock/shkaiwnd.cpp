/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaiwnd.cpp,v 1.1 1998/11/23 09:16:24 JON Exp $
//
//
//

#include <shkaiwnd.h>

#include <lg.h>
#include <mprintf.h>

#include <aiactwnd.h>
#include <aiapibhv.h>
#include <aigoal.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIWander::GetName()
{
   return "Wandering ability";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIWander::Save(ITagFile * pTagFile)
{
   return TRUE;   
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIWander::Load(ITagFile * pTagFile)
{
   return TRUE;   
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIWander::OnActionProgress(IAIAction * pAction)
{
   if (IsOwn(pAction) && !pAction->InProgress())
      SignalAction();
}

///////////////////////////////////////

STDMETHODIMP cAIWander::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   *ppGoal = new cAIIdleGoal(this);
   (*ppGoal)->priority = kAIP_VeryLow;

   SignalAction();
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIWander::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAIWanderAction* pWanderAction = new cAIWanderAction(this);
   pWanderAction->SetRange(10, 0);
   pNew->Append(pWanderAction);

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
