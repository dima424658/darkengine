/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/pg8rnull.h,v 1.4 1998/08/17 19:49:07 mahk Exp $
#pragma once  
#ifndef __PG8RNULL_H
#define __PG8RNULL_H

#include <comtools.h>

#include <stimtype.h>
#include <stimbase.h>

#include <pg8rtype.h>
#include <pg8rbase.h>
#include <propag8r.h>

////////////////////////////////////////////////////////////
// NULL PROPAGATOR
//
// Supports no stimuli, has no sources or sensors, etc. 
//

class cNullPropagator : public IPropagator
{
   DECLARE_UNAGGREGATABLE();
public:
   STDMETHOD_(const sPropagatorDesc*,Describe)();

   STDMETHOD_(PropagatorID,GetID)();
   STDMETHOD(AddStimulus)(StimID stim);
   STDMETHOD(RemoveStimulus)(StimID stim);
   STDMETHOD_(BOOL,SupportsStimulus)(StimID stim);
   STDMETHOD(InitSource)(sStimSourceDesc* desc);
   STDMETHOD_(const struct sStructDesc*,DescribeShapes)();
   STDMETHOD_(const struct sStructDesc*,DescribeLifeCycles)();
   STDMETHOD(SourceEvent)(sStimSourceEvent* event); 
   STDMETHOD(DescribeSource)(StimSourceID id, sStimSourceDesc* desc);
   STDMETHOD_(tStimLevel,GetSourceLevel)(StimSourceID id);
   STDMETHOD(SensorEvent)(sStimSensorEvent* event); 
   STDMETHOD(Connect)(IUnknown* stimulator);
   STDMETHOD(Propagate)(tStimTimeStamp curtime, tStimDuration duration);
   STDMETHOD(Reset)();
   STDMETHOD(Start)() { return S_OK; }; 
   STDMETHOD(Stop)() { return S_OK; }; 

};



#endif // __PG8RNULL_H
