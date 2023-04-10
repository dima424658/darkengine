/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimsens.cpp,v 1.28 1998/10/24 00:55:35 mahk Exp $
#include <appagg.h>

#include <stimtype.h>
#include <stimbase.h>

#include <stimsen_.h>
#include <senstype.h>
#include <sensbase.h>
#include <receptro.h>
#include <reaction.h>

#include <pg8rtype.h>
#include <pg8rbase.h>
#include <propag8n.h>

#include <linkman.h>
#include <linkint.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <lnkbyobj.h>
#include <dataops_.h>
#include <bintrait.h>
#include <linkid.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <objquery.h>
#include <osysbase.h>
#include <objremap.h>
#include <iobjsys.h>

#include <traitbas.h>
#include <traitman.h>
#include <trait.h>

#include <propman.h>
#include <proptype.h>

#include <dbasemsg.h>
#include <objnotif.h>

#include <osetlnkq.h>
#include <stimqfct.h>

#include <hshpptem.h>
#include <dlisttem.h>

#include <allocapi.h>


// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
//
// cStimSensors Implementation
//
//

IMPLEMENT_AGGREGATION_SELF_DELETE(cStimSensors);

static sRelativeConstraint Constraints[] =
{
   { kConstrainAfter, &IID_IStimuli},
   { kConstrainAfter, &IID_IPropagation},
   { kConstrainAfter, &IID_ILinkManager},
   { kConstrainAfter, &IID_ITraitManager},

   { kNullConstraint}
};

#define INITIAL_ORDER 1


cStimSensors::cStimSensors(IUnknown* pOuter)
   : pStimuli(NULL),
     pPropagation(NULL),
     pTraitMan(NULL),
     pReceptrons(NULL),
     pSensors(NULL),
     pObjParams(NULL),
     NextOrder(INITIAL_ORDER)
{
   INIT_AGGREGATION_1(pOuter,IID_IStimSensors,this,kPriorityNormal,Constraints);
}

cStimSensors::~cStimSensors()
{
}

//------------------------------------------------------------
// SENSORS RELATION
//

//
// Descriptor
//

static sRelationDesc sensors_desc =
{
   "StimSensor",
   kRelationNoEdit  // no flags
};

//
// Satellite data type and descriptor
//

typedef ulong tSensorCount;

StructDescDefineSingleton(SensorCountDesc,tSensorCount,kFieldTypeInt,kFieldFlagUnsigned);

static sRelationDataDesc sensors_ddesc = LINK_DATA_DESC(tSensorCount);

//
// Listener
//

static tStimSourceEvent LinkMsg2SensMsg(eRelationListenMsg msg)
{
   switch(msg)
   {
      case kListenLinkBirth:
         return kStimSensorCreate;
      case kListenLinkDeath:
         return kStimSensorDestroy;

      default:
         Warning(("Stim Sens listener got message %X\n",msg));
   }
   return 0xFFFFFFFF;
}

// convert a link message to a sensor message (linkbirth -> sensorcreate etc)
void cStimSensors::SensorListener(sRelationListenMsg* msg, RelationListenerData data)
{
   cStimSensors* us = (cStimSensors*)data;
   sStimSensorEvent event;

   event.id = us->Link2Sensor(msg->id);
   event.type = LinkMsg2SensMsg(msg->type);
   event.elems.obj = msg->link.source;
   event.elems.stim = msg->link.dest;

   us->pPropagation->SensorEvent(&event);
}


//
// Factory
//

// these are the query cases we want optimized
#define SENSORS_QUERY_CASES (kQCaseSetSourceKnown|kQCaseSetBothKnown)

void cStimSensors::CreateSensorsRelation()
{
   StructDescRegister(&SensorCountDesc);
   pSensors = CreateStandardRelation(&sensors_desc,&sensors_ddesc,SENSORS_QUERY_CASES);
   pSensors->Listen(kListenLinkBirth|kListenLinkDeath,SensorListener,this);
}


//------------------------------------------------------------
// RECEPTRONS RELATION
//

//
// Descriptor
//

static sRelationDesc receptrons_desc =
{
   "Receptron",
   kRelationNoEdit, // flags
   1, // version
   1, // oldest acceptable version
};

//
// Satellite data type and descriptor
//

static sFieldDesc receptron_fields[] =
{
   { "Order", kFieldTypeInt, FieldLocation(sReceptron,order), },
   { "Trigger min", kFieldTypeFloat, FieldLocation(sReceptron,trigger.min), },
   { "Trigger max", kFieldTypeFloat, FieldLocation(sReceptron,trigger.max), },
   { "Trigger flags", kFieldTypeBits, FieldLocation(sReceptron,trigger.flags), },
   { "Reaction", kFieldTypeInt, FieldLocation(sReceptron,effect.kind), },
   { "Direct Object", kFieldTypeInt, FieldLocation(sReceptron,effect.param.obj[0]) },
   { "Indirect Object", kFieldTypeInt, FieldLocation(sReceptron,effect.param.obj[1]) },
   { "Reaction param", kFieldTypeString, FieldLocation(sReceptron,effect.param.data), },
};

static sStructDesc ReceptronDesc = StructDescBuild(sReceptron,kStructFlagNone,receptron_fields);

static sRelationDataDesc receptrons_ddesc = LINK_DATA_DESC(sReceptron);

//
// Listener
//

void cStimSensors::ReceptronListener(sRelationListenMsg* msg, RelationListenerData data)
{
   cStimSensors* us = (cStimSensors*)data;
   switch (msg->type)
   {
      case kListenLinkBirth:
         us->AddAllSensors(msg->link.source,msg->link.dest);
         us->AddObjParams(msg->id,msg->link.source);
         break;
      case kListenLinkDeath:
         us->RemoveAllSensors(msg->link.source,msg->link.dest);
         us->RemObjParams(msg->id,msg->link.source);
         break;
   }
}

//
// Special query database that sorts links by "order" field
//

class OrderedLinksByObj : public LinksByObj
{
protected:
   int CompareLinks(LinkID l1, LinkID l2)
   {
      const sReceptron* r1 = (sReceptron*)LinkMan()->GetData(l1);
      const sReceptron* r2 = (sReceptron*)LinkMan()->GetData(l2);

      return (int)r1->order - (int)r2->order;
   }

};

//
// Data ops for reading and writing receptron data
//

class cReceptronDataOps : public cClassDataOps<sReceptron> 
{
   IReactions* Reactions;

   enum eVersion
   {
      kVerInitial     = 1,
      kVerObjParamsAdded,  // Added special object params to sReactionParam
      
      kVerCurrent = kVerObjParamsAdded,  // current version
      kVerLast = kVerInitial, // Last acceptible version
   };

   static eReactionResult LGAPI DummyReaction(sReactionEvent* event, const sReactionParam* param, tReactionFuncData);

public:
   cReceptronDataOps()
      :Reactions(AppGetObj(IReactions))
   {
   }

   ~cReceptronDataOps()
   {
      SafeRelease(Reactions);
   }

   STDMETHOD_(int,Version)() { return kVerCurrent;};

   STDMETHOD_(long,Write)(sDatum dat, IDataOpsFile* file)
   {
      const sReceptron* tron = (const sReceptron*)dat.value;

      int len = 0;
      // write out order and trigger literally
      len += file->Write(&tron->order,sizeof(tron->order));
      len += file->Write(&tron->trigger,sizeof(tron->trigger));

      // swizzle kind into a name
      const sReactionDesc* react = Reactions->DescribeReaction(tron->effect.kind);
      len += file->Write(react->name,sizeof(react->name));
      // write out param
      len += file->Write(&tron->effect.param,sizeof(tron->effect.param));

      return (len == sizeof(tron->order)
              + sizeof(tron->trigger)
              + sizeof(react->name)
              + sizeof(tron->effect.param)) ? S_OK : E_FAIL; 
   }

   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* file, int version)
   {
      if (version < kVerLast)
         return FALSE;

      if (!dat->value) 
         *dat = New(); 
      sReceptron* tron = (sReceptron*) dat->value; 
      memset(tron,0,sizeof(*tron));

      int len = 0;
      // read in order and trigger literally
      len += file->Read(&tron->order,sizeof(tron->order));
      len += file->Read(&tron->trigger,sizeof(tron->trigger));

      // ok, here we wastefully put a whole desc on the stack even though we only
      // use the "name" field.
      sReactionDesc react;

      // read in effect name, convert into kind
      len += file->Read(react.name,sizeof(react.name));
      tron->effect.kind = Reactions->GetReactionNamed(react.name);

      if (tron->effect.kind == REACTION_NULL)
      {
         // add a dummy reaction with that name
         sReactionDesc dummy = { "" , NO_REACTION_PARAM, };
         strncpy(dummy.name,react.name,sizeof(dummy.name));
         dummy.name[sizeof(dummy.name)] = '\0';

         tron->effect.kind = Reactions->Add(&dummy,DummyReaction,NULL);
      }

      sReactionParam* param = &tron->effect.param;
      // read in param.  Version difference
      if (version == kVerInitial)
      {
         // Just read in "data" field
         len += file->Read(&param->data,sizeof(param->data));
      }         
      else
      {
         len += file->Read(param,sizeof(*param));
         
         // Remap obj ID's on load
         for (int i = 0; i < kReactNumObjs; i++)
            param->obj[i] = ObjRemapOnLoad(param->obj[i]);
      }

      BOOL success = len == sizeof(tron->order)
         + sizeof(tron->trigger)
         + sizeof(react.name)
         + sizeof(tron->effect.param);

      if (!success)
      {
         Delete(*dat); 
         dat->value = NULL; 
         return E_FAIL; 
      }
      return S_OK; 

   }

};

eReactionResult LGAPI cReceptronDataOps::DummyReaction(sReactionEvent* , const sReactionParam* , tReactionFuncData )
{
   return kReactionNormal;
}

//
// Factory
//

// these are the query cases we want optimized
#define RECEPTRONS_QUERY_CASES (kQCaseSetSourceKnown|kQCaseSetBothKnown)

void cStimSensors::CreateReceptronsRelation()
{
   LGALLOC_AUTO_CREDIT(); 
   StructDescRegister(&ReceptronDesc);

   sCustomRelationDesc desc;
   memset(&desc,0,sizeof(desc));

   OrderedLinksByObj* table = new OrderedLinksByObj;

   // optimize source known & both known
   ILinkQueryDatabase* fromdb = new cFromObjQueryDatabase(table,TRUE);
   desc.DBs[kRelationSourceKnown] = fromdb; 
   desc.DBs[kRelationBothKnown] = desc.DBs[kRelationBothKnown];

   // create data ops and stuff into data desc
   receptrons_ddesc.data_ops = new cReceptronDataOps;

   pReceptrons = CreateCustomRelation(&receptrons_desc,&receptrons_ddesc,&desc);
   pReceptrons->Listen(kListenLinkBirth|kListenLinkDeath,ReceptronListener,this);

   SafeRelease(fromdb); 
}


//------------------------------------------------------------
// OBJECT PARAMS RELATION
//
// This relation is used when a receptron effect has a direct or indirect object, 
// so that we can track destruction of the objects.
//  

//
// Descriptor
//

static sRelationDesc objparams_desc =
{
   "ReactParam",
   kRelationNoEdit|kRelationTransient,
   1, // version
   1, // oldest acceptible
};

//
// Satellite data type and descriptor
//


StructDescDefineSingleton(ObjParamDesc,ReceptronID,kFieldTypeInt,kFieldFlagUnsigned|kFieldFlagHex);

static sRelationDataDesc objparams_ddesc = LINK_DATA_DESC(ReceptronID);

//
// Data ops
//

class cObjParamOps : public cSimpleDataOps
{
   ILinkManager* mpLinkMan; 

public:
   cObjParamOps()
      : mpLinkMan(AppGetObj(ILinkManager))
   {
   }

   ~cObjParamOps()
   {
      SafeRelease(mpLinkMan); 
   }

   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* file, int version)
   {
      HRESULT result = cSimpleDataOps::Read(dat,file,version); 

      // Remap the link id
      if (SUCCEEDED(result))
      {
         LinkID id = (LinkID)dat->value; 
         dat->value = (void*)mpLinkMan->RemapOnLoad(id); 
      }
      return result; 
   }

};

//
// Listener
//


// convert a link message to a sensor message (linkbirth -> sensorcreate etc)
void cStimSensors::ObjParamListener(sRelationListenMsg* msg, RelationListenerData data)
{
   cStimSensors* us = (cStimSensors*)data;

   switch (msg->type)
   {
      case kListenLinkDeath:
      {
         ReceptronID id = *(ReceptronID*)us->pObjParams->GetData(msg->id);
         if (id != RECEPTRON_NULL)
            us->RemoveReceptron(id);
      }
      break;
   }
}


//
// Factory
//

// these are the query cases we want optimized
#define OBJPARAMS_QUERY_CASES (kQCaseSetSourceKnown|kQCaseSetBothKnown)

void cStimSensors::CreateObjParamsRelation()
{
   StructDescRegister(&ObjParamDesc);
   pObjParams = CreateStandardRelation(&objparams_desc,&objparams_ddesc,OBJPARAMS_QUERY_CASES);
   pObjParams->Listen(kListenLinkDeath,ObjParamListener,this);
}

//------------------------------------------------------------
// AGGREGATE PROTOCOL
//

HRESULT cStimSensors::Init()
{
   // Grab the aggregate members we use
   pStimuli = AppGetObj(IStimuli);
   pPropagation = AppGetObj(IPropagation);
   pTraitMan = AppGetObj(ITraitManager);

   // Create our relations
   CreateReceptronsRelation();
   CreateSensorsRelation();
   CreateObjParamsRelation();

   // Listen to hierarchy changes
   pTraitMan->Listen(HierarchyListener,this); 

   // Create the generic receptron trait
   //   static const sTraitDesc desc = { "Receptron", 0}; 
   //   sTraitPredicate pred = { AnyStimTrait, this } ; 
   pAnyStimTrait = MakeTraitFromRelation(pReceptrons); 

   return S_OK;
}

HRESULT cStimSensors::End()
{
   SafeRelease(pStimuli);
   SafeRelease(pPropagation);
   SafeRelease(pTraitMan);
   SafeRelease(pReceptrons);
   SafeRelease(pSensors);
   SafeRelease(pObjParams);
   return S_OK;
}

//------------------------------------------------------------
// Helper functions
//

static LinkID singleton_link(IRelation* pRel, ObjID src, ObjID dest)
{
   cAutoIPtr<ILinkQuery> q ( pRel->Query(src,dest));
   if (q->Done()) return LINKID_NULL;
   LinkID result = q->ID();
#ifdef DBG_ON
   // check for singletonness
   q->Next();
   AssertMsg(q->Done(),"Non-singleton link");
#endif
   return result;
}

void cStimSensors::AddSensorLink(ObjID obj, ObjID stim)
{
   // check for an existing sensor link
   LinkID senslink = singleton_link(pSensors,obj,stim);
   if (senslink != LINKID_NULL) // make a new link
   {
      tSensorCount* pcnt = (tSensorCount*)pSensors->GetData(senslink);
      // bump sensor count
      (*pcnt)++;
      pSensors->SetData(senslink,pcnt);
   }
   else
   {
      tSensorCount cnt = 1;
      senslink = pSensors->AddFull(obj,stim,&cnt);
   }
}

void cStimSensors::AddSensor(ObjID obj, ObjID stimulus)
{
   Assert_(OBJ_IS_CONCRETE(obj) && OBJ_IS_ABSTRACT(stimulus));

   AddSensorLink(obj,stimulus);

   IObjectQuery* query = pStimuli->QueryHeirs(stimulus);
   for (; !query->Done(); query->Next())
   {
      AddSensorLink(obj,query->Object());
   }
   SafeRelease(query);

}



void cStimSensors::AddAllSensors(ObjID arch, ObjID stim)
{
   if (arch == OBJ_NULL)
      return ;
   if (OBJ_IS_CONCRETE(arch))
   {
      AddSensor(arch,stim);
      return;
   }
   IObjectQuery* q = pTraitMan->Query(arch,kTraitQueryAllDescendents);
   for (; !q->Done(); q->Next())
   {
      ObjID obj = q->Object();
      if (OBJ_IS_CONCRETE(obj))
         AddSensor(obj,stim);
   }
   SafeRelease(q);
}

void cStimSensors::RemoveSensorLink(ObjID obj, ObjID stim)
{
   // check for an existing sensor link
   LinkID senslink = singleton_link(pSensors,obj,stim);
   if (senslink != LINKID_NULL) // make a new link
   {
      tSensorCount* pcnt = (tSensorCount*)pSensors->GetData(senslink);
      // bump sensor count
      (*pcnt)--;
      if (*pcnt == 0)
         pSensors->Remove(senslink);
      else
         pSensors->SetData(senslink,pcnt);
   }
}

void cStimSensors::RemoveSensor(ObjID obj, ObjID stimulus)
{
   Assert_(OBJ_IS_CONCRETE(obj) && OBJ_IS_ABSTRACT(stimulus));

   RemoveSensorLink(obj,stimulus); 

   IObjectQuery* query = pStimuli->QueryHeirs(stimulus);
   for (; !query->Done(); query->Next())
   {
      RemoveSensorLink(obj,query->Object());
   }
   SafeRelease(query);
}

void cStimSensors::RemoveAllSensors(ObjID arch, ObjID stim)
{
   if (OBJ_IS_CONCRETE(arch))
   {
      RemoveSensor(arch,stim);
      return;
   }
   IObjectQuery* q = pTraitMan->Query(arch,kTraitQueryAllDescendents);
   for (; !q->Done(); q->Next())
   {
      ObjID obj = q->Object();
      if (OBJ_IS_CONCRETE(obj))
         RemoveSensor(obj,stim);
   }
   SafeRelease(q);
}

void cStimSensors::RecomputeSensors(void)
{
   // burn through the sensors setting their counts to zero.  This is
   // so that the id's will be preserved
   ILinkQuery* query = pSensors->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
   {
      tSensorCount count = 0;
      pSensors->SetData(query->ID(),&count);
   }
   SafeRelease(query);

   // Now burn through all receptrons, adding sensors
   query = pReceptrons->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
   {
      LinkID id = query->ID(); 
      sLink link;
      pReceptrons->Get(id,&link);
      AddAllSensors(link.source,link.dest); 
      AddObjParams(id,link.source); 
   }
   SafeRelease(query);

   // Now blast any zero-count sensors
   query = pSensors->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
   {
      tSensorCount count = *(tSensorCount*)query->Data();
      if (count == 0)
         pSensors->Remove(query->ID());
   }
   SafeRelease(query);
}


void cStimSensors::AddObjParams(LinkID tronlink, ObjID obj)
{
   // Blast the old links
   RemObjParams(tronlink,obj);

   // Now make new ones
   LinkID id = Link2Receptron(tronlink);
   sReceptron* tron = (sReceptron*)pReceptrons->GetData(tronlink);
   for (int i = 0; i < kReactNumObjs; i++)
   {
      ObjID param = tron->effect.param.obj[i];
      if (param != OBJ_NULL)
      {
         // Add a link
         pObjParams->AddFull(obj,param,&id);
      }
   }
}

void cStimSensors::RemObjParams(LinkID tronlink, ObjID obj)
{
   ReceptronID id = Link2Receptron(tronlink);
   // Blast all existing objparam links for this receptron
   ILinkQuery* q = pObjParams->Query(obj,LINKOBJ_WILDCARD);
   for (; !q->Done(); q->Next())
   {
      ReceptronID tron = *(ReceptronID*)q->Data();
      if (tron == id)
      {
         tron = RECEPTRON_NULL;
         pObjParams->SetData(q->ID(),&tron); // prevent deletion from deleting me. 
         pObjParams->Remove(q->ID());
      }
   }
   SafeRelease(q);
}

void cStimSensors::ResetOrder(void)
{
   NextOrder = INITIAL_ORDER;
   // burn through the receptrons, find the max order

   ILinkQuery* query = pReceptrons->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
   {
      const sReceptron* tron = (sReceptron*)query->Data();
      if (NextOrder <= tron->order)
         NextOrder = tron->order+1;
   }
   SafeRelease(query);
}


void cStimSensors::AddInheritanceLink(ObjID obj, ObjID newdonor)
{
   //
   // @TODO: when stimulus traits happen, pull the inside loop to the outside, and
   // take advantage of the cache
   //

   // get our new donors, and add sensors
   //   IObjectQuery* donors = pTraitMan->Query(newdonor,kTraitQueryAllDonors);
   IObjectQuery* donors = pAnyStimTrait->GetAllDonors(newdonor);

   for (; !donors->Done(); donors->Next())
   {
      ObjID donor = donors->Object();
      ILinkQuery* trons = pReceptrons->Query(donor,LINKOBJ_WILDCARD);
      for (; !trons->Done(); trons->Next())
      {
         sLink link;
         trons->Link(&link);

         ObjID stimulus = link.dest;
         //
         // Ok, at this point link is a receptron link that obj is
         // newly inheriting , so we AddAllSensors it
         //
         AddAllSensors(obj,stimulus);
      }
      SafeRelease(trons);
   }
   SafeRelease(donors);
}


void cStimSensors::RemoveInheritanceLink(ObjID obj, ObjID olddonor)
{
   //
   // @TODO: when stimulus traits happen, pull the inside loop to the outside, and
   // take advantage of the cache
   //

   // get our new donors, and add sensors
   IObjectQuery* donors = pAnyStimTrait->GetAllDonors(olddonor);
   for (; !donors->Done(); donors->Next())
   {
      ObjID donor = donors->Object();
      ILinkQuery* trons = pReceptrons->Query(donor,LINKOBJ_WILDCARD);
      for (; !trons->Done(); trons->Next())
      {
         sLink link;
         trons->Link(&link);

         ObjID stimulus = link.dest;
         //
         // Ok, at this point link is a receptron link that obj is
         // no longer inheriting, so we RemoveAllSensors it
         //
         RemoveAllSensors(obj,stimulus);
      }
      SafeRelease(trons);
   }
   SafeRelease(donors);
}

//
// Sensor id swizzling
//
// The point here is that relation ID's are not persistent across sessions, where as
// idx-part of link ID is.  So, we are re-swizzling the relation part of the link ID
// into something persistent.
//

#define INDEX_BITS LINK_FLAVOR_SHF
#define INDEX_MASK (~LINK_FLAVOR_BITS)
#define SENTINEL_BIT (1 << INDEX_BITS)


StimSensorID cStimSensors::Link2Sensor(LinkID id)
{
   RelationID rel = LINKID_RELATION(id);
   if (rel != pSensors->GetID())
      return 0;
   else
      return id; 
}

LinkID cStimSensors::Sensor2Link(StimSensorID sensid)
{
   RelationID rel = LINKID_RELATION(sensid);
   if (rel != pSensors->GetID())
      return 0;
   else
      return sensid; 
}

//
// Receptron ID swizzling
//

ReceptronID cStimSensors::Link2Receptron(LinkID id)
{
   RelationID rel = LINKID_RELATION(id);
   if (rel != pReceptrons->GetID())
      return 0;
   else
      return id; 


}

LinkID cStimSensors::Receptron2Link(ReceptronID tronid)
{
   RelationID rel = LINKID_RELATION(tronid);
   if (rel != pReceptrons->GetID())
      return 0;
   else
      return tronid; 
}

//------------------------------------------------------------
// IStimSensors methods
//

STDMETHODIMP_(ReceptronID) cStimSensors::AddReceptron(ObjID obj, StimID stimulus, const sReceptron* tron)
{
   sReceptron addme = *tron;

   // fix up order
   // @HACK: we don't test this, because the editor doesn't set it 
   // to zero 
   if (addme.order == 0)
      addme.order = NextOrder++;
   else
   {
      addme.order++;
      if (NextOrder <= addme.order)
         NextOrder = addme.order + 1; 
   }

   // just add the link, receptron listener does the rest
   LinkID id = pReceptrons->AddFull(obj,stimulus,(void*)&addme);
   return Link2Receptron(id);
}

////////////////////////////////////////

STDMETHODIMP cStimSensors::RemoveReceptron(ReceptronID tronid)
{
   LinkID id = Receptron2Link(tronid);
   return pReceptrons->Remove(id);
}

////////////////////////////////////////

STDMETHODIMP cStimSensors::GetReceptron(ReceptronID tronid, sReceptron* tron)
{
   LinkID id = Receptron2Link(tronid);
   sReceptron* getron = (sReceptron*)pReceptrons->GetData(id);
   if (getron != NULL)
   {
      *tron = *getron;
      return S_OK;
   }
   return E_FAIL;
}

////////////////////////////////////////

STDMETHODIMP cStimSensors::SetReceptron(ReceptronID tronid, const sReceptron* tron)
{
   LinkID id = Receptron2Link(tronid);
   sReceptron* getron = (sReceptron*)pReceptrons->GetData(id);
   if (getron != NULL)
   {
      pReceptrons->SetData(id,(void*)tron); 
      return S_OK;
   }
   return E_FAIL;
}

////////////////////////////////////////

STDMETHODIMP_(sObjStimPair) cStimSensors::GetReceptronElems(ReceptronID tronid)
{
   sObjStimPair pair = { OBJ_NULL, OBJ_NULL};
   LinkID id = Receptron2Link(tronid);
   sLink link;
   if (pReceptrons->Get(id,&link))
   {
      pair.obj = link.source;
      pair.stim = link.dest;
   }
   return pair;
}

////////////////////////////////////////

STDMETHODIMP_(StimSensorID) cStimSensors::LookupSensor(THIS_ ObjID obj, StimID stim)
{
   if (obj == OBJ_NULL || stim == OBJ_NULL)
      return SENSORID_NULL;
   LinkID id = singleton_link(pSensors,obj,stim);
   return Link2Sensor(id);
}

////////////////////////////////////////

STDMETHODIMP_(sObjStimPair) cStimSensors::GetSensorElems(THIS_ StimSensorID sensid)
{
   sObjStimPair pair = { OBJ_NULL, OBJ_NULL};
   LinkID id = Sensor2Link(sensid);
   sLink link;
   if (pSensors->Get(id,&link))
   {
      pair.obj = link.source;
      pair.stim = link.dest;
   }
   return pair;
}

////////////////////////////////////////

STDMETHODIMP_(ObjID) cStimSensors::ObjParam(eReactObjParam which)
{
   Assert_(which < kNumSpecialObjParams); 

   return SpecialObjParams[which];
}

////////////////////////////////////////

static const char* objnames[] = { "This Sensor", "This Source" }; 

void cStimSensors::FillObjParams()
{
   AutoAppIPtr_(ObjectSystem,pObjSys);
   for (int i = 0; i < kNumSpecialObjParams; i++)
   {
      const char* name = objnames[i];
      ObjID obj = pObjSys->GetObjectNamed(name);
      if (obj == OBJ_NULL)
      {
         obj = pObjSys->Create(ROOT_ARCHETYPE,kObjectAbstract);
         pTraitMan->SetArchetype(obj,OBJ_NULL);
         pObjSys->NameObject(obj,name);
      }
      SpecialObjParams[i] = obj;
   }
}

STDMETHODIMP cStimSensors::DatabaseNotify(tStimDatabaseMsg msg, IUnknown* )
{
   switch (DB_MSG(msg))
   {
      case kDatabaseReset: 
         break;

      case kDatabasePostLoad:
      {
         if (msg & kObjPartConcrete)
         {
            RecomputeSensors();
            ResetOrder();
         }

         if (msg & kObjPartAbstract)
            FillObjParams();
      }
      break;

      case kDatabaseDefault:
         ResetOrder();
         FillObjParams();
         break;
   }
   return S_OK;
}

////////////////////////////////////////


STDMETHODIMP cStimSensors::ObjectNotify(THIS_ eObjNotifyMsg msg, ObjNotifyData data)
{
   ObjID obj = (ObjID)data;
   switch (NOTIFY_MSG(msg))
   {
      case kObjNotifyCreate:
      {
         // We don't need to do this, because we're listening to the archetype link.
#ifdef INSTANTIATE_SENSORS
         IReceptronQuery* query = QueryInheritedReceptrons(obj,OBJ_NULL);
         for (query->Start(); !query->Done(); query->Next())
         {
            AddAllSensors(obj,query->Elems().stim);
         }
         SafeRelease(query);
#endif
      }
      break;
   }
   return S_OK;

}

//------------------------------------------------------------
// Hierarchy listener (for creating new sensors on hierarchy changes)
//



void LGAPI cStimSensors::HierarchyListener(const sHierarchyMsg* msg, HierarchyListenerData data)
{
   cStimSensors* pStimSensors = (cStimSensors*)data;


   if (msg->kind == kDonorAdded)
   {
      pStimSensors->AddInheritanceLink(msg->obj,msg->donor);

   }
   else if (msg->kind == kDonorRemoved)
   {
      pStimSensors->RemoveInheritanceLink(msg->obj,msg->donor);
   }
}

//------------------------------------------------------------
// BASE RECEPTRON QUERY
//

class cBaseReceptronQuery : public IReceptronQuery
{
public:
   DECLARE_UNAGGREGATABLE();

   virtual ~cBaseReceptronQuery() {};
};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseReceptronQuery,IReceptronQuery);

//------------------------------------------------------------
// EMPTY RECEPTRON QUERY
//

class cEmptyReceptronQuery : public cBaseReceptronQuery
{
public:
   STDMETHOD(Start)() { return S_OK;};
   STDMETHOD_(BOOL,Done)() { return TRUE;};
   STDMETHOD(Next)() { return S_OK; };

   STDMETHOD_(sObjStimPair,Elems)()
   {
      sObjStimPair pair = { OBJ_NULL, OBJ_NULL};
      return pair;
   };
   STDMETHOD_(const sReceptron*, Receptron)() { return NULL; };
   STDMETHOD_(ReceptronID,ID)() { return RECEPTRON_NULL; } ;

};

//------------------------------------------------------------
// RECEPTRON QUERY WRAPPER AROUND LINK QUERY
//

class cLinkReceptronQuery : public cBaseReceptronQuery
{
   ILinkQuery* Query;
   cStimSensors* Sensors;

public:
   cLinkReceptronQuery(ILinkQuery* q, cStimSensors* s)
      : Query(q), Sensors(s)
   {
      Query->AddRef();
   }

   ~cLinkReceptronQuery()
   {
      SafeRelease(Query);
   }

   STDMETHOD(Start)() { return S_OK;} ;
   STDMETHOD_(BOOL,Done)() { return Query->Done();};
   STDMETHOD(Next)() { return Query->Next();};

   STDMETHOD_(sObjStimPair,Elems)()
   {
      sLink link = { OBJ_NULL, OBJ_NULL};
      Query->Link(&link);
      sObjStimPair pair = { link.source, link.dest };
      return pair;
   }

   STDMETHOD_(const sReceptron*, Receptron)()
   {
      return (const sReceptron*)Query->Data();
   }

   STDMETHOD_(ReceptronID,ID)()
   {
      return Sensors->Link2Receptron(Query->ID());
   }
};


//------------------------------------------------------------
// BASE SENSOR QUERY
//

class cBaseSensorQuery : public IStimSensorQuery
{
public:
   DECLARE_UNAGGREGATABLE();

   virtual ~cBaseSensorQuery() {};
};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseSensorQuery,IStimSensorQuery);

//------------------------------------------------------------
// EMPTY SENSOR QUERY
//

class cEmptySensorQuery : public cBaseSensorQuery
{
public:
   STDMETHOD(Start)() { return S_OK;};
   STDMETHOD_(BOOL,Done)() { return TRUE;};
   STDMETHOD(Next)() { return S_OK; };
   STDMETHOD_(sObjStimPair,Elems)()
   {
      sObjStimPair pair = { OBJ_NULL, OBJ_NULL};
      return pair;
   }

   STDMETHOD_(StimSensorID,ID)() { return SENSORID_NULL; } ;
};

//------------------------------------------------------------
// SENSOR QUERY WRAPPER AROUND LINK QUERY
//

class cLinkSensorQuery : public cBaseSensorQuery
{
   ILinkQuery* Query;
   cStimSensors* Sensors;

public:
   cLinkSensorQuery(ILinkQuery* q, cStimSensors* s)
      : Query(q), Sensors(s)
   {
      Query->AddRef();
   }

   ~cLinkSensorQuery()
   {
      SafeRelease(Query);
   }

   STDMETHOD(Start)() { return S_OK;} ;
   STDMETHOD_(BOOL,Done)() { return Query->Done();};
   STDMETHOD(Next)() { return Query->Next();};

   STDMETHOD_(sObjStimPair,Elems)()
   {
      sLink link = { OBJ_NULL, OBJ_NULL};
      Query->Link(&link);
      sObjStimPair pair = { link.source, link.dest };
      return pair;
   }

   STDMETHOD_(StimSensorID,ID)()
   {
      return Sensors->Link2Sensor(Query->ID());
   }
};



//------------------------------------------------------------
// IStimSensors Query Methods
//

//
// Sensors
//

STDMETHODIMP_(IStimSensorQuery*) cStimSensors::QuerySensors(ObjID obj, StimID stim)
{
   if (OBJ_IS_ABSTRACT(obj))
      return new cEmptySensorQuery; // abstract objects never have sensors
   else
   {
      ILinkQuery* links = pSensors->Query(obj,stim);
      IStimSensorQuery* result = new cLinkSensorQuery(links,this);
      SafeRelease(links);
      return result;
   }
}

//
// Receptrons
//

STDMETHODIMP_(IReceptronQuery*) cStimSensors::QueryReceptrons(ObjID obj, StimID stim)
{
   if (obj == OBJ_NULL)
      obj = LINKOBJ_WILDCARD;
   if (stim == OBJ_NULL)
      stim = LINKOBJ_WILDCARD;

   ILinkQuery* links;

   // simple case
   if (stim == LINKOBJ_WILDCARD)
      links = pReceptrons->Query(obj,stim);
   else
   {
      cLinkQueryFactory* fact = CreateDestSetQueryFactory(pReceptrons,obj);
      IObjectQuery* objset = pStimuli->QueryDonors(stim);

      links = CreateObjSetLinkQuery(objset,fact);
      SafeRelease(objset);
   }

   IReceptronQuery* result = new cLinkReceptronQuery(links,this);
   SafeRelease(links);
   return result;
}

//
// Inherited version
//

STDMETHODIMP_(IReceptronQuery*) cStimSensors::QueryInheritedReceptrons(ObjID obj, StimID stim)
{
   if (obj == OBJ_NULL)
      obj = LINKOBJ_WILDCARD;
   if (stim == OBJ_NULL)
      stim = LINKOBJ_WILDCARD;


   if (obj == LINKOBJ_WILDCARD)
      return QueryReceptrons(obj,stim); 

   ILinkQuery* links;

   if (stim == LINKOBJ_WILDCARD)
      links = QueryInheritedLinksSingle(pAnyStimTrait,pReceptrons,obj,stim);
   else
      links = QueryInheritedLinks(pAnyStimTrait,pReceptrons,obj,stim);
      
   IReceptronQuery* result = new cLinkReceptronQuery(links,this);
   SafeRelease(links);
   return result;
}


//------------------------------------------------------------
// Factory for aggregate member
//

void StimSensorsCreate(void)
{
   AutoAppIPtr_(Unknown,pUnk);
   new cStimSensors(pUnk);
}

