/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/propag8n.cpp,v 1.5 1998/08/17 19:48:56 mahk Exp $
#include <prpag8n_.h>
#include <appagg.h>
#include <aggmemb.h>

#include <stimtype.h>
#include <stimbase.h>
#include <stimuli.H>

#include <ssrctype.h>
#include <ssrcbase.h>
#include <stimsrc.h>

#include <senstype.h>
#include <sensbase.h>
#include <stimsens.h>

#include <objpos.h>

#include <hshpptem.h>
#include <hshsttem.h>
#include <dlisttem.h>
#include <lststtem.h>
#include <simman.h>
#include <dspchdef.h>
#include <simdef.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// cPropagation implementation
//

IMPLEMENT_AGGREGATION_SELF_DELETE(cPropagation);

static sRelativeConstraint Constraints[] = 
{
   { kConstrainAfter, &IID_ISimManager }, 
   { kNullConstraint}
};

cPropagation::cPropagation(IUnknown* pOuter)
   :pStimuli(NULL),
    pSources(NULL),
    pSensors(NULL),
    pStimulator(NULL)
{
   INIT_AGGREGATION_1(pOuter,IID_IPropagation,this,kPriorityNormal,Constraints);
   // reserve slot zero 
   ByID.Append(&NullGator);
};

cPropagation::~cPropagation()
{

}

//------------------------------------------------------------
// IPropagation methods
//

STDMETHODIMP_(PropagatorID) cPropagation::AddPropagator(IPropagator* gator)
{
   PropagatorID id = ByID.Append(gator);
   ByName.Insert(gator);
   gator->Connect((IUnknown*)pStimulator);
   gator->AddRef();
   return id;
}

////////////////////////////////////////

STDMETHODIMP cPropagation::AddPropagatorStimulus(PropagatorID id, StimID stim)
{
   cGatorList* gators = NULL;

   ByStim.Lookup(stim,&gators);
   if (gators == NULL)
   {
      gators = new cGatorList;
      ByStim.Insert(stim,gators);
   }
   gators->Add(id);

   IPropagator* gator = ByID[id];
   
   IStimSensorQuery* query = pSensors->QuerySensors(OBJ_NULL,stim); 
   for (query->Start(); !query->Done(); query->Next())
   {
      // send a create event
      sStimSensorEvent event = 
      {
         kStimSensorCreate,
         query->ID(),
      };

      event.elems = query->Elems();

      gator->SensorEvent(&event);
   }
   SafeRelease(query);

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPropagation::RemovePropagatorStimulus(PropagatorID id, StimID stim)
{
   cGatorList* gators = ByStim.Search(stim); 
   if (gators == NULL || !gators->HasElem(id))
      return S_FALSE; 

   IPropagator* gator = ByID[id];
   

   // remove sensors
   {
      IStimSensorQuery* query = pSensors->QuerySensors(OBJ_NULL,stim); 
      for (query->Start(); !query->Done(); query->Next())
      {
         // send a create event
         sStimSensorEvent event = 
         {
            kStimSensorDestroy,
            query->ID(),
         };

         event.elems = query->Elems();

         gator->SensorEvent(&event);
      }
      SafeRelease(query);
   }

   // remove sources, but really remove them, don't just send events
   {
      IStimSourceQuery* query = pSources->QuerySources(OBJ_NULL,stim);
      for (; !query->Done(); query->Next())
      {
         StimSourceID srcid = query->ID(); 
         sStimSourceDesc desc;
         pSources->DescribeSource(srcid,&desc);
         if (desc.propagator == id)
            pSources->RemoveSource(srcid); 
      }
      SafeRelease(query); 
   }

   return S_OK;
}

////////////////////////////////////////


STDMETHODIMP_(BOOL) cPropagation::SupportsStimulus(PropagatorID id, StimID stim)
{
   cGatorList* gators = ByStim.Search(stim);

   if (gators)
      return gators->HasElem(id); 
   return FALSE; 
}


////////////////////////////////////////

STDMETHODIMP_(IPropagator*) cPropagation::GetPropagator(PropagatorID id)
{
   IPropagator* result; 
   if (id < 0 || id >= ByID.Size())
      result = &NullGator;
   else 
      result = ByID[id];
   result->AddRef();
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(IPropagator*) cPropagation::GetPropagatorNamed(const char* name)
{
   IPropagator* gator = ByName.Search(name);
   if (gator == NULL)
      gator = &NullGator;
   gator->AddRef();
   return gator;
}

////////////////////////////////////////

STDMETHODIMP cPropagation::SensorEvent(sStimSensorEvent* event)
{
   cGatorList* gators = NULL;

   // Look up all propagators that care about this stimulus
   ByStim.Lookup(event->elems.stim,&gators);

   if (gators != NULL)
   {
      cGatorList::cIter iter;
      for (iter = gators->Iter(); !iter.Done(); iter.Next())
      {
         PropagatorID id = iter.Value();
         ByID[id]->SensorEvent(event);
      }
   }
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPropagation::AddStimulus(StimID stim)
{

   for (int i = 0; i < ByID.Size(); i++)
   {
      const sPropagatorDesc* desc = ByID[i]->Describe(); 
      if (desc->flags & kPGatorAllStimuli)
         AddPropagatorStimulus(i,stim); 
   }
   return S_OK;
}


////////////////////////////////////////

STDMETHODIMP cPropagation::SimpleSensorEvent(StimSensorID id, tStimSensorEvent kind)
{
   sStimSensorEvent event = { kind, id, }; 
   event.elems = pSensors->GetSensorElems(id);

   SensorEvent(&event);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPropagation::SourceEvent(sStimSourceEvent* event)
{
   sStimSourceDesc desc;
   if (event->desc == NULL)
   {
      pSources->DescribeSource(event->id,&desc);
      event->desc = &desc;
   }

   PropagatorID id = event->desc->propagator;

   Assert_(id >= 0 && id < ByID.Size());
   ByID[id]->SourceEvent(event);

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPropagation::SimpleSourceEvent(StimSensorID id, tStimSensorEvent kind)
{
   sStimSourceEvent event = { kind, id };

   event.elems = pSources->GetSourceElems(id);
   event.desc = NULL;
   
   SourceEvent(&event);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPropagation::Propagate(tStimTimeStamp t, tStimDuration dt)
{
   for (int i = PGATOR_NULL + 1; i < ByID.Size(); i++)
   {
      ByID[i]->Propagate(t,dt);
   }
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cPropagation::Reset()
{
   for (int i = PGATOR_NULL + 1; i < ByID.Size(); i++)
   {
      ByID[i]->Reset();
   }
   return S_OK;
}

//------------------------------------------------------------
// PROPAGATOR QUERIES 
//

//
// Base gator query, supports IUnknown
//

class cBaseGatorQuery : public IPropagatorQuery
{
public:
   DECLARE_UNAGGREGATABLE();

   virtual ~cBaseGatorQuery() {};
};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseGatorQuery,IPropagatorQuery);

//
// Empty Query
//

class cEmptyGatorQuery : public cBaseGatorQuery
{

public:
   STDMETHOD(Start)() { return S_OK;};
   STDMETHOD_(BOOL,Done)() { return TRUE; }; 
   STDMETHOD(Next)() { return S_OK; }; 

   STDMETHOD_(PropagatorID,ID)() { return PGATOR_NULL; }; 
   STDMETHOD_(IPropagator*,Propagator)()
   {
      Warning(("Getting the propagator from an empty query\n"));  
      // I'm too lazy to actually return the null propagator
      return NULL;
   }
};

//
// Vector query 
//

class cGatorVecQuery : public cBaseGatorQuery
{
   cGatorVec& Gators;
   int Idx;

public:
   cGatorVecQuery(cGatorVec& vec)
      : Gators(vec),
        Idx(PGATOR_NULL + 1)
   {
   }

   ~cGatorVecQuery() {}

   
   STDMETHOD(Start)() { return S_OK;};
   STDMETHOD_(BOOL,Done)() { return Idx >= Gators.Size(); }; 
   STDMETHOD(Next)() { Idx++; return S_OK; }; 

   STDMETHOD_(PropagatorID,ID)() { return (Idx < Gators.Size()) ? Idx : PGATOR_NULL; }; 
   STDMETHOD_(IPropagator*,Propagator)()
   {
      IPropagator* result = Gators[ID()]; 
      result->AddRef();
      return result;
   }
};


//
// List query
// 

class cGatorListQuery : public cBaseGatorQuery
{
   cGatorVec& GVec;   // for looking up ID's 
   cGatorList& GList;
   cGatorList::cIter Iter;  

public:
   cGatorListQuery(cGatorVec& vec, cGatorList& list)
      : GVec(vec),
        GList(list)
   {
   }

   ~cGatorListQuery() {}

   
   STDMETHOD(Start)() { Iter = GList.Iter(); return S_OK;};
   STDMETHOD_(BOOL,Done)() { return Iter.Done();  }; 
   STDMETHOD(Next)() { Iter.Next();  return S_OK; }; 

   STDMETHOD_(PropagatorID,ID)() { return Iter.Done() ? PGATOR_NULL : Iter.Value(); }; 
   STDMETHOD_(IPropagator*,Propagator)()
   {
      IPropagator* result = GVec[ID()]; 
      result->AddRef();
      return result;
   }
};

////////////////////////////////////////

STDMETHODIMP_(IPropagatorQuery*) cPropagation::GetPropagators(StimID stim)
{
   if (stim == OBJ_NULL)
   {
      return new cGatorVecQuery(ByID);
   }
   else
   {
      cGatorList* list = NULL;
      ByStim.Lookup(stim,&list);
      if (list == NULL)
         return new cEmptyGatorQuery;
      else
         return new cGatorListQuery(ByID,*list);
   }
}


//------------------------------------------------------------
// Aggregate protocol
//

HRESULT cPropagation::Init()
{
   pStimuli = AppGetObj(IStimuli);
   pSources = AppGetObj(IStimSources);
   pSensors = AppGetObj(IStimSensors);
   pStimulator = AppGetObj(IStimulator);

   AutoAppIPtr(SimManager); 
   sDispatchListenerDesc desc = gSimDesc; 
   desc.data = this; 
   pSimManager->Listen(&desc); 

   return S_OK;
}

HRESULT cPropagation::End()
{
   SafeRelease(pSources);
   SafeRelease(pSensors);
   SafeRelease(pStimuli);
   SafeRelease(pStimulator);
   for (int i = PGATOR_NULL + 1; i < ByID.Size(); i++)
   {
      ByID[i]->Release();
   }

   return S_OK;
}

//------------------------------------------------------------
// Sim Listener
//

void cPropagation::SimMsgCB(const struct sDispatchMsg* msg, const struct sDispatchListenerDesc* desc)
{
   cPropagation* us = (cPropagation*)desc->data; 
   us->OnSimMsg(msg->kind); 
} 

void cPropagation::OnSimMsg(ulong msg)
{
   switch (msg)
   {
      case kSimInit:
      case kSimResume:
      {
         for (int i = PGATOR_NULL + 1; i < ByID.Size(); i++)
            ByID[i]->Start(); 
      }
      break; 

      case kSimTerm:
      case kSimSuspend:
      {
         for (int i = PGATOR_NULL + 1; i < ByID.Size(); i++)
            ByID[i]->Stop(); 
      }
      break; 

   }
}

sDispatchListenerDesc cPropagation::gSimDesc = 
{
   &IID_IPropagation, 
   kSimInit|kSimTerm|kSimSuspend|kSimResume, 
   cPropagation::SimMsgCB, 
}; 


//------------------------------------------------------------
// Factory
// 

void PropagationCreate(void)
{
   AutoAppIPtr_(Unknown,pUnk);
   new cPropagation(pUnk);
}


