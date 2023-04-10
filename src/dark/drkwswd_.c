/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkwswd_.c,v 1.12 1998/09/13 10:05:25 CCAROLLO Exp $

#include <plyablty.h>
#include <plyablt_.h>
#include <mschbase.h>
#include <drkcret.h>
#include <drkwswd_.h>

// Must be last header
#include <dbmem.h>

// internal drkwswd.cpp stuff. needs to be in c file for Watcom compile

//////////////
// THE SWORD-WIELDING ABILITY


// @NOTE: the arm position and rotation offsets here get overridden by the global
// position and rotation offsets passed in to the cPlayerSwordAbility constructor.
// If you want these to win out, then pass in NULL for pos and rot offsets to the 
// constructor.  Probably what we really want is a property for global pos and rot offset 
// hanging off sword and bow archetypes where the listener updates the
// associated player ability pos and rot offsets.  KJ 4/98

sPlayerSkillFullDesc g_SwordSkillDesc[] = {
// Sword Ready
   {          
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_Ready, kPSS_SwingMediumLeft, kPSS_Ready, kPSS_End }, // connecting skills
      {                0 // maneuver skill data
      }, 
      "PlyrSword 0",
      NULL,                   // schema name
      0,         // time to max
      0,         // mouse speed
      0,         // mouse zone
      0,         // slewSpeedScale
      0,         // rotateSpeedScale
      TRUE,      // IsIdleMotion
   },
// Sword Block
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_AtBlock, kPSS_SwingMediumLeft, kPSS_Ready, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 3",
      NULL,                   // schema name
      0, // time to max
   },
// Sword At Block
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_Ready, kPSS_SwingMediumLeft, kPSS_Ready, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      }, 
      NULL,
      NULL,                   // schema name
      0, // time to max
   },
// Sword Directed block
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_Ready, kPSS_Ready, kPSS_Ready, kPSS_Ready }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 6",
      NULL,                   // schema name
      0, // time to max
   },
// Sword Windup Short
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_AtWoundShort, kPSS_SwingMediumLeft, kPSS_SwingShort, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 1, PlyrSwordSwing 0",
      NULL,                   // schema name
      0, // time to max
   },
// Sword At Wound Short
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_Ready, kPSS_SwingMediumLeft, kPSS_SwingShort, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      NULL,
      NULL,                   // schema name
      0, // time to max
   },
// Sword Swing Short
   {
      kDMC_PlayerArm, // controller id
      kTrans_AtEnd,
      { kPSS_Ready, kPSS_SwingMediumLeft, kPSS_Ready, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 2, PlyrSwordSwing 0",
      "sword_swing_S",                   // schema name
      0, // time to max
   },
// Sword Windup Medium Left
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_AtWoundMediumLeft, kPSS_SwingMediumLeft, kPSS_SwingMediumLeft, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 1, PlyrSwordSwing 1, Direction 1",
      NULL,                   // schema name
      0, // time to max
   },
// Sword At Wound Medium Left
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_Ready, kPSS_SwingMediumLeft, kPSS_SwingMediumLeft, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      NULL,
      NULL,                   // schema name
      0, // time to max
   },
// Sword Swing Medium Left
   {
      kDMC_PlayerArm, // controller id
      kTrans_AtEnd,
      { kPSS_Ready, kPSS_SwingMediumLeft, kPSS_Ready, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 2, PlyrSwordSwing 1, Direction 1",
      "sword_swing_M",                   // schema name
      0, // time to max
   },
// Sword Windup Medium Right
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_AtWoundMediumRight, kPSS_SwingMediumRight, kPSS_SwingMediumRight, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 1, PlyrSwordSwing 1, Direction 2",
      NULL,                   // schema name
      0, // time to max
   },
// Sword At Wound Medium Right
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_Ready, kPSS_SwingMediumRight, kPSS_SwingMediumRight, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      NULL,
      NULL,                   // schema name
      0, // time to max
   },
// Sword Swing Medium Right
   {
      kDMC_PlayerArm, // controller id
      kTrans_AtEnd,
      { kPSS_Ready, kPSS_SwingMediumRight, kPSS_Ready, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 2, PlyrSwordSwing 1, Direction 2",
      "sword_swing_M",                   // schema name
      0, // time to max
   },
// Sword Windup Long Left
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_AtWoundLongLeft, kPSS_SwingLongLeft, kPSS_SwingLongLeft, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 1, PlyrSwordSwing 2",
      NULL,                   // schema name
      0, // time to max
   },
// Sword At Wound Long Left
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_Ready, kPSS_SwingLongLeft, kPSS_SwingLongLeft, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      NULL,
      NULL,                   // schema name
      0, // time to max
   },
// Sword Swing Long Left
   {
      kDMC_PlayerArm, // controller id
      kTrans_AtEnd,
      { kPSS_Ready, kPSS_SwingLongLeft, kPSS_Ready, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 2, PlyrSwordSwing 2",
      "sword_swing_L",                   // schema name
      0, // time to max
   },
// Sword Windup Long Right
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_AtWoundLongRight, kPSS_SwingLongRight, kPSS_SwingLongRight, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 1, PlyrSwordSwing 2",
      NULL,                   // schema name
      0, // time to max
   },
// Sword At Wound Long Right
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_Ready, kPSS_SwingLongRight, kPSS_SwingLongRight, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      NULL,
      NULL,                   // schema name
      0, // time to max
   },
// Sword Swing Long Right
   {
      kDMC_PlayerArm, // controller id
      kTrans_AtEnd,
      { kPSS_Ready, kPSS_SwingLongRight, kPSS_Ready, kPSS_End }, // connecting skills
      {   0 // maneuver skill data
      },              
      "PlyrSword 2, PlyrSwordSwing 2",
      "sword_swing_L",                   // schema name
      0, // time to max
   },
// Sword Begin
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kPSS_Ready, kPSS_SwingMediumLeft, kPSS_Ready, kPSS_End }, // connecting skills
      {                0 // maneuver skill data
      }, 
      "PlyrSword 4",
      "sword_begin",                   // schema name
   },
// Sword End
   {
      kDMC_PlayerArm, // controller id
      kTrans_Immediate,
      { kSkillInvalid, kSkillInvalid, kSkillInvalid, kSkillInvalid}, // connecting skills
      {                0 // maneuver skill data
      }, 
      "PlyrSword 5",
      "sword_end",                   // schema name
   },
};
