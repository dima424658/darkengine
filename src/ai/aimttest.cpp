/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimttest.cpp,v 1.3 2000/02/28 11:30:00 toml Exp $
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
#include <aimttest.h>

// Must be last header
#include <dbmem.h>

const char * g_ppszTags[] =
{
   "Stand 0",
   "Stand 0, Halt 0",
   "Stand 0, Halt 0, NearHazard 0", // (I thought we got mocaps for this, but don't appear to be in any schemas)
   "Stand 0, Wheel 0",

   "IdleGesture",

   "Locomote 0",              // (walk forwards)
   "Locomote 0, LocoUrgent 0",
   "Locomote 0, Direction 4", // (walk backwards)
   "Locomote 0, Direction 1", // (walk left. Hammers have schema, but other humans don't currently)
   "Locomote 0, Direction 2", // (walk right.  Hammers have schema, but other humans don't currently)

   "Crumple 0, Knockout 0",
   "Crumple 0, Die 0",
   "Crumple 0, Die 0, Direction 1", // (left)
   "Crumple 0, Die 0, Direction 2", // (right)
   "Crumple 0, Die 0, Direction 3", // (forwards)
   "Crumple 0, Die 0, Direction 4", // (backwards)
   "Crumple 0, Die 0, IsConfined 0",

   "ReadyItem 0",
   "ReadyItem 0, ItemSword 0",
   "ReadyItem 0, ItemBow 0",

   "WorldFrob 0, AtWaist 0",
   "WorldFrob 0, AtWaist 0, Lever 0",
   "WorldFrob 0, AtWaist 0, Door 0",
   "WorldFrob 0, AtWaist 0, BellPull 0",
   "WorldFrob 0, OnFloor 0",
   "WorldFrob 0, AtChest 0",

   "Stand 0, Search 0",
   "Locomote 0, Search 0",
   "Search 0, Scan 0",
   "Search 0, Peek 0, Direction 1", // (peek left)
   "Search 0, Peek 0, Direction 2", // (peek right)
   "Search 0, Peek 0, Direction 3", // (peek forwards)

   "Discover 0, PointOut 0",
   "Discover 0, PointOut 0, Direction 0", // (middle)
   "Discover 0, PointOut 0, Direction 5", // (high)
   "Discover 0, PointOut 0, Direction 6", // (low)
   "Discover 0, Recoil 0",
   "Discover 0, Recoil 0, IsConfined 0",
   "Discover 0, Thwarted 0",
   "Discover 0, Salute 0",
   "Discover 0, Challenge 0",

   "Stalled 0, Poisoned 0",
   "Stalled 0, Stunned 0",
   "Stalled 0, Stunned 0, Long 0",
   "Stalled 0, Blinded 0",
   "Stalled 0, Flail 0",
   "Stalled 0, Flail 0, InWater 0",
   "Stalled 0, Flail 0, InAir 0", // (same schema as "stalled 0, flail 0" for humans, but different for Burricks)
   "Stalled 0, Flail 0, Long 0", // (same schema as "stalled 0, flail 0" for humans, but different for Burricks)
   "Stalled 0, Rebalance 0",

   "ReceiveWound 0",
   "ReceiveWound 0, Direction 1", // (left)
   "ReceiveWound 0, Direction 2", // (right)
   "ReceiveWound 0, Direction 7", // (front)
   "ReceiveWound 0, Direction 8", // (back)
   "ReceiveWound 0, SevereWound 0",
   "ReceiveWound 0, SevereWound 0, Direction 1", // (left)
   "ReceiveWound 0, SevereWound 0, Direction 2", // (right)
   "ReceiveWound 0, SevereWound 0, Direction 7", // (front)
   "ReceiveWound 0, SevereWound 0, Direction 8", // (back)

   "RangedCombat 0", // (spellcast, fire bow, belch etc, depending on persistent tags and creature type)

   "MeleeCombat 0, Stand 0",
   "MeleeCombat 0, Locomote 0, Direction 1", // (left)
   "MeleeCombat 0, Locomote 0, LocoUrgent 0, Direction 1", // (dodge left)
   "MeleeCombat 0, Locomote 0, Direction 2", // (right)
   "MeleeCombat 0, Locomote 0, LocoUrgent 0, Direction 2", // (dodge right)
   "MeleeCombat 0, Locomote 0, Direction 3", // (forward)
   "MeleeCombat 0, Locomote 0, Direction 4", // (backward)
   "MeleeCombat 0, Block 0", // (general)
   "MeleeCombat 0, Block 0, Direction 1", // (left)
   "MeleeCombat 0, Block 0, Direction 2", // (right)
   "MeleeCombat 0, Block 0, Direction 5", // (high)
   "MeleeCombat 0, Block 0, Direction 6", // (low)
   "MeleeCombat 0, Attack 0, Direction 0", // (middle)
   "MeleeCombat 0, Attack 0, Direction 5", // (high)
   "MeleeCombat 0, Attack 0, Direction 6", // (low)
   "MeleeCombat 0, Attack 0, SpecialAttack 0",
   "Discover 0, Recoil 0, Direction 4"
};

#define kNumTags (sizeof(g_ppszTags) / sizeof(const char *))


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMotionTagsTester
//


STDMETHODIMP_(const char *) cAIMotionTagsTester::GetName()
{
   return "Test motion player ability";
}

///////////////////////////////////////
//
// Scheduling calls
//

STDMETHODIMP_(void) cAIMotionTagsTester::ConnectAbility(unsigned ** ppSignals)
{
   cAIAbility::ConnectAbility(ppSignals);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIMotionTagsTester::SetControl(BOOL fInControl)
{
   cAIAbility::SetControl(fInControl);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIMotionTagsTester::InControl()
{
   return cAIAbility::InControl();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIMotionTagsTester::Init()
{
   m_current = 0;
   SetNotifications(kAICN_ActionProgress);
   cAIAbility::Init();
}

///////////////////////////////////////

DECLARE_TIMER(cAIMotionTagsTester_OnActionProgress, Average);

STDMETHODIMP_(void) cAIMotionTagsTester::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIMotionTagsTester_OnActionProgress);

   if (!pAction->InProgress())
      SignalAction();
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIMotionTagsTester::UpdateGoal(cAIGoal *)
{
   return kAIR_NoResult;
}

///////////////////////////////////////

STDMETHODIMP cAIMotionTagsTester::SuggestMode(sAIModeSuggestion * pModeSuggestion)
{
   pModeSuggestion->mode     = kAIM_Normal;
   pModeSuggestion->priority = kAIP_Normal;
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIMotionTagsTester::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
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

STDMETHODIMP cAIMotionTagsTester::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   cAIAction * pAction = NULL;

   if (!previous.Size() || previous[0]->GetType() == kAIAT_Wait)
   {
      // motion
      mprintf("Playing \"%s\"\n", g_ppszTags[m_current]);
      cAIMotionAction * pMotion = CreateMotionAction();
      pMotion->AddTags(cTagSet(g_ppszTags[m_current]));
      pAction = pMotion;
   }
   else
   {
      mprintf("Done \"%s\"\n", g_ppszTags[m_current]);
      m_current++;
      if (m_current == kNumTags)
         m_current = 0;

      // wait
      cAIWaitAction * pWait = CreateWaitAction();
      pWait->Set(700);
      pAction = pWait;
   }

   pNew->Append(pAction);

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
