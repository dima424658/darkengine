/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimsrc.cpp,v 1.17 1998/11/02 02:57:12 mahk Exp $

#include <appagg.h>

#include <ssrctype.h>
#include <ssrcbase.h>
#include <stimsrc_.h>

#include <objtype.h>
#include <osystype.h>
#include <osysbase.h>
#include <objquery.h>
#include <objnotif.h>

#include <relation.h>
#include <linktype.h>
#include <linkbase.h>
#include <linkman.h>
#include <linkint.h>
#include <lnkquery.h>
#include <dataops_.h>
#include <linkid.h>

#include <propman.h>
#include <proptype.h>
#include <propbase.h>

#include <osetlnkq.h>
#include <stimqfct.h>

#include <stimuli.h>
#include <propag8n.h>
#include <propag8r.h>
#include <pg8rtype.h>
#include <pg8rbase.h>

#include <traitman.h>
#include <traitbas.h>
#include <traitlnk.h>
#include <trait.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <simtime.h>

#include <dbasemsg.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// cStimSources IMPLEMENTATION
//


IMPLEMENT_AGGREGATION_SELF_DELETE(cStimSources);

//------------------------------------------------------------
// AGGREGATE INIT CONSTRAINTS

static sRelativeConstraint Constraints[] = 
{
   { kConstrainAfter, &IID_IStimuli },
   { kConstrainAfter, &IID_IPropagation },
   { kConstrainAfter, &IID_ITraitManager },
   { kConstrainAfter, &IID_ILinkManager },

   { kNullConstraint, },
};

//------------------------------------------------------------
// CONSTRUCTION/DECONSTRUTION
//

cStimSources::cStimSources(IUnknown* pOuter)
   : pStimuli(NULL),
     pPropagation(NULL),
     pTraitMan(NULL),
     pSourceDescs(NULL),
     pSources(NULL),
     pSourceDescTrait(NULL),
     pLinkMan(NULL)
{
   INIT_AGGREGATION_1(pOuter,IID_IStimSources,this,kPriorityNormal,Constraints);
}

cStimSources::~cStimSources()
{
}

//------------------------------------------------------------
// SOURCES RELATION
//

//
// Descriptor
//

static sRelationDesc source_desc = 
{
   "arSrc",
   kRelationNoEdit,
   1, // version
   1, // last acceptible
};

//
// Satellite data type 
//

struct sStimSourceData 
{
   StimSourceID desc;         // source desc ID
   tStimTimeStamp birthdate;  // date of birth
   ulong count;               // number of times this source has been added
};

struct sRelationDataDesc source_ddesc = LINK_DATA_DESC(sStimSourceData);



//
// Struct desc
//

static sFieldDesc source_fields[] = 
{
   { "Description", kFieldTypeInt, FieldLocation(sStimSourceData,desc), kFieldFlagHex|kFieldFlagNotEdit }, 
   { "Birthdate",  kFieldTypeInt, FieldLocation(sStimSourceData,birthdate) },
   { "Count", kFieldTypeInt, FieldLocation(sStimSourceData,count), kFieldFlagNotEdit }, 
}; 

static sStructDesc source_sdesc = StructDescBuild(sStimSourceData, kStructFlagNone, source_fields); 

//
// Listener
//

// compute the appropriate tStimSensorEvent for this msg type. 

static tStimSourceEvent LinkMsg2SrcMsg(eRelationListenMsg msg)
{
   switch(msg)
   {
      case kListenLinkBirth:
         return kStimSourceCreate;
      case kListenLinkDeath:
         return kStimSourceDestroy;

      default:
         Warning(("Stim Src listener got message %X\n",msg));
   }
   return 0xFFFFFFFF;
}


void cStimSources::SourceListener(sRelationListenMsg* msg, RelationListenerData data)
{
   cStimSources* us = (cStimSources*)data;

   sStimSourceEvent event;
   event.id = us->Link2Source(msg->id);
   event.type = LinkMsg2SrcMsg(msg->type);
   event.elems.obj = msg->link.source;
   event.elems.stim = msg->link.dest;

   sStimSourceData* srcdata = (sStimSourceData*)us->pSources->GetData(msg->id);
   StimSourceID descid = srcdata->desc;
   event.desc = (sStimSourceDesc*)us->pSourceDescs->GetData(descid);
   us->pPropagation->SourceEvent(&event);
}

//
// Source data ops
//

class cSourceDataOps : public cClassDataOps<sStimSourceData>
{
   ILinkManager* mpLinkMan; 
   RelationID desc_rel; 

public:

   cSourceDataOps()
      : mpLinkMan(AppGetObj(ILinkManager)),desc_rel(RELID_NULL)
   {
   }

   ~cSourceDataOps()
   {
      SafeRelease(mpLinkMan); 
   }


   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* file, int version)
   {
      // lookup desc_rel if we need to 
      if (desc_rel == RELID_NULL)
      {
         cAutoIPtr<IRelation> pRel = mpLinkMan->GetRelationNamed("arSrcDesc"); 
         desc_rel = pRel->GetID(); 
      }

      HRESULT result = cClassDataOps<sStimSourceData>::Read(dat,file,version); 
      if (SUCCEEDED(result))
      {
         sStimSourceData* data = (sStimSourceData*)dat->value; 
         // There's a bug in Link ID mapping, where you can't refer to 
         // a link in a different partition, so I'm remapping it by hand. 
         LinkID id = data->desc ; 
         data->desc = LINKID_MAKE2(desc_rel, LINKID_NON_RELATION(id)); 
      }
      return result; 
   }

}; 

//
// Factory 
//

// Optimized query cases
#define SOURCES_QUERY_CASES (kQCaseSetSourceKnown|kQCaseSetDestKnown|kQCaseSetBothKnown)

void cStimSources::CreateSourceRelation()
{
   StructDescRegister(&source_sdesc);
   source_ddesc.data_ops = new cSourceDataOps; 
   pSources = CreateStandardRelation(&source_desc,&source_ddesc,SOURCES_QUERY_CASES);
   pSources->Listen(kListenLinkBirth|kListenLinkDeath, SourceListener, this); 
}

//------------------------------------------------------------
// SOURCE DESCRIPTORS RELATION
//

//
// Descriptor
//

static sRelationDesc sourcedesc_desc = 
{
   "arSrcDesc",
   kRelationNoEdit, // flags
};

//
// Satellite data type and descriptor
//

static sFieldDesc sourcedesc_fields[] = 
{
   { "Propagator", kFieldTypeInt, FieldLocation(sStimSourceDesc,propagator), },
   { "Intensity", kFieldTypeFloat, FieldLocation(sStimSourceDesc,level), },
   { "Valid Fields", kFieldTypeBits, FieldLocation(sStimSourceDesc,valid_fields), },

#ifdef SPECIFIC_FIELDS
   { "Shape Kind", kFieldTypeInt, FieldLocation(sStimSourceDesc,shape.kind), },
   { "Shape Dims", kFieldTypeVector, FieldLocation(sStimSourceDesc,shape.dims), },
   { "Shape Ang X", kFieldTypeInt, FieldLocation(sStimSourceDesc,shape.ang.tx), },
   { "Shape Ang Y", kFieldTypeInt, FieldLocation(sStimSourceDesc,shape.ang.ty), },
   { "Shape Ang Z", kFieldTypeInt, FieldLocation(sStimSourceDesc,shape.ang.tz), },

   { "Growth Time", kFieldTypeInt, FieldLocation(sStimSourceDesc,life.growth), },
   { "Decay Curve", kFieldTypeInt, FieldLocation(sStimSourceDesc,life.decay_time), },
   { "Decay Level", kFieldTypeFloat, FieldLocation(sStimSourceDesc,life.decay_level), },
   { "Lifespan",    kFieldTypeInt, FieldLocation(sStimSourceDesc,life.lifespan), },
#endif // 
};

static sStructDesc sourcedesc_sdesc = StructDescBuild(sStimSourceDesc,kStructFlagNone,sourcedesc_fields);

static sRelationDataDesc sourcedesc_ddesc = LINK_DATA_DESC(sStimSourceDesc);

//
// Listener
//

void cStimSources::SourceDescListener(sRelationListenMsg* msg, RelationListenerData data)
{
   cStimSources* us = (cStimSources*)data;
   StimSourceID src = us->Link2Source(msg->id);
   switch (msg->type)
   {
      case kListenLinkBirth:
         us->AddAllSources(src, msg->link.source,msg->link.dest);
         break;
      case kListenLinkDeath:
         us->DestroyAllSources(src,msg->link.source,msg->link.dest);
         break;
   }
}

//
// Data ops for reading and writing source descs
//



enum eSrcDescVersions
{
   kInitialVersion = 1,
   kPaddingAdded,
};

#define SOURCE_DESC_VERSION kPaddingAdded

//
// Old, pre-padded structures
//

struct sStimSrcPrePad
{
   PropagatorID propagator;  // propagator to which source belongs
   tStimLevel level; // source's intensity
   ulong valid_fields; // bitmask of which fields below are valid
   ulong shape_kind; // kind of shape (interpreted by propagator)
   mxs_vector dims; // dimensions
   mxs_angvec ang;  // angular displacement
   tStimDuration growth;        // growth time  (Tg)
   ulong decay_kind;             // decay curve
   tStimDuration decay_time;    // time to decay (Td)
   tStimLevel decay_level; // final value after decay
   tStimDuration lifespan;      // time before dying
};


//
// The data ops proper
//

class cSourceDescDataOps : public cClassDataOps<sStimSourceDesc> 
{
   IPropagation* Propagation;

public:
   cSourceDescDataOps()
      :Propagation(AppGetObj(IPropagation))
   {
   }

   ~cSourceDescDataOps()
   {
      SafeRelease(Propagation);
   }

   STDMETHOD_(int,Version)() { return SOURCE_DESC_VERSION;};
   
   STDMETHOD(Write)(sDatum datum, IDataOpsFile* file)
   {
      const sStimSourceDesc* desc = (const sStimSourceDesc*)datum.value;
      
      int len = 0;

      // write out the struct literally 
      len += file->Write(desc,sizeof(*desc));

      // swizzle propagator into a name
      IPropagator* gator = Propagation->GetPropagator(desc->propagator);
      const sPropagatorDesc* gatordesc = gator->Describe();
      len += file->Write(gatordesc->name,sizeof(gatordesc->name));
      SafeRelease(gator);
      
      return (len == sizeof(*desc) + sizeof(gatordesc->name)) ? S_OK : E_FAIL; 
   }

   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* file, int version)
   {

      int len = 0;

      if (!dat->value)
         *dat = New(); 

      sStimSourceDesc* desc = (sStimSourceDesc*)dat->value;
      
      // back-compatibility
      // we secretly know that no sources up to this 
      // point have had meaningful life cycle data. 
      // So we don't care that it would get misaligned
      if (version < kPaddingAdded)
      {
         memset(desc,0,sizeof(*desc)); 
         len = file->Read(desc,sizeof(sStimSrcPrePad));

         if (len != sizeof(sStimSrcPrePad))
             goto fail; 
      }
      else
      {
         // read in the struct literally 
         len = file->Read(desc,sizeof(*desc));
         if (len != sizeof(*desc))
             goto fail; 
      }

      // swizzle propagator into a name
      sPropagatorDesc gatordesc;
      len = file->Read(gatordesc.name,sizeof(gatordesc.name));
      if (len != sizeof(gatordesc.name))
         goto fail; 


      {
         IPropagator* gator = Propagation->GetPropagatorNamed(gatordesc.name);
         desc->propagator = gator->GetID();
         SafeRelease(gator);
      }

      return S_OK; 

   fail:
      Warning(("Stim source read failed\n")); 
      Delete(*dat); 
      dat->value = NULL; 
      return E_FAIL; 
   }

};

//
// Factory 
// 

// Optimized query cases
#define SOURCE_DESC_QUERY_CASES (kQCaseSetSourceKnown|kQCaseSetBothKnown)

void cStimSources::CreateSourceDescRelation()
{
   StructDescRegister(&sourcedesc_sdesc);

   sourcedesc_ddesc.data_ops = new cSourceDescDataOps;
   pSourceDescs = CreateStandardRelation(&sourcedesc_desc,&sourcedesc_ddesc,SOURCE_DESC_QUERY_CASES);
   pSourceDescs->Listen(kListenLinkBirth|kListenLinkDeath,SourceDescListener,this);
   
}

//------------------------------------------------------------
// HIERARCHY LISTENERS
//

void LGAPI cStimSources::HierarchyListener(const sHierarchyMsg* msg, HierarchyListenerData data)
{
   cStimSources* pStimSources = (cStimSources*)data;

   if (msg->kind  == kDonorAdded)  
   {
      pStimSources->AddInheritanceLink(msg->obj,msg->donor);
   }

   if (msg->kind == kDonorRemoved)  
   {
      pStimSources->RemoveInheritanceLink(msg->obj,msg->donor);
   }   

}

////////////////////////////////////////

BOOL cStimSources::SourceTraitPredicate(ObjID obj, TraitPredicateData data)
{
   cStimSources* us = (cStimSources*)data;
   cAutoIPtr<ILinkQuery> query (us->pSourceDescs->Query(obj,LINKOBJ_WILDCARD));
   return !query->Done();
}

static sTraitDesc src_trait_desc = 
{
   "arSrcDesc" 
};


//------------------------------------------------------------
// AGGREGATE PROTOCOL
//

HRESULT cStimSources::Init()
{
   // Grab the interfaces we use
   pStimuli = AppGetObj(IStimuli);  
   pPropagation = AppGetObj(IPropagation);
   pTraitMan = AppGetObj(ITraitManager);
   pLinkMan = AppGetObj(ILinkManager); 
   
   // Create our relations
   CreateSourceRelation();
   CreateSourceDescRelation();

   // Grab their ids
   RelIDs[kSourceIDX] = pSources->GetID();
   RelIDs[kSourceDescIDX] = pSourceDescs->GetID();

   pTraitMan->Listen(HierarchyListener,this); 

   // Create a srcdesc trait
   sTraitPredicate pred = { SourceTraitPredicate, (TraitPredicateData)this };
   pSourceDescTrait = pTraitMan->CreateTrait(&src_trait_desc,&pred); 

   return S_OK;
}

HRESULT cStimSources::End()
{
   SafeRelease(pStimuli);
   SafeRelease(pPropagation);
   SafeRelease(pTraitMan);
   SafeRelease(pSources);
   SafeRelease(pSourceDescs);
   SafeRelease(pLinkMan); 

   return S_OK;
}

//------------------------------------------------------------
// HELPERS
//

void cStimSources::AddSource(StimSourceID descid, ObjID obj, StimID stim)
{
   Assert_(OBJ_IS_CONCRETE(obj));
   BOOL found = FALSE;

   ILinkQuery* query = pSources->Query(obj,stim);
   for (; !query->Done(); query->Next())
   {
      sStimSourceData* data = (sStimSourceData*)query->Data();
      if (data->desc == descid)
      {  
         data->count++;
         pSources->SetData(query->ID(),data);
         found = TRUE;
         break;
      }
      
   }
   SafeRelease(query);

   // Add the link
   if (!found)
   {
      sStimSourceData srcdata = { descid, GetSimTime(), 1 }; 
      pSources->AddFull(obj,stim,&srcdata);
   }
}

////////////////////////////////////////

void cStimSources::AddAllSources(StimSourceID descid, ObjID obj, StimID stim)
{
   if (OBJ_IS_CONCRETE(obj))
   {
      AddSource(descid,obj,stim);
      return;
   }

   IObjectQuery* query = pTraitMan->Query(obj,kTraitQueryAllDescendents);
   for (; !query->Done(); query->Next())
   {
      ObjID heir = query->Object();
      if (OBJ_IS_CONCRETE(heir))
         AddSource(descid,heir,stim);
   }
   SafeRelease(query);
}

////////////////////////////////////////

void cStimSources::RemoveSource(StimSourceID descid, ObjID obj, ObjID stim)
{
   Assert_(OBJ_IS_CONCRETE(obj));
   ILinkQuery* query = pSources->Query(obj,stim);
   for (; !query->Done(); query->Next())
   {
      sStimSourceData* srcdata = (sStimSourceData*)query->Data();
      if (srcdata->desc == descid)
      {
         srcdata->count--;
         if (srcdata->count == 0)
            pSources->Remove(query->ID());
         else
            pSources->SetData(query->ID(),srcdata);
      }
   }
   SafeRelease(query);
}

////////////////////////////////////////

void cStimSources::RemoveAllSources(StimSourceID descid, ObjID obj, ObjID stim)
{
   if (OBJ_IS_CONCRETE(obj))
   {
      RemoveSource(descid,obj,stim);
      return; 
   }

   IObjectQuery* query = pTraitMan->Query(obj,kTraitQueryAllDescendents);
   for (; !query->Done(); query->Next())
   {
      ObjID heir = query->Object();
      if (OBJ_IS_CONCRETE(heir))
         RemoveSource(descid,heir,stim);
   }
   SafeRelease(query);
}

////////////////////////////////////////

void cStimSources::DestroyAllSources(StimSourceID descid, ObjID obj, ObjID stim)
{
   ILinkQuery* query = pSources->Query(LINKOBJ_WILDCARD,stim);
   for (; !query->Done(); query->Next())
   {
      sStimSourceData* data = (sStimSourceData*)query->Data();
      if (data->desc == descid)
      {
         // Blast it, no questions asked
         pSources->Remove(query->ID());
      }
   }
   SafeRelease(query);
}

////////////////////////////////////////

void cStimSources::AddInheritanceLink(ObjID obj, ObjID newdonor)
{
   IObjectQuery* donors = pSourceDescTrait->GetAllDonors(newdonor); 
   //   IObjectQuery* donors = pTraitMan->Query(newdonor,kTraitQueryAllDonors);
   for (; !donors->Done(); donors->Next())
   {
      ObjID donor = donors->Object();
      // Find all source descs
      ILinkQuery* query = pSourceDescs->Query(donor,LINKOBJ_WILDCARD);
      for (; !query->Done(); query->Next())
      {
         sLink link;
         StimSourceID src = Link2Source(query->ID());
         query->Link(&link);
         AddAllSources(src,obj,link.dest);
      }
      SafeRelease(query);
   }
   SafeRelease(donors);
}

////////////////////////////////////////

void cStimSources::RemoveInheritanceLink(ObjID obj, ObjID newdonor)
{
   IObjectQuery* donors = pSourceDescTrait->GetAllDonors(newdonor); 
   //   IObjectQuery* donors = pTraitMan->Query(newdonor,kTraitQueryAllDonors);
   for (; !donors->Done(); donors->Next())
   {
      ObjID donor = donors->Object();
      // Find all source descs
      ILinkQuery* query = pSourceDescs->Query(donor,LINKOBJ_WILDCARD);
      for (; !query->Done(); query->Next())
      {
         sLink link;
         StimSourceID src = Link2Source(query->ID());
         query->Link(&link);
         RemoveAllSources(src,obj,link.dest);
      }
      SafeRelease(query);
   }
   SafeRelease(donors);
}

////////////////////////////////////////

void cStimSources::RecomputeSources()
{
   RelationID srcrel = pSourceDescs->GetID(); 
   // Stomp all refcounts to zero.  
   tSimTime time = GetSimTime(); 
   ILinkQuery* query = pSources->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
   {
      sStimSourceData* data = (sStimSourceData*)query->Data();
      data->count = 0;  
      if (data->birthdate > time)
         data->birthdate = time; 
      pSources->SetData(query->ID(),data); 
   }
   SafeRelease(query);

   // Now re-instantiate all source descs
   query = pSourceDescs->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
   {
      sLink link; 
      query->Link(&link);
      StimSourceID id = Link2Source(query->ID());
      AddAllSources(id,link.source,link.dest);
   }
   SafeRelease(query);

   // now blast all links of zero count 
   query = pSources->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   for (; !query->Done(); query->Next())
   {
      sStimSourceData* data = (sStimSourceData*)query->Data();
      LinkID linkid = query->ID(); 

      if (data->count == 0) // it's dead, jim 
      {
         pSources->Remove(linkid);
      }
      else  // send a source event 
      {
         sLink link;
         query->Link(&link); 
         sStimSourceEvent event = { kStimSourceCreate, Link2Source(linkid), { link.source, link.dest }, }; 

         sStimSourceData* srcdata = (sStimSourceData*)pSources->GetData(linkid);
         StimSourceID descid = srcdata->desc;
         event.desc = (sStimSourceDesc*)pSourceDescs->GetData(descid);
         pPropagation->SourceEvent(&event);
      }
   }
   SafeRelease(query);

}

//------------------------------------------------------------
// REMAPPERS
//


StimSourceID cStimSources::Link2Source(LinkID id)
{
   RelationID rel = LINKID_RELATION(id);
   int idx = 0;
   
   // I'm too cool for a loop
   if (rel == RelIDs[kSourceIDX] || rel == RelIDs[kSourceDescIDX])
         return id; 
      

   return SRCID_NULL;
}

LinkID cStimSources::Source2Link(StimSourceID id)
{
   RelationID rel = LINKID_RELATION(id);
   if (rel == RelIDs[kSourceIDX] || rel == RelIDs[kSourceDescIDX])
         return id; 

   return LINKID_NULL;
}

//------------------------------------------------------------
// ISTIMSOURCES METHODS
//

STDMETHODIMP_(StimSourceID) cStimSources::AddSource(ObjID obj, StimID stim, const sStimSourceDesc* desc)
{
   sStimSourceDesc copydesc = *desc;
   // sanity check desc first? 
   LinkID id = pSourceDescs->AddFull(obj,stim,&copydesc);
   return Link2Source(id);
}

////////////////////////////////////////

STDMETHODIMP cStimSources::RemoveSource(StimSourceID id)
{
   if (id == SRCID_NULL) return E_FAIL;
   LinkID linkid = Source2Link(id);
   RelationID rel = LINKID_RELATION(linkid);

   if (rel == RelIDs[kSourceDescIDX])
   {
      // Just blast the link, and let the listeners take care of the rest.  No worries. 
      pSourceDescs->Remove(linkid);
   }
   else 
   {
      sLink srclink;
      sLink desclink;

      if (!pSources->Get(linkid,&srclink))  // no link here!
         return S_FALSE;

      sStimSourceData* data = (sStimSourceData*)pSources->GetData(linkid);
      pSourceDescs->Get(data->desc,&desclink);

         // if the source desc is right here on the object, remove the desc
      if (desclink.source == srclink.source)
      {
         pSourceDescs->Remove(data->desc);
      }
      else          // Otherwise, just mark the source as dead
      {
         data->birthdate = kSourceBirthDateDead; 
         // Perhaps this should move to listener?
         // pPropagation->RemoveSource(id);
      }
   }
   return S_OK;

}

////////////////////////////////////////

STDMETHODIMP_(sObjStimPair) cStimSources::GetSourceElems(StimSourceID id)
{
   if (id == SRCID_NULL) 
   {
      sObjStimPair result = { OBJ_NULL, OBJ_NULL }; 
      return result ;
   }

   LinkID linkid = Source2Link(id);
 
   sLink link = { OBJ_NULL, OBJ_NULL}; 
   pLinkMan->Get(linkid,&link);

   sObjStimPair pair = { link.source,link.dest}; 
   return pair;
}

////////////////////////////////////////

STDMETHODIMP_(tStimTimeStamp) cStimSources::GetSourceBirthDate(StimSourceID id)
{
   LinkID linkid = Source2Link(id);

   RelationID rel = LINKID_RELATION(id);

   if (rel == RelIDs[kSourceIDX])
   {
      sStimSourceData* data = (sStimSourceData*)pSources->GetData(linkid);
      if (data)
         return data->birthdate;
   }
   return kSourceBirthDateNone;
}

////////////////////////////////////////

STDMETHODIMP cStimSources::DescribeSource(StimSourceID id, sStimSourceDesc* desc)
{
   static sStimSourceDesc zeroes = { 0 }; 
   LinkID linkid = Source2Link(id);
   RelationID rel = LINKID_RELATION(linkid); 

   if (rel == RelIDs[kSourceIDX]) // it's a source link
   {
      sStimSourceData* data = (sStimSourceData*)pSources->GetData(linkid);
      if (data == NULL)
      {
         *desc = zeroes;
         return S_FALSE;
      }
         
      linkid = Source2Link(data->desc);
   }

   sStimSourceDesc* srcdesc = (sStimSourceDesc*)pSourceDescs->GetData(linkid);
   
   *desc = (srcdesc) ? *srcdesc : zeroes;

   
   return (srcdesc) ? S_OK : S_FALSE;
}



//------------------------------------------------------------
// STIMSOURCE QUERY CLASSES
//

//
// Base query class
//

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseStimSourceQuery,IStimSourceQuery);


//
// Empty stim source factory 
//

class cEmptyStimSourceQuery : public cBaseStimSourceQuery 
{
public: 
   STDMETHOD(Start)() { return S_OK; }; 
   STDMETHOD_(BOOL,Done)() { return TRUE; };
   STDMETHOD(Next)() { return E_FAIL; };

   STDMETHOD_(StimSourceID,ID)() { return SRCID_NULL; }; 
   STDMETHOD_(sObjStimPair,Elems)() 
   { 
      sObjStimPair pair = { OBJ_NULL, OBJ_NULL}; 
      return pair; 
   };
   STDMETHOD_(const sStimSourceDesc*, Source)() { return NULL; }; 
};

////////////////////////////////////////

STDMETHODIMP_(IStimSourceQuery*) cStimSources::QuerySources(ObjID obj, StimID stim)
{
   ILinkQuery* links;
   if (obj == LINKOBJ_WILDCARD)
   {
      links = pSources->Query(obj,stim);
   }
   else if (OBJ_IS_CONCRETE(obj))
   {
      links = pSources->Query(obj,stim); 
   }
   else if (stim == LINKOBJ_WILDCARD)
   {
      IObjectQuery* donors = pSourceDescTrait->GetAllDonors(obj); 
      // IObjectQuery* donors = pTraitMan->Query(obj,kTraitQueryAllDonors);
      cLinkQueryFactory* fact = CreateSourceSetQueryFactory(pSourceDescs,LINKOBJ_WILDCARD);
      links = CreateObjSetLinkQuery(donors,fact);
      SafeRelease(donors);
   }
   else 
   {
      cLinkQueryFactory* fact = new cStimulusQueryFactory(pStimuli,stim,pSourceDescs);      
      IObjectQuery* objset = pSourceDescTrait->GetAllDonors(obj); 
      //      IObjectQuery* objset = pTraitMan->Query(obj,kTraitQueryAllDonors);

      links = CreateObjSetLinkQuery(objset,fact);
      SafeRelease(objset);
   }

   IStimSourceQuery* result = new cSourceQuery(links,this);
   SafeRelease(links);
   return result;
}

////////////////////////////////////////


STDMETHODIMP cStimSources::DatabaseNotify(tStimDatabaseMsg msg, IUnknown* )
{
   switch (DB_MSG(msg))
   {
      case kDatabasePostLoad:
         if (msg & kObjPartConcrete)
         {
            RecomputeSources();
         }
         break;

      case kDatabaseDefault:
         break;
   }
   return S_OK;
}


////////////////////////////////////////


STDMETHODIMP cStimSources::ObjectNotify(THIS_ eObjNotifyMsg msg, ObjNotifyData data)
{
   ObjID obj = (ObjID)data;
   switch (NOTIFY_MSG(msg))
   {
      case kObjNotifyCreate:
      break;
   }
   return S_OK;
}


//------------------------------------------------------------
// CSOURCEQUERY METHODS
//


cStimSources::cSourceQuery::cSourceQuery(ILinkQuery* q, cStimSources* s)
   : Query(q), Sources(s)
{
   Query->AddRef();
}

cStimSources::cSourceQuery::~cSourceQuery()
{
   SafeRelease(Query);
}

STDMETHODIMP cStimSources::cSourceQuery::Start()
{
   return S_OK;
}

STDMETHODIMP_(BOOL) cStimSources::cSourceQuery::Done()
{
   return Query->Done();
}

STDMETHODIMP cStimSources::cSourceQuery::Next()
{
   Query->Next();
   return S_OK;
}

STDMETHODIMP_(StimSourceID) cStimSources::cSourceQuery::ID()
{
   return Sources->Link2Source(Query->ID()); 
}

   
STDMETHODIMP_(sObjStimPair) cStimSources::cSourceQuery::Elems() 
{
   sLink link = { OBJ_NULL, OBJ_NULL} ; 
   Query->Link(&link);
   sObjStimPair pair = { link.source, link.dest};
   return pair;
}

STDMETHODIMP_(const sStimSourceDesc*) cStimSources::cSourceQuery::Source() 
{
   LinkID linkid = Query->ID();
   if (linkid == LINKID_NULL) return NULL;

   RelationID rel = LINKID_RELATION(linkid);

   if (rel == Sources->RelIDs[kSourceIDX])
   {
      sStimSourceData* data = (sStimSourceData*)Sources->pSources->GetData(linkid);
      linkid = data->desc;
   }
   return (sStimSourceDesc*)Sources->pSourceDescs->GetData(linkid); 
}

////////////////////////////////////////

void StimSourcesCreate(void)
{
   AutoAppIPtr_(Unknown,pUnk);
   new cStimSources(pUnk);
}




