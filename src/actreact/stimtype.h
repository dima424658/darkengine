/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimtype.h,v 1.2 1997/09/29 16:24:22 mahk Exp $

#ifndef __STIMTYPE_H
#define __STIMTYPE_H
#pragma once 

#include <objtype.h>

////////////////////////////////////////////////////////////
// ACT/REACT STIMULUS TYPES
//

//
// Stimulus ID (actually an archetype)
// 
typedef ObjID StimID; 


//
// Other Scalars
//

typedef float tStimLevel;  // intensity level
typedef ulong tStimDuration;   // length of time
typedef ulong tStimTimeStamp;  // time stamp
typedef ulong tStimDatabaseMsg; // Database message

//
// Structures
//
typedef struct sStimDesc sStimDesc;
typedef struct sStimEvent sStimEvent; 
typedef struct sStimSensitivity sStimSensitivity;
typedef struct Position sStimPosition; // source sensor position
typedef struct sObjStimPair sObjStimPair; 




#endif // __STIMTYPE_H
