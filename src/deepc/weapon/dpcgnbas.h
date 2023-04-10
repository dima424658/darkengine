/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCGNBAS_H
#define __DPCGNBAS_H

#ifndef __OSYSTYPE_H
#include <osystype.h>
#endif // !__OSYSTYPE_H

#ifndef __SIMTIME_H
#include <simtime.h>
#endif // !__SIMTIME_H

#ifndef __LABEL_H
#include <label.h>
#endif // ! __LABEL_H

#ifndef __MATRIX_H
#include <matrix.h>
#endif // !__LABEL_H

#ifndef __LINKTYPE_H
#include <linktype.h>
#endif // !__LINKTYPE_H

// Max number of gun settings there are
// Note: must keep this is synch with a number of sdescs in dpcgunpr.cpp!
const int kNumGunSettings = 3;

// info structure attached to a gun archetype object
// can vary with setting
typedef struct sBaseGunDesc
{
   int      m_burst;                // number of projectiles/trigger pull (over time)    
   int      m_clip;                                                                      
   int      m_spray;                // number of projectiles/trigger pull (instantaneous)
   float    m_stimModifier;         // multiplier on stimulus intensity of projectile    
   tSimTime m_burstInterval;        // interval between bursts                           
   tSimTime m_shotInterval;         // interval between shots                            
   int      m_ammoUsage;            // number of ammo units used per shot                
   float    m_speedModifier;        // multiplier on projectile speed                    
   tSimTime m_reloadTime;           // time to reload                                    
   int      m_pad;                  // i'm bad                                           
} sBaseGunDesc;

typedef int ePlayerGunDescFlags;
enum ePlayerGunDescFlags_
{
   kPlayerGunKickUp    = 0x0001,
   kPlayerGunKickDown  = 0x0002,
   kPlayerGunKickLeft  = 0x0004,
   kPlayerGunKickRight = 0x0008,
   kPlayerGunJoltUp    = 0x0010,
   kPlayerGunJoltDown  = 0x0020,
   kPlayerGunJoltLeft  = 0x0040,
   kPlayerGunJoltRight = 0x0080,
   kPlayerGunDelayedExplosion = 0x0100, // Projectile doesn't explode until trigger pulled second time
};

typedef int ePlayerGunHandedness;
enum ePlayerGunHandedness_ {kPlayerGunOneHanded, kPlayerGunTwoHanded,};

typedef struct sPlayerGunDesc
{
   ePlayerGunDescFlags  m_flags;                // gun in hand model               
   Label                m_handModel;            // icon bitmap                            
   Label                m_iconName;             // gun model offset for player            
   mxs_vector           m_posOffset;            // where the bullet comes out        
   mxs_vector           m_fireOffset;           // for hand art                      
   mxs_ang              m_headingOffset;        // angle to tilt gun to for reload      
   mxs_ang              m_reloadPitch;          // rate to move gun when reloading      
   mxs_ang              m_reloadRate;           // 1/2 handed                           
   ePlayerGunHandedness m_handedness;   
} sPlayerGunDesc;

// This data is variable per setting
typedef struct sGunKick
{
   float   m_preKickPct;                    // pct of kickback to apply before shot
   mxs_ang m_kickPitch;                     // kick pitch                          
   mxs_ang m_kickPitchMax;                  // maximum kick pitch                  
   mxs_ang m_kickHeading;                   // kick heading                        
   mxs_ang m_kickAngularReturnRate;         // return rate from pitch kick         
   float   m_kickBack;                      // kick back (towards player)          
   float   m_kickBackMax;                   // maximum kick back (towards player)  
   float   m_kickBackReturnRate;            // kick back return rate               
   mxs_ang m_joltPitch;                     // jolt (of player) pitch              
   mxs_ang m_joltHeading;                   // jolt (of player) heading            
   float   m_joltBack;                      // jolt (of player) backwards          
} sGunKick;

typedef struct sGunReliability
{
   float m_minBreak;    // break chance for condition == threshBreak
   float m_maxBreak;    // break chance for condition == 0   
   float m_degradeRate; // degrade amt per use
   float m_threshBreak; // no break at condition above this
   float m_altDegradeRate; // degrade rate for alternate condition
                           // (for special properties)
} sGunReliability;

const float kDPCGunDefaultMaxRange = 10;

typedef struct sAIGunDesc
{
   float      m_maxRange;           // max range at which an AI will fire this gun
   mxs_vector m_fireOffset;         // where the bullet comes out                 
   tSimTime   m_startTime;          // lag before shooting starts                 
   tSimTime   m_burstTime;          // interval between shots in a burst          
   tSimTime   m_endTime;            // lag between bursts                         
   mxs_ang    m_aimError; 
} sAIGunDesc;

// describes a cannister-like projectile
typedef struct sCannister
{
    int   m_num;            // number of "pellets" created        
    float m_speed;          // pellet speed                       
    int   m_generations;    // how many times pellets split again 
} sCannister;

// for an instance of a gun
typedef struct sGunState
{
    int   m_ammoCount;
    float m_condition;      // 0->1 pct condition (1 is perfect) 
    int   m_setting;        // current setting                   
    int   m_modification;   // current mod level                 
    float m_condition2;     // Like m_condition but used for 
                            // "special" degradation such as overheating
} sGunState;

const float kCannisterDefaultSpeed = 10;

// Projectile relation
extern IRelation* g_pProjectileLinks;

#endif  // __DPCGNBAS_H
