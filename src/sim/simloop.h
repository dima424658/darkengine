/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simloop.h,v 1.4 2000/01/31 10:00:48 adurant Exp $
#pragma once
#ifndef __SIMLOOP_H
#define __SIMLOOP_H

//
// SIM LOOP CLIENT
// This client sits on the border between simulation and rendering 
// in the loop. (It's assumed that we simulate first and then we render.) 
// All "simulation" stuff should constrain before this, and all "rendering"
// stuff should constrain after
//



// Loop client GUID
DEFINE_LG_GUID(LOOPID_SimFinish, 0x3f);

//
// @INSTRUCTIONS: Rename this loop factory variable to something appropriate to your system.
//
// Loop Client Factory Descriptor
EXTERN struct sLoopClientDesc SimLoopClientDesc;

//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

 
#endif // __SIMLOOP_H
