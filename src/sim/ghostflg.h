/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostflg.h,v 1.10 2000/01/29 13:41:07 adurant Exp $
// public flags for ghost
#pragma once

#ifndef __GHOSTFLG_H
#define __GHOSTFLG_H

// overall configuration flags
typedef enum {
   // external flags
   kGhostCfAI         = (0x01),  // im an AI
   kGhostCfIsObj      = (0x02),  // im an object
   kGhostCfPlayer     = (0x03),  // im a player
   kGhostCfNoType     = (0),     // ok, this indicates an error, nothing should be "NoType"
   kGhostCfTypeMask   = (0x03),  // mask for type
   
   kGhostCfNoCret     = (1<<2),  // im not a creature, so don't do mocaps
   kGhostCfNew        = (1<<3),  // im new
   kGhostCfCreate     = (1<<4),  // create me
   kGhostCfForce      = (1<<5),  // force an update
   kGhostCfObjPos     = (1<<6),  // use object pos, not physics pos

   // internal flags
   kGhostCfNoPacket   = (1<<16), // have not yet even seen a packet
   kGhostCfLoaded     = (1<<17), // ive just been loaded from a file
   kGhostCfNoPhys     = (1<<18), // savetime only: has no physics
   kGhostCfDontRun    = (1<<19), // although i have physics, dont run me on rcv
   kGhostCfDisable    = (1<<20), // do nothing, period, ever, just shut up
   kGhostCfFirstTPort = (1<<21), // set until we have processed first packet
} eGhostConfigFlags;

#define _GhostGetType(flags)     ((flags)&kGhostCfTypeMask)
#define _GhostIsType(flags,type) (_GhostGetType(flags)==kGhostCf##type##)

// how to interpret certain heartbeat fields
typedef enum {
   kGhostHBUseG       = (1<<0),  // use guaranteed packets
   kGhostHBFullAngs   = (1<<1),  // use full angles, not the tz,dtz,p format
   kGhostHBObjRel     = (1<<2),  // this has a new RelObj object attached
   kGhostHBAbsVel     = (1<<3),  // this vel is abs, not a target control vel
   kGhostHBHaveMode   = (1<<4),  // i have a valid mode set
   kGhostHBStartSw    = (1<<5),  // start a swing
   kGhostHBGravity    = (1<<6),  // ghost should have gravity
   kGhostHBTeleport   = (1<<7),  // ghost should be teleported
   kGhostHBDead       = (1<<8),  // ghost is dead    - move these two into Mode!
   kGhostHBWeap       = (1<<9),  // this packet has weap data
   kGhostHBOnObj      = (1<<10), // this is relative to an obj, not absolute
   kGhostHBHaveCap    = (1<<11), // this packet has a capture attached
   kGhostHBSlowDown   = (1<<12), // i am slowing down
   kGhostHBAxisRot    = (1<<13), // rotate on y and x as well, for fun
   kGhostHBAngOnly    = (1<<14), // actually filled in on receive...
   kGhostHBNoZPos     = (1<<15), //  as is this, in minipacket setups
} eGhostHeartBeatFlags;

typedef enum {
   kGhostStDying      = (1<<0),
   kGhostStWounded    = (1<<1),
   kGhostStSwinging   = (1<<2),
   kGhostStFiring     = (1<<3),
   kGhostStSleep      = (1<<4),
   kGhostStRevive     = (1<<5),
   kGhostStDead       = (1<<6),  // internal only - for between dying and revive
   kGhostStMax        = kGhostStDead,
   kGhostStLastPerFrame = kGhostStFiring,
   kGhostStIrqChecks  = kGhostStDying|kGhostStWounded|kGhostStSwinging|kGhostStFiring,
} eGhostState;

#endif  // __GHOSTFLG_H
