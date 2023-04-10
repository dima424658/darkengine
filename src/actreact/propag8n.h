/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/propag8n.h,v 1.6 2000/01/29 12:44:29 adurant Exp $
#pragma once

#ifndef __PROPAG8N_H
#define __PROPAG8N_H

#include <comtools.h>
#include <stimtype.h>
#include <pg8rtype.h>
#include <senstype.h>
#include <ssrctype.h>

F_DECLARE_INTERFACE(IPropagation);
F_DECLARE_INTERFACE(IPropagator);
F_DECLARE_INTERFACE(IPropagatorQuery);

////////////////////////////////////////////////////////////
// PROPAGATION SYSTEM COM INTERFACE
// 
// The propagation system mainly manages the set of IPropagators, 
// making sure the correct events get sent to the correct propagators.
//

#undef INTERFACE
#define INTERFACE IPropagation 
DECLARE_INTERFACE_(IPropagation,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //------------------------------------------------------------
   // PROPAGATORS
   //

   //
   // Add a Propagator, done by the propagator at creation time to get
   // it's id. 
   //
   STDMETHOD_(PropagatorID,AddPropagator)(THIS_ IPropagator* gator) PURE;

   //
   // Register a propagator as supporting a specific stimulus, done by the 
   // propagator 
   //
   STDMETHOD(AddPropagatorStimulus)(THIS_ PropagatorID id, StimID stim) PURE;
   STDMETHOD(RemovePropagatorStimulus)(THIS_ PropagatorID id, StimID stim) PURE;


   //
   // Test to see whether a propagator supports a stimulus
   //
   STDMETHOD_(BOOL,SupportsStimulus)(THIS_ PropagatorID id, StimID stim) PURE; 

   //
   // Look up a propagator by ID or name 
   //
   STDMETHOD_(IPropagator*,GetPropagator)(THIS_ PropagatorID id) PURE;
   STDMETHOD_(IPropagator*,GetPropagatorNamed)(THIS_ const char* name) PURE;

   //
   // Look up all propagators for a given stimulus,
   // OBJ_NULL means all propagators
   //
   STDMETHOD_(IPropagatorQuery*,GetPropagators)(THIS_ StimID ID) PURE;


   //------------------------------------------------------------
   // SENSOR EVENTS
   //

   STDMETHOD(SensorEvent)(THIS_ sStimSensorEvent* event) PURE; 
   STDMETHOD(SimpleSensorEvent)(THIS_ StimSensorID sensor, tStimSensorEvent kind) PURE; 

   //------------------------------------------------------------
   // SOURCE EVENTS
   //

   STDMETHOD(SourceEvent)(THIS_ sStimSourceEvent* event) PURE; 
   STDMETHOD(SimpleSourceEvent)(THIS_ StimSourceID source, tStimSourceEvent kind) PURE; 


   //------------------------------------------------------------
   // PROPAGATION, EVENT GENERATION & CONTROL FLOW
   //

   //
   // Run all propagators for a length of time 
   //
   STDMETHOD(Propagate)(THIS_ tStimTimeStamp curtime, tStimDuration duration) PURE;

   //
   // Reset all propagators
   //
   STDMETHOD(Reset)(THIS) PURE;

   //------------------------------------------------------------
   // STIMULUS EVENTS
   // 

   STDMETHOD(AddStimulus)(THIS_ StimID stim) PURE; 

};

//------------------------------------------------------------
// Propagator Query
//

#undef INTERFACE
#define INTERFACE IPropagatorQuery
DECLARE_INTERFACE_(IPropagatorQuery,IUnknown)
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
   STDMETHOD_(PropagatorID,ID)(THIS) PURE;
   STDMETHOD_(IPropagator*,Propagator) (THIS) PURE;

};


#undef INTERFACE

//
// Create the IPropagation
//


EXTERN void PropagationCreate(void);



#endif // __PROPAG8N_H




