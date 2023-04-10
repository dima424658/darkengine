/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/pg8rbase.h,v 1.4 1998/08/14 17:48:14 mahk Exp $

#ifndef __PG8RBASE_H
#define __PG8RBASE_H
#pragma once

#include <stimtype.h>
#include <stimbase.h>

#include <ssrctype.h>
#include <senstype.h>
#include <pg8rtype.h>

/////////////////////////////////////////////////////////////
// PROPAGATOR BASE TYPES
//

enum ePropagatorFlags
{
   kPGatorAllStimuli = 1 << 0,  // auto-add all stimuli 
}; 

//
// Propagator descriptor
//

struct sPropagatorDesc 
{
   char name[32];
   ulong flags;
};

//
// Sensor event types
//

enum eStimSensorEventType
{
   kStimSensorCreate, 
   kStimSensorDestroy,
   kStimSensorMove,
};

//
// Sensor events
//

struct sStimSensorEvent
{
   tStimSensorEvent type;  // see above
   StimSensorID id; 
   sObjStimPair elems;
};

//
// Source event types.  Messages are a single one of these, not a bitmask
//

enum eStimSourceEventType
{
   kStimSourceCreate       , 
   kStimSourceDestroy      ,
   kStimSourceMove         ,
   kStimSourceLevelChange  , 
   kStimSourceShapeChange  ,
   kStimSourceLifeChange   ,
};


//
// Source events
//

struct sStimSourceEvent
{
   tStimSourceEvent type;  // see above
   StimSourceID id;
   sObjStimPair elems;
   sStimSourceDesc* desc;  // might be NULL
}; 

#endif // __PG8RBASE_H




