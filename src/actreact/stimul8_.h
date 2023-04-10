/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimul8_.h,v 1.7 2000/01/29 12:44:51 adurant Exp $
#pragma once

#ifndef __STIMUL8__H
#define __STIMUL8__H

#include <comtools.h>
#include <aggmemb.h>
#include <stimul8r.h>
#include <stimsens.h>
#include <stimsrc.h>
#include <reaction.h>
#include <stimbase.h>
#include <propface.h>

#include <dlistsim.h>

////////////////////////////////////////////////////////////
// cStimulator Implementation Class
//

class cStimulator : public cCTDelegating<IStimulator>,
                    public cCTAggregateMemberControl<kCTU_Default>
{
public:
   cStimulator(IUnknown* pOuter);
   virtual ~cStimulator(); 

   //----------------------------------------
   // IStimulator methods
   //
   
   STDMETHOD(StimulateSensor)(StimSensorID sensor, const sStimEvent* event);
   STDMETHOD(GetSensitivity)(StimSensorID id, sStimSensitivity* sensitivity);
   STDMETHOD_(const sStimPosition*, GetSensorPosition)(StimSensorID id);
   STDMETHOD(UpdateFrame)(tStimTimeStamp time, tStimDuration dt);
   STDMETHOD_(tStimFilterID,AddFilter)(tStimFilterFunc,tStimFilterData); 
   STDMETHOD(RemoveFilter)(tStimFilterID id); 


public:
   // for scale property filtering 
   eReactionResult scale_filter(sStimEvent* event);

protected: 
   //----------------------------------------
   // Helpers
   //

   BOOL trigger_test(const sStimEvent* event, const sReceptron* tron);
   void remap_obj_params(const sStimEvent* event, sReceptron* tron);
   void handle_event(const sStimEvent* event);

   static eReactionResult LGAPI stimulate_func(sReactionEvent* ev, const sReactionParam* param, tReactionFuncData data);
   void add_reactions();


   void init_props(); 
   void term_props(); 


   class cEventQueue : public cSimpleDList<sStimEvent>
   {
   };

   void burn_queue(cEventQueue& queue);


   struct sFilter
   {
      tStimFilterFunc func; 
      tStimFilterData data; 
      tStimFilterID id; 
   }; 

   class cFilterList : public cSimpleDList<sFilter>
   {
   }; 

   //----------------------------------------
   // Aggregate Protocol 
   // 

   STDMETHOD(Init)(); 
   STDMETHOD(End)(); 

   //
   // Data members
   //

   // agg members we use
   IStimSensors* pSensors;
   IStimSources* pSources;
   IReactions* pReactions;

   BOOL SpewOn;

   // We use two queues, to avoid problems with infinite looping
   int NextQueue;
   cEventQueue Queues[2];

   // A property that scales all of an object's sources
   IFloatProperty *pScaleProp; 

   // Our filter list
   cFilterList Filters; 
   tStimFilterID NextFilter; 
   
};

#endif // __STIMUL8__H

