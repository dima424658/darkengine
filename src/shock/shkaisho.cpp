/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaisho.cpp,v 1.2 2000/02/19 12:36:31 toml Exp $
//
//
//

#include <shkaisho.h>

#include <shkaicha.h>
#include <shkaiexp.h>

#include <appagg.h>

#include <autolink.h>
#include <speech.h>
#include <traitman.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>

#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactseq.h>
#include <aiactinv.h>
#include <aiapisnd.h>
#include <aiprops.h>
#include <aisndtyp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////

IFloatProperty * g_pAIShodanExplodeRangeProperty;

static sPropertyDesc g_AIShodanExplodeRangePropertyDesc =
{
  "ShodanExpl", 
  0,
  NULL, 0, 0, 
  { AI_ABILITY_CAT, "Shodan: Explode range" }, 
};

#define AIGetShodanExplodeRange(obj) AIGetProperty(g_pAIShodanExplodeRangeProperty, (obj), (float)5.0)

///////////////////////////////////////

void ShockAIInitShodan()
{
   g_pAIShodanExplodeRangeProperty = CreateFloatProperty(&g_AIShodanExplodeRangePropertyDesc, kPropertyImplVerySparse);
}

///////////////////////////////////////

void ShockAITermShodan()
{
   SafeRelease(g_pAIShodanExplodeRangeProperty);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShodanCombat
//

cAIShodanCombat::cAIShodanCombat()
{
   SetMode(kAIShodanClosing);
}

///////////////////////////////////////

void cAIShodanCombat::SetMode(eAIShodanCombatMode mode)
{
   m_mode = mode;
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIShodanCombat::GetName()
{
   return "Shodan combat";
}

///////////////////////////////////////

void cAIShodanCombat::TestTarget(void)
{
   ObjID target = GetTarget();
   if (target)
   {
      float rangeSq = AIGetShodanExplodeRange(GetID());
      rangeSq *= rangeSq;
      float targDistSq = m_pAIState->DistSq(*GetObjLocation(target));
      
      if (targDistSq < rangeSq)
      {
         SetMode(kAIShodanStartExplode);
         SignalAction();
      }
   }
}

//////////////////////////////////////

STDMETHODIMP_(void) cAIShodanCombat::OnActionProgress(IAIAction * pAction)
{
   cAICombat::OnActionProgress(pAction);

   if (IsOwn(pAction) && pAction->GetResult() >= kAIR_Success)
   {
      SetMode(kAIShodanClosing);
      SignalAction();
      TestTarget();
   }
   else if (IsOwn(pAction) && (m_mode == kAIShodanClosing) && pAction->GetResult() >= kAIR_NoResult)
      TestTarget();
}

///////////////////////////////////////

STDMETHODIMP cAIShodanCombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   ObjID target = GetTarget();

   if (target == OBJ_NULL)
      return S_OK;

   if (m_mode == kAIShodanStartExplode)
   {
      ObjID projID;
      sAIProjectileRel* pProjData;

      if (GetProjectile(&projID, &pProjData))
      {
         cAIExplodeAction* pAction = new cAIExplodeAction(this);
         pAction->Set(projID);
         SetMode(kAIShodanExploding);
         pNew->Append(pAction);
      }
      else
      {
         Warning(("No projectile link for shodan creature %d\n", m_pAIState->GetID()));
         SetMode(kAIShodanClosing);
      }
   }
   if (m_mode == kAIShodanClosing)
   {
      m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_CombatAttack);
      cAIChargeAction* pAction = new cAIChargeAction(this, 0);
      pAction->Set(target, kAIT_1Sec);
      pNew->Append(pAction);
   }
   return S_OK;
}
 
///////////////////////////////////////////////////////////////////////////////

BOOL cAIShodanCombat::GetProjectile(ObjID* pProjectileID, sAIProjectileRel** ppProjData)
{
   AutoAppIPtr(TraitManager);
   cAutoLinkQuery query(g_pAIProjectileRelation, pTraitManager->GetArchetype(m_pAIState->GetID()), LINKOBJ_WILDCARD);
   if (!query->Done())
   {
      *pProjectileID = query.GetDest();
      *ppProjData = (sAIProjectileRel*)(query->Data());
      return TRUE;
   }
   return FALSE;
}
