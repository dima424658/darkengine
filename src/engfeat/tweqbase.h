/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/tweqbase.h,v 1.10 2000/01/31 09:45:42 adurant Exp $
// tweq structures/base data
#pragma once

#ifndef __TWQBASE_H
#define __TWQBASE_H

#include <matrixs.h>

#include <label.h>
#include <tweqflgs.h>
#include <tweqenum.h>

/////////////
// structures

// Base Configuration of the Tweq - includes anim, misc, and time config info
typedef struct {
   char   type;      // what sort of Tweq are we
   char   flg_curve; // Curve Convig (TWEQ_CC_) - what sort of curve does our state we follow
   char   flg_anim;  // Anim Config flags (TWEQ_AC_) how do i update, what rules, wrap/so on
   char   halt_act;  // action to take upon halting
   ushort flg_misc;  // Misc Config flags (TWEQ_MC_) Physics/Controls of state
   ushort rate;      // if we are a time based Tweq, this is our Base Time
} sTweqBaseConfig;

// base state info for a tweq - all tweqs have this
typedef struct {
   ushort  flg_anim;  // Anim State Flags (TWEQ_AS_)
   ushort  flg_misc;  // Misc State Flags (TWEQ_MS_)
} sTweqBaseState;

// while rate is in the BaseConfig (for packing reasons) its state is an extra struct
// this struct is inlined, ie. tweqs which need it just have it, they dont have 2 props
typedef struct {
   ushort cur_time;  // current data on time (in ms)
   short  cur_frame; // "frame" id if there is such a thing
} sTweqTimeState;

// here are the actual per use tweqs, as it were, Config then State for each one

////////
// FULL
// default full state structure (used by delete, emit, several)

typedef struct {
   sTweqBaseConfig cfg;
} sTweqSimpleConfig;

typedef struct {
   sTweqBaseState  state;
   sTweqTimeState  tm_info;
} sTweqSimpleState;

////////
// VECTOR

#define MAX_VEC_AXIS 3
typedef struct {
   sTweqBaseConfig cfg;
   mxs_vector      limits[MAX_VEC_AXIS];
   char            primary_axis;
   char            pad[3];
} sTweqVectorConfig;

typedef struct {
   sTweqBaseState  state;
   sTweqBaseState  axis_flgs[MAX_VEC_AXIS];
} sTweqVectorState;

////////
// JOINT
#define MAX_TWEQ_JOINTS 6

typedef struct {
   sTweqBaseConfig flags;                   // config for this joint
   mxs_vector      limits;                  // rate-low-high
} sJointData;

typedef struct {
   sTweqBaseConfig cfg;                     // overall config
   sJointData      jnt_data[MAX_TWEQ_JOINTS];    // config info for limits and Anim flags for each joint
   char            primary_joint;
   char            pad[3];
} sTweqJointsConfig;

typedef struct {
   sTweqBaseState  state;                   // base run-time state for whole thing
   sTweqBaseState  jnt_flgs[MAX_TWEQ_JOINTS];   // state flags for each joint
} sTweqJointsState;

////////
// LOCK

typedef struct {
   sTweqBaseConfig cfg;                     // overall config
   sJointData      jnt_data;                // joint data for the lock
   char            lock_joint;              // which joint is the lock
   char            pad[3];
} sTweqLockConfig;

typedef struct {
   sTweqBaseState  state;                   // base run-time state for whole thing
   float           targ_ang;
   int             stage;
} sTweqLockState;
   
////////
// MODEL
#define MAX_TWEQ_MODELS 6
typedef struct {
   sTweqBaseConfig cfg;
   Label           names[MAX_TWEQ_MODELS];  // name of each model we use   
} sTweqModelsConfig;

// use default full state here

////////
// DELETE
// use default full config and state here

////////
// EMIT
typedef struct {
   sTweqBaseConfig cfg;
   int             max_frames;    // max frames i run for (limit)
   Label           emittee;       // name of archetype object we emit
   mxs_vector      vel;           // initial velocity
   mxs_vector      rand;          // random ranges (-theta to theta) for each of three axes
} sTweqEmitterConfig;

// use default full state here

////////
// FLICKER
// use default full config and state here

// ?????????
//   TweqConcentrator?
//   TweqScript?

////////////////////
// connection to A/R

typedef struct {
   enum eTweqType type;
   enum eTweqDo   action;
   int            xtra_bits;
   int            xtra_parm;      // for now just used for joint_id+1 in the joint case
} sTweqControl;

#endif  //__TWQBASE_H
