/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasabl.cpp,v 1.18 2000/02/17 19:00:42 bfarquha Exp $
//
//
//

#include <lg.h>
#include <timings.h>

#include <aiapibhv.h>
#include <aitrace.h>
#include <aibasact.h>
#include <aibasabl.h>
#include <aibasctm.h>
#include <aigoal.h>
#include <aitagtyp.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAbility
//

cAIAbility::~cAIAbility()
{
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIAbility::ConnectAbility(unsigned ** ppSignals)
{
   *ppSignals = &m_signals;

#ifndef SHIP
   // Used only for setting breakpoints
   m_DebugID       = m_pAIState->GetID();
   m_pDebugAI      = m_pAI;
   m_pDebugAIState = m_pAIState;
#endif
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIAbility::Init()
{
   cAIComponentBase<IAIAbility, &IID_IAIAbility>::Init();
   SignalMode();
   SignalGoal();
   SetNotifications(kAICN_SimStart | kAICN_GoalChange | kAICN_ModeChange);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIAbility::SaveGoal(ITagFile * pTagFile, cAIGoal * pGoal)
{
   BOOL fHaveGoal = !!pGoal;
   AITagMove(pTagFile, &fHaveGoal);
   if (!fHaveGoal)
      return TRUE;

   eAIGoalType type = pGoal->GetType();
   AITagMove(pTagFile, &type);
   pGoal->Save(pTagFile);

   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIAbility::LoadGoal(ITagFile * pTagFile, cAIGoal ** ppGoal)
{
   BOOL fHaveGoal;
   AITagMove(pTagFile, &fHaveGoal);
   if (!fHaveGoal)
   {
      *ppGoal = NULL;
      return TRUE;
   }

   cAIGoal *   pGoal;
   eAIGoalType type;

   AITagMove(pTagFile, &type);

   pGoal = AICreateGoalFromType(type, this);

   if (!pGoal)
      return FALSE;

   pGoal->Load(pTagFile);

   *ppGoal = pGoal;
   return TRUE;
}

///////////////////////////////////////


STDMETHODIMP_(void) cAIAbility::SetControl(BOOL fInControl)
{
   if (fInControl)
      m_flags |= kInControl;
   else
      m_flags &= ~kInControl;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIAbility::InControl()
{
   return !!(m_flags & kInControl);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIAbility::OnSimStart()
{
   SignalGoal();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIAbility::OnModeChange(eAIMode previous, eAIMode mode)
{
   if (mode >= kAIM_Normal)
      SignalGoal();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIAbility::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   if (IsOwn(pGoal))
      SignalAction();
}

///////////////////////////////////////
//
// Default handling for goals -- not all goals are handled
//

STDMETHODIMP_(eAIResult) cAIAbility::UpdateGoal(cAIGoal * pGoal)
{
   // @TBD (toml 04-23-98): does this function even get called without the goal being in progress?
   if (pGoal->InProgress())
   {
      switch (pGoal->GetType())
      {
         case kAIGT_Goto:
         {
            if (((cAIGotoGoal *)pGoal)->Reached())
            {
               pGoal->pctComplete = 100;
               pGoal->result = kAIR_Success;
            }
            else
               pGoal->pctComplete = 0;

            break;
         }
      }
   }
   return pGoal->result;
}

///////////////////////////////////////

STDMETHODIMP cAIAbility::SuggestMode(sAIModeSuggestion *)
{
   return S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP cAIAbility::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
{
   return S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIAbility::FinalizeGoal(cAIGoal *pGoal)
{
   return TRUE;
}

///////////////////////////////////////

BOOL cAIAbility::IsOwn(const cAIGoal * pGoal) const
{
   return (pGoal && pGoal->pOwner == (IAIAbility *)this);
}

///////////////////////////////////////

BOOL cAIAbility::IsRelevant(const cAIGoal * pPrevious, const cAIGoal * pGoal) const
{
   return ((pPrevious && pPrevious->pOwner == (IAIAbility *)this) || (pGoal && pGoal->pOwner == (IAIAbility *)this));
}

///////////////////////////////////////

BOOL cAIAbility::IsGainingControl(const cAIGoal * pPrevious, const cAIGoal * pGoal) const
{
   return (pGoal && pGoal->pOwner == (IAIAbility *)this && (!pPrevious || pPrevious->pOwner != (IAIAbility *)this));
}

///////////////////////////////////////

BOOL cAIAbility::IsLosingControl(const cAIGoal * pPrevious, const cAIGoal * pGoal) const
{
   return ((!pGoal || pGoal->pOwner != (IAIAbility *)this) && (pPrevious && pPrevious->pOwner == (IAIAbility *)this));
}

///////////////////////////////////////

void cAIAbility::BaseAbilitySave(ITagFile * pTagFile)
{
   SaveNotifications(pTagFile);
   AITagMove(pTagFile, &m_flags);
   AITagMove(pTagFile, &m_signals);
}

///////////////////////////////////////

void cAIAbility::BaseAbilityLoad(ITagFile * pTagFile)
{
   LoadNotifications(pTagFile);
   AITagMove(pTagFile, &m_flags);
   AITagMove(pTagFile, &m_signals);
}

///////////////////////////////////////////////////////////////////////////////
