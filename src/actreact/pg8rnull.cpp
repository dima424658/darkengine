/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/pg8rnull.cpp,v 1.5 1998/10/05 17:26:59 mahk Exp $

#include <pg8rtype.h>
#include <pg8rbase.h>
#include <pg8rnull.h>

#include <ssrctype.h>
#include <ssrcbase.h>


#include <string.h>

// Must be last header 
#include <dbmem.h>


////////////////////////////////////////////////////////////
// STUB PROPAGATOR
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cNullPropagator,IPropagator);

#define Scream() Warning(("cNullPropagation method called %s line %d\n",__FILE__,__LINE__))

#pragma off(unreferenced)

STDMETHODIMP_(const sPropagatorDesc*) cNullPropagator::Describe()
{
   static sPropagatorDesc desc = 
   {
      "Null"
   };

   return &desc;
}

STDMETHODIMP_(PropagatorID) cNullPropagator::GetID()
{
   return PGATOR_NULL;
}

STDMETHODIMP cNullPropagator::AddStimulus(StimID stim)
{
   Scream();
   return E_FAIL;
}

STDMETHODIMP cNullPropagator::RemoveStimulus(StimID stim)
{
   Scream();
   return E_FAIL;
}

STDMETHODIMP_(BOOL) cNullPropagator::SupportsStimulus(StimID stim)
{
   Scream();
   return FALSE;
}

STDMETHODIMP cNullPropagator::InitSource(sStimSourceDesc* desc)
{
   Scream();
   memset(desc,0,sizeof(*desc));
   return E_FAIL;
}


STDMETHODIMP cNullPropagator::SourceEvent(sStimSourceEvent* event)
{
   Scream();
   return E_FAIL;
}

STDMETHODIMP_(tStimLevel)  cNullPropagator::GetSourceLevel(StimSourceID id)
{
   Scream();
   return 0;
}

STDMETHODIMP_(const sStructDesc* ) cNullPropagator::DescribeShapes()
{
   Scream();
   return NULL;
}

STDMETHODIMP_(const sStructDesc* ) cNullPropagator::DescribeLifeCycles()
{
   Scream();
   return NULL;
}


STDMETHODIMP cNullPropagator::DescribeSource(StimSourceID id, sStimSourceDesc* desc)
{
   Scream();
   return E_FAIL;
}

STDMETHODIMP cNullPropagator::SensorEvent(sStimSensorEvent* event)
{
   Scream();
   return E_FAIL;
}


STDMETHODIMP cNullPropagator::Connect(IUnknown* stimulator)
{
   Scream();
   return E_FAIL;
}

STDMETHODIMP cNullPropagator::Propagate(tStimTimeStamp curtime, tStimDuration duration)
{
   Scream();
   return E_FAIL;
}

STDMETHODIMP cNullPropagator::Reset()
{
   Scream();
   return E_FAIL;
}



#pragma on(unreferenced)

