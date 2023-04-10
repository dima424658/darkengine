/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mtagvals.h,v 1.4 2000/01/31 09:51:12 adurant Exp $
#pragma once

#ifndef __MTAGVALS_H
#define __MTAGVALS_H

enum eMTagUnitary
{
   kMTV_set,
};

enum eMTagDirectionValues
{
   kMTV_middle,
   kMTV_left,
   kMTV_right,
   kMTV_forward,
   kMTV_backward,
   kMTV_high,
   kMTV_low,
   kMTV_front,
   kMTV_back,
};

enum eMTagSwordSwingValues
{
   kMTV_swing_short,
   kMTV_swing_medium,
   kMTV_swing_long,
};

enum eMTagStairValues
{
   kMTV_upstairs,
   kMTV_downstairs,
};

//////
//////
//    THESE ARE SOON TO BE OBSOLETE   KJ 5/8/98

// this defines the enums of values for motion tags used in code
// these values are also used by the motion schema file parser.

enum eMTagBoolean
{
   kMTV_true,
};

/* add this when needed.  ground-based ("walk") is default when tag not present
enum eMTagLocoModeValues
{
   kMTV_swim,
   kMTV_fly,
   kMTV_climb,
   kMTV_jump,
};
*/

enum eMTagLocoSpeedValues
{
   kMTV_fast,
   kMTV_stationary,
};

enum eMTagFacingOffsetValues
{
   kMTV_perpleft,
   kMTV_perpright,
   kMTV_backwards,
};

enum eMTagActivityValues
{
   kMTV_searching,
   kMTV_swordmelee,
   kMTV_alert,
};

enum eMTagStandardActionValues
{
   kMTV_gesture,
   kMTV_hurt,
   kMTV_die,
   kMTV_stunned,
   kMTV_surprised,
   kMTV_alerttrans1,
   kMTV_alerttrans2,
   kMTV_alerttrans3,
};

enum eMTagMeleeActionValues
{
   kMTV_swing,
   kMTV_dodge,
   kMTV_parry,
};

enum eMTagHurtValues
{
   kMTV_hurt_light,
   kMTV_hurt_heavy,
};

enum eMTagPoseValues
{
   kMTV_pose_calib,
   kMTV_pose_swordready,
};

/* do we want this or does it fall out of occupation/actor and activity tags?
enum eMTagWeaponValues
{
   kMTV_withsword,
};
*/

#endif // mtagvals
