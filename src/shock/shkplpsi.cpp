/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkplpsi.cpp,v 1.8 2000/02/19 13:25:55 toml Exp $

#include "shkplpsi.h"
#include "shkpsi.h"
#include "shkpsibs.h"
#include "playrobj.h"
#include <memall.h>
#include <dbmem.h>   // must be last header! 

cPsionics *g_playerPsi = NULL;

// @HACK: should get this from character
const float kPlayerPsiPower = 5;

void PlayerPsiInit(void)
{
   if (g_playerPsi == NULL)
      g_playerPsi = new cPsionics(PlayerObject(), kPlayerPsiPower);
   else
      g_playerPsi->Init(PlayerObject(), kPlayerPsiPower);
   g_playerPsi->Select(kPsiNone);
}

void PlayerPsiHandleRDown(void)
{
   switch (g_playerPsi->GetType())
   {
   case kPsiNone:
      break;
   case kPsiLevitation:
   case kPsiSuggestion:
   case kPsiForce:
      g_playerPsi->Activate();
      break;
   case kPsiRadar:
   case kPsiProjection:
   case kPsiHealing:
      if (g_playerPsi->IsActive())
         g_playerPsi->Deactivate();
      else
         g_playerPsi->Activate();
      break;
   }
}

void PlayerPsiHandleRUp(void)
{
   switch (g_playerPsi->GetType())
   {
   case kPsiNone:
   case kPsiSuggestion:
   case kPsiRadar:
      break;
   case kPsiLevitation:
   case kPsiForce:
      if (g_playerPsi->IsActive())
         g_playerPsi->Deactivate();
      break;
   }
}

// these can't be inline, cos we need to include in c files...

void PlayerPsiFrame(int dt) {g_playerPsi->Frame(dt);}
void PlayerPsiSelect(ePsiType type) {g_playerPsi->Select(type);}
BOOL PlayerPsiActivate(void) {return g_playerPsi->Activate();}
void PlayerPsiDeactivate(void) {g_playerPsi->Deactivate();}
BOOL PlayerPsiIsActive(void) {return g_playerPsi->IsActive();}
ePsiType PlayerPsiGetType(void) {return g_playerPsi->GetType();}
float PlayerPsiGetPower(void) {return g_playerPsi->GetPower();}
float PlayerPsiGetMaxPower(void) {return g_playerPsi->GetMaxPower();}
void PlayerPsiSetMaxPower(float maxPower) {g_playerPsi->SetMaxPower(maxPower);}
void PlayerPsiSetPower(float power) {g_playerPsi->SetPower(power);}
BOOL PlayerPsiIsCrashing(void) {return g_playerPsi->IsCrashing();}
eDamageResult PlayerPsiCrashImpactHandler(void) {return g_playerPsi->CrashImpactHandler();}

