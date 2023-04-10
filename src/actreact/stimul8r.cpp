/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimul8r.cpp,v 1.17 1999/03/11 17:47:08 mahk Exp $
#include <appagg.h>
#include <stimul8_.h>
#include <sensbase.h>
#include <stimbase.h>
#include <reacbase.h>
#include <receptro.h>
#include <objpos.h>
#include <string.h>
#include <iobjsys.h>
#include <linkint.h>
#include <lazyagg.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <simtime.h>
#include <stimuli.h>
#include <pg8rtype.h>
#include <ssrcbase.h>

#include <config.h>
#include <mprintf.h>

#include <label.h>

#include <dlisttem.h>

#include <stimprop.h>
#include <propbase.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// cStimulator IMPLEMENTATION
//

F_DECLARE_INTERFACE(IPropertyManager); 

static sRelativeConstraint Constraints[] = 
{
   { kConstrainAfter, &IID_IStimSensors},
   { kConstrainAfter, &IID_IStimSources},
   { kConstrainAfter, &IID_IReactions},
   { kConstrainAfter, &IID_IPropertyManager},

   { kNullConstraint}
};


cStimulator::cStimulator(IUnknown* pOuter)
   : pSensors(NULL),
     pReactions(NULL),
     pSources(NULL),
     SpewOn(FALSE),
     NextQueue(0),
     NextFilter(1)
{
   MI_INIT_AGGREGATION_1(pOuter,IStimulator,kPriorityNormal,Constraints);
}

cStimulator::~cStimulator()
{

}

#ifdef DBG_ON
#define StimSpew(x) if (SpewOn) { mprintf x;}
#define SetSpew()  do { SpewOn = config_is_defined("stimulation_spew");} while (0) 
#else
#define StimSpew(x) 
#define SetSpew()
#endif 

//------------------------------------------------------------
// Aggregate Protocol
//

STDMETHODIMP cStimulator::Init()
{
   pSensors = AppGetObj(IStimSensors);
   pReactions = AppGetObj(IReactions);
   pSources = AppGetObj(IStimSources);
   add_reactions();
   init_props(); 
   
   return S_OK;
}

STDMETHODIMP cStimulator::End()
{
   SafeRelease(pSensors);
   SafeRelease(pReactions);
   SafeRelease(pSources);
   term_props(); 

   return S_OK;
}

//------------------------------------------------------------
// Helpers
//

//
// Test an event against a trigger
//

BOOL cStimulator::trigger_test(const sStimEvent* event, const sReceptron* tron)
{
   const sStimTrigger& trig = tron->trigger;

   if (trig.flags & kStimTrigInactive)
      return FALSE;

   if (!(trig.flags & kStimTrigNoMin))
   {
      if (event->intensity < trig.min)
         return FALSE;
   }

   if (!(trig.flags & kStimTrigNoMax))
   {
      if (event->intensity >= trig.max)
         return FALSE;
   }
   return TRUE;
}

//
// Convert the "source" and "sensor" proxy objects into the actual 
// source and sensor object for the event
//

void cStimulator::remap_obj_params(const sStimEvent* event, sReceptron* tron)
{
   for (int i = 0; i < kReactNumObjs; i++)
   {
      // Note use of reference var as shorthand
      ObjID& obj = tron->effect.param.obj[i];

      if (obj == pSensors->ObjParam(kObjParamSource))
      {
         obj = pSources->GetSourceElems(event->source).obj;
         continue;
      }

      if (obj == pSensors->ObjParam(kObjParamSensor))
      {
         obj = pSensors->GetSensorElems(event->sensor).obj;
         continue;
      }
   }
}

//
// Actually do the work of firing receptrons
//

static LazyAggMember(IObjectSystem) gpObjSys; 
static LazyAggMember(ILinkManagerInternal) gpLinkMan; 

typedef cSimpleDList<ReceptronID> cTronList; 

void cStimulator::handle_event(const sStimEvent* event)
{
   StimSensorID sensor = event->sensor;

  // modifiable copy of event
   sStimEvent use_event = *event;

   SetSpew();

   // get the object this sensor is attached to.
   ObjID obj = pSensors->GetSensorElems(sensor).obj;

   // Lock down object system and link manager
   gpObjSys->Lock(); 
   tQueryDate lock = gpLinkMan->Lock(); 

   // These must be declared *before* the goto abort, or the compiler
   // will hassle us:
   IReceptronQuery *query = NULL;
   cTronList tronList; 
   sStimEvent last_event;
   cTronList::cIter iter;
   // note original source id
   StimSourceID event_src = event->source; 


   // Pass the event through the filters 
   cFilterList::cIter iterf;
   for (iterf = Filters.Iter(); !iterf.Done(); iterf.Next())
   {
      sFilter& filter = iterf.Value(); 
      if (filter.func)
      {
         eReactionResult result = filter.func(&use_event,filter.data); 
         // Note that we don't do the anal enforcement of kReactionNormal
         // that we do below for receptrons.  
         if (result == kReactionAbort)
            goto abort; 
      }
   }

// #ifdef NEW_NETWORK_ENABLED
   // This is now being handled in cReactions::React instead, so specific
   // reactions can decide to work on proxy objects...
#if 0
   // if the object is a proxy, then don't perform any secondary
   // effects on it
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      AutoAppIPtr(ObjectNetworking);
      if (pObjectNetworking->ObjIsProxy(obj)) {
         goto abort;
      }
   }
#endif

   StimSpew(("Stimulating object %d stimulus %d level %g\n",obj,use_event.kind,use_event.intensity));


   //
   // gather up all receptrons in a list
   // The point here is to make the inherited receptron query an atomic operation
   // because receptrons firing can change the hierarchy. 
   //
   
   // find me all the receptrons that respond to this stimulus
   query = pSensors->QueryInheritedReceptrons(obj,use_event.kind);

   for (query->Start(); !query->Done(); query->Next())
      tronList.Append(query->ID()); 

   SafeRelease(query); 

      
   last_event = use_event;

   for (iter = tronList.Iter(); !iter.Done(); iter.Next())
   {
      ReceptronID tronid = iter.Value(); 
      sReceptron copytron; 
      pSensors->GetReceptron(tronid,&copytron); 

      if (!trigger_test(&use_event,&copytron))
      {
         ObjID obj = pSensors->GetReceptronElems(tronid).obj;
         StimSpew(("Receptron on %d failed trigger test\n",obj));
         continue;
      }

      sReactionEvent reactevent = { &use_event, (sStimTrigger*)&copytron.trigger, obj};

      remap_obj_params(&use_event,&copytron);

      eReactionResult result = pReactions->React(copytron.effect.kind,&reactevent,&copytron.effect.param); 

      if (SpewOn)
      {
         const sReactionDesc* desc = pReactions->DescribeReaction(copytron.effect.kind);
         ObjID tronobj = pSensors->GetReceptronElems(tronid).obj;
         StimSpew(("Fired %s effect on obj %d.  Result was %d\n",desc->name,tronobj,result));
      }

      switch (result)
      {
         case kReactionMutate:
            last_event = use_event;
            break;

         case kReactionNormal:
            // you promised not to mutate, so we'll stomp you
            // this may be too costly
            use_event = last_event; 
            break;

         case kReactionAbort:
            goto abort;
      }

   }
abort:
   gpLinkMan->Unlock(lock); 
   gpObjSys->Unlock(); 

   // blast all non-propagated sources
   if (event_src != SRCID_NULL)
   {
      if (use_event.flags & kStimDestroySrcObj)
      {
         sObjStimPair pair = pSources->GetSourceElems(event_src); 
         AutoAppIPtr_(ObjectSystem,pObjSys); 
         pObjSys->Destroy(pair.obj); 
      }
      else
      {
         sStimSourceDesc desc;
         pSources->DescribeSource(event_src,&desc);
         if (desc.propagator == PGATOR_NULL)
            pSources->RemoveSource(event_src);
      }
   }


}

//
// Burn through an event queue, firing all events
//

void cStimulator::burn_queue(cEventQueue& queue)
{
   cEventQueue::cIter iter;
   for (iter = queue.Iter(); !iter.Done(); iter.Next())
   {
      handle_event(&iter.Value());
      queue.Delete(iter.Node());
   }
}

//------------------------------------------------------------
// Built-in reaction: stimulate
//

struct sStimulateParam
{
   Label stimname;
   tStimLevel intensity;
   float coeff;
};

eReactionResult LGAPI cStimulator::stimulate_func(sReactionEvent* ev, const sReactionParam* param, tReactionFuncData data)
{
   sStimulateParam* stimparam = (sStimulateParam*)&param->data;
   cStimulator* us = (cStimulator*)data;
   sStimEvent event; 
   
   memset(&event,0,sizeof(event));

   // Lookup the stimulus
   AutoAppIPtr(Stimuli);
   event.kind = pStimuli->GetStimulusNamed(stimparam->stimname.text);

   if (event.kind == OBJ_NULL) 
      return kReactionNormal;

   event.intensity = stimparam->intensity + ev->stim->intensity*stimparam->coeff;

   // Find the sensor
   ObjID sensobj = param->obj[kReactDirectObj];
   
   event.sensor = us->pSensors->LookupSensor(sensobj,event.kind);
   if (event.sensor == SENSORID_NULL)  // no such sensor
      return kReactionNormal;

   // Create the "source" 
   ObjID srcobj = param->obj[kReactIndirectObj];
   
   if (srcobj != OBJ_NULL)
   {
      sStimSourceDesc srcdesc;
      memset(&srcdesc,0,sizeof(srcdesc));

      srcdesc.propagator = PGATOR_NULL;
      srcdesc.level = event.intensity;

      event.source = us->pSources->AddSource(srcobj,event.kind,&srcdesc);
   }
   else
      event.source = SRCID_NULL;

   
      
   // Ok, stimulate!
   us->StimulateSensor(event.sensor,&event);

   return kReactionNormal;
}

static sReactionDesc stimulate_desc = 
{
   "stimulate",
   "Stimulate Object",
   REACTION_PARAM_TYPE(sStimulateParam),
   kReactionHasDirectObj|kReactionHasIndirectObj,
};

static sFieldDesc stimulate_fields[] = 
{
   { "Stimulus", kFieldTypeString, FieldLocation(sStimulateParam,stimname) },
   { "Multiply By", kFieldTypeFloat, FieldLocation(sStimulateParam,coeff) },
   { "Then Add", kFieldTypeFloat, FieldLocation(sStimulateParam,intensity) },
};

static sStructDesc stimulate_sdesc = StructDescBuild(sStimulateParam,kStructFlagNone,stimulate_fields);

void cStimulator::add_reactions()
{
   StructDescRegister(&stimulate_sdesc);
   pReactions->Add(&stimulate_desc,stimulate_func,this);
}

//------------------------------------------------------------
// IStimulator methods
//

STDMETHODIMP cStimulator::StimulateSensor(StimSensorID sensor, const sStimEvent* ev) 
{
   sStimEvent event = *ev; 
   event.sensor = sensor; 
   if (event.time == 0) event.time = GetSimTime(); 

   if (ev->flags & kStimEventNoDefer)
   {
      // Send it now
      handle_event(&event); 
   }
   else
   {
      // queue the event
      Queues[NextQueue].Append(event);
   }

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cStimulator::UpdateFrame(tStimTimeStamp, tStimDuration)
{
   // Burn through each queue once

   //   StimSpew(("Queue 0:\n"));

   // Burn through queue 0, setting queue 1 as the new event-accumulating queue
   NextQueue = 1;
   burn_queue(Queues[0]); 

   //   StimSpew(("Queue 1:\n"));

   // Burn through queue 1, setting queue 0 as the new event-accumulating queue
   NextQueue = 0;
   burn_queue(Queues[1]); 

   // At this point, there may be events in queue 0 that will have to wait til next frame. 
   // This keeps an event that causes more events from hanging the game. 
   
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cStimulator::GetSensitivity(StimSensorID sensor, sStimSensitivity* sens)
{
   memset(sens,0,sizeof(*sens));

   // get the object this sensor is attached to.
   sObjStimPair pair = pSensors->GetSensorElems(sensor);
   
   // find me all the receptrons that respond to this stimulus
   IReceptronQuery* query = pSensors->QueryInheritedReceptrons(pair.obj,pair.stim);

   for (query->Start(); !query->Done(); query->Next())
   {
      const sReceptron* tron = query->Receptron();

      sens->flags |= tron->trigger.flags & kStimTrigNoMin; 
      if (!(sens->flags & kStimSensNoMin))
      {
         if (tron->trigger.min < sens->min)
            sens->min = tron->trigger.min;
      }

      sens->flags |= tron->trigger.flags & kStimTrigNoMax; 
      if (!(sens->flags & kStimSensNoMax))
      {
         if (tron->trigger.max > sens->max)
            sens->max = tron->trigger.max;
      }
      
   }
   SafeRelease(query);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(const sStimPosition*) cStimulator::GetSensorPosition(StimSensorID sensor) 
{
   // get the object this sensor is attached to.
   ObjID obj = pSensors->GetSensorElems(sensor).obj;
   return ObjPosGet(obj);
}

////////////////////////////////////////

STDMETHODIMP_(tStimFilterID) cStimulator::AddFilter(tStimFilterFunc func, tStimFilterData data)
{
   sFilter filter = { func, data, NextFilter++}; 
   Filters.Append(filter); 

   return filter.id; 
}

STDMETHODIMP cStimulator::RemoveFilter(tStimFilterID id)
{
   cFilterList::cIter iter;
   for (iter = Filters.Iter(); !iter.Done(); iter.Next())
   {
      sFilter& filter = iter.Value(); 
      if (filter.id == id)
      {
         Filters.Remove(&iter.Node());
         return S_OK; 
      }; 
            
   }
   return S_FALSE; 
}


//------------------------------------------------------------
// Scale property definition
// 

IFloatProperty* g_pSourceScaleProperty;

static sPropertyDesc scale_desc = 
{
   PROP_SOURCE_SCALE_NAME, 
   0, // flags
   NULL, 0, 0, // constraints, version
   { "Act/React", "Source Scale" }, 
}; 

eReactionResult cStimulator::scale_filter(sStimEvent* event)
{
   // scale the stimulus using the scale property
   if (event->source != SRCID_NULL)
   {
      sObjStimPair pair = pSources->GetSourceElems(event->source); 
      float scale = 1.0; 
      if (pScaleProp->Get(pair.obj,&scale))
         event->intensity *= scale; 
   }
   return kReactionNormal; 
}

static eReactionResult scale_filter_func(sStimEvent* event, void* data)
{
   cStimulator* us = (cStimulator*)data;
   return us->scale_filter(event); 
}

void cStimulator::init_props()
{
   pScaleProp = CreateFloatProperty(&scale_desc,kPropertyImplSparseHash); 
   g_pSourceScaleProperty = pScaleProp;
   g_pSourceScaleProperty->AddRef();

   AddFilter(scale_filter_func,this); 
}

void cStimulator::term_props()
{
   SafeRelease(pScaleProp); 
   SafeRelease(g_pSourceScaleProperty);
}

//------------------------------------------------------------
// Factory
//

void StimulatorCreate(void)
{
   AutoAppIPtr_(Unknown,pUnk);
   new cStimulator(pUnk);
}
