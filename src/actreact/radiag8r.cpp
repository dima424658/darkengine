/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/radiag8r.cpp,v 1.5 2000/02/24 23:39:32 mahk Exp $
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

#include <radiag8r.h>
#include <objpos.h>
#include <bspsphr.h>
#include <refsys.h>
#include <osysbase.h>
#include <math.h>
#include <wr.h>
#include <ssrclife.h>
#include <iobjsys.h>
#include <simtime.h>
#include <physcast.h>

#include <dlisttem.h>

#include <string.h>

#include <sdesbase.h>
#include <sdestool.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// CLASS: cRadiusPropagator
//


//------------------------------------------------------------
// Construction/Destruction 
//

// Our propagator descriptor

static sPropagatorDesc radius_desc = 
{
   "Radius",
   kPGatorAllStimuli, 
};

////////////////////////////////////////

cRadiusPropagator::cRadiusPropagator()
   : cBasePropagator(&radius_desc),
     mpSensors(AppGetObj(IStimSensors)),
     mpSources(AppGetObj(IStimSources)),
     mLastTime(0),
     mRemoving(0)
{
   
}

////////////////////////////////////////

cRadiusPropagator::~cRadiusPropagator()
{
   SafeRelease(mpSources);
   SafeRelease(mpSensors);
}

//------------------------------------------------------------
// IPROPAGATOR METHODS
//

STDMETHODIMP cRadiusPropagator::InitSource(sStimSourceDesc* desc)
{
   static sShape def_shape = { 1.0, kRaycast, kDisperseNone }; 
   static sLifeCycle def_life = { 0, 5000, 1, 0.0 }; 

   memset(desc,0,sizeof(*desc));
   desc->propagator = ID;
   desc->valid_fields = kStimSrcShapeValid|kStimSrcLifeValid;
   *(sShape*)&desc->shape = def_shape; 
   *(sLifeCycle*)&desc->life = def_life; 
   return S_OK;
}

//------------------------------------------------------------
// Shape Description
//

static const char* radius_bits[] = 
{
   "Line of Sight (raycast)",
};

#define NUM_BITS (sizeof(radius_bits)/sizeof(radius_bits[0]))

static const char* disperse_names[] = 
{
   "None",
   "Linear",
   "Inverse Quadratic",
}; 

#define NUM_DISPERSE (sizeof(disperse_names)/sizeof(disperse_names[0]))


typedef cRadiusPropagator::sShape sRadiusSourceShape; 

static sFieldDesc shape_fields[] = 
{
   { "Radius", kFieldTypeFloat, FieldLocation(sRadiusSourceShape,radius), kFieldFlagUnsigned, },
 
   { "Flags", kFieldTypeBits, FieldLocation(sRadiusSourceShape,flags), kFieldFlagUnsigned, 0, NUM_BITS, NUM_BITS, radius_bits },
   { "Dispersion", kFieldTypeEnum, FieldLocation(sRadiusSourceShape,dispersion), kFieldFlagUnsigned, 0, NUM_DISPERSE, NUM_DISPERSE, disperse_names },
   
}; 

static sStructDesc shape_desc = StructDescBuild(sRadiusSourceShape,kStructFlagNone,shape_fields);

////////////////////////////////////////

STDMETHODIMP_(const struct sStructDesc*) cRadiusPropagator::DescribeShapes()
{
   return &shape_desc;
}

STDMETHODIMP_(const struct sStructDesc*) cRadiusPropagator::DescribeLifeCycles()
{
   return sLifeCycle::gpDesc;
}

////////////////////////////////////////


#define NIGH_INFINITY 256

void cRadiusPropagator::GenerateOneEvent(tStimTimeStamp t, StimSourceID srcid, const sStimSourceDesc* desc, const sObjStimPair& elems, ObjID sensobj, StimSensorID sensid,ulong flags)
{
   const sShape & shape = *(sShape*)&desc->shape; 

   sStimEventData evdata = { elems.stim, 0.0, 0.0, sensid, srcid, t, flags}; 

   ObjPos* srcpos =  ObjPosGet(elems.obj);
   ObjPos* senspos = ObjPosGet(sensobj); 


   float distsq = (srcpos && senspos) ? mx_dist2_vec(&srcpos->loc.vec,&senspos->loc.vec) : 1.0;  
   float radsq = shape.radius*shape.radius; 
   if (distsq > radsq)
      return; 

   if (shape.flags & kRaycast)
   {
      Location hit;
      ObjID hit_obj = sensobj;  

      if (PhysRaycast(srcpos->loc,senspos->loc,&hit,&hit_obj,0.0) && hit_obj != sensobj)
          return; 
   }

   switch (shape.dispersion)
   {
      case kDisperseNone:
         evdata.intensity = desc->level; 
         break; 
      case kDisperseLinear:
         evdata.intensity = desc->level - desc->level*sqrt(distsq)/shape.radius; 
         break;
      case kDisperseInvSquare:
      {
         float k = radsq / NIGH_INFINITY; 

         // clamp at desc->level 
         if (distsq < k)
            evdata.intensity = desc->level; 
         else
            evdata.intensity = desc->level * k / distsq; 
      }
      break; 

   }

   sStimEvent event(&evdata); 
   pStimulator->StimulateSensor(sensid,&event); 

}


////////////////////////////////////////

void cRadiusPropagator::GenerateEvents(tStimTimeStamp t, StimSourceID srcid, const sStimSourceDesc* desc, ulong flags)
{

   sObjStimPair elems = mpSources->GetSourceElems(srcid); 
   const sShape & shape = *(sShape*)&desc->shape; 

   ObjPos* pos = ObjPosGet(elems.obj);
   
   if (!pos)
      return; 

   int cells[BSPSPHR_OUTPUT_LIMIT]; 
   int n = portal_cells_intersecting_sphere(&pos->loc,shape.radius,cells); 

   // if you thought that array was decadent, check out this one 
   bool visited[HACK_MAX_OBJ]; 
   memset(visited,0,sizeof(visited));

   for (int c = 0; c < n; c++)
   {
      PortalCell* cell = WR_CELL(cells[c]); 
      ObjRefID id = *(int*)&cell->refs; 
      while (id)
      {
         ObjRef* r = OBJREFID_TO_PTR(id); 
         ObjID obj = r->obj; 
         
         if (!visited[obj])
         {
            StimSensorID sensid = mpSensors->LookupSensor(obj,elems.stim);
            if (sensid)
               GenerateOneEvent(t,srcid,desc,elems,obj,sensid,flags); 
            visited[obj] = TRUE; 
         }
         id = r->next_in_bin; 
      }
   }
}

////////////////////////////////////////

STDMETHODIMP cRadiusPropagator::SourceEvent(sStimSourceEvent* event)
{
   switch(event->type)
   {
      case kStimSourceCreate:
         if (InSim)
         {
            tStimTimeStamp birth = mpSources->GetSourceBirthDate(event->id); 
            sSourceElem elem = { event->id, birth, TRUE }; 
            mQueue.AddElem(elem); 
         }
         break;

      case kStimSourceDestroy:
         if (InSim && event->id != mRemoving)
            mQueue.RemoveID(event->id); 
         break; 
   }
   return S_OK;
}

 
STDMETHODIMP_(tStimLevel) cRadiusPropagator::GetSourceLevel(StimSourceID id)
{
   sStimSourceDesc desc;
   mpSources->DescribeSource(id,&desc);
   return desc.level;
}


STDMETHODIMP cRadiusPropagator::DescribeSource(StimSourceID id, sStimSourceDesc* desc)
{
   mpSources->DescribeSource(id,desc);
   return S_OK;
}

STDMETHODIMP cRadiusPropagator::SensorEvent(sStimSensorEvent* event)
{
   // I get my sensors fresh each day
   return S_OK;
}


STDMETHODIMP cRadiusPropagator::Propagate(tStimTimeStamp t, tStimDuration /* dt */)
{
   cSourceQueue::cIter iter;
   tStimTimeStamp last = mLastTime; 
   mLastTime = t; 

   cSourceQueueBase requeue_list; 

   for (iter = mQueue.Iter(); !iter.Done(); iter.Next())
   {
      sSourceElem elem = iter.Value(); 

      if (elem.time > t) // everything is in the future
         break; 

      sStimSourceDesc desc; 
      mpSources->DescribeSource(elem.id,&desc);
      
      sLifeCycle& life = *(sLifeCycle*)&desc.life; 
      tStimTimeStamp birth = mpSources->GetSourceBirthDate(elem.id); 

      sLifeCycle::sFire* fire = life.BeginFiring(elem.first,desc.level,birth,last,t); 
      tStimTimeStamp ftime; 
      ulong flags; 
      while (life.FireNext(fire,&desc.level,&ftime,&flags))
         GenerateEvents(ftime,elem.id,&desc,flags); 
      BOOL requeue = life.EndFiring(fire); 

      iter.Value().first = FALSE; 

      cSourceQueue::cNode& node = iter.Node(); 

      if (requeue)
      {  
         node.Value().time = ftime; 
         mQueue.Remove(&node); 
         requeue_list.AppendNode(&node); 
      }
      else
      {
         mQueue.Delete(node); 
      }
   
   }

   // requeue
   cSourceQueueBase::cIter riter;
   for (riter = requeue_list.Iter(); !riter.Done(); riter.Next())
   {
      cSourceQueueBase::cNode& node = riter.Node(); 
      requeue_list.Remove(&node); 
      mQueue.AddNode(&node); 
   }

   return S_OK;
}


STDMETHODIMP cRadiusPropagator::Reset()
{
   mQueue.DestroyAll(); 
   mLastTime = 0; 
   return S_OK;
}

STDMETHODIMP cRadiusPropagator::Start()
{
   mLastTime = GetSimTime(); 

   // recompute pending 
   mQueue.DestroyAll(); 
   IStimSourceQuery* query = mpSources->QuerySources(OBJ_NULL,OBJ_NULL); 
   for (; !query->Done(); query->Next())
   {
      StimSourceID srcid = query->ID(); 
      const sStimSourceDesc* desc = query->Source(); 
      if (desc->propagator == ID)
      {
         tStimTimeStamp birth = mpSources->GetSourceBirthDate(srcid); 
         sSourceElem elem = { srcid, birth, birth >= mLastTime }; 
         mQueue.AddElem(elem); 
      }
   }
   SafeRelease(query); 

   return cBasePropagator::Start();
}

STDMETHODIMP cRadiusPropagator::Stop()
{
   mQueue.DestroyAll(); 
   return cBasePropagator::Stop(); 
}

//------------------------------------------------------------
// Source Queue Member Functions
//

void cRadiusPropagator::cSourceQueue::AddNode(cNode* elem)
{
   // really, we want to iterate backwards, because we expect our 
   // new node will be at the back. 
   for (cNode* node = GetLast(); node != NULL; node = node->GetPrevious())
      if (elem->Value().time >= node->Value().time)
      {
         // call the cparent, because we already have the memory for the node
         cParent::InsertAfter(node,elem); 
         return; 
      }
   cParent::Prepend(elem); 
}

void cRadiusPropagator::cSourceQueue::AddElem(const sSourceElem& elem) 
{ 
   AddNode( new cNode(elem)); 
}

void cRadiusPropagator::cSourceQueue::RemoveID(StimSourceID id)
{
   for (cIter iter = Iter(); !iter.Done(); iter.Next())
   {
      if (iter.Value().id == id)
      {
         Delete(iter.Node());
         break;
      }
   }
}

//------------------------------------------------------------
// Factory
//

IPropagator* CreateRadiusPropagator(void)
{
   return new cRadiusPropagator;
}


