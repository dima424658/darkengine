/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DRKWSWD__H
#define __DRKWSWD__H

// this must be in same order as skills in static array
enum ePlayerSwordSkill
{
   kPSS_Ready,
   kPSS_Block,
   kPSS_AtBlock,
   kPSS_BlockDirected,

   kPSS_WindupShort,
   kPSS_AtWoundShort,
   kPSS_SwingShort,

   kPSS_WindupMediumLeft,
   kPSS_AtWoundMediumLeft,
   kPSS_SwingMediumLeft,
   kPSS_WindupMediumRight,
   kPSS_AtWoundMediumRight,
   kPSS_SwingMediumRight,

   kPSS_WindupLongLeft,
   kPSS_AtWoundLongLeft,
   kPSS_SwingLongLeft,
   kPSS_WindupLongRight,
   kPSS_AtWoundLongRight,
   kPSS_SwingLongRight,

   kPSS_Begin,
   kPSS_End,
   kPSS_NumSkills
};

EXTERN sPlayerSkillFullDesc g_SwordSkillDesc[];

#endif
