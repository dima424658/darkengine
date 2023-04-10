/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkmelab.cpp,v 1.4 1999/03/10 16:00:50 JON Exp $

#include <shkmelab.h>

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <ai.h>
#include <aiapiiai.h>

#include <plyrhdlr.h>
#include <config.h>
#include <matrixs.h>
#include <objtype.h>
#include <command.h>
#include <playrobj.h>
#include <timer.h>
#include <headmove.h>
#include <schema.h>
#include <mtagvals.h>
#include <creatext.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phctrl.h>
#include <phmoapi.h>

#include <invlimb.h>

#include <plyrtype.h>
#include <plycbllm.h>   // PlayerSwitchMode
#include <crattach.h>
#include <matrixc.h>

#include <shkmelee.h>
#include <weapon.h>
#include <weaphalo.h>
#include <weapprop.h>
#include <weapreac.h>
#include <weapcb.h>

#include <rendprop.h>
#include <objpos.h>
#include <plyablt_.h>
#include <shkmel_.h>
#include <shkcret.h>

#include <mvrflags.h>

#include <plyrmode.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

static sPlayerAbilityDesc g_meleeAbilityDesc={ {"armsw2"},kShCRTYPE_PlayerLimb,kPSS_Begin,kPSS_NumSkills,g_meleeSkillDesc};
cPlayerMeleeAbility g_meleeAbility(&g_meleeAbilityDesc);

int g_meleeStartActionSkill[kPlayerMeleeNumActions] =
{
   kPSS_WindupMediumLeft,
   kPSS_WindupLong,
};

cPlayerMeleeAbility::cPlayerMeleeAbility(const sPlayerAbilityDesc *pDesc)
{
   DefaultInit(pDesc);
}

int cPlayerMeleeAbility::GetNextSkillID(int skillID, ePlayerInput input, tPlayerActionType actType)
{
   if(skillID<0||skillID>=m_Desc.nSkills)
      return kSkillInvalid;

   if(input==kPlyrInput_Start)
   {
      sSkillConnectDesc *pConnect=&m_Desc.pSkills[skillID].connect;
      int nextSkill=pConnect->nextIfStart;
      if(nextSkill&&actType!=kPlayerActionTypeInvalid)
      {
         // get special start action for type
         // XXX this may need to be more sophisticated - check if you would
         // otherwise be idling or something.  Only really made to work for
         // Melee swings and blocks.  
         // only call if startaction would otherwise at least have you
         // doing _something_        HACK HACK HACK  KJ 1/98

         // note this leaves nextSkill unchanged unless there is a start action
         nextSkill=g_meleeStartActionSkill[actType];
      }
      return nextSkill;
   } else
   {
      return DefaultGetNextSkillID(skillID, input, actType);
   }
}

void cPlayerMeleeAbility::SetupSkill(int skillID)
{
   if ((skillID == kPSS_SwingMediumLeft) || (skillID == kPSS_SwingLong))
      if (GetPlayerMode() == kPM_Crouch)
         PlayerMotionActivate(kMoWeaponSwingCrouch);
      else
         PlayerMotionActivate(kMoWeaponSwing);

   DefaultSetupSkill(skillID);
}

BOOL cPlayerMeleeAbility::GetSoundSchemaName(int skillID,Label *pName)
{
   return cPlayerAbility::GetSoundSchemaName(skillID, pName);
}

