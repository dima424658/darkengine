/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyskill.cpp,v 1.9 2000/02/19 13:27:45 toml Exp $
// 
// This contains static data about the set of skills that the player knows
// for each mode he has (where mode determined by wielded item).

#include <plyskill.h>
#include <cfgdbg.h>
#include <lg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static tSkillID g_aUnarmedSkills[] =
{
   "PlayerEmptyIdle",
};

static tSkillID g_aBowSkills[] =
{
   "PlayerBowBegin",
};

static tSkillID g_aSwordSkills[] =
{
   "PlayerSwordBegin",
};

static tSkillID g_aSwordStartActionSkills[kPlayerSwordNumActions] =
{
   "PlayerSwordBlock",
   "PlayerSwordWindShort",
   "PlayerSwordWindMedium",
   "PlayerSwordWindLong",
};

static tSkillID g_aLockPickSkills[] =
{
   "PlayerLockPickIdle",
};

static tSkillID g_aFlexiBowSkills[] =
{
   "FlexBowBegin",
};




typedef struct sSkillSetInfo
{
   tSkillID *pSkills;
   int       nSkills;
   tSkillID *pStartActionSkills;
   int       nStartActionSkills;
} sSkillSetInfo;

static sSkillSetInfo g_aPlayerSkills[kPlayerNumModes] = \
{
   { // Unarmed skills
      g_aUnarmedSkills, sizeof(g_aUnarmedSkills)/sizeof(tSkillID), \
   },
   { // Bow skills
      g_aBowSkills, sizeof(g_aBowSkills)/sizeof(tSkillID),
   },
   { // Sword skills
      g_aSwordSkills, sizeof(g_aSwordSkills)/sizeof(tSkillID),
      g_aSwordStartActionSkills, sizeof(g_aSwordStartActionSkills)/sizeof(tSkillID),
   },
   { // Lockpick skills
      g_aLockPickSkills, sizeof(g_aLockPickSkills)/sizeof(tSkillID),
   },
   { // Flexibow skills
      g_aFlexiBowSkills, sizeof(g_aFlexiBowSkills)/sizeof(tSkillID),
   },
};


void PlayerSkillsGetForMode(const ePlayerMode mode, int *nSkills, tSkillID **ppSkills)
{
   if(mode!=kPlayerModeInvalid)
   {
      *nSkills=g_aPlayerSkills[mode].nSkills;
      *ppSkills=g_aPlayerSkills[mode].pSkills;
   } else
   {
      nSkills=0;
      *ppSkills=NULL;
   }
   return;
}

// returns the skill to begin with in specified player mode
//
// Assuming that convention is followed that beginning skill always appears first
tSkillID PlayerSkillsGetStartingForMode(const ePlayerMode mode)
{
   if(mode==kPlayerModeInvalid || mode>=kPlayerNumModes || g_aPlayerSkills[mode].nSkills<1)
   {
      return NULL;
   } else
   {
      return g_aPlayerSkills[mode].pSkills[0];
   }
}

// returns the skill to begin action from in specified player mode
BOOL PlayerSkillGetStartActionSkillForMode(const ePlayerMode mode, tPlayerActionType actionType, tSkillID *pSkillID)
{
   if(mode==kPlayerModeInvalid || mode>=kPlayerNumModes ||
   g_aPlayerSkills[mode].nStartActionSkills<=actionType || g_aPlayerSkills[mode].pStartActionSkills==NULL )
   {
      Warning(("No player action %d for mode %d\n",actionType,mode));
      return FALSE;
   } else
   {
      *pSkillID=g_aPlayerSkills[mode].pStartActionSkills[actionType];
      return TRUE;
   }
}
