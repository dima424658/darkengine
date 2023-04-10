/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/skildtab.c,v 1.42 2000/02/19 13:17:40 toml Exp $
// This contains all the static data structures for the skill table, because
// Watcom doesn't compile if these are in a c++ file.

#include <skildtab.h>
#include <fix.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// MOTOR CONTROLLERS
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////


////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// DEFAULT MOTION DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////


EXTERN sDefaultFullDesc g_aDefaultDesc[] = { { kTeleportController } };



////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// SINGLEPLAY DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////

EXTERN sSinglePlayFullDesc g_aSinglePlayDesc[] = { {kMultipedPlayController} };

////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// COMBAT DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////

EXTERN sCombatFullDesc g_aCombatDesc[] = { {kCombatController} };

////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// LOCOMOTION DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////

EXTERN sLocoFullDesc g_aLocoDesc[] = { \
// Walk
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Forwards,kMS_Speed_Normal,kMS_LocoKind_Normal,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_WALK_LEFT_GUARD_READY, MDT_WALK_RIGHT_GUARD_READY, 1.5, -1.5, 1.0, 1.0, 180, 15, 0.5, 5.0, 0 } // gait data
   },
// Stand Normal
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Forwards,kMS_Speed_Stationary,kMS_LocoKind_Normal,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_STAND_GUARD_CASUAL, MDT_STAND_GUARD_CASUAL, 1.5, -1.5, 1.0, 1.0, 180, 15, 0,0, 0 } // gait data
   },
// Stand Searching
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Forwards,kMS_Speed_Stationary,kMS_LocoKind_Searching,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_STAND_GUARD_READY, MDT_STAND_GUARD_READY, 1.5, -1.5, 1.0, 1.0, 180, 15, 0,0, 0 } // gait data
   },
// Stand Combat
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Forwards,kMS_Speed_Stationary,kMS_LocoKind_SwordCombat,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_STAND_GUARD_READY, MDT_STAND_GUARD_READY, 1.5, -1.5, 1.0, 1.0, 180, 15, 0,0, 0 } // gait data
   },
// Walk Searching
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Forwards,kMS_Speed_Normal,kMS_LocoKind_Searching,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_WALK_SEARCH_CYCLE_GUARD_READY, MDT_WALK_SEARCH_CYCLE_GUARD_READY, 1.5, -1.5, 1.0, 1.0, 180, 15, 0.5, 5.0, 0 }
   },
// Run
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Forwards,kMS_Speed_Fast,kMS_LocoKind_Normal,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_RUN_LEFT_SERVANT, MDT_RUN_RIGHT_SERVANT, 1.5, -1.5, 1.0, 1.0, 180, 15, 0.5, 5.0, 1, { {180, MDT_RUN_TURN_180 } } } // gait data
   },
// Sword Combat Advance
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Forwards,kMS_Speed_Normal,kMS_LocoKind_SwordCombat,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_ADVANCE_GUARD, MDT_ADVANCE_GUARD, 1.5, -1.5, 0.8, 1.0, 90, 15, 0.5, 5.0, 0 }
   },
// Sword Combat Backup
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Backwards,kMS_Speed_Normal,kMS_LocoKind_SwordCombat,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_RETREAT_GUARD, MDT_RETREAT_GUARD, 1.5, -1.5, 0.8, 1.0, 90, 15, 0.5, 5.0, 0 }
   },
// Sword Combat Sidestep Left
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Right,kMS_Speed_Normal,kMS_LocoKind_SwordCombat,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_SIDESTEP_LEFT_GUARD, MDT_SIDESTEP_LEFT_GUARD, 1.5, -1.5, 0.8, 1.0, 180, 15, 0.5, 5.0, 0 }
   },
// Sword Combat Sidestep Right
   {
      kMultipedGroundLocoController, // controller id
      {kMS_Mode_OnGround,kMS_Fac_Left,kMS_Speed_Normal,kMS_LocoKind_SwordCombat,kMS_PhysCon_Normal}, // skill selection desc
      { MDT_SIDESTEP_RIGHT_GUARD, MDT_SIDESTEP_RIGHT_GUARD, 1.5, -1.5, 0.8, 1.0, 180, 15, 0.5, 5.0, 0 }
   },
};


////////////////////////////////////////////////////
////////////////////////////////////////////////////
///
/// PLAYER SKILLS DATA 
///
////////////////////////////////////////////////////
////////////////////////////////////////////////////


EXTERN sPlayerSkillFullDesc g_aPlayerSkillDesc[] = { \
// Sword Ready
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerSwordReady", "PlayerSwordSwingMedium", "PlayerSwordReady", "PlayerSwordEnd" }, // connecting skills
      {                // maneuver skill data
         "ph212203",
//         "GSW031C",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         4.0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      }, 
      "armsw2",               // model name
      NULL,                   // schema name
      0,         // time to max
      0,         // shoot
      0,         // mouse speed
      0,         // mouse zone
      0,         // slewSpeedScale
      0,         // rotateSpeedScale
      TRUE,      // IsIdleMotion
   },
// Sword Block
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerSwordAtBlock", "PlayerSwordSwingMedium", "PlayerSwordReady", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
//         "GA213086", 
         "AH213211",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0.9, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      "sword_swing",                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword At Block
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerSwordReady", "PlayerSwordSwingMedium", "PlayerSwordReady", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
         NULL,
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
      },              
      "armsw2",               // model name
      NULL,                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword Windup Short
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerSwordAtWoundShort", "PlayerSwordSwingMedium", "PlayerSwordSwingShort", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
           "sh213003",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      NULL,                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword At Wound Short
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerSwordReady", "PlayerSwordSwingMedium", "PlayerSwordSwingShort", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
         NULL,
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      NULL,                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword Swing Short
   {
      kPlayerStandardController, // controller id
      kTrans_AtEnd,
      { "PlayerSwordReady", "PlayerSwordSwingMedium", "PlayerSwordReady", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
           "mh213003",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0.67, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      "sword_swing",                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword Windup Medium
   {
      kPlayerStandardController, // controller id
      kTrans_AtEnd,
      { "PlayerSwordAtWoundMedium", "PlayerSwordSwingMedium", "PlayerSwordSwingMedium", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
           "sh213056",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      NULL,                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword At Wound Medium
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerSwordReady", "PlayerSwordSwingMedium", "PlayerSwordSwingMedium", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
         NULL,
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      NULL,                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword Swing Medium
   {
      kPlayerStandardController, // controller id
      kTrans_AtEnd,
      { "PlayerSwordReady", "PlayerSwordSwingMedium", "PlayerSwordReady", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
           "mh213056",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0.7, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      "sword_swing",                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword Windup Long
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerSwordAtWoundLong", "PlayerSwordSwingMedium", "PlayerSwordSwingLong", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
           "GA213070",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         3.0, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      NULL,                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword At Wound Long
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerSwordReady", "PlayerSwordSwingMedium", "PlayerSwordSwingLong", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
         NULL,
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      NULL,                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword Swing Long
   {
      kPlayerStandardController, // controller id
      kTrans_AtEnd,
      { "PlayerSwordReady", "PlayerSwordSwingMedium", "PlayerSwordReady", "PlayerSwordEnd" }, // connecting skills
      {   // maneuver skill data
           "GA213030",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0.8, // time scale (0 means not specified)
      },              
      "armsw2",               // model name
      "sword_swing",                   // schema name
      0, // time to max
      0, // shoot
   },
// Sword Begin
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerSwordReady", "PlayerSwordSwingMedium", "PlayerSwordReady", "PlayerSwordEnd" }, // connecting skills
      {                // maneuver skill data
//         "GSW031A",
         "ph212202",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0.6, // time scale (0 means not specified)
      }, 
      "armsw2",               // model name
      "sword_begin",                   // schema name
   },
// Sword End
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { NULL, NULL, NULL, NULL}, // connecting skills
      {                // maneuver skill data
//         "GSW031B",
         "ph212205",
         {1.57,-0.267,-3.077}, {0,0,0},
         {1.57,-0.267,-3.077}, {0,0,0},
         0, // duration (0 means not specified)
         0.6, // time scale (0 means not specified)
      }, 
      "armsw2",               // model name
      "sword_end",                   // schema name
   },
// Empty-Handed Idle
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerEmptyIdle", "PlayerEmptyIdle", "PlayerEmptyIdle", NULL }, // connecting skills
      {                // maneuver skill data
         NULL, 
         {0,-1.0,-2.0}, {0,0,0},
         {0,-1.0,-2.0}, {0,0,0}, 
         4.0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      }, 
      NULL,               // model name
      NULL,                   // schema name
      0, // time to max
      0, // shoot
      0,         // mouse speed
      0,         // mouse zone
      0,         // slewSpeedScale
      0,         // rotateSpeedScale
      TRUE,      // IsIdleMotion
   },
// Lockpick Idle
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerLockPickIdle","PlayerLockPickRaise", "PlayerLockPickIdle", NULL}, // connecting skills
      {                // maneuver skill data
         NULL, 
         {0.447,0.2485,-2.517}, {degrees_to_fixang(356),degrees_to_fixang(350),degrees_to_fixang(19)},
         {0.447,0.2485,-2.517}, {degrees_to_fixang(356),degrees_to_fixang(350),degrees_to_fixang(19)},
         4.0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      }, 
      "armtrl4",               // model name
      NULL,                   // schema name
   },
// Lockpick Raise
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerLockPickFrob","PlayerLockPickFrob", "PlayerLockPickLower", "PlayerLockPickLower"}, // connecting skills
      {                // maneuver skill data
         "GA114322", 
         {0.447,0.2485,-2.517}, {degrees_to_fixang(356),degrees_to_fixang(350),degrees_to_fixang(19)},
         {0.447,0.2485,-2.517}, {degrees_to_fixang(356),degrees_to_fixang(350),degrees_to_fixang(19)},
         0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      }, 
      "armtrl4",               // model name
      NULL,                   // schema name
   },
// Lockpick Frob
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerLockPickFrob","PlayerLockPickFrob", "PlayerLockPickLower", "PlayerLockPickLower"}, // connecting skills
      {                // maneuver skill data
         "GA114323", 
         {0.447,0.2485,-2.517}, {degrees_to_fixang(356),degrees_to_fixang(350),degrees_to_fixang(19)},
         {0.447,0.2485,-2.517}, {degrees_to_fixang(356),degrees_to_fixang(350),degrees_to_fixang(19)},
         0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      }, 
      "armtrl4",               // model name
      NULL,                   // schema name
   },
// Lockpick Lower
   {
      kPlayerStandardController, // controller id
      kTrans_Immediate,
      { "PlayerLockPickIdle","PlayerLockPickRaise", "PlayerLockPickIdle", "PlayerLockPickEnd"}, // connecting skills
      {                // maneuver skill data
         "GA114324", 
         {0.447,0.2485,-2.517}, {degrees_to_fixang(356),degrees_to_fixang(350),degrees_to_fixang(19)},
         {0.447,0.2485,-2.517}, {degrees_to_fixang(356),degrees_to_fixang(350),degrees_to_fixang(19)},
         0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      }, 
      "armtrl4",               // model name
      NULL,                   // schema name
   },
// FlexBow idle
   {
      kPlayerFlexBowController, // controller id
      kTrans_Immediate,
      { "FlexBowIdle","FlexBowAim", "FlexBowIdle", "FlexBowEnd" }, // connecting skills
      {                // maneuver skill data
         "d99bdown",
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         4.0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      }, 
      "bowempt9",               // model name
      NULL,                   // schema name
   },
// FlexBow aim
   {
      kPlayerFlexBowController, // controller id
      kTrans_Immediate,
      { "FlexBowDraw", "FlexBowDraw", "FlexBowIdle", "FlexBowRelax" }, // connecting skills
      {                // maneuver skill data
         "GABaim", 
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         0, // duration (0 means not specified)
         0.5, // time scale (0 means not specified)
      }, 
      "bowempt9",               // model name
      NULL,                   // schema name
   },
// FlexBow draw
   {
      kPlayerFlexBowController, // controller id
      kTrans_Immediate,
      { "FlexBowDraw", "FlexBowDraw", "FlexBowFire", "FlexBowRelax" }, // connecting skills
      {                // maneuver skill data
         NULL, 
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
         1.0, // flexing duration
         30, // maximum flex, in degrees
      }, 
      "bowempt9",               // model name
      "bowpull",                   // schema name
      1.0, // time to max, for powering reasons
      0, // shoot
      0x1800, // mouse speed
      0, // mouse zone
      0.5,  // player slew speed scale
      0.7,  // player rotate speed scale
   },
// FlexBow fire
   {
      kPlayerFlexBowController, // controller id
      kTrans_Immediate,
      { "FlexBowRelax", "FlexBowDraw", "FlexBowRelax", "FlexBowRelax" }, // connecting skills
      { // maneuver skill data 
         NULL,
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         0.8,  // duration
         0, // time scale (0 means not specified)
         0, // flexing duration
         0, // maximum flex, in degrees
      },
      "bowempt9",              // model name
      "bowtwang_player",       // schema name
      0, // time to max
      TRUE, // shoot!
   },
// FlexBow relax
   {
      kPlayerFlexBowController, // controller id
      kTrans_Immediate,
      { "FlexBowIdle", "FlexBowAim", "FlexBowIdle", "FlexBowEnd" }, // connecting skills
      {                // maneuver skill data
         "d99brelx", 
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         0, // duration (0 means not specified)
         0.5, // time scale (0 means not specified)
      }, 
      "bowempt9",               // model name
      NULL,                   // schema name
   },
// FlexBow begin
   {
      kPlayerFlexBowController, // controller id
      kTrans_Immediate,
      { "FlexBowIdle","FlexBowAim", "FlexBowIdle", "FlexBowEnd"}, // connecting skills
      {                // maneuver skill data
         "d99bbegn", 
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         1.0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      }, 
      "bowempt9",               // model name
      "bow_begin",                   // schema name
   },
// FlexBow end
   {
      kPlayerFlexBowController, // controller id
      kTrans_Immediate,
      { NULL, NULL, NULL, NULL}, // connecting skills
      {                // maneuver skill data
         NULL, 
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         { -0.87,0.8,-2.59}, {degrees_to_fixang(11), degrees_to_fixang(14), degrees_to_fixang(284) },
         1.0, // duration (0 means not specified)
         0, // time scale (0 means not specified)
      }, 
      "bowempt9",               // model name
      "bow_end",                   // schema name
   },
};
