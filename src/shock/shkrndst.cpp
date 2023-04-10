/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrndst.cpp,v 1.1 1999/04/13 18:55:40 JON Exp $

#include <appagg.h>
#include <stimul8_.h>
#include <sensbase.h>
#include <stimbase.h>
#include <reacbase.h>
#include <receptro.h>
#include <iobjsys.h>

#include <rand.h>
#include <stdlib.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <stimprop.h>
#include <propbase.h>

// Must be last header
#include <dbmem.h>


//------------------------------------------------------------
// Randomize stim property definition
// 

static IFloatProperty* _g_pSourceRandomizeProperty;

#define PROP_SOURCE_RANDOMIZE_NAME "SrcRand"

static sPropertyDesc sourceRandomizePDesc = 
{
   PROP_SOURCE_RANDOMIZE_NAME, 
   0, // flags
   NULL, 0, 0, // constraints, version
   { "Act/React", "Source Randomize" }, 
}; 

eReactionResult ShockSourceRandomizeFilter(sStimEvent* event, void* data)
{
   // scale the stimulus using the property
   if (event->source != SRCID_NULL)
   {
      AutoAppIPtr(StimSources);
      sObjStimPair pair = pStimSources->GetSourceElems(event->source); 
      float randMag;
      if (_g_pSourceRandomizeProperty->Get(pair.obj,&randMag))
      {
         float add = (2.*randMag*(float(Rand()/float(RAND_MAX))))-randMag;
         event->intensity += add; 
         if (event->intensity<0)
            event->intensity = 0;
      }
   }
   return kReactionNormal; 
}

void ShockSourceRandomizeInit(void)
{
   _g_pSourceRandomizeProperty = CreateFloatProperty(&sourceRandomizePDesc, kPropertyImplSparseHash);

   AutoAppIPtr(Stimulator);
   pStimulator->AddFilter(ShockSourceRandomizeFilter, NULL); 
}

void ShockSourceRandomizeTerm(void)
{
   SafeRelease(_g_pSourceRandomizeProperty);
}
