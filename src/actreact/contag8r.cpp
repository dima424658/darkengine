/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/contag8r.cpp,v 1.17 1998/10/22 15:06:45 CCAROLLO Exp $
#include <appagg.h>

#include <stimtype.h>
#include <stimbase.h>

#include <ssrctype.h>
#include <ssrcbase.h>
#include <stimsrc.h>

#include <senstype.h>
#include <sensbase.h>
#include <stimsens.h>

#include <stimul8r.h>
#include <propag8n.h>

#include <contag8r.h>
#include <ctactbas.h>

#include <string.h>

#include <simtime.h>

#include <dmgmodel.h>
#include <dmgbase.h>

#include <dlisttem.h>
#include <hshpptem.h>
#include <lststtem.h>

#include <reaction.h>
#include <reacbase.h>

#include <sdesbase.h>
#include <sdestool.h>

#include <physapi.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// CLASS: cContactPropagator
//


//------------------------------------------------------------
// Construction/Destruction 
//

// Our propagator descriptor

static sPropagatorDesc contact_desc = 
{
   "Contact",
   kPGatorAllStimuli, 
};

////////////////////////////////////////

cContactPropagator* cContactPropagator::gpGator = NULL; 

cContactPropagator::cContactPropagator()
   : cBasePropagator(&contact_desc),
     pSensors(AppGetObj(IStimSensors)),
     pSources(AppGetObj(IStimSources)),
     pImpactEvent(NULL)
{
   AutoAppIPtr_(DamageModel,pDmgModel);
   pDmgModel->Listen(kDamageMsgImpact,ImpactListener,this);
   AddReactions(); 
   gpGator = this; 
   
}

////////////////////////////////////////

cContactPropagator::~cContactPropagator()
{
   SafeRelease(pSources);
   SafeRelease(pSensors);
}

//------------------------------------------------------------
// IPROPAGATOR METHODS
//

STDMETHODIMP cContactPropagator::InitSource(sStimSourceDesc* desc)
{
   memset(desc,0,sizeof(*desc));
   desc->propagator = ID;
   desc->valid_fields = kStimSrcShapeValid;
   sContactSourceShape* shape = (sContactSourceShape*)&desc->shape; 
   shape->flags = kPropagateCollide;
   return S_OK;
}

//------------------------------------------------------------
// Shape Description
//

static const char* contact_bits[] = 
{
   "Collision",
   "Frob in World",
   "Frob in Inv",
   "Frob Inv -> World",
   "Frob Inv -> Inv",
   "Frob World -> World",
   "Frob World -> Inv", 
   "Weapon Swing Low",
   "Weapon Swing Med",
   "Weapon Swing High",
   "Weapon Block",
};

static sFieldDesc shape_fields[] = 
{
   { "Contact Types", kFieldTypeBits, FieldLocation(sContactSourceShape,flags), kFieldFlagUnsigned, 0, kNumContactBits, kNumContactBits, contact_bits },
   { "Velocity Coeff", kFieldTypeFloat, FieldLocation(sContactSourceShape,coeffs[0]) } ,
   { "Frob Time Coeff", kFieldTypeFloat, FieldLocation(sContactSourceShape,coeffs[1]) }, 
   
}; 

static sStructDesc shape_desc = StructDescBuild(sContactSourceShape,kStructFlagNone,shape_fields);

STDMETHODIMP_(const struct sStructDesc*) cContactPropagator::DescribeShapes()
{
   return &shape_desc;
}

STDMETHODIMP_(const struct sStructDesc*) cContactPropagator::DescribeLifeCycles()
{
   Warning(("cContactPropagator::DescribeLifeCycles() not supported\n")); 
   return NULL;
}

////////////////////////////////////////

STDMETHODIMP cContactPropagator::SourceEvent(sStimSourceEvent* event)
{
   AssertMsg(pSources->GetSourceElems(event->id).obj == event->elems.obj,"Act/React Source event is out of synch"); 

   switch(event->type)
   {
      case kStimSourceCreate:
      {
         cSourceList* src = Sources.Search(event->elems.obj);
         if (src == NULL)
         {
            src = new cSourceList;
            Sources.Insert(event->elems.obj,src);
         }
         src->AddElem(event->id);
      }
      break;

      case kStimSourceDestroy:
      {
         cSourceList* src = Sources.Search(event->elems.obj);
         
         if (src)
         {
            if (!src->RemoveElem(event->id))
               Warning(("Failed attempt to remove source\n"));
               
            if (src->Size() == 0)
            {
               delete src;
               Sources.Delete(event->elems.obj); 
            }
            
         }
         else
            Warning(("No sources to remove from %d\n", event->elems.obj)); 

      }
      break;
   }
   return S_OK;
}

 
STDMETHODIMP_(tStimLevel) cContactPropagator::GetSourceLevel(StimSourceID id)
{
   sStimSourceDesc desc;
   pSources->DescribeSource(id,&desc);
   return desc.level;
}


STDMETHODIMP cContactPropagator::DescribeSource(StimSourceID id, sStimSourceDesc* desc)
{
   pSources->DescribeSource(id,desc);
   return S_OK;
}

STDMETHODIMP cContactPropagator::SensorEvent(sStimSensorEvent* event)
{
   // I get my sensors fresh each day
   return S_OK;
}


STDMETHODIMP cContactPropagator::Propagate(tStimTimeStamp /* t */, tStimDuration /* dt */)
{
   // I'm a totally passive propagator 
   return S_OK;
}


STDMETHODIMP cContactPropagator::Reset()
{
   Sources.RemoveAll();
   return S_OK;
}


//------------------------------------------------------------
// Helper functions
//

void cContactPropagator::DoPropagationEvent(ulong flagmask, ObjID from, ObjID to, float magnitude, const sChainedEvent* cause, ulong propflags)
{
   cSourceList* srclist = Sources.Search(from);
   if (srclist != NULL)
   {
      cSourceList::cIter iter;
      // Iterate over all sources, looking for matching sensors

      for (iter = srclist->Iter(); !iter.Done(); iter.Next())
      {  
         StimSourceID srcid = iter.Value();

         sObjStimPair pair = pSources->GetSourceElems(srcid);

         AssertMsg2(from == pair.obj, "Act/React source %X does not match obj %d\n",srcid,from); 

         StimID stim = pair.stim;
         StimSensorID sensid = pSensors->LookupSensor(to,stim);

         // we have a sensor, so stimulate it
         if (sensid != SENSORID_NULL)
         {
            sStimSourceDesc desc;   
            pSources->DescribeSource(srcid,&desc);
            sContactSourceShape* shape = (sContactSourceShape*)&desc.shape; 
            if (shape->flags & flagmask)
            {
               sStimEventData evdat = { stim, desc.level, 0, sensid, srcid, GetSimTime() };
               sStimEvent event(&evdat,(sChainedEvent*)cause); 
 
               if (propflags & kPhysical)
               {
                  // divide out sensor's mass
                  if (PhysObjHasPhysics(to))
                     magnitude /= PhysGetMass(to); 

                  event.intensity += shape->coeffs[kVelocityCoeff] * magnitude; 
               }
               else
                  event.intensity += shape->coeffs[kFrobTimeCoeff] * magnitude; 

               if (propflags & kNoDefer)
                  event.flags |= kStimEventNoDefer; // Don't defer collide events

               pStimulator->StimulateSensor(sensid,&event); 
            }
         }
      }

   }
}

eDamageResult cContactPropagator::HandleImpact(const sDamageMsg* msg)
{
   // @TODO: make this thread-safe
   ImpactResult = kDamageNoOpinion; 
   pImpactEvent = msg; 

   DoPropagationEvent(kPropagateCollide,msg->culprit,msg->victim,msg->data.impact->magnitude,(sDamageMsg*)msg,kNoDefer|kPhysical); 

   pImpactEvent = NULL;
   return ImpactResult; 
}


eDamageResult cContactPropagator::ImpactListener(const sDamageMsg* msg, tDamageCallbackData data)
{
   if (msg->culprit == OBJ_NULL || msg->victim == OBJ_NULL)
      return kDamageNoOpinion;
   cContactPropagator* us = (cContactPropagator*)data;
   return us->HandleImpact(msg);
}

//----------------------------------------
// "COLLISION RESULT" effect
//

struct sCollParam 
{
   eDamageResult result; 
}; 

eReactionResult LGAPI cContactPropagator::ImpactResultReaction(sReactionEvent* event, const sReactionParam* param, tReactionFuncData data)
{
   cContactPropagator* us = (cContactPropagator*)data;
   return us->SetImpactResult(event,param); 
}


eReactionResult cContactPropagator::SetImpactResult(sReactionEvent* event, const sReactionParam* param)
{
   if (pImpactEvent && pImpactEvent->victim == event->sensor_obj)
   {
      sCollParam* coll = (sCollParam*)&param->data; 
      ImpactResult = coll->result; 
   }
   return kReactionNormal; 
}

//
// Reaction desc
//

static sReactionDesc impact_rdesc =
{
   "impact_result",
   "Impact Result",
    REACTION_PARAM_TYPE(sCollParam),
};


//
// Param sdesc
//

static const char* coll_results[] = { "Bounce", "Stick", "Slay" }; 

#define NUM_COLL_RESULTS (sizeof(coll_results)/sizeof(coll_results[0]))

static sFieldDesc collparm_fields[] = 
{
   { "Result", kFieldTypeEnum, FieldLocation(sCollParam,result), kFieldFlagUnsigned, kImpactBounce,kDamageSlay, NUM_COLL_RESULTS, coll_results }, 
};

static sStructDesc collparm_sdesc = StructDescBuild(sCollParam,kStructFlagNone,collparm_fields); 

void cContactPropagator::AddReactions()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&collparm_sdesc); 

   AutoAppIPtr(Reactions); 
   pReactions->Add(&impact_rdesc,ImpactResultReaction,this); 
}

//------------------------------------------------------------
// CLASS: cSourceTable 
//

void cContactPropagator::cSourceTable::RemoveAll()
{
   for (cIter iter = Iter(); !iter.Done(); iter.Next())
   {
      delete iter.Value();
   }
   Clear();
}


//------------------------------------------------------------
// Factory
//

IPropagator* CreateContactPropagator(void)
{
   return new cContactPropagator;
}
