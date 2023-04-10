/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/samploop.h,v 1.4 2000/01/29 12:41:44 adurant Exp $
#pragma once
#ifndef __SAMPLOOP_H
#define __SAMPLOOP_H

//
// @INSTRUCTIONS: Change __SAMPLOOP_H to the new name of this file.
//

//
// @INSTRUCTION: I recommend deleting instructions after you follow them.
//

//
// @INSTRUCTIONS: 1) Check out camguids.c and grab a GUID (instructions are in that file)
//                2) Name the GUID and put it's definition below.
//                3) Change MY_GUID in samploop.c to point to your new GUID.
//

// Loop client GUID
DEFINE_LG_GUID(LOOPID_Sample, 0x2a);

//
// @INSTRUCTIONS: Rename this loop factory variable to something appropriate to your system.
//
// Loop Client Factory Descriptor
EXTERN struct sLoopClientDesc SampleLoopClientDesc;

//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

 
#endif // __SAMPLOOP_H
