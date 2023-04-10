/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkwbow_.h,v 1.2 2000/01/31 09:40:08 adurant Exp $
#pragma once

#ifndef __DRKWBOW__H
#define __DRKWBOW__H


// this must be in same order as skills in static array
enum ePlayerBowSkill
{
   kPBS_Idle,
   kPBS_Aim,
   kPBS_Draw,
   kPBS_Fire,
   kPBS_Relax,
   kPBS_Begin,
   kPBS_End,
   kPBS_NumSkills,
};

EXTERN sPlayerSkillFullDesc g_BowSkillDesc[kPBS_NumSkills];

#endif
