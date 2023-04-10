/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simflags.h,v 1.3 2000/01/31 10:00:47 adurant Exp $
#pragma once

#ifndef SIMFLAGS_H
#define SIMFLAGS_H

enum SimFlagsEnum
{
   kSimPhysics    =  0x0001,   // is physics on
   kSimAI         =  0x0002,   // are AIs running
   kSimParticles  =  0x0004,   // create/update particles?  NYI
   kSimTimePasses =  0x0008,   // Does time pass? NYI
   kSimNetPhys    =  0x0010,   // update net physics...
   kSimSound      =  0x0020,   // Sound on
   kSimGameSpec   =  0x0040,   // gamespec running
   kSimRender     =  0x8000,   // render now 
};


#endif // SIMFLAGS_H
