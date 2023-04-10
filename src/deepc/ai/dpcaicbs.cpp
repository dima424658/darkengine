/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Camera Behavior Set
//

#include <dpcaicbs.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aicamera.h>
#include <aidbgcmp.h>
#include <aifreeze.h>
#include <aiscrabl.h>
#include <aisound.h>
#include <aisnsdev.h>

#include <aigunact.h>
#include <dpcaicmd.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICameraBehaviorSet
//

STDMETHODIMP_(const char *) cAICameraBehaviorSet::GetName()
{
   return "camera";
}

////////////////////////////////////

void cAICameraBehaviorSet::CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
{
   // Debugging/development tools
   pComponents->Append(new cAIFlowDebugger);

   // Enactors
   pComponents->Append(new cAISoundEnactor);

   // Senses
   pComponents->Append(new cAIDeviceSenses);

}

////////////////////////////////////
 
void cAICameraBehaviorSet::CreateGenericAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAICameraDeath);
   pComponents->Append(new cAIBasicScript);
}

///////////////////////////////////////

void cAICameraBehaviorSet::CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
{
   pComponents->Append(new cAIFreeze);
}

///////////////////////////////////////
 
void cAICameraBehaviorSet::CreateCombatAbilities(cAIComponentPtrs * pComponents)
{
   cAICamera *pCamera = new cAICamera();
   pComponents->Append(pCamera);
}

 