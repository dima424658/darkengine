/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiacttyp.h,v 1.14 2000/01/29 12:45:00 adurant Exp $
//
//
//
#pragma once

#ifndef __AIACTTYP_H
#define __AIACTTYP_H

///////////////////////////////////////////////////////////////////////////////
//
// Types of actions
//
// @Note (toml 07-17-98): when adding to this, update table of strings in
//                        aibasact.cpp
// @Note (mtr 10/98): changing the order will invalidate saved AI's.

enum eAIActionTypeEnum
{
   //
   // Core actions
   //

   // The null action
   kAIAT_Null,
   
   // Wait for a specified period of time
   kAIAT_Wait,

   // Instantaneous movement
   kAIAT_Move,

   // Complex locomotion
   kAIAT_Locomote,

   // Raw motion
   kAIAT_Motion,

   // Sound
   kAIAT_Sound,

   // Orient relative to another object
   kAIAT_Orient,

   // Stand ready, but not immobile
   kAIAT_Stand,

   //
   // Compound actions
   //

   // General sequenced compound action
   kAIAT_Sequence,
   
   // Object frob
   kAIAT_Frob,

   // Object follow
   kAIAT_Follow,

   // Investigation
   kAIAT_Investigate,

   // Wandering
   kAIAT_Wander,

   // Moving to a marker
   kAIAT_MoveToMarker,
   
   // Play a psuedo-script
   kAIAT_PsuedoScript,
   
   // A proxy action
   kAIAT_Proxy,

   //
   // Game-specific actions
   //

   // Windup, fire a ranged weapon, winddown
   kAIAT_AttackRanged,

   // Launch a projectile
   kAIAT_Launch,

   // Customs
   kAIAT_CustomBase = 0x10000,

   // joint motions
   kAIAT_JointRotate,
   kAIAT_JointSlide,
   kAIAT_JointScan,

   // Shock specific
   kAIAT_FireGun,
   kAIAT_Bite,
   kAIAT_Charge,
   kAIAT_Explode,

   kAIAT_LastAction,

   kAIAT_IntMax = 0xffffffff
};

typedef unsigned tAIActionType;

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIACTTYP_H */

