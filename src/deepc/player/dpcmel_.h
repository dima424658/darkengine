/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCMEL__H
#define __DPCMEL__H

// This must be in same order as skills in static array
enum ePlayerMeleeSkill
{
   kPSS_Ready,
   kPSS_WindupMediumLeft,
   kPSS_AtWoundMediumLeft,
   kPSS_SwingMediumLeft,
   kPSS_WindupLong,
   kPSS_AtWoundLong,
   kPSS_SwingLong,
   kPSS_Begin,
   kPSS_End,
   kPSS_NumSkills
};

EXTERN sPlayerSkillFullDesc g_meleeSkillDesc[];

#endif
