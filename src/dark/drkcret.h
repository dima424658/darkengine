/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkcret.h,v 1.18 2000/02/23 17:38:48 adurant Exp $
#pragma once

#ifndef __DRKCRET_H
#define __DRKCRET_H

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

// Dark-specific creature types
enum eDarkCreatureType
{
   kDCRTYPE_Humanoid,
   kDCRTYPE_PlayerLimb,
   kDCRTYPE_PlayerBowLimb,
   kDCRTYPE_Burrick,
   kDCRTYPE_Spider,
   kDCRTYPE_BugBeast,
   kDCRTYPE_CrayMan,
   kDCRTYPE_Constantine,
   kDCRTYPE_Apparition,
   kDCRTYPE_Sweel,
   kDCRTYPE_Rope,    // this must stay creature 10!!!
   kDCRTYPE_Zombie,
   kDCRTYPE_SpiderSmall,
   kDCRTYPE_Frog,
   kDCRTYPE_Cutty,
   kDCRTYPE_Avatar,
   kDCRTYPE_Robot,
   kDCRTYPE_SmallRobot,
   kDCRType_SpiderBot,
   kNumDarkCreatureTypes,
   kDCRTYPE_Invalid = 0xffffffffL // to ensure enum has size of int in all compilers
};


// Dark-specific actor types
enum eDarkActorType
{
   kDATYPE_Humanoid,
   kDATYPE_PlayerLimb,
   kDATYPE_PlayerBowLimb,
   kDATYPE_Burrick,
   kDATYPE_Spider,
   kDATYPE_Constantine,
   kDATYPE_Sweel,
   kDATYPE_Rope,
   kDATYPE_Apparition,
   kDATYPE_Robot,
   kNumDarkActorTypes,
   kDATYPE_Invalid = 0xffffffffL // to ensure enum has size of int in all compilers
};

// Dark-specific creature weapons
enum eDarkCreatureWeapon
{
   kDCrWeap_Sword,
   kDCrWeap_NumWeapons,
};

// arrays in drkcret.cpp should be kept in same order as this
enum eDarkMotorControllers
{
   kDMC_GroundLoco,
   kDMC_Combat,
   kDMC_GroundAction,
   kDMC_PlayerArm,
   kDMC_PlayerBow,
   kDMC_Single,
};

// Dark player arm modes
enum
{
   kPlayerModeUnarmed,
   kPlayerModeFlexiBow,
   kPlayerModeSword,
   kPlayerModeLockPick,
   kPlayerModeBodyCarry,
   kPlayerNumModes,
};

// Game-specific creature initialization.  Registers creature types used by
// game, and builds sdesc for creature property
EXTERN void DarkCreaturesInit();
EXTERN void DarkCreaturesTerm();

#endif
