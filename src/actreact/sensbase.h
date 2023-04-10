/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/sensbase.h,v 1.3 2000/01/29 12:44:41 adurant Exp $
#pragma once

#ifndef __SENSBASE_H
#define __SENSBASE_H

//////////////////////////////////////////////////////////////
// STIMULUS SENSOR BASE TYPES
//

//
// Trigger structure
//

struct sStimTrigger 
{
   tStimLevel min;  // range: min <= x < max 
   tStimLevel max; 
   ulong flags; // eStimTriggerFlags   
};

//
// Trigger flags
//

enum eStimTriggerFlags 
{
   kStimTrigNoMin       = 1 << 0,   // treat my min as -oo
   kStimTrigNoMax       = 1 << 1,   // treat my max as +oo
   kStimTrigInactive    = 1 << 2,   // never fire this trigger
};

//
// Special Object Params
// 

enum eReactObjParam_
{
   kObjParamSensor,  // The event's sensor object  
   kObjParamSource,  // The event's stimulus object

   kNumSpecialObjParams 
};


#endif // __SENSBASE_H
