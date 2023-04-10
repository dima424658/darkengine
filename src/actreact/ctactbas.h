/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/ctactbas.h,v 1.5 1998/10/21 20:14:17 mahk Exp $
#pragma once  
#ifndef __CTACTBAS_H
#define __CTACTBAS_H

#include <ssrcbase.h>

//////////////////////////////////////////////////////////////
// Contact Propagator Source Descritor
//


//
// Shape flags
//
enum eContactPropagatorFlags 
{
   kPropagateCollide    = 1 << 0,
   kPropagateInWorld    = 1 << 1,
   kPropagateInInv      = 1 << 2,
   kPropagateInv2World   = 1 << 3,
   kPropagateInv2Inv     = 1 << 4,
   kPropagateWorld2World = 1 << 5,
   kPropagateWorld2Inv   = 1 << 6,
   kPropagateSwingLow    = 1 << 7, 
   kPropagateSwingMed    = 1 << 8, 
   kPropagateSwingHigh   = 1 << 9, 
   kPropagateSwingBlock  = 1 << 10,

   kNumContactBits = 11
}; 

enum eContactCoeffIdx 
{
   kVelocityCoeff,
   kFrobTimeCoeff, 

   kNumContactCoeffs
}; 

//
// Shape, cast an sStimShape to one of these 
//

struct sContactSourceShape 
{
   ulong flags; // see above
   float coeffs[kNumContactCoeffs];
   uchar pad[sizeof(sStimShape)-sizeof(ulong)-sizeof(float)*kNumContactCoeffs]; 
}; 


#endif // __CTACTBAS_H
