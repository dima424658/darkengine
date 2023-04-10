/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimul8r.h,v 1.5 2000/01/29 12:44:50 adurant Exp $
#pragma once

#ifndef __STIMUL8R_H
#define __STIMUL8R_H

#include <comtools.h>
#include <stimtype.h>
#include <senstype.h>
#include <reactype.h>

////////////////////////////////////////////////////////////
// OBJECT STIMULATION API
//
// This interface is presented by the Act/React Object System layer for use by the 
// Act/React Propagation system.  It is the path by which stimulus sensors 
// are notified of stimulus events that hit them. 
//

typedef void* tStimFilterData; 
typedef eReactionResult (* tStimFilterFunc)(sStimEvent* event, tStimFilterData data); 
typedef ulong tStimFilterID; 

F_DECLARE_INTERFACE(IStimulator);

#undef INTERFACE
#define INTERFACE IStimulator
DECLARE_INTERFACE_(IStimulator,IUnknown)
{
   DECLARE_UNKNOWN_PURE();
   
   //
   // Send a stimulus event to a sensor
   //
   STDMETHOD(StimulateSensor)(THIS_ StimSensorID sensor, const sStimEvent* event) PURE;

   //
   // Describe the sensitive intensity range of an object
   // returns S_FALSE if the object is not sensitive to that stimulus
   //
   STDMETHOD(GetSensitivity)(THIS_ StimSensorID id, sStimSensitivity* sensitivity) PURE;

   //
   // Look up the position of a sensor
   //
   STDMETHOD_(const sStimPosition*, GetSensorPosition)(THIS_ StimSensorID id) PURE;

   //
   // Update Frame
   //
   STDMETHOD(UpdateFrame)(THIS_ tStimTimeStamp time, tStimDuration dt) PURE;

   //
   // Add a "filter" callback for stimulus events.  The filter is called before 
   // the event is applied to the sensor.  The filter may mutate the event.   
   //
   STDMETHOD_(tStimFilterID,AddFilter)(THIS_ tStimFilterFunc func, tStimFilterData data) PURE; 
   STDMETHOD(RemoveFilter)(THIS_ tStimFilterID id) PURE; 

};

#undef INTERFACE

EXTERN void StimulatorCreate(void);

#endif // __STIMUL8R_H

