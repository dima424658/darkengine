/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/contag8r.h,v 1.8 1998/10/21 18:52:21 mahk Exp $
#pragma once  
#ifndef __CONTAG8R_H
#define __CONTAG8R_H
#include <prpag8r_.h>

#include <dmgmodel.h>
#include <dlistsim.h>
#include <hashpp.h>

#include <reactype.h>
#include <reacbase.h>

#include <listset.h>

////////////////////////////////////////////////////////////
// PHYSICAL CONTACT PROPAGATOR
//

F_DECLARE_INTERFACE(IStimSources);
F_DECLARE_INTERFACE(IStimSensors);

class cContactPropagator : public cBasePropagator
{

public: 
   cContactPropagator();
   ~cContactPropagator();

   //------------------------------------------------------------
   // IPropagator Methods
   //

   STDMETHOD(InitSource)(sStimSourceDesc* desc);
   STDMETHOD_(const struct sStructDesc*,DescribeShapes)();
   STDMETHOD_(const struct sStructDesc*,DescribeLifeCycles)();
   STDMETHOD(SourceEvent)(sStimSourceEvent* event); 
   STDMETHOD_(tStimLevel,GetSourceLevel)(StimSourceID id);
   STDMETHOD(DescribeSource)(StimSourceID id, sStimSourceDesc* desc);
   STDMETHOD(SensorEvent)(sStimSensorEvent* event); 
   STDMETHOD(Propagate)(tStimTimeStamp curtime, tStimDuration duration);
   STDMETHOD(Reset)();

protected:

   //------------------------------------------------------------
   // Helper classes
   //
   
   // List of sources
   class cSourceList : public cSimpleListSet<StimSourceID>
   {

   };

   // Hash functions
   typedef cScalarHashFunctions<ObjID> cSourceTableHashFuncs; 

   // Table of lists of sources
   class cSourceTable : public cHashTable<ObjID,cSourceList*,cSourceTableHashFuncs>
   {
   public:
      void RemoveAll();
      
   };

public:
   //------------------------------------------------------------
   // API for frob and weapon swing 
   //
   
   enum ePropFlags
   {
      kNoDefer  = 1 << 0,
      kPhysical = 1 << 1, 
   }; 

   void DoPropagationEvent(ulong eventmask, ObjID from, ObjID to, float magnitude, const struct sChainedEvent* cause = NULL, ulong propflags = 0);

   // the propagator to use
   static cContactPropagator* gpGator; 

protected:
   //------------------------------------------------------------
   // Helper Functions
   //

   eDamageResult HandleImpact(const struct sDamageMsg* msg);
   static eDamageResult LGAPI ImpactListener(const struct sDamageMsg* msg, tDamageCallbackData data);

   eReactionResult SetImpactResult(sReactionEvent* event, const sReactionParam* param); 
   static eReactionResult LGAPI ImpactResultReaction(sReactionEvent* event, const sReactionParam* param, tReactionFuncData data); 

   void AddReactions();

   //------------------------------------------------------------
   // Data Members
   //

   // Interfaces we use
   IStimSensors* pSensors;
   IStimSources* pSources;

   // Internals
   cSourceTable Sources;   // Sources we handle 

   // Collision Data
   const sDamageMsg* pImpactEvent; 
   eDamageResult ImpactResult; 
}; 



#endif // __CONTAG8R_H
