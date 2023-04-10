/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/radiag8r.h,v 1.3 1998/10/02 22:14:25 mahk Exp $
#pragma once  
#ifndef __RADIAG8R_H
#define __RADIAG8R_H
#include <prpag8r_.h>

#include <dlistsim.h>
#include <stimtype.h>
#include <ssrclife.h>

#include <listset.h>

////////////////////////////////////////////////////////////
// INSTANT RADIUS PROPAGATOR
//

F_DECLARE_INTERFACE(IStimSources);
F_DECLARE_INTERFACE(IStimSensors);


//
// Radius propagator class
//

class cRadiusPropagator : public cBasePropagator
{

public: 
   cRadiusPropagator();
   ~cRadiusPropagator();

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
   STDMETHOD(Start)();
   STDMETHOD(Stop)(); 


   //------------------------------------------------------------
   // Helper classes
   //
   
   //
   // Source shape structure
   //

public:
   enum eShapeFlags
   {
      kRaycast    = 1 << 0, // Only hit things I can raycast to
   }; 

   enum eDispersion
   {
      kDisperseNone,       // constant 
      kDisperseLinear,     // -kX
      kDisperseInvSquare,  // 1/X^2 
   }; 

   struct sShape
   {
      float radius; 
      ulong flags; 
      int dispersion; 
      BOOL fired; 
   }; 

   typedef struct sPeriodicLifeCycle sLifeCycle; 
   
   enum eLifeCycleFlags // new & old flags for life cycle 
   {
      kNoMaxFirings  = sLifeCycle::kNoMaxFirings,
      kDestroy       = sLifeCycle::kDestroy,
   };

protected:


   //
   // Source queue   
   // 

   struct sSourceElem
   {
      StimSourceID id; 
      tStimTimeStamp time; 
      BOOL first; // have we never fired 
   }; 

   class cSourceQueueBase : public cSimpleDList<sSourceElem> {}; 

   class cSourceQueue : public cSourceQueueBase
   {
   public:
      void AddNode(cNode* elem);
      void AddElem(const sSourceElem& elem);
      void RemoveID(StimSourceID id);
   }; 

   //------------------------------------------------------------
   // Helpers
   //


   void GenerateEvents(tStimTimeStamp t, StimSourceID srcid, const sStimSourceDesc* desc, ulong flags); 

   void GenerateOneEvent(tStimTimeStamp t, StimSourceID srcid, const sStimSourceDesc* desc, const sObjStimPair& elems, ObjID sensobj, StimSensorID sensid, ulong flags); 

   //------------------------------------------------------------
   // Data Members
   //

   cSourceQueue mQueue; 
   tStimTimeStamp mLastTime;  // last time we propagated 
   StimSourceID mRemoving; // which source are we removing right now

   // Interfaces we use
   IStimSensors* mpSensors;
   IStimSources* mpSources;

}; 



#endif // __RADIAG8R_H





