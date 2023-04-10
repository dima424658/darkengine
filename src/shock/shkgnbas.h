/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkgnbas.h,v 1.17 2000/01/31 09:55:49 adurant Exp $
#pragma once

This file has been moved to gunbase.h AMSD


#ifndef __SHKGNBAS_H
#define __SHKGNBAS_H

#include <osystype.h>
#include <simtime.h>
#include <label.h>
#include <shklabel.h>
#include <matrix.h>
#include <linktype.h>

// Max number of gun settings there are
// Note: must keep this is synch with a number of sdescs in shkgunpr.cpp!
const int kNumGunSettings = 3;

// info structure attached to a gun archetype object
// can vary with setting
typedef struct sBaseGunDesc
{
   int m_burst;             // number of projectiles/trigger pull (over time)
   int m_clip;          
   int m_spray;             // number of projectiles/trigger pull (instantaneous)
   float m_stimModifier;    // multiplier on stimulus intensity of projectile
   tSimTime m_burstInterval;  // interval between bursts
   tSimTime m_shotInterval;   // interval between shots
   int m_ammoUsage;           // number of ammo units used per shot
   float m_speedModifier;  // multiplier on projectile speed
   tSimTime m_reloadTime;     // time to reload
   int m_pad;               // i'm bad
} sBaseGunDesc;

typedef int ePlayerGunDescFlags;
enum ePlayerGunDescFlags_
{
   kPlayerGunKickUp =   0x0001,
   kPlayerGunKickDown = 0x0002,
   kPlayerGunKickLeft = 0x0004,
   kPlayerGunKickRight =0x0008,
   kPlayerGunJoltUp =   0x0010,
   kPlayerGunJoltDown = 0x0020,
   kPlayerGunJoltLeft = 0x0040,
   kPlayerGunJoltRight =0x0080,
   kPlayerGunDelayedExplosion = 0x0100, // Projectile doesn't explode until trigger pulled second time
};

typedef int ePlayerGunHandedness;
enum ePlayerGunHandedness_ {kPlayerGunOneHanded, kPlayerGunTwoHanded,};

typedef struct sPlayerGunDesc
{
   ePlayerGunDescFlags m_flags;
   Label m_handModel;       // gun in hand model
   Label m_iconName;        // icon bitmap
   mxs_vector m_posOffset;  // gun model offset for player
   mxs_vector m_fireOffset; // where the bullet comes out
   mxs_ang m_headingOffset; // for hand art
   mxs_ang m_reloadPitch;   // angle to tilt gun to for reload
   mxs_ang m_reloadRate;    // rate to move gun when reloading
   ePlayerGunHandedness m_handedness;  // 1/2 handed
} sPlayerGunDesc;

// This data is variable per setting
typedef struct sGunKick
{
   float m_preKickPct;      // pct of kickback to apply before shot
   mxs_ang m_kickPitch;     // kick pitch
   mxs_ang m_kickPitchMax;  // maximum kick pitch
   mxs_ang m_kickHeading;   // kick heading
   mxs_ang m_kickAngularReturnRate;     // return rate from pitch kick
   float m_kickBack;        // kick back (towards player)
   float m_kickBackMax;        // maximum kick back (towards player)
   float m_kickBackReturnRate; // kick back return rate
   mxs_ang m_joltPitch;     // jolt (of player) pitch
   mxs_ang m_joltHeading;   // jolt (of player) heading
   float m_joltBack;        // jolt (of player) backwards
} sGunKick;

typedef struct sGunReliability
{
   float m_minBreak;   // break chance for condition == threshBreak
   float m_maxBreak;   // break chance for condition == 0   
   float m_degradeRate; // degrade amt per use
   float m_threshBreak; // no break at condition above this
} sGunReliability;

const float kShockGunDefaultMaxRange = 10;

typedef struct sAIGunDesc
{
   float m_maxRange;      // max range at which an AI will fire this gun
   mxs_vector m_fireOffset; // where the bullet comes out
   tSimTime m_startTime;   // lag before shooting starts
   tSimTime m_burstTime;   // interval between shots in a burst
   tSimTime m_endTime;     // lag between bursts
   mxs_ang m_aimError; 
} sAIGunDesc;

// describes a cannister-like projectile
typedef struct sCannister
{
   int m_num;           // number of "pellets" created
   float m_speed;       // pellet speed
   int m_generations;   // how many times pellets split again
} sCannister;

// for an instance of a gun
typedef struct sGunState
{
   int m_ammoCount;
   float m_condition;   // 0->1 pct condition (1 is perfect)
   int m_setting;       // current setting
   int m_modification;  // current mod level
} sGunState;

const float kCannisterDefaultSpeed = 10;

// Projectile relation
extern IRelation* g_pProjectileLinks;

#endif
