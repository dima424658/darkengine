/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aincbdmg.cpp,v 1.12 2000/02/24 21:53:49 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>

#include <dmgbase.h>
#include <propface.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>

#include <aiapiact.h>
#include <aiapisnd.h>
#include <aiactmot.h>
#include <aigoal.h>
#include <aincbdmg.h>
#include <aiprabil.h>
#include <aiprops.h>
#include <aisndtyp.h>
#include <aitagtyp.h>

// Must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////

static sPropertyDesc _g_NonCombatDamageResponsePropertyDesc =
{
   "AI_NCDmRsp",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Non-combat: Respond to damage" },
   kPropertyChangeLocally,  // net_flags
};

static IBoolProperty * g_pAINonCombatDamageResponseProperty;

#define AIRespondToDamage(obj) AIGetProperty(g_pAINonCombatDamageResponseProperty, (obj), (BOOL)TRUE)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitNonCombatDamageResponseAbility(IAIManager *)
{
   g_pAINonCombatDamageResponseProperty = CreateBoolProperty(&_g_NonCombatDamageResponsePropertyDesc, kPropertyImplDense);
   return TRUE;
}

///////////////////////////////////////

BOOL AITermNonCombatDamageResponseAbility()
{
   SafeRelease(g_pAINonCombatDamageResponseProperty);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINonCombatDamageResponse
//

cAINonCombatDamageResponse::cAINonCombatDamageResponse()
 : m_fDamageSignal(0),
   m_SpeakAmbushTimer(AICustomTime(15000))
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAINonCombatDamageResponse::GetName()
{
   return "Non-combat damage response";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINonCombatDamageResponse::Init()
{
   cAINonCombatAbility::Init();

   SetNotifications(kAICN_Damage | kAICN_ActionProgress);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAINonCombatDamageResponse::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_NonCombatDamageResponse, 0, 0, pTagFile))
   {
      BaseAbilitySave(pTagFile);
      AITagMove(pTagFile, &m_fDamageSignal);
      m_SpeakAmbushTimer.Save(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAINonCombatDamageResponse::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_NonCombatDamageResponse, 0, 0, pTagFile))
   {
      BaseAbilityLoad(pTagFile);
      AITagMove(pTagFile, &m_fDamageSignal);
      m_SpeakAmbushTimer.Load(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINonCombatDamageResponse::OnDamage(const sDamageMsg * pMsg, ObjID realCulpritID)
{
   sAINonCombatDmgResp *pResponse;

   if (pMsg->kind != kDamageMsgDamage)
      return;
   if (g_pAINonCombatDmgRespProperty->Get(GetID(), &pResponse))
   {
      // drop out if damage below threshold
      if (pMsg->data.damage->amount<pResponse->woundThreshold)
         return;
      // drop out randomly
      if (int((float(Rand())/float(RAND_MAX+1))*100)>pResponse->responseChance)
         return;
      m_severe = (pMsg->data.damage->amount>pResponse->severeThreshold);
   }
   else
      m_severe = 0;

   m_fDamageSignal = TRUE;
   SignalGoal();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAINonCombatDamageResponse::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   if (!IsOwn(pGoal))
   {
      SignalGoal();
      Assert_(!m_fDamageSignal);
   }

   ClearNotifications(kAICN_GoalChange);
}

///////////////////////////////////////

DECLARE_TIMER(cAINonCombatDamageResponse_OnActionProgress, Average);

STDMETHODIMP_(void) cAINonCombatDamageResponse::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAINonCombatDamageResponse_OnActionProgress);

   if (IsOwn(pAction))
   {
      if (!pAction->InProgress())
      {
         SignalGoal();
      }
   }
}

///////////////////////////////////////

STDMETHODIMP cAINonCombatDamageResponse::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   if (!m_fDamageSignal || !AIRespondToDamage(GetID()))
   {
      *ppGoal = NULL;
      return S_FALSE;
   }
   m_fDamageSignal = FALSE;

   *ppGoal = new cAIAttackGoal(this);
   (*ppGoal)->priority = kAIP_Normal;

   SignalAction();
   SetNotifications(kAICN_GoalChange);
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAINonCombatDamageResponse::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   if (m_pAI->AccessSoundEnactor())
   {
      if (m_SpeakAmbushTimer.Expired())
      {
         m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_CombatHitAmbush);
         m_SpeakAmbushTimer.Reset();
      }
      else
         m_pAI->AccessSoundEnactor()->RequestConcept((AIRandom(1, 3) == 1) ? kAISC_CombatHitDamageLow : kAISC_CombatHitDamageHigh);
   }

   cAIMotionAction * pAction = CreateMotionAction();
   if (m_severe)
      pAction->AddTags("MeleeCombat 0, ReceiveWound 0, SevereWound 0");
   else
      pAction->AddTags("MeleeCombat 0, ReceiveWound 0");
   pNew->Append(pAction);
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
