/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghosttyp.h,v 1.10 2000/01/29 13:41:13 adurant Exp $
// ghost data typedefs and structures
#pragma once

#ifndef __GHOSTTYP_H
#define __GHOSTTYP_H

#include <matrixs.h>
#include <objtype.h>

#include <ghostflg.h> // get the public flags too
#include <mclntapi.h> // for MotionCoordinator - replace with forward decl?

#define EPS_ZERO_VEC  (0.01)

// this gives a range of +/- 720 degrees/sec before we overflow, which I'm 
// guessing is good enough 

#define FIXVEL_TO_DTANG(x,t)  (short)((((long)(x)) * (long)(t)) / 250)
#define DTANG_TO_FIXVEL(x,t)  (short)((((long)(x)) * 250) / (long)(t))

////////////////////////////
// enums

// motion modes, initial hack
typedef enum {
   kGhostModeStand,        // first group is mirror of kPM_
   kGhostModeCrouch,
   kGhostModeSwim,
   kGhostModeClimb,
   kGhostModeCarry,        // umm, ok, well, whatever
   kGhostModeSlide,        // speaking of whatever...
   kGhostModeJumpBase,
   kGhostModeDead,         // secretly, this is in both plyrmode and in our secret set

   kGhostModeLeanLeft,     // for non PlayerGetMode player modes
   kGhostModeLeanRight,
   
   kGhostModeJumpStart,    // now the "internal" ghost state stuff
   kGhostModeJumping,

   kGhostModeRevive,       // implies stand, secretly resets ghost as well
   kGhostModeSleep,

   kGhostModeMax=kGhostModeJumping+1,          // for tests and sanity checking
   kGhostModeNone=kGhostModeMax,
} eGhostMotionModes;

// speeds
typedef enum {
   kGhostSpeedNone,
   kGhostSpeedNorm,
   kGhostSpeedFast
} eGhostMotionSpeed;

typedef enum {
   kGhostRcvDidWeap = (1<<0), // have we set our weapon state bits...
   kGhostRcvCustom  = (1<<1), // are we playing a custom capture
   kGhostRcvNoInt   = (1<<2), // 
   kGhostRcvHasDied = (1<<3), // 
   kGhostRcvStand   = (1<<4), // is our tag based motion a stand
   kGhostRcvLoco    = (1<<5), // is our tag based motion a loco
   kGhostRcvDieNext = (1<<6), // should we die after playing this motion
} eGhostRecvState;

// or'd in above
typedef enum {
   kGhostSendStBase       = (kGhostStMax<<1),
   kGhostSendStSleeping   = (kGhostSendStBase<<0),
} eGhostSendState;

typedef enum {
   kGhostMotSchemaNoCustom = -1,
   kGhostMotSchemaGLoco    = -2,
} eGhostMotSchema;

////////////////////////////
// actual ghost structures

typedef struct {
   float      detail;      // detail "slider" - 0.0 means minimal, 2.0 means tons???? dumb!
   int        flags;       // global config flags
} sGhostConfig;

//////////////////////////////
// basic position/time data structure

typedef struct {
   mxs_vector pos;         // 12 - position
   mxs_vector vel;         // 12 - current or control velocity (check flags)
   union {
      struct {             // interpret as heading, heading velocity, and pitch
         mxs_ang p;
         short   dtz;      // in wack units, since isnt a float
         mxs_ang tz;       // use tz last, so same place as in fac - ideally
      };
      mxs_angvec fac;      // interpret it as a full facing
   } angle_info;           // 6 bytes
   ushort     flags;       // 2  - control flags 
   uchar      mode;        // 1  - motion mode, for relevant cases
   uchar      weap;        // 1  - weapon index
} sGhostHeartbeat;         // 34 bytes

typedef struct {
   sGhostHeartbeat pos;     // 34 - core positional data
   ObjID           rel_obj; // 4  - relative object, if we need one (could be 2bytes)
   int             time;    // 4  - local time at which this packet is "fresh"
} sGhostPos;                // 42 bytes - ack

//////////////////////////////
// in case we need this for bandwidth reduction later

typedef struct {
   mxs_vector pos;         // 12 - position
   float      velx,vely;   // 8  - x,y current or control vel
   mxs_ang    tz;          // 2  - heading (tz)
   ushort     flags;       // 2  - control flags
} sGhostMiniHeartbeat;     // 24 bytes

// hmmm, something like this, maybe?
typedef struct {
   float      posx,posy;   // 8  - position
   float      velx,vely;   // 8  - x,y current or control vel
   mxs_ang    tz,dtz;      // 4  - heading (tz), and dtz for vel
} sGhostAIMiniHB;          // 20 bytes

// for small Obj stuff
typedef struct {
   mxs_vector pos;         // 12 - position
   mxs_vector vel;         // 12 - current or control velocity
} sGhostObjMiniHB;         // 24 bytes

typedef struct {
   union {
      struct {             // interpret as heading, heading velocity, and pitch
         mxs_ang p;
         short   dtz;      // in wack units, since isnt a float
         mxs_ang tz;       // use tz last, so same place as in fac - ideally
      };
      mxs_angvec fac;      // interpret it as a full facing
   } angle_info;           // 6 bytes
} sGhostRotMiniHB;

//////////////////////////////
// total position info for send and recv

typedef struct {
   mxs_ang        s_tz;
   short          s_dtz;
   int            s_time;
} sGhostDtzData;

#define NUM_DTZ_SAMPLES  (8)
#define MASK_DTZ_SAMPLES (NUM_DTZ_SAMPLES-1)
typedef struct {
   sGhostDtzData  samples[NUM_DTZ_SAMPLES];
   int            s_ptr;   // where to put next sample
   int            s_cnt;   // how many valid samples we have
   int            streak;  // streak, + means cons non-zero, - means consec zero
   int            dtz_sum; // dtz_sum for valid samples
} sGhostDtzInfo;

typedef struct {
   sGhostPos      last;    // where we last sent info about it
   sGhostPos      pred;    // predicted position info only
   ushort         seq_id;  // current sequence ID for packets
   ushort         pad;     // pad to dword
   ObjID          rel_obj; // current relative object - for edge trigger detect
   sGhostDtzInfo  dtz_dat; // data about dtz handling
} sGhostSendInfo;

typedef struct {
   sGhostPos      last;    // where we last heard about it
   sGhostPos      pred;    // where our predition model currently takes it
   //   sGhostRating   rating;  // rating for how well we are doing
   ushort         seq_id;  // current sequence ID for packets
   ushort         last_g_seq; // last guaranteed packet
} sGhostRecvInfo;

//////////////////////////////
// the peril of creatures/motion system attach

// this is very "in progress", but hey
typedef struct {
   short schema_idx;  // which schema
   short motion_num;  // which motion within the schema
} sGhostMoCap;

#define MAX_MOCAP_STR_LEN (96)  // who knows, whatever, etc
struct _sGhostCreature {
   IMotionCoordinator *pMotCoord;
   sGhostMoCap         cur_mocap;
   uchar               mmode;
   uchar               speed;
   uchar               r_state;                        // eGhostRcvState, really
   uchar               stand_cnt;                      // number of stands in a row
   char                mocap_name[MAX_MOCAP_STR_LEN];
};  // @TBD: should convert to TagSet, i suppose, for now, im just gonna use a string
typedef struct _sGhostCreature sGhostCreature;  // in case we ever modularize/fdeclare

typedef struct {
   ObjID          weaponObj;
   char           nWeapon;
   char           pad[3];
} sGhostRecvWeap;

//////////////////////////////
// the actual elements in the hash
//  i.e. full info for a ghost of the right type

typedef struct {
   ObjID          obj;     // my obj ID
   char           nWeapon; // looked up index of local weapon number
   char           pad[3];  // pad, woo woo
   sGhostConfig   cfg;     // my setup
   sGhostSendInfo info;    // where am I
   sGhostMoCap    playing; // what mocap am i playing
   int            state;   // state im tracking about events, eGhostState
   int            last_fr; // time of last frame we ran
} sGhostLocal;

typedef struct {
   ObjID          obj;     // my obj ID
   sGhostConfig   cfg;     // my setup
   sGhostRecvInfo info;    // where am I
   sGhostCreature critter;
   sGhostRecvWeap weap;
} sGhostRemote;

#endif  // __GHOSTTYP_H
