/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimbase.h,v 1.7 1998/10/02 01:10:09 mahk Exp $

#ifndef __STIMBASE_H
#define __STIMBASE_H
#pragma once

#include <senstype.h>
#include <ssrctype.h>
#include <chainevt.h>

////////////////////////////////////////////////////////////
// Non-forward-declared stimulus base types

//
// Stimulus descriptor
//

struct sStimDesc
{
   char name[16];
   ulong flags;
};

//
// Obj/Stim relationship pair
//

struct sObjStimPair
{
   ObjID obj;
   StimID stim;
};

//
// Stimulus Event, a chained event of kind kEventKindStim
//


struct sStimEventData 
{
   StimID kind;               // kind of stimulus
   tStimLevel intensity;      // intensity
   tStimLevel last;           // last intensity
   StimSensorID sensor;       // sensor receiving event
   StimSourceID source;       // source sending event
   tStimTimeStamp time;       // time at which event occured
   ulong flags;               // Flags, see below
};

struct sStimEvent : public sChainedEvent, public sStimEventData 
{
   sStimEvent(sStimEventData* data = NULL, sChainedEvent* last = NULL);
}; 

enum eStimEventFlags
{
   kStimEventNoDefer = 1 << 0,  // Don't queue, send it right now 
   kStimDestroySrcObj = 1 << 1, // Destroy the source object after delivering this event. 
};


//
// Stimulus Sensitivity
//

enum eStimSensitivityFlags_
{
   kStimSensNoMin = 1 << 0,
   kStimSensNoMax = 1 << 1,
};

struct sStimSensitivity
{
   tStimLevel min;  // range: min <= x < max 
   tStimLevel max; 
   ulong flags; // eStimSensitivityFlags
};


#endif // __STIMBASE_H

