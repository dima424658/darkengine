/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

// $Header: r:/t2repos/thief2/src/engfeat/tweqflgs.h,v 1.17 2000/01/31 09:45:46 adurant Exp $
// internal definitions

#ifndef __TWQFLGS_H
#define __TWQFLGS_H

///////////////
// misc utility flags

// meaning of fields in the mxs_vector
#define TWEQ_LIMIT_RATE (0)
#define TWEQ_LIMIT_LOW  (1)
#define TWEQ_LIMIT_HIGH (2)

///////////////
// TWEQ FLAGS
   
// Flags are combo of either A or M or C, S or C
// A is Anim, M is Misc, C is Curve - S is state, C is configuration
// So a TWEQ_AC_ is a flag regarding Anim Configuration

// ANIMATION FLAGS (config then state)
// Configuration regarding animation behavior
#define TWEQ_AC_NOLIMIT  (1<<0)  // ignore the limits (so i dont have to set them)
#define TWEQ_AC_SIM      (1<<1)  // update me continually - else just update when i was on screen
#define TWEQ_AC_WRAP     (1<<2)  // Wrap from low to high, else bounce
#define TWEQ_AC_1BOUNCE  (1<<3)  // Bounce off the top, then down and stop
#define TWEQ_AC_SIMRADSM (1<<4)  // update if within a small radius
#define TWEQ_AC_SIMRADLG (1<<5)  // update if within a large radius
#define TWEQ_AC_OFFSCRN  (1<<6)  // only run if im offscreen

// State regarding animation behavior
#define TWEQ_AS_ONOFF    (1<<0)  // am i on or off?
#define TWEQ_AS_REVERSE  (1<<1)  // model anim direction - set/cleared internally - really
#define TWEQ_AS_RESYNCH  (1<<2)  // resynch me next time im seen (ie. ive been restarted)
#define TWEQ_AS_GOEDGE   (1<<3)  // if resynch-ing, go to an edge, dont continue mid range
#define TWEQ_AS_LAPONE   (1<<4)  // is this the "first lap"? ie. havent hit any frames/edges yet

// Halt action fields - what to do on stop
// not a real enum, since 
#define TWEQ_HALT_KILL   (0)  // kill when i reach limits/size 0, (object + script)
#define TWEQ_HALT_REM    (1)  // remove tweq when i reach limits/size 0, leave object\n
#define TWEQ_HALT_STOP   (2)  // stop when i reach limits/size 0, but leave tweq attached
#define TWEQ_STATUS_QUO  (3)  // just keep going, continuing, for ever.... - never complete
#define TWEQ_HALT_SLAY   (4)  // slay when i reach limits/size 0, (object + script)
#define TWEQ_FRAME_EVENT (5)  // we have reached a frame boundary, and are continuing
// note: those must order same as the real enum in tweqenum
// but it has to be put in a real char field, so it cant be that enum

// CURVE FLAGS
// Configuration regarding curve behavior
#define TWEQ_CC_JITTER   (3<<0)  // jitter in proportion to rate while animating - can be 1-3 for "scale"
#define TWEQ_CC_MUL      (1<<2)  // do you want to multiply by rate - else adds to rate

// MISC FLAGS
// Configuration parameters to other systems
#define TWEQ_MC_ANCHOR   (1<<0)  // anchor bottom of model, not center - in modelanim
#define TWEQ_MC_SCRIPTS  (1<<1)  // do you want messages to scripts on interesting tweq events?
#define TWEQ_MC_RANDOM   (1<<2)  // random - used for tweqModel and tweqEmit vels (scales v0)
#define TWEQ_MC_GRAV     (1<<3)  // for emitted, is just passed on to launchProjectile
#define TWEQ_MC_ZEROVEL  (1<<4)  // for emitted, is just passed on to launchProjectile
#define TWEQ_MC_TELLAI   (1<<5)  // for emitted, is just passed on to launchProjectile
#define TWEQ_MC_PUSHOUT  (1<<6)  // for emitted, is just passed on to launchProjectile
#define TWEQ_MC_NEGLOGIC (1<<7)  // negative logic, script meaning of forward flipped
#define TWEQ_MC_RELVEL   (1<<8)  // emit velocity is relative to object orientation
#define TWEQ_MC_NOPHYS   (1<<9)  // for emitted, is just passed on to launchProjectile
#define TWEQ_MC_VHOT     (1<<10) // Anchor at vhot 1, rather than at center
#define TWEQ_MC_HOSTONLY (1<<11) // run only on object host, not proxies


// State which is transient
// none currently
#endif  // __TWQFLGS_H
