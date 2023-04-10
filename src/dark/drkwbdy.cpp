/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkwbdy.cpp,v 1.5 1998/10/10 11:38:48 CCAROLLO Exp $

#include <drkwbdy.h>
#include <plyrhdlr.h>
#include <plyablty.h>
#include <plyablt_.h>
#include <drkcret.h>
#include <mprintf.h>
#include <plycbllm.h>
#include <plyrmode.h>

extern "C" {
#include <invlimb.h>
}
#include <dbmem.h> // must be last header

class cBodyCarryHandler : public IPlayerLimbHandler
{
   virtual BOOL Equip(ObjID item); 
   virtual void UnEquip(ObjID item);

   // is meaningless to use a corpse!
   virtual void StartUse(void) {}
   virtual void FinishUse(void) {}
};

cBodyCarryHandler g_BodyCarryHandler;

enum
{
   kPCS_Idle,
   kPCS_End,
};

sPlayerSkillFullDesc g_CorpseSkillDesc = { 
// Idle
   kDMC_PlayerArm, // controller id
   kTrans_Immediate,
   { kPCS_Idle,kPCS_Idle,kPCS_Idle, kSkillInvalid }, // connecting skills
   {                0 // maneuver skill data
   }, 
   "PlyrCorpseCarry 0",                   // tags
};

static sPlayerAbilityDesc g_CorpseAbilityDesc={ {"carry5"},kDCRTYPE_PlayerLimb,0,1,&g_CorpseSkillDesc};
static cPlayerAbility g_BodyCarryAbility(&g_CorpseAbilityDesc);


///////////////////////
//
// Body Carry Handler Implementation
//

BOOL cBodyCarryHandler::Equip(ObjID item)
{
   Label *pName;

   if(NULL!=(pName=invGetLimbModelName(item)))
   {
      g_BodyCarryAbility.SetModelName(pName);
   }
   PlayerSwitchMode(kPlayerModeBodyCarry);
   return TRUE;
}

void cBodyCarryHandler::UnEquip(ObjID item)
{
   // Tell the cerebellum to switch modes
   if ((PlayerGetMode() == kPlayerModeBodyCarry) &&
       ((PlayerGetNextMode() == kPlayerModeInvalid) || (PlayerGetNextMode() == kPlayerModeBodyCarry)))
      PlayerSwitchMode(kPlayerModeUnarmed);
}


///////////////////////
//
//    Player Handler Callbacks
//

void BodyCarryModeExitCallback(int mode, void *data)
{
   if (g_pPlayerMode->GetMode() != kPM_Swim)
      g_pPlayerMode->SetMode(kPM_Stand);
}

void BodyCarryModeEnterCallback(int mode, void *data)
{
   if (g_pPlayerMode->GetMode() != kPM_Swim)
      g_pPlayerMode->SetMode(kPM_BodyCarry);
}

void BodyCarryAbilityCallback(int mode, cPlayerAbility **pAbility, void *data)
{
   *pAbility=&g_BodyCarryAbility;
}

BOOL BodyCarryFrameCallback(ulong dt,void *data)
{
   return FALSE;
}

void BodyCarryInit(void)
{
   // Set the callback for when it's done unequipping
   PlayerHandlerSetLeaveModeCallback(kPlayerModeBodyCarry, BodyCarryModeExitCallback, NULL);
   // Set the callback for when it's done equipping
   PlayerHandlerSetEnterModeCallback(kPlayerModeBodyCarry, BodyCarryModeEnterCallback, NULL);
   PlayerHandlerSetAbilityCallback(kPlayerModeBodyCarry, BodyCarryAbilityCallback, NULL);
   PlayerHandlerSetFrameCallback(kPlayerModeBodyCarry, BodyCarryFrameCallback,NULL);

   PlayerHandlerRegister(kPlayerModeBodyCarry, &g_BodyCarryHandler);
}

void BodyCarryTerm(void)
{
}
