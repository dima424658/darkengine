/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/propag8r.cpp,v 1.3 1998/10/05 17:27:09 mahk Exp $

#include <comtools.h>
#include <appagg.h>
#include <propag8n.h>
#include <prpag8r_.h>
#include <stimul8r.h>

// Must be last header 
#include <dbmem.h>


////////////////////////////////////////////////////////////
// CLASS: cBasePropagator
// 

//------------------------------------------------------------
// Construction/Destruction
//

cBasePropagator::cBasePropagator(sPropagatorDesc* desc)
   : Desc(*desc),
     pPropagation(AppGetObj(IPropagation)),
     pStimulator(AppGetObj(IStimulator)),
     InSim(FALSE)
{
   ID = pPropagation->AddPropagator(this);
}

cBasePropagator::~cBasePropagator()
{
   SafeRelease(pStimulator);
   SafeRelease(pPropagation);
}

//------------------------------------------------------------
// IPropagator Methods
//

STDMETHODIMP_(const sPropagatorDesc*) cBasePropagator::Describe()
{
   return &Desc;
}

STDMETHODIMP_(PropagatorID) cBasePropagator::GetID()
{
   return ID;
}

STDMETHODIMP cBasePropagator::Connect(IUnknown* stimulator)
{
   return stimulator->QueryInterface(IID_IStimulator,(void**)&pStimulator);
}

STDMETHODIMP cBasePropagator::AddStimulus(StimID stim)
{
   if (!SupportsStimulus(stim))
      pPropagation->AddPropagatorStimulus(ID,stim);
   return S_OK;
}

STDMETHODIMP cBasePropagator::RemoveStimulus(StimID stim)
{
   if (SupportsStimulus(stim))
      pPropagation->RemovePropagatorStimulus(ID,stim);
   return S_OK;
}

STDMETHODIMP_(BOOL) cBasePropagator::SupportsStimulus(StimID stim)
{
   return pPropagation->SupportsStimulus(ID,stim); 
}





