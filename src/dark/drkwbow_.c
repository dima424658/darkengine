/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkwbow_.c,v 1.6 1998/10/22 00:11:00 dc Exp $

#include <plyablty.h>
#include <plyablt_.h>
#include <mschbase.h>
#include <drkcret.h>
#include <drkwbow_.h>
#include <fix.h>
#include <dbmem.h> // must be last header

// THE BOW-WIELDING ABILITY

// default start finish abort

sPlayerSkillFullDesc g_BowSkillDesc[kPBS_NumSkills] = {
// 0 - FlexBow idle - this is idle the bow at side
   {
      kDMC_PlayerBow, // controller id
      kTrans_Immediate,
      { kPBS_Idle, kPBS_Aim, kPBS_Idle, kPBS_End }, // connecting skills
      {                0 // maneuver skill data
      }, 
      "PlyrBow 0",
      NULL,                   // schema name
   },
// 1 - FlexBow aim - this is actually lift the bow to ready
   {
      kDMC_PlayerBow, // controller id
      kTrans_Immediate,
      { kPBS_Draw, kPBS_Draw, kPBS_Idle, kPBS_Relax }, // connecting skills
      {                0 // maneuver skill data
      }, 
      "PlyrBow 1",
      NULL,                   // schema name
   },
// 2 - FlexBow draw - this is draw back and go
   {
      kDMC_PlayerBow, // controller id
      kTrans_Immediate,
      { kPBS_Draw, kPBS_Draw, kPBS_Fire, kPBS_Relax }, // connecting skills
      {                 // maneuver skill data
         1.0, // flexing duration
         30, // maximum flex, in degrees
      }, 
      NULL, 
      "bowpull",                   // schema name
      1.0, // time to max, for powering reasons
      0x1800, // mouse speed
      0, // mouse zone
      0.4,  // player slew speed scale
      0.6,  // player rotate speed scale
   },
// 3 - FlexBow fire - actually do the fire
   {
      kDMC_PlayerBow, // controller id
      kTrans_Immediate,
      { kPBS_Fire, kPBS_Draw, kPBS_Idle, kPBS_Idle }, // connecting skills
      { 0 // maneuver skill data 
      },
      "PlyrBow 6",
      NULL, // "bowtwang_player",       // schema name - now done in code
      0, // time to max
   },
// 4 - FlexBow relax - return to rest, i fear
   {
      kDMC_PlayerBow, // controller id
      kTrans_Immediate,
      { kPBS_Idle, kPBS_Aim, kPBS_Idle, kPBS_End }, // connecting skills
      {                0 // maneuver skill data
      }, 
      "PlyrBow 4",
      NULL,                   // schema name
   },
// 5 - FlexBow begin - 
   {
      kDMC_PlayerBow, // controller id
      kTrans_Immediate,
      { kPBS_Idle,kPBS_Aim, kPBS_Idle, kPBS_End }, // connecting skills
      {                0 // maneuver skill data
      }, 
      "PlyrBow 5",
      "bow_begin",                   // schema name
   },
// 6 - FlexBow end
   {
      kDMC_PlayerBow, // controller id
      kTrans_Immediate,
      { kSkillInvalid, kSkillInvalid, kSkillInvalid, kSkillInvalid}, // connecting skills
      {                0 // maneuver skill data
      }, 
      "PlyrBow 7", 
      "bow_end",                   // schema name
   },
};

