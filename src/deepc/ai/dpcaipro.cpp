/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//
//

#include <dpcaipro.h>

#include <appagg.h>

#include <speech.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>

#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactseq.h>
#include <aiactinv.h>
#include <aiapisnd.h>
#include <aiprops.h>
#include <aiprrngd.h>
#include <aisndtyp.h>

#include <dmgbase.h>

///////////////////////////////////////////////////////////////////////////////

const int kAIProtocolRePathTime = 3000;

///////////////////////////////////////////////////////////////////////////////

IFloatProperty * g_pAIProtocolExplodeRangeProperty;

static sPropertyDesc g_AIProtocolExplodeRangePropertyDesc =
{
  "ProtocolExpl", 
  0,
  NULL, 0, 0, 
  { AI_ABILITY_CAT, "Protocol Droid: Explode range" }, 
};

#define AIGetProtocolExplodeRange(obj) AIGetProperty(g_pAIProtocolExplodeRangeProperty, (obj), (float)5.0)

///////////////////////////////////////

void DPCAIInitProtocolDroid()
{
   g_pAIProtocolExplodeRangeProperty = CreateFloatProperty(&g_AIProtocolExplodeRangePropertyDesc, kPropertyImplVerySparse);
}

///////////////////////////////////////

void DPCAITermProtocolDroid()
{
   SafeRelease(g_pAIProtocolExplodeRangeProperty);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIProtocolCombat
//

STDMETHODIMP_(void) cAIProtocolCombat::Init()
{
   cAICombat::Init();
   m_pathFailed = FALSE;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(const char *) cAIProtocolCombat::GetName()
{
   return "Protocol combat";
}

///////////////////////////////////////

void cAIProtocolCombat::SetState(eAIProtocolCombatState state)
{
   m_state = state;
}

///////////////////////////////////////

BOOL cAIProtocolCombat::PlaySound(eAISoundConcept CombatSound)
{
   if (!m_pAI->AccessSoundEnactor())
      return FALSE;
   return (m_pAI->AccessSoundEnactor()->RequestConcept(CombatSound));
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIProtocolCombat::OnDamage(const sDamageMsg *pMsg, ObjID realCulpritID)
{
   // This is all cut 'n paste from aicbrnew. Stupid, but easiest at this stage.

   cAICombat::OnDamage(pMsg, realCulpritID);

   if (pMsg->kind!=kDamageMsgDamage)
      return;   // if it isnt "damage" damage, go home

   // Sound 
   sAIRangedWound* pWoundResponse = AIGetRangedWoundSound(GetID());
   // check propertized thresholds
   if ((int(pMsg->data.damage->amount)>pWoundResponse->m_woundThreshold) && 
      (AIRandom(0, 100)<pWoundResponse->m_responseChance))
   {
      if (int(pMsg->data.damage->amount)<pWoundResponse->m_severeThreshold)
         PlaySound(kAISC_CombatHitDamageLow);
      else
         PlaySound(kAISC_CombatHitDamageHigh);
   }
   else
      PlaySound(kAISC_CombatHitNoDam);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIProtocolCombat::OnActionProgress(IAIAction * pAction)
{
   cAICombat::OnActionProgress(pAction);

   if (IsOwn(pAction) && pAction->GetResult() >= kAIR_NoResult)
   {
      switch (m_state)
      {
      case kAIPC_Normal:
         {
            ObjID target = GetTarget();
            
            if (target)
            {
               float rangeSq = AIGetProtocolExplodeRange(GetID());
               rangeSq *= rangeSq;
               float targDistSq = m_pAIState->DistSq(*GetObjLocation(target));
               
               if (targDistSq < rangeSq)
                  SetState(kAIPC_StartExplosion);
               SignalAction();
            }
         }
         break;
      case kAIPC_StartExplosion:
         if (pAction->GetResult()>kAIR_NoResultSwitch)
         {
            SetState(kAIPC_EndExplosion);
            SignalAction();
         }
         break;
      default:
         Warning(("cAIProtocolCombat::OnActionProgress - bad state %d\n", m_state));
      }
   }
}

///////////////////////////////////////

STDMETHODIMP cAIProtocolCombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   ObjID target = GetTarget();

   switch (m_state)
   {
   case kAIPC_Normal:
      if (target && (!m_pathFailed || ((GetSimTime() - m_lastPathFailTime)>kAIProtocolRePathTime)))
      {
         m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_CombatAttack);
         cAILocoAction * pAction = CreateLocoAction();
         pAction->Set(target, kAIS_Fast, 1);
         if (pAction->Pathfind())
            pNew->Append(pAction);
         else
         {
            m_pathFailed = TRUE;
            m_lastPathFailTime = GetSimTime();
            SafeRelease(pAction);
            cAIMotionAction* pMotionAction = CreateMotionAction();
            pMotionAction->AddTags("Discover 0, Thwarted 0");
            pNew->Append(pMotionAction);
         }
      }
      break;
   case kAIPC_StartExplosion:
      {
         cTagSet tags("Explode 0");
         cAIMotionAction* pAction = CreateMotionAction();
         pAction->AddTags(tags);
         pNew->Append(pAction);
      }
      break;
   case kAIPC_EndExplosion:
      SpeechHalt(GetID());
      AutoAppIPtr(AIManager);
      pAIManager->SelfSlayAI(GetID());
      break;
   }
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

