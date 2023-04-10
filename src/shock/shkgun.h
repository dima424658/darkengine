/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkgun.h,v 1.15 2000/01/31 09:55:50 adurant Exp $
#pragma once

#ifndef __SHKGUN_H
#define __SHKGUN_H

#include <osystype.h>
#include <simtime.h>
#include <label.h>
#include <shklabel.h>
#include <matrix.h>

const float kShockGunDefaultMaxRange = 10;
const int kShockGunDefaultClip = 10;
const int kShockGunDefaultLag = 1000;  // 1 second lag

const int kShockGunContinuousFire = 0x0001;  // gun fires continuously while trigger down

// info structure attached to a gun archetype object
typedef struct sBaseGunDesc
{
   int m_flags;        
   Label m_projName;        // what kind of projectile does this gun fire?
   tSimTime m_lag;          // pause between successive shots
   int m_burst;             // number of projectiles/shot
   tSimTime m_rate;         // pause between shots while continuous firing (for continuous fire only)
   int m_clip;              // shots/clip
   Label m_fireSound;       // name of .wav when fired
} sBaseGunDesc;

typedef struct sPlayerGunDesc
{
   Label m_handModel;       // gun in hand model
   Label m_iconName;        // icon bitmap
   mxs_vector m_posOffset;  // gun model offset for player
   mxs_vector m_fireOffset; // where the bullet comes out
   mxs_ang m_headingOffset; // for hand art
} sPlayerGunDesc;

typedef struct sAIGunDesc
{
   float m_maxRange;      // max range at which an AI will fire this gun
   mxs_vector m_fireOffset; // where the bullet comes out
} sAIGunDesc;

// describes a cannister-like projectile
typedef struct sCannister
{
   int m_num;           // number of "pellets" created
   float m_speed;       // pellet speed
   int m_generations;   // how many times pellets split again
} sCannister;

enum eShockGunState {kGunStateReady, kGunStateFiring,};

// for an instance of a gun
typedef struct sGunState
{
   eShockGunState m_state;
   int m_ammo;
   tSimTime m_lastFired;
} sGunState;

const float kCannisterDefaultSpeed = 10;

EXTERN void ShockGunInit(void);
EXTERN BOOL IsGun(ObjID objID);
EXTERN BOOL PullTrigger(ObjID shooterID, ObjID gunID);
EXTERN void ReleaseTrigger(ObjID shooterID, ObjID gunID);
EXTERN BOOL HoldTrigger(ObjID shooterID, ObjID gunID);
EXTERN BOOL EnactFire(ObjID shooterID, ObjID gunID);
// retained for compatibility with old AI code
EXTERN BOOL FireGun(ObjID shooterID, ObjID gunID);
#endif