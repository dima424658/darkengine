/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motmngr.h,v 1.3 2000/01/29 13:22:16 adurant Exp $
#pragma once

#ifndef __MOTMNGR_H
#define __MOTMNGR_H

/////////
//
// This is the motion manager module.  
// It's just a little game-independent module
// responsible for instantiating the motion set, providing commands for
// rebuilding the motion database etc.
//
// Basically, all non-object-specific, non-maneuver-specific,
// non-game-specific motion stuff goes in here.
//

EXTERN void MotionManagerInit();
EXTERN void MotionManagerClose();
EXTERN void MotionManagerReset();
EXTERN void MotionManagerLoad();

#ifdef __cplusplus

#include <motset.h>
#include <dynarray.h>
#include <motdb.h>

EXTERN IMotionSet *g_pMotionSet;

EXTERN cMotionDatabase *g_pMotionDatabase;

// include maneuver factory set in here, too, since why not.
// doesn't seem worthy of its own module
//
typedef class cManeuverFactory cManeuverFactory;
typedef cDynArray<cManeuverFactory *> cManeuverFactoryList;

EXTERN cManeuverFactoryList g_ManeuverFactoryList;


#endif // CPLUSPLUS


#endif // MOTMNGR
