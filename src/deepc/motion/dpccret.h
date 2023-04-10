/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

#ifndef __DPCCRET_H
#define __DPCCRET_H

//////
// What are creature types and actor types, you ask?
//
// Every creature has a creature type and an actor type
// All creatures of a given creature type have the same actor type.
//
// Creature type specifies the skeletal configuration and phys model size
// for a creature.
//
// Actor type specifies the set of motion schemas that make sense to play
// a creature of that type (every schema has exactly one actor). 
//
// Both are defined on a per-game basis.  Similarly, the creature type
// definition modules (crhumand.c etc) are considered game-specific, and 
// include this header to specify their actor type.
//
// Creature type and actor type are two different things because it's possible
// for two creatures to have different skeletal configurations but use the
// same motion captures (a legless droid and a human, for example).
//
// While we could have disjoint sets of motions to be played on two creatures
// of the same skeletal configuration (a human and a burrick, for example),
// I'm still making it so that the creature type determines the actor type.
// This is for simplicity, because tags can be used take care of this case, 
// and because things like humans and burricks probably need to be different 
// actor types anyway because of phys model size and placement.  
//
//                                           -- KJ 4/98

// eDPCCreatureType: Deep Cover-specific creature types
// Make sure you add new ones onto the end.
// Note:  I stubbed out the rope as a reminder that 10 is reserved by Thief.
//        This has to remain unused by Deep Cover,
//        since the engine assumes creature type 10 is a rope.(Bodisafa 1/7/2000)
enum eDPCCreatureType
{
   kDPCCRTYPE_Humanoid     = 0,
   kDPCCRTYPE_PlayerLimb   = 1,
   kDPCCRTYPE_Avatar       = 2,
   kDPCCRTYPE_OldDog       = 3,
   kDPCCRTYPE_Dog          = 4,
   kDPCCRTYPE_DeepHuman    = 5,
//   kDPCCRTYPE_Rope         = 10,   // Har har.  The engine needs this for Thief 2.
   kNumDPCCreatureTypes,
   kDPCCRTYPE_Invalid = 0xffffffffL // to ensure enum has size of int in all compilers
};

// Deep Cover-specific actor types
enum eDPCActorType
{
   kDPCATYPE_Humanoid,
   kDPCATYPE_PlayerLimb,
   kDPCATYPE_OldDog,
   kDPCATYPE_Dog,
   kDPCATYPE_DeepHuman,
   kNumDPCActorTypes,
   kDPCATYPE_Invalid = 0xffffffffL // to ensure enum has size of int in all compilers
};

// arrays in drkcret.cpp should be kept in same order as this
enum eDPCMotorControllers
{
   kDPCMC_GroundLoco,
   kDPCMC_Combat,
   kDPCMC_GroundAction,
   kDPCMC_PlayerArm,
   kDPCMC_PlayerBow,
   kDPCMC_Single,
};

// Deep Cover player arm modes
enum
{
   kDPCPlayerModeUnarmed,
   kDPCPlayerModeMelee,
   kDPCPlayerNumModes,
};

// Deep Cover-specific creature weapons
// As far as I can tell this is bogus and not used anywhere....
enum eDPCCreatureWeapon
{
   kDPCCrWeapon_Default,
   kDPCCrWeap_NumWeapons,
};

// Game-specific creature initialization.  Registers creature types used by
// game, and builds sdesc for creature property
EXTERN void DPCCreaturesInit();
EXTERN void DPCCreaturesTerm();

#endif
