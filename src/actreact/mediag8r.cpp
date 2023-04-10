/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/mediag8r.cpp,v 1.2 1998/09/06 00:36:34 mahk Exp $
#include <appagg.h>
#include <mediag8r.h>
#include <periog8r.h>
#include <pg8rbase.h>
#include <wr.h>
#include <flowarch.h>
#include <propman.h>
#include <phmedtr.h>
#include <physapi.h>
#include <objpos.h>
#include <contain.h>
#include <config.h>
#include <cfgdbg.h>
#include <simman.h>
#include <dspchdef.h>
#include <simdef.h>
#include <iobjsys.h>
#include <objquery.h>
#include <objdef.h>

// permeation
#include <reacbase.h>
#include <reaction.h>
#include <sdesbase.h>
#include <sdestool.h>
#include <stimul8r.h>
#include <stimsens.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <medg8rid.h>

#define FLOW_CONTACT_RELATION "FlowContact"
#define FLOW_GATOR_NAME "Flow"

static IPeriodicPropagator* gpFlowGator = NULL; 
static IFlowGroupProperty* gpFlowProp = NULL; 
static IContainSys* gpContainSys = NULL; 
static IStimulator* gpStimulator = NULL; 
static IStimSensors* gpSensors = NULL; 

//
// Update contacts
//
static void update_contacts(ObjID obj, int cell, ObjID bag = -1)
{

   if (bag == -1)
      bag = gpContainSys->GetContainer(obj); 

   ConfigSpew("media_contact_spew",("Media contacts obj: %d cell: %d bag: %d\n",obj,cell,bag)); 

   ObjID contact = OBJ_NULL; 

   if (bag != OBJ_NULL)
   {
      contact = bag; 
   }
   else if (cell >= 0)
   {
      int idx = WR_CELL(cell)->motion_index; 
      if (idx > 0) // valid flow group 
      {
         // find the flow archetype 
         ObjID flow = gpFlowProp->GetObj(idx);
   
         // initiate the new contact 
         contact = flow; 
      }
   }
   if (contact != OBJ_NULL)
      gpFlowGator->SetSingleSensorContact(contact,obj); 
   else
      gpFlowGator->EndContact(OBJ_NULL,obj); 
}

//
// Physics callback
//

static void LGAPI media_handler(const sPhysMediaEvent* event, void* )
{
   int bag = -1; 
   // supress container logic if I actually have a cell
   if (event->cell >= 0) bag = OBJ_NULL; 
   update_contacts(event->obj,event->cell,bag);
}

//
// Position callback
//

static void obj_pos_handler(ObjID obj, const ObjPos* pos, void* )
{
   if (pos && !PhysObjHasPhysics(obj))
      update_contacts(obj,CellFromLoc((Location*)&pos->loc));
}

//
// Sim callback
//

static void sim_handler(const sDispatchMsg* msg, const sDispatchListenerDesc* )
{
   switch (msg->kind)
   {
      case kSimInit:
      case kSimResume:
      {
         // Update all concrete objects
         AutoAppIPtr_(ObjectSystem,pObjSys); 
         IObjectQuery* query = pObjSys->Iter(kObjectConcrete); 
         for (; !query->Done(); query->Next())
         {
            ObjID obj = query->Object(); 
            ObjPos* pos = ObjPosGet(obj); 
            if (pos)
               update_contacts(obj,CellFromLoc(&pos->loc)); 
         }
         SafeRelease(query); 
      }
      break; 
         
   }; 
}

static sDispatchListenerDesc sim_desc = 
{
   &SIMID_MediaPropagator,
   kSimInit|kSimResume, 
   sim_handler,
}; 


//
// Container callback 
//

static BOOL contain_handler(eContainsEvent event, ObjID outer, ObjID inner, eContainType , ContainCBData )
{
   switch (event)
   {
      case kContainAdd:
         update_contacts(inner,-1,outer); 
         break;
   }  

   return TRUE; 
}

//------------------------------------------------------------
// "Permeate" reaction
//

struct sPermeate
{
   float coeff; 
   float cutoff; 
}; 

eReactionResult LGAPI permeate_func(sReactionEvent* ev, const sReactionParam* param, tReactionFuncData data)
{
   sPermeate* perm = (sPermeate*)&param->data;
   sStimEvent event = ev->stim; 
   
   event.intensity *= perm->coeff; 
   if (event.intensity < perm->cutoff && event.intensity > -perm->cutoff) 
      return kReactionNormal; 

   // Find the sensors
   ObjID container = param->obj[kReactDirectObj];

   sContainIter* iter = gpContainSys->IterStart(container); 
   for (;!iter->finished; gpContainSys->IterNext(iter))
   {
      StimSensorID sensid = gpSensors->LookupSensor(iter->containee,event.kind);
      if (sensid != SENSORID_NULL)
         gpStimulator->StimulateSensor(sensid,&event);
   }
   gpContainSys->IterEnd(iter); 
   
   return kReactionNormal;
}


static sReactionDesc permeate_desc = 
{
   "permeate",
   "Permeate Into Container", 
   REACTION_PARAM_TYPE(sPermeate), 
   kReactionHasDirectObj, 
};

static sFieldDesc permeate_fields[] = 
{
   { "Coefficient", kFieldTypeFloat, FieldLocation(sPermeate,coeff) },
   { "Minimum Magnitude", kFieldTypeFloat, FieldLocation(sPermeate,cutoff) },
}; 

static sStructDesc permeate_sdesc = StructDescBuild(sPermeate,kStructFlagNone,permeate_fields);

static void add_reaction()
{
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&permeate_sdesc); 
   AutoAppIPtr(Reactions); 
   pReactions->Add(&permeate_desc,&permeate_func,NULL); 
}

//
// Init/Term
//

void MediaPropagatorInit()
{
   sPropagatorDesc flowdesc = { FLOW_GATOR_NAME, kPGatorAllStimuli}; 
   gpFlowGator = CreatePeriodicPropagator(&flowdesc,FLOW_CONTACT_RELATION); 
   //   gpPermeator = new cPermeator; 
   //   gpFlowGator->Connect((IUnknown*)gpPermeator); 
   gpStimulator = AppGetObj(IStimulator); 
   gpSensors = AppGetObj(IStimSensors); 

   AutoAppIPtr_(PropertyManager,pPropMan); 
   cAutoIPtr<IProperty> prop (pPropMan->GetPropertyNamed(PROP_FLOW_GROUP_NAME)); 
   Verify(SUCCEEDED(prop->QueryInterface(IID_IFlowGroupProperty,(void**)&gpFlowProp)));

   gpContainSys = AppGetObj(IContainSys); 
   gpContainSys->Listen(OBJ_NULL,contain_handler,NULL); 

   PhysListenToMediaEvents(media_handler,NULL); 
   ObjPosListen(obj_pos_handler,NULL); 

   AutoAppIPtr_(SimManager,pSimMan); 
   pSimMan->Listen(&sim_desc); 

   add_reaction(); 
}

void MediaPropagatorTerm()
{
   SafeRelease(gpFlowGator); 
   SafeRelease(gpFlowProp); 
   SafeRelease(gpContainSys); 
   SafeRelease(gpStimulator); 
   SafeRelease(gpSensors); 
}





