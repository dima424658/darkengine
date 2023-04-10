/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostmot.h,v 1.3 2000/01/29 13:41:10 adurant Exp $
//
#pragma once

#ifndef __GHOSTMOT_H
#define __GHOSTMOT_H

#include <physapi.h>
#include <phmod.h>    // for kPMF_Player
#include <wrtype.h>   // for Position type
#include <mclntapi.h> // for MotSysCreateMotionCoordinator()
#include <creatext.h> // for CreatureMakeBallistic()
#include <creatapi.h> // for CreatureGetMotorInterface()

struct {
   IMotionCoordinator *pMotCoord;
   // perhaps some mocap info, eh?
} _sGhostCreature;

#endif  // __GHOSTMOT_H
