/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/daifrog.cpp,v 1.4 2000/03/17 14:23:26 bfarquha Exp $
//
//
//

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
#include <aisndtyp.h>
#include <daifrog.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!

///////////////////////////////////////////////////////////////////////////////

IFloatProperty * g_pAIFrogExplodeRangeProperty;

static sPropertyDesc g_AIFrogExplodeRangePropertyDesc =
{
  "DAI_FrogExpl",
  0,
  NULL, 0, 0,
  { AI_ABILITY_CAT, "Frog-beast: Explode range" },
};

#define AIGetFrogExplodeRange(obj) AIGetProperty(g_pAIFrogExplodeRangeProperty, (obj), (float)5.0)

///////////////////////////////////////

void DarkAIInitFrogBeast()
{
   g_pAIFrogExplodeRangeProperty = CreateFloatProperty(&g_AIFrogExplodeRangePropertyDesc, kPropertyImplVerySparse);
}

///////////////////////////////////////

void DarkAITermFrogBeast()
{
   SafeRelease(g_pAIFrogExplodeRangeProperty);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFrogCombat
//

STDMETHODIMP_(const char *) cAIFrogCombat::GetName()
{
   return "Dark frog combat";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIFrogCombat::OnActionProgress(IAIAction * pAction)
{
   cAICombat::OnActionProgress(pAction);

   if (IsOwn(pAction) && pAction->GetResult() >= kAIR_NoResult)
   {
      ObjID target = GetTarget();

      if (target)
      {
         float rangeSq = AIGetFrogExplodeRange(GetID());
         rangeSq *= rangeSq;
         float targDistSq = m_pAIState->DistSq(*GetObjLocation(target));

         if (targDistSq < rangeSq)
         {
            SpeechHalt(GetID());
            AutoAppIPtr(AIManager);
            pAIManager->SelfSlayAI(GetID());
         }
      }
   }
}

///////////////////////////////////////

STDMETHODIMP cAIFrogCombat::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   ObjID target = GetTarget();

   if (target)
   {
      m_pAI->AccessSoundEnactor()->RequestConcept(kAISC_CombatAttack);
      cAIInvestAction * pAction = CreateInvestAction();
      pAction->Set(target);
// wsf: InitScan removed for T2, since frogs are only used as weapon for player. With InitScan there
// the frog, once thrown, would just sit there and scan a long time before actually making its way
// to its target and exploding. If we wanted to, we could setup a property that specifies whether or
// not we want InitScan.
//      pAction->SetStyle(kAIIS_InitOrient | kAIIS_InitScan);
      pAction->SetStyle(kAIIS_InitOrient);
      pNew->Append(pAction);
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

