/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/periog8r.h,v 1.1 1998/08/21 00:00:53 mahk Exp $
#pragma once  
#ifndef __PERIOG8R_H
#define __PERIOG8R_H
#include <propag8r.h>

////////////////////////////////////////////////////////////
// "PERIODIC" PROPAGATOR 
//
// Sources in this propagator transmit their stimuli at regular intervals. 
// The propagator keeps track of "contacts" between sources and
// sensors.  An external simulation tells the propagator when these
// contacts begin and end.  
//

F_DECLARE_INTERFACE(IPeriodicPropagator); 

#undef INTERFACE
#define INTERFACE IPeriodicPropagator
DECLARE_INTERFACE_(IPeriodicPropagator,IPropagator)
{

   DECLARE_UNKNOWN_PURE();

   //
   // IPropagator Methods
   //
   STDMETHOD_(const sPropagatorDesc*,Describe)(THIS) PURE;
   STDMETHOD_(PropagatorID,GetID)(THIS) PURE;
   STDMETHOD(AddStimulus)(THIS_ StimID stim) PURE;
   STDMETHOD(RemoveStimulus)(THIS_ StimID stim) PURE;
   STDMETHOD_(BOOL,SupportsStimulus)(THIS_ StimID stim) PURE;
   STDMETHOD(InitSource)(THIS_ sStimSourceDesc* desc) PURE;
   STDMETHOD_(const struct sStructDesc*,DescribeShapes)(THIS) PURE;
   STDMETHOD_(const struct sStructDesc*,DescribeLifeCycles)(THIS) PURE;
   STDMETHOD(SourceEvent)(THIS_ sStimSourceEvent* event) PURE;
   STDMETHOD_(tStimLevel,GetSourceLevel)(THIS_ StimSourceID id) PURE;
   STDMETHOD(DescribeSource)(THIS_ StimSourceID id, sStimSourceDesc* desc) PURE;
   STDMETHOD(SensorEvent)(THIS_ sStimSensorEvent* event) PURE;
   STDMETHOD(Connect)(THIS_ IUnknown* stimulator) PURE;
   STDMETHOD(Propagate)(THIS_ tStimTimeStamp curtime, tStimDuration duration) PURE;
   STDMETHOD(Reset)(THIS) PURE;
   STDMETHOD(Start)(THIS) PURE;
   STDMETHOD(Stop)(THIS) PURE; 

   // ------------------------------------------------------------
   // NEW METHODS
   //

   //
   // Notify the propagator that a contact has begun.
   // 
   STDMETHOD(BeginContact)(ObjID src, ObjID sensor) PURE;  
   
   //
   // Notify the propagator that a contact has ended.
   // Accepts wildcards.
   // 
   STDMETHOD(EndContact)(ObjID src, ObjID sensor) PURE; 

   //
   // Like BeginContact, but ends any other contacts this sensor
   // has.  Does nothing if the specified contact already exists.
   // 
   STDMETHOD(SetSingleSensorContact)(ObjID src,ObjID sensor) PURE; 

}; 
#undef INTERFACE

//
// Factory: takes the propagator name and the name
// of the link relation to use for "contacts." 
// the propagator will create the relation itself. 
//

EXTERN IPeriodicPropagator* CreatePeriodicPropagator(const sPropagatorDesc* desc, const char* relname); 


#endif // __PERIOG8R_H

