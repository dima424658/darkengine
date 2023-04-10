/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/prpag8n_.h,v 1.5 2000/01/29 12:44:32 adurant Exp $
#pragma once

#ifndef __PRPAG8N__H
#define __PRPAG8N__H

#include <propag8n.h>
#include <stimtype.h>
#include <aggmemb.h>
#include <pg8rnull.h>
#include <dynarray.h>
#include <listset.h>
#include <hashset.h>
#include <hashpp.h>

F_DECLARE_INTERFACE(IStimuli);
F_DECLARE_INTERFACE(IStimSources);
F_DECLARE_INTERFACE(IStimSensors);
F_DECLARE_INTERFACE(IStimulator);

//------------------------------------------------------------
// Data Structure Types
//

// 
// Hash table  name -> propagator
//
class cGatorTable : public cStrIHashSet<IPropagator *>
{
public:
   cGatorTable () {};
   tHashSetKey GetKey (tHashSetNode node) const
   {
      return (tHashSetKey) (((IPropagator *) (node))->Describe()->name);
   };
};

//
// DynArray of gator ids
//
typedef cDynArray<IPropagator*> cGatorVec;

//
// List of propagatorIDs
//
class cGatorList: public cSimpleListSet<PropagatorID> 
{
public:
     
   // Add, in a non-duplicating way
   void Add(PropagatorID id)
   {
      AddElem(id); 
   }
      
};

// 
// Hash table StimID -> list (gator id's)
//
   
typedef cScalarHashFunctions<StimID> cStimHashFuncs;

class cGatorStimTable: public cHashTable<ObjID, cGatorList*, cStimHashFuncs>
{ 
public:
   void DestroyAll()
   {
      for (cIter iter = Iter(); !iter.Done(); iter.Next())
      {
         delete iter.Value(); 
      }
   };

   ~cGatorStimTable() { DestroyAll(); } ; 
};

////////////////////////////////////////////////////////////
// cPropagation:  implementation class for IPropagation
//

class cPropagation : public IPropagation
{
   DECLARE_AGGREGATION(cPropagation);

public:
   cPropagation(IUnknown* outer);
   virtual ~cPropagation();

   //
   // IPropagation methods
   //
   STDMETHOD_(PropagatorID,AddPropagator)(IPropagator* gator);
   STDMETHOD(AddPropagatorStimulus)(PropagatorID id, StimID stim);
   STDMETHOD(RemovePropagatorStimulus)(PropagatorID id, StimID stim);
   STDMETHOD_(BOOL,SupportsStimulus)(PropagatorID id, StimID stim);

   STDMETHOD_(IPropagator*,GetPropagator)(PropagatorID);
   STDMETHOD_(IPropagator*,GetPropagatorNamed)(const char* );
   STDMETHOD_(IPropagatorQuery*,GetPropagators)(StimID ID);
   STDMETHOD(SensorEvent)(sStimSensorEvent* event); 
   STDMETHOD(SimpleSensorEvent)(StimSensorID sensor, tStimSensorEvent kind); 
   STDMETHOD(SourceEvent)(sStimSourceEvent* event); 
   STDMETHOD(SimpleSourceEvent)(StimSourceID source, tStimSourceEvent kind); 
   STDMETHOD(Propagate)(tStimTimeStamp curtime, tStimDuration duration);
   STDMETHOD(Reset)();
   STDMETHOD(AddStimulus)(StimID stim);


protected:
   //
   // Sim Listener
   //
   static struct sDispatchListenerDesc gSimDesc; 
   void OnSimMsg(ulong msg); 
   static void SimMsgCB(const struct sDispatchMsg* msg, const struct sDispatchListenerDesc* desc); 

   //
   // Aggregate protocol
   //

   HRESULT Init();
   HRESULT End();

   //
   // Data members
   //

   // Convenience refs
   IStimuli* pStimuli;
   IStimSources* pSources;
   IStimSensors* pSensors;
   IStimulator*  pStimulator;

   cNullPropagator NullGator;    // Null propagator
   cGatorVec ByID;               // Master propagator list
   cGatorTable ByName;           // hashed by name
   cGatorStimTable ByStim;       // Propagators by stimulus
};



#endif // __PRPAG8N__H
