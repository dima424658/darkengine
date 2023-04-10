/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

// $Header: r:/t2repos/thief2/src/shock/shkcret.h,v 1.12 2000/01/31 09:55:21 adurant Exp $

#ifndef __SHKCRET_H
#define __SHKCRET_H

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

// Shock-specific creature types
enum eShockCreatureType
{
   kShCRTYPE_Humanoid = 0,
   kShCRTYPE_PlayerLimb = 1,
   kShCRTYPE_Avatar = 2,
   kShCRTYPE_Rumbler = 3,
   kShCRTYPE_Droid = 4,
   kShCRTYPE_Overlord = 5,
   kShCRTYPE_Arachnid = 6,
   kShCRTYPE_Monkey = 7,
   kShCRTYPE_BabyArachnid = 8,
   kShCRTYPE_Shodan = 9,
   kNumShockCreatureTypes,
   kShCRTYPE_Invalid = 0xffffffffL // to ensure enum has size of int in all compilers
};

// Shock-specific actor types
enum eShockActorType
{
   kShATYPE_Humanoid,
   kShATYPE_PlayerLimb,
   kShATYPE_Droid,
   kShATYPE_Overlord,
   kShATYPE_Arachnid,
   kNumShockActorTypes,
   kShATYPE_Invalid = 0xffffffffL // to ensure enum has size of int in all compilers
};

// arrays in drkcret.cpp should be kept in same order as this
enum eShockMotorControllers
{
   kShMC_GroundLoco,
   kShMC_Combat,
   kShMC_GroundAction,
   kShMC_PlayerArm,
   kShMC_PlayerBow,
   kShMC_Single,
};

// Shock player arm modes
enum
{
   kShPlayerModeUnarmed,
   kShPlayerModeMelee,
   kShPlayerNumModes,
};

// Shock-specific creature weapons
// As far as I can tell this is bogus and not used anywhere....
enum eShockCreatureWeapon
{
   kShCrWeapon_Default,
   kShCrWeap_NumWeapons,
};

// Game-specific creature initialization.  Registers creature types used by
// game, and builds sdesc for creature property
EXTERN void ShockCreaturesInit();
EXTERN void ShockCreaturesTerm();

#endif
