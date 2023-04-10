/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/propag8r.h,v 1.7 2000/01/29 12:44:31 adurant Exp $
#pragma once

#ifndef __PROPAG8R_H
#define __PROPAG8R_H

#include <comtools.h>
#include <stimtype.h>
#include <ssrctype.h>
#include <objtype.h>
#include <pg8rtype.h>

struct sStructDesc;

////////////////////////////////////////////////////////////
// PROPAGATOR COM INTERFACE
//
// A propagator is a simulation that generates stimulus events for a single
// stimulus.
//

F_DECLARE_INTERFACE(IPropagator);

#undef INTERFACE
#define INTERFACE IPropagator
DECLARE_INTERFACE_(IPropagator,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //------------------------------------------------------------
   // ADMINISTRATIVE INFO
   //

   //
   // Describe myself
   //
   STDMETHOD_(const sPropagatorDesc*,Describe)(THIS) PURE;

   //
   // Return my ID
   //
   STDMETHOD_(PropagatorID,GetID)(THIS) PURE;

   //
   // Add a stimulus to my set of supported stimuli
   //
   STDMETHOD(AddStimulus)(THIS_ StimID stim) PURE;

   //
   // Remove a stimulus, removing all sources and stimuli for that stimulus
   //
   STDMETHOD(RemoveStimulus)(THIS_ StimID stim) PURE;

   //
   // Test to see whether a stimulus has been added
   //
   STDMETHOD_(BOOL,SupportsStimulus)(THIS_ StimID stim) PURE;

   //------------------------------------------------------------
   // SOURCES
   //

   //
   // Set up a "default" source, setting which fields are valid
   // and filling them with default values
   //
   STDMETHOD(InitSource)(THIS_ sStimSourceDesc* desc) PURE;


   //
   // Return a structure descriptor for describing/editing sources
   //
   STDMETHOD_(const struct sStructDesc*,DescribeShapes)(THIS) PURE;
   STDMETHOD_(const struct sStructDesc*,DescribeLifeCycles)(THIS) PURE;

   //
   // Source Events
   //
   STDMETHOD(SourceEvent)(THIS_ sStimSourceEvent* event) PURE;

   //
   // Get my opinion of a particular source's intensity right now
   //
   STDMETHOD_(tStimLevel,GetSourceLevel)(THIS_ StimSourceID id) PURE;

   //
   // Fill out a full description of a source
   //
   STDMETHOD(DescribeSource)(THIS_ StimSourceID id, sStimSourceDesc* desc) PURE;

   //
   // Sensor Events
   //
   STDMETHOD(SensorEvent)(THIS_ sStimSensorEvent* event) PURE;

   //------------------------------------------------------------
   // PROPAGATION & EVENT GENERATION
   //

   //
   // Establish the connection to the "stimulator" through which events
   // will be delivered.  Currently, must conform to the IStimulator interface
   //
   STDMETHOD(Connect)(THIS_ IUnknown* stimulator) PURE;

   //
   // Run propagation for a specified amount of time
   //
   STDMETHOD(Propagate)(THIS_ tStimTimeStamp curtime, tStimDuration duration) PURE;

   //
   // Delete all sources and sensors, remove all stimuli
   //
   STDMETHOD(Reset)(THIS) PURE;

   //
   // Start/Stop Propagation, all calls to Propagate must be after a Start()
   //
   STDMETHOD(Start)(THIS) PURE;
   STDMETHOD(Stop)(THIS) PURE; 

};


#undef INTERFACE

// Propagator factories
EXTERN IPropagator* CreateContactPropagator(void);
EXTERN IPropagator* CreateRadiusPropagator(void);


#endif // __PROPAG8R_H
