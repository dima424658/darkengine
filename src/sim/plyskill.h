/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyskill.h,v 1.3 2000/01/31 10:00:29 adurant Exp $
#pragma once

#ifndef __PLYSKILL_H
#define __PLYSKILL_H

#include <plyrtype.h>
#include <skilltyp.h>

void PlayerSkillsGetForMode(const ePlayerMode mode, int *nSkills, tSkillID **ppSkills);

// returns the skill to begin with in specified player mode
tSkillID PlayerSkillsGetStartingForMode(const ePlayerMode mode);

// returns the skill to begin action from in specified player mode
BOOL PlayerSkillGetStartActionSkillForMode(const ePlayerMode mode, tPlayerActionType actionType, tSkillID *pSkillID);

EXTERN tSkillID g_PlayerSkills[kPlayerNumModes];

#endif
