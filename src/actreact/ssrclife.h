/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/ssrclife.h,v 1.2 1998/10/02 01:19:06 mahk Exp $
#pragma once  
#ifndef __SSRCLIFE_H
#define __SSRCLIFE_H

#include <ssrctype.h>
#include <stimtype.h>

////////////////////////////////////////////////////////////
// STANDARD LIFE CYCLE IMPLEMENTATIONS FOR STIM SOURCE
//

//------------------------------------------------------------
// Periodic
// This kind of stimulus source generates impulses at some frequency. 
// It can fire a finite or infinite number of times, and can grow
// or shrink over time. 
//
struct sPeriodicLifeCycleFiring; 

struct sPeriodicLifeCycle
{
   // Members
   ulong flags;            // flags, see below 
   tStimDuration period;   // msec between firings
   int max_firings;        // How many times am I allowed to fire 
   tStimLevel slope;       // change in intensity, per firing 

   // Flags
   enum eFlags
   {
      kNoMaxFirings  = 1 << 0,  // fire infinitely
      kDestroy       = 1 << 1,  // destroy my object when done

      kNumFlags = 1, // number of flags 
   }; 
   
   // Globals
   static const struct sStructDesc* gpDesc;  // My sdesc


   // Firing Iterator 
   typedef struct sPeriodicLifeCycleFire sFire; 

   //
   // Begin firing the source. 
   // Specify when we started firing, when we last fired, and what time it is now.
   // Initial means "this is the first time we've done this for this source" 
   //

   sFire* BeginFiring(BOOL initial, tStimLevel level, tStimTimeStamp start, tStimTimeStamp last, tStimTimeStamp now);

   // get the next level, time to fire at, and stim event flags
   // return TRUE iff it's in the past
   BOOL FireNext(sFire* fire, tStimLevel* nextlevel, tStimTimeStamp* nexttime, ulong* flags);
 
   // clean up after firing, return true if there is more firing to be done
   // in the future 
   BOOL EndFiring(sFire* );  
}; 



#endif // __SSRCLIFE_H

