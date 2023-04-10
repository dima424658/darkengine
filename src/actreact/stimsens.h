/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimsens.h,v 1.6 2000/01/29 12:44:48 adurant Exp $
#pragma once

#ifndef __STIMSENS_H
#define __STIMSENS_H

#include <comtools.h>
#include <stimtype.h>
#include <senstype.h>
#include <objtype.h>
#include <osystype.h>

////////////////////////////////////////////////////////////
// STIMULUS SENSORS COM INTERFACE
//

#undef INTERFACE
#define INTERFACE IStimSensors

DECLARE_INTERFACE_(IStimSensors,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Add a receptron to an object (copies the data)
   // If the object does not have a "sensor" for the receptron's stimulus, creates
   // one.  
   //
   STDMETHOD_(ReceptronID,AddReceptron)(THIS_ ObjID obj, StimID stimulus, const sReceptron* tron) PURE;
   // Remove a receptron, removes the sensor if it was the last one for that 
   // stimulus
   STDMETHOD(RemoveReceptron)(THIS_ ReceptronID) PURE;
   
   // 
   // Look up a receptron's obj/stim pair by ID
   // 
   STDMETHOD_(sObjStimPair, GetReceptronElems)(THIS_ ReceptronID id) PURE; 
   //
   // Get a receptron's data structure
   //
   STDMETHOD(GetReceptron)(THIS_ ReceptronID id, sReceptron* tron) PURE;
   //
   // Mutate a receptron 
   //
   STDMETHOD(SetReceptron)(THIS_ ReceptronID id, const sReceptron* tron) PURE; 

   //
   // Query the receptron database.  does not account for inheritance on the
   // part of obj.
   //
   STDMETHOD_(IReceptronQuery*,QueryReceptrons)(THIS_ ObjID obj, StimID stim) PURE;

   //
   // Query receptrons, including those inherited by obj
   //
   STDMETHOD_(IReceptronQuery*,QueryInheritedReceptrons)(THIS_ ObjID obj, StimID stim) PURE;

   //
   // Return the objects that have special meaning when used as receptron
   // object parameters.
   //
   STDMETHOD_(ObjID,ObjParam)(THIS_ eReactObjParam which) PURE;

   //
   // Query sensors, OBJ_NULL is a wildcard.
   // 
   STDMETHOD_(IStimSensorQuery*,QuerySensors)(THIS_ ObjID obj, StimID stim) PURE;

   //
   // Look up the ID of a sensor on an object for a stimulus
   //
   STDMETHOD_(StimSensorID, LookupSensor)(THIS_ ObjID obj, StimID stim) PURE;

   //
   // Get the the object and stimulus associated with a sensor
   // 
   STDMETHOD_(sObjStimPair,GetSensorElems)(THIS_ StimSensorID ID) PURE;

   //
   // Notify the stimsensors about database changes
   // 
   STDMETHOD(DatabaseNotify)(THIS_ tStimDatabaseMsg msg, IUnknown* file) PURE;

   //
   // Notify about object creation/deletion
   //
   STDMETHOD(ObjectNotify)(THIS_ eObjNotifyMsg msg, ObjNotifyData data) PURE;
   

};

//------------------------------------------------------------
// Aggregate member factory
//

EXTERN void StimSensorsCreate(void);


//------------------------------------------------------------
// Receptron query API
//

#undef INTERFACE
#define INTERFACE IReceptronQuery

DECLARE_INTERFACE_(IReceptronQuery,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Iteration
   //
   STDMETHOD(Start)(THIS) PURE;
   STDMETHOD_(BOOL,Done)(THIS) PURE;
   STDMETHOD(Next)(THIS) PURE;
   
   //
   // Accessors
   //
   STDMETHOD_(sObjStimPair,Elems)(THIS) PURE;
   STDMETHOD_(ReceptronID,ID)(THIS) PURE;
   // Ptr is only valid until Next() or final Release()
   STDMETHOD_(const sReceptron*, Receptron)(THIS) PURE;
   
}; 


//------------------------------------------------------------
// Sensor Query API
//

#undef INTERFACE
#define INTERFACE IStimSensorQuery

DECLARE_INTERFACE_(IStimSensorQuery,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Iteration
   //
   STDMETHOD(Start)(THIS) PURE;
   STDMETHOD_(BOOL,Done)(THIS) PURE;
   STDMETHOD(Next)(THIS) PURE;
   
   //
   // Accessors
   //
   STDMETHOD_(sObjStimPair,Elems)(THIS) PURE;
   STDMETHOD_(StimSensorID,ID)(THIS) PURE;
   
}; 


#endif // __STIMSENS_H





