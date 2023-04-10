/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/prpag8r_.h,v 1.2 1998/08/17 19:49:11 mahk Exp $
#pragma once  
#ifndef __PRPAG8R__H
#define __PRPAG8R__H

#include <pg8rtype.h>
#include <pg8rbase.h>
#include <propag8r.h>

F_DECLARE_INTERFACE(IStimulator);
F_DECLARE_INTERFACE(IPropagation);

////////////////////////////////////////////////////////////
// BASE IMPLEMENTATION CLASSES FOR PROPAGATORS
//


class cUnknownPropagator : public cCTUnaggregated<IPropagator,&IID_IPropagator,kCTU_Default>
{
public:

   virtual ~cUnknownPropagator() {};
   
};

////////////////////////////////////////
// Base propagator, with core functionality 
//

class cBasePropagator : public cUnknownPropagator
{
public: 
   cBasePropagator(sPropagatorDesc* desc);
   ~cBasePropagator(); 

   STDMETHOD_(const sPropagatorDesc*,Describe)();
   STDMETHOD_(PropagatorID,GetID)();
   STDMETHOD(Connect)(IUnknown* stimulator);
   STDMETHOD(AddStimulus)(StimID stim); 
   STDMETHOD(RemoveStimulus)(StimID stim); 
   STDMETHOD_(BOOL,SupportsStimulus)(StimID stim); 
   STDMETHOD(Start)() { InSim = TRUE; return S_OK; }; 
   STDMETHOD(Stop)() { InSim = FALSE; return S_OK; }; 

protected:

   BOOL InSim; 
   sPropagatorDesc Desc;
   PropagatorID ID;
   IStimulator* pStimulator;
   IPropagation* pPropagation;

};





#endif // __PRPAG8R__H
