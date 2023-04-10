/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: 
//
// AI Combat - ranged idle
//

#include <aicbridl.h>

#include <aicbrngd.h>

#include <aiactmov.h>
#include <aimovsug.h>

#include <crwpnapi.h>
#include <crwpnlst.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////
  
cAIAction* cAIRangedIdle::SuggestAction(void)
{
   cAIMoveAction *pMoveAction = m_pOwner->CreateMoveAction();
   sAIMoveSuggestion *pSuggestion = new sAIMoveSuggestion;

   pSuggestion->SetWeightedBias(kAIMS_Loco, 100);
   pSuggestion->speed = kAIS_Stopped;
   pSuggestion->facing.type = kAIF_SpecificDir;
   pSuggestion->facing.ang  = m_pOwner->m_pAIState->AngleTo(GetTargetLoc()).value;
   pSuggestion->dirArc.SetByCenterAndSpan(pSuggestion->facing.ang, DEGREES(1.0));

   pMoveAction->Add(pSuggestion);

   return pMoveAction;
}


void cAIRangedIdle::OnGainingControl(void)
{
   if (CreatureWeaponObjSwizzle)
   {
      ((*CreatureWeaponObjSwizzle)(GetID(), kWeaponModeRanged));
   }
   
   cAIRangedMode::OnGainingControl();
}
