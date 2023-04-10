/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aitest.cpp,v 1.13 1998/10/10 14:01:52 TOML Exp $
//
//
//

#include <lg.h>

#include <ctagset.h>
#include <playrobj.h>

#include <aiman.h>
#include <aiapibhv.h>
#include <aiapisnd.h>
#include <aiactloc.h>
#include <aiactfol.h>
#include <aiactmov.h>
#include <aiactwnd.h>
#include <aigoal.h>
#include <aiactori.h>
#include <aisndtyp.h>
#include <aitest.h>
#include <aiutils.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum eAIPScriptStepType
{
   kAIPS_Script,
   kAIPS_Play,
   kAIPS_Alertness,
   kAIPS_BecomeHostile,
   kAIPS_EnableInvestigate,
   kAIPS_Goto,
   kAIPS_Frob,
   kAIPS_Wait
};


struct sAIPScriptStep
{
   eAIPScriptStepType type;
};


struct sAIPScript
{
   
};


class cAIPScript
{
   
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITest
//


STDMETHODIMP_(const char *) cAITest::GetName()
{
   return "Test ability";
}

///////////////////////////////////////
//
// Scheduling calls
//

STDMETHODIMP_(void) cAITest::ConnectAbility(unsigned ** ppSignals)
{
   cAIAbility::ConnectAbility(ppSignals);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAITest::SetControl(BOOL fInControl)
{
   cAIAbility::SetControl(fInControl);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAITest::InControl()
{
   return cAIAbility::InControl();
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAITest::UpdateGoal(cAIGoal *)
{
   if (m_timer.Expired())
   {
      m_timer.Reset();
      SignalGoal();
   }
   return kAIR_NoResult;
}

///////////////////////////////////////

STDMETHODIMP cAITest::SuggestMode(sAIModeSuggestion * pModeSuggestion)
{
   pModeSuggestion->mode     = kAIM_Normal;
   pModeSuggestion->priority = kAIP_Normal;
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAITest::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppNew)
{
   cAIGoal * pGoal;
   if (!pPrevious)
   {
      pGoal = new cAIGotoGoal(this);
      pGoal->object = PlayerObject();
      pGoal->priority = kAIP_Absolute;
   }
   else
   {
      pGoal = pPrevious;
      pGoal->AddRef();
   }
   

   pGoal->LocFromObj();

   SignalAction();
   *ppNew = pGoal;
   
   m_timer.Set(AICustomTime(600000));

   return S_OK;
}

///////////////////////////////////////
//
// Default action suggestor
//
// The default behavior is to move towards the goal
//

STDMETHODIMP cAITest::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
#if 0
   static cAITimer timer(kAIT_2Sec);

   mprintf("sa\n");
   if (timer.Expired())
   {
      timer.Reset();
      m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_AlertZeroBroadcast);
   }

   cAILocoAction * pAction = CreateLocoAction();
   pAction->SetFromGoal((cAIGotoGoal *)pGoal);
   pNew->Append(pAction);
#endif
#if 0
   cAIMoveAction * pAction = CreateMoveAction();
   sAIMoveSuggestion * pSuggestion = new sAIMoveSuggestion;

   pSuggestion->SetWeightedBias(kAIMS_Loco, 100);
   pSuggestion->dirArc.SetByCenterAndSpan(m_pAIState->AngleTo(pGoal->location),
                                          floatang(PI*1.8));
   pSuggestion->speed       = kAIS_VeryFast;
   pSuggestion->facing.type = kAIF_MoveDir;
   pSuggestion->dest        = pGoal->location;

   pAction->Add(pSuggestion);

   pNew->Append(pAction);

#endif

#if 0
   cAIFollowAction * pAction = CreateFollowAction();
   
   pAction->SetObject(PlayerObject());
   pAction->AddVector(DEGREES(30), 10.0 * (g_pAIManager->GetAIIndex(GetID()) + 1));
   pAction->AddVector(DEGREES(-30), 10.0 * (g_pAIManager->GetAIIndex(GetID()) + 1));
#endif
#if 0
   
   cAIOrientAction * pAction = CreateOrientAction();
   
   switch (AIRandom(0, 0))
   {
      case 0:
         mprint("Facing player\n");
         pAction->Set(PlayerObject());
         break;
      case 1:
         mprint("Facing 90 off player\n");
         pAction->Set(DEGREES(90), PlayerObject());
         break;
      case 2:
         mprint("Facing absolute 180\n");
         pAction->Set(DEGREES(180));
         break;
   }

   pNew->Append(pAction);
#endif
#if 1
   mprintf("test suggesting action\n");
   cAIWanderAction * pAction = CreateWanderAction();
   pAction->AddTags("Search 0");
   pAction->SetPauseMotion("Search 0, Scan 0");
   pNew->Append(pAction);
#endif

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
