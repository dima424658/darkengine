/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/pickbase.h,v 1.3 1999/12/03 14:58:30 adurant Exp $
// lockpick service, properties, the whole thing...

#pragma once
#ifndef __PICKBASE_H
#define __PICKBASE_H

#include <objtype.h>

enum ePickResult { kPickNoMatch, kPickMatch, kPickComplete, kPickNoLock, kPickNoData, kPickAlreadyUnlocked };

typedef struct {
   char  cur_tumbler; // which of the 3 lockbit sets are we on
   char  pin;         // which pin are we on
   bool  done;        // is the lock opened, now
   char  tm_rand;     // pad...
   int   tm_total;    // total time spent
   int   tm_stage;    // time spent so far this stage
   ObjID picker;      // who is picking the lock
} sPickState;

#define NUM_TUMBLERS 3

typedef struct {
   int   data_bits;   // actual bitfield for this tumbler
   short pins;        // number of pins in this tumblers
   short time_pct;    // delta off 100% - so 0 means default 100%
   char  flags;
   char  pad[3];
} sPickStage;

typedef struct {
   sPickStage tumbler[NUM_TUMBLERS];
} sPickCfg;

#define PICKSTATESTRINGLENGTH 24

typedef struct {
   int  firstpickstate;  // dest state if using pick 1, -1 means nowhere
   int  secondpickstate; // dest state if using pick 2, -1 means nowhere   
   int  thirdpickstate;  // dest state if using pick 3(nonexistent currently)
 } sAdvFsmTransState;

typedef struct {
   int  time;            // number of millisecs to do state transition
 } sAdvFsmInfoState;
     
typedef struct{
   char firstpicksound[PICKSTATESTRINGLENGTH];
   char secondpicksound[PICKSTATESTRINGLENGTH];
   char thirdpicksound[PICKSTATESTRINGLENGTH];
} sAdvFsmSoundState;

//sounds are tags for schemas.

#define NUMPICKFSMSTATES 9

typedef struct {
  sAdvFsmTransState picktransfsm[NUMPICKFSMSTATES];
} sAdvPickTransCfg;


typedef struct {
  sAdvFsmSoundState picksoundfsm[NUMPICKFSMSTATES];
} sAdvPickSoundCfg;

typedef struct {
  BOOL enable;
  int firstpickbits;
  int secondpickbits;
  int thirdpickbits;
  sAdvFsmInfoState pickstatefsm[NUMPICKFSMSTATES];
} sAdvPickStateCfg;

#define PICK_FLG_RESET  (1<<0)
#define PICK_FLG_RANDOM (1<<1)

#endif  // __PICKBASE_H
