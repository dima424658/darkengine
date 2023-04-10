/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaitbs.cpp,v 1.7 1999/11/19 14:54:35 adurant Exp $
//
// Turret Behavior Set
//

This file has been moved to aiturbs.cpp AMSD


#include <shkaitbs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aicbrngd.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aiscrabl.h>
#include <aisnsdev.h>
#include <aisound.h>
#include <aiturret.h>

#include <shkaigna.h>
#include <shkaidth.h>

// Must be last header
#include <dbmem.h>

cAIAction* ShockGunActionFactoryFn(cAITurret *pTurret, ObjID targetObj, int subModel)
{
   cAIGunAction *pAction;

   pAction = new cAIGunAction(pTurret);
   pAction->Set(targetObj, subModel, pTurret->GetRotateJoint());
   return pAction;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITurretBehaviorSet
//

STDMETHODIMP_(const char *) cAITurretBehaviorSet::GetName()
{
   return "Turret";
}

////////////////////////////////////

void cAITurretBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
{
   // Debugging/development tools
   pComponents->Append(new cAIFlowDebugger);

   // Enactors
   pComponents->Append(new cAISoundEnactor);

   // Senses
   pComponents->Append(new cAIDeviceSenses);

}

////////////////////////////////////
 
void cAITurretBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIQuickDeath);
   pComponents->Append(new cAIBasicScript);
}

///////////////////////////////////////

void cAITurretBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIFreeze);
}

///////////////////////////////////////

void cAITurretBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAITurret *pTurret = new cAITurret;
   pTurret->Set(ShockGunActionFactoryFn);
   pComponents->Append(pTurret);
}

 
