/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/physloop.h,v 1.5 2000/01/31 09:52:15 adurant Exp $
#pragma once
#ifndef __PHYSLOOP_H
#define __PHYSLOOP_H


// Loop client GUID
DEFINE_LG_GUID(LOOPID_Physics, 0x3b);
DEFINE_LG_GUID(LOOPID_PhysicsBase, 0x1ce);

//
// Loop Client Factory Descriptor
EXTERN struct sLoopClientDesc PhysicsLoopClientDesc;
EXTERN struct sLoopClientDesc PhysicsBaseClientDesc;

//
// KEEP THIS FILE RELATIVELY CLEAR OF JUNK!  DO NOT PUT YOUR SYSTEM'S API HERE! 
// DO NOT INCLUDE OTHER HEADER FILES FROM THIS FILE!
//

 
#endif // __PHYSLOOP_H
