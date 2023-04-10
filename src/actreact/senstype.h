/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/senstype.h,v 1.4 2000/01/29 12:44:42 adurant Exp $
#pragma once

#ifndef __SENSTYPE_H
#define __SENSTYPE_H
#include <comtools.h>

////////////////////////////////////////////////////////////
// FORWARD-DECLARED BASE TYPES FOR STIMULUS SENSORS
//

//
// Scalar types
//

typedef ulong ReceptronID;  // Unique receptron ID
typedef ulong StimSensorID;  // unique sensor id (ObjID?)
typedef ulong eReactObjParam;

#define SENSORID_NULL 0
#define RECEPTRON_NULL 0


//
// Structures
//

typedef struct sReceptron sReceptron;
typedef struct sStimTrigger sStimTrigger;

//
// Interfaces
//

F_DECLARE_INTERFACE(IReceptronQuery);
F_DECLARE_INTERFACE(IStimSensors);
F_DECLARE_INTERFACE(IStimSensorQuery);

#endif // __SENSTYPE_H
