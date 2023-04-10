/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitman.cpp,v 1.35 1999/05/19 16:06:08 mahk Exp $
#include <appagg.h>
#include <allocapi.h>

#include <objtype.h>
#include <iobjsys.h>
#include <osystype.h>
#include <osysbase.h>
#include <objnotif.h>

#include <nameprop.h>

#include <lnkquery.h>
#include <linktype.h>
#include <linkbase.h>
#include <relation.h>

#include <propbase.h>
#include <proptype.h>

#include <traitma_.h>
#include <trait.h>
#include <trait_.h>

#include <traitprp.h>
#include <traitpr_.h>
#include <traitlnk.h>
#include <traitln_.h>
#include <objquery.h>
#include <donorq_.h>
#include <traitbas.h>
#include <traisrch.h>
#include <trcache.h>

#include <propman.h>
#include <linkman.h>

#include <propface.h>

#include <dlistsim.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
//
// cTraitManager 
//
////////////////////////////////////////////////////////////

IMPLEMENT_AGGREGATION_SELF_DELETE(cTraitManager);


F_DECLARE_INTERFACE(IPropertyManager);
F_DECLARE_INTERFACE(ILinkManager);
F_DECLARE_INTERFACE(IObjectSystem);


static sRelativeConstraint Constraints[] =
{
   { kConstrainAfter, &IID_IPropertyManager },
   { kConstrainAfter, &IID_ILinkManager },
   { kConstrainAfter, &IID_IObjectSystem },
   { kNullConstraint}
};

cTraitManager::cTraitManager(IUnknown* pOuter)
   : DonorType(NULL),
     SymName(NULL),
     MetaProps(NULL),
     ObjSys(NULL),
     Cache(NULL),
     MetaPropArchetype(OBJ_NULL)
{
   Traits.Append(CreateEmptyTrait());
   INIT_AGGREGATION_1(pOuter,IID_ITraitManager,this,kPriorityNormal,Constraints);
}

cTraitManager::~cTraitManager()
{
   for (int i = 0; i < Traits.Size(); i++)
      SafeRelease(Traits[i]);
}

//------------------------------------------------------------
// Aggregate Protocol
//

HRESULT cTraitManager::Init()
{
   LGALLOC_PUSH_CREDIT();
   TraitPropertiesInit();
   InitTraitRelations();
   LGALLOC_POP_CREDIT();

   ObjSys = AppGetObj(IObjectSystem);
   Cache = AppGetObj(IDonorCache);
   IPropertyManager* propman = AppGetObj(IPropertyManager);

   IProperty* prop; 
   prop = propman->GetPropertyNamed(PROP_DONOR_TYPE_NAME);
   Verify(SUCCEEDED(COMQueryInterface(prop,IID_IIntProperty,(void**)&DonorType)));
   SafeRelease(prop);

   // Grab the symname property
   prop = propman->GetPropertyNamed(PROP_SYMNAME_NAME);
   Verify(SUCCEEDED(COMQueryInterface(prop,IID_IInvStringProperty,(void**)&SymName)));

   // Grab the metaproperty relation
   ILinkManager* linkman = AppGetObj(ILinkManager);
   MetaProps = linkman->GetRelationNamed(LINK_METAPROP_NAME);

   // Install da listeners
   MetaProps->Listen(kListenLinkBirth|kListenLinkPostMortem,MetaPropListener,this); 

   SafeRelease(propman);
   SafeRelease(linkman);

   return S_OK;
}

HRESULT cTraitManager::End()
{
   SafeRelease(ObjSys);
   SafeRelease(Cache);
   SafeRelease(MetaProps);
   SafeRelease(DonorType);
   TraitPropertiesTerm();
   TermTraitRelations();
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cTraitManager::CreateBaseArchetype(const char* name, ObjID* pID)
{
   if (SymName->GetObj(name,pID))
   {
      return S_FALSE;
   }
   *pID = CreateArchetype(name,ROOT_ARCHETYPE);
   return (*pID != OBJ_NULL) ? S_OK : E_FAIL;
}

////////////////////////////////////////

STDMETHODIMP_(ObjID) cTraitManager::CreateArchetype(const char* name, ObjID parent)
{
   ObjID obj;
   if (SymName->GetObj(name,&obj))
   {
      Warning(("Attempted to create duplicate %s archetype\n",name));
      return OBJ_NULL;
   }
   obj = ObjSys->BeginCreate(parent,kObjectAbstract);
   if (obj == OBJ_NULL) return obj; 
   DonorType->Set(obj,kDonorArchetype);
   SymName->Set(obj,name);
   ObjSys->EndCreate(obj);
   return obj;
}

////////////////////////////////////////

STDMETHODIMP_(ObjID) cTraitManager::CreateConcreteArchetype(const char* name, ObjID parent)
{
   ObjID obj;
   if (SymName->GetObj(name,&obj))
   {
      Warning(("Attempted to create duplicate %s archetype\n",name));
      return OBJ_NULL;
   }
   obj = ObjSys->BeginCreate(parent,kObjectConcrete);
   DonorType->Set(obj,kDonorArchetype);
   SymName->Set(obj,name);
   ObjSys->EndCreate(obj);
   return obj;
}

////////////////////////////////////////

STDMETHODIMP cTraitManager::AddObject(ObjID obj, ObjID archetype)
{
   if (archetype == OBJ_NULL) 
   {
      RemoveArchetypeLinks(obj,obj); 
      return S_FALSE;
   }

   if (!ObjSys->Exists(archetype)) 
   {
      AssertMsg1(FALSE,"Creating an object with non-existent archetype: %d",archetype);
      return E_FAIL;
   }
   
   // Make sure our archetype is valid for inheritance
   AssertMsg(ObjIsDonor(archetype), "Attempt to inherit from non-inheritable archetype");

   SetArchetype(obj, archetype); 

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cTraitManager::RemoveObject(ObjID obj)
{
   Cache->Flush(obj,FLUSH_ALL_TRAITS);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(ObjID) cTraitManager::CreateMetaProperty(const char* name, ObjID parent)
{
   AssertMsg(ObjSys->Exists(parent),"Creating an object with non-existent archetype");
   if (!ObjSys->Exists(parent)) return E_FAIL;

   ObjID obj;   
   if (SymName->GetObj(name,&obj))
   {
      Warning(("Tried to create duplicate %s metaproperty\n"));
      return OBJ_NULL;
   }

#ifdef DBG_ON
   int dtype;
   DonorType->Get(parent,&dtype);
   AssertMsg(OBJ_IS_ABSTRACT(parent) && dtype == kDonorMetaProp,"Creating a metaproperty with a non-metaproperty parent");
#endif // DBG_ON

   obj = ObjSys->BeginCreate(parent,kObjectAbstract);
   DonorType->Set(obj,kDonorMetaProp);
   SetArchetype(obj,parent); 
   SymName->Set(obj,name);
   ObjSys->EndCreate(obj);
   return obj;
}

////////////////////////////////////////

#define DEFAULT_PRI 1024
#define DEFAULT_PRI_DELTA 8


////////////////////////////////////////

#define HIGHER_PRIORITY(x) ((x) + DEFAULT_PRI_DELTA)
#define LOWER_PRIORITY(x)  (((x)*(DEFAULT_PRI-DEFAULT_PRI_DELTA) - DEFAULT_PRI)/DEFAULT_PRI)

//
// Figure out a good priority value for a new link with a special priority.
//

static tMetaPropertyPriority ComputeNewSpecialPriority(IRelation* metaprops, ObjID obj, tMetaPropertyPriority pri)
{
   BOOL above = (pri & kMPropPriorityBelowArchetype) == 0;
   tMetaPropertyPriority def = (above) ? DEFAULT_PRI : -DEFAULT_PRI;
   

   MetaPropLinks* links = GetObjectMetaPropLinks(obj);

   if (links == NULL || links->GetFirst() == NULL) 
      return def;

   switch (pri)
   {
      case kMPropHighestPriority:
         // Get the front link.  Check the right sign
         pri = links->GetFirst()->Value().pri;
         if (pri > 0)
            return HIGHER_PRIORITY(pri);
         break;

      case kMPropLowestPriority|kMPropPriorityBelowArchetype:
         // Get the back link.  Check the right sign
         pri = links->GetLast()->Value().pri;
         if (pri < 0)
            return -HIGHER_PRIORITY(-pri);
         break;         

      case kMPropHighestPriority|kMPropPriorityBelowArchetype:
      {
         // Iter backwards to find the last pos
         MetaPropLinks::cIter iter = links->Iter();
         for (; !iter.Done(); iter.Next())
         {
            pri = iter.Value().pri; 
            if (pri < 0) // Get the link just prior
               return -LOWER_PRIORITY(-pri);
         }
      }
      break;

      case kMPropLowestPriority:
      {
         MetaPropLinks::cNode* node;
         // Iter backwards to find the last nonneg priority
         for (node = links->GetLast(); node != NULL; node = node->GetPrevious())
         {
            pri = node->Value().pri; 
            if (pri > 0) 
               return LOWER_PRIORITY(pri);
         }
      }
      break;
       
      default:
         Warning(("Unknown Special MetaProp Priority\n"));
         break;
   }
   return def;
}

////////////////////////////////////////

STDMETHODIMP cTraitManager::AddObjMetaPropertyPrioritized(ObjID obj, ObjID meta, tMetaPropertyPriority pri)
{
   if (!OBJ_IS_ABSTRACT(meta)) return E_FAIL;

   if (IsMetaProperty(obj))
   {
      Warning(("Metaproperties cannot have metaproperties\n"));
      return E_FAIL;
   }

   if (pri & kMPropSpecialPriority)
   {

      // Compute min and max priority values.
      // We could conceivably cache these.  
      pri = ComputeNewSpecialPriority(MetaProps,obj,pri);
   }



   MetaProps->AddFull(obj,meta,&pri);
   return S_OK;
}


STDMETHODIMP cTraitManager::AddObjMetaProperty(ObjID obj, ObjID meta)
{
   return AddObjMetaPropertyPrioritized(obj,meta,kMPropHighestPriority);
}

////////////////////////////////////////

//
// Interpret the meaning of a "special" priority
//

static LinkID FindPriority(ILinkQuery* metaprops, tMetaPropertyPriority pri)
{
   BOOL above = (pri & kMPropPriorityBelowArchetype) == 0;
   tMetaPropertyPriority most = 0 ;
   LinkID best = LINKID_NULL;

   for (; !metaprops->Done(); metaprops->Next())
   {
      sLink link; 
      LinkID id = metaprops->ID();
      // @TODO: Faster LinkQuery API
      metaprops->Link(&link);
      tMetaPropertyPriority linkpri = *(tMetaPropertyPriority*)metaprops->Data();
      
      if ((above && linkpri < 0) || (!above && linkpri > 0)) continue;

      switch (pri & ~kMPropPriorityBelowArchetype)
      {
         case kMPropHighestPriority:
            if (linkpri > most || best == LINKID_NULL) 
               most = linkpri,best = id;
            break;
         case kMPropLowestPriority:
            if (linkpri < most || best == LINKID_NULL) 
               most  = linkpri, best = id;
            break;
       
         default:
            if (linkpri == pri)
               return id;
            break;
      }
   }

   return best;
}

////////////////////////////////////////


STDMETHODIMP cTraitManager::RemoveObjMetaPropertyPrioritized(ObjID obj, ObjID meta, tMetaPropertyPriority pri)
{
   Assert_(OBJ_IS_ABSTRACT(meta));
   ILinkQuery* query = MetaProps->Query(obj,meta); 
   LinkID id = FindPriority(query,pri);
   SafeRelease(query);
   if (!id)
      return S_FALSE;

   MetaProps->Remove(id);

   // blow out the donor cache
   Cache->Flush(obj,FLUSH_ALL_TRAITS); 
   IObjectQuery* oquery = Query(obj,kTraitQueryAllDescendents); 
   Cache->FlushObjSet(oquery,FLUSH_ALL_TRAITS); 
   SafeRelease(oquery); 

   return S_OK;
}

STDMETHODIMP cTraitManager::RemoveObjMetaProperty(ObjID obj, ObjID meta)
{
   return RemoveObjMetaPropertyPrioritized(obj,meta,kMPropHighestPriority);
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cTraitManager::ObjHasDonorIntrinsically(ObjID obj, ObjID donor)
{
   return MetaProps->AnyLinks(obj,donor); 
}

STDMETHODIMP_(BOOL) cTraitManager::ObjHasDonor(ObjID obj, ObjID donor)
{
   cAutoIPtr<IObjectQuery> query(Query(obj,kTraitQueryAllDonors));
   for (; !query->Done(); query->Next())
   {
      if (query->Object() == donor)
         return TRUE;
   }
   return FALSE;
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cTraitManager::ObjIsDonor(ObjID donor)
{
   return DonorType->IsRelevant(donor);
}

////////////////////////////////////////

STDMETHODIMP_(ObjID) cTraitManager::GetArchetype(ObjID obj)
{
   if (obj == OBJ_NULL) 
      return obj;
   cAutoIPtr<ILinkQuery> query ( MetaProps->Query(obj,LINKOBJ_WILDCARD)); 
   for (; !query->Done(); query->Next())
      if (*(tMetaPropertyPriority*)query->Data() == 0)
      {
         sLink link;
         query->Link(&link);
         return link.dest; 
      }
   return OBJ_NULL;
}

////////////////////////////////////////

STDMETHODIMP cTraitManager::SetArchetype(ObjID obj, ObjID arch)
{
   if (obj == OBJ_NULL)
      return S_FALSE;
   if (arch == OBJ_NULL)
      RemoveArchetypeLinks(obj,obj); 
   else if (!RemoveArchetypeLinks(obj,arch))
      AddArchetypeLink(obj,arch); 

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cTraitManager::IsArchetype(ObjID obj)
{
   int type = -1;
   DonorType->Get(obj,&type);
   return type == kDonorArchetype;
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cTraitManager::IsMetaProperty(ObjID obj)
{
   int type = -1;
   DonorType->Get(obj,&type);
   return type == kDonorMetaProp;
}


////////////////////////////////////////

STDMETHODIMP_(ITrait*) cTraitManager::CreateTrait(const sTraitDesc* desc, const sTraitPredicate* pred)
{
   ITrait* result;
   if (desc->flags & kTraitUninherited)
      result = new cUninheritedTrait(*desc,*pred);
   else if (desc->flags & kTraitUncached)
      result = new cInheritedTrait(*desc,*pred);
   else
      result = new cCachedTrait(*desc,*pred);

   Traits.Append(result);
   result->AddRef();
   return result;
}

////////////////////////////////////////

#define METAPROP_ARCHETYPE_NAME "MetaProperty"
#define ROOT_ARCHETYPE_NAME "Object"


STDMETHODIMP cTraitManager::Notify(eObjNotifyMsg msg, ObjNotifyData data)
{
   uObjNotifyData info;
   info.raw = data;
   AutoAppIPtr_(DonorCache,pCache);

   switch(NOTIFY_MSG(msg))
   {
      case kObjNotifyDelete:
         RemoveObject(info.obj);
         break;
         

      case kObjNotifyReset:
      {
         pCache->Clear();
      }
      break;

      case kObjNotifyLoad:
      {
         sDonorCacheParams params;
         pCache->GetParams(&params);
         params.flags |= kDonorCacheLoading;
         pCache->SetParams(&params); 
      }
      break; 

      case kObjNotifyDefault:
      case kObjNotifyPostLoad:
      {
         // Set up the root archetype appropriately. 
         DonorType->Set(ROOT_ARCHETYPE,kDonorArchetype);
         SymName->Set(ROOT_ARCHETYPE,ROOT_ARCHETYPE_NAME);

         // Set up the metaproperty archetype
         CreateBaseArchetype(METAPROP_ARCHETYPE_NAME,&MetaPropArchetype);
         DonorType->Set(MetaPropArchetype,kDonorMetaProp);
         SetArchetype(MetaPropArchetype,OBJ_NULL); 

         if (msg == NOTIFY_MSG(msg))  // no partition bits means final post-load
         {
            sDonorCacheParams params;
            pCache->GetParams(&params);
            params.flags &= ~kDonorCacheLoading;
            pCache->SetParams(&params); 
         }
      }

      break;
   }
   return S_OK;
}

STDMETHODIMP cTraitManager::Listen(HierarchyListenFunc func, HierarchyListenerData data)
{
   sListener listen = { func, data}; 
   Listeners.Append(listen); 
   return S_OK;
}

void cTraitManager::SendListenMessage(int kind, ObjID obj, ObjID donor)
{
   // First, blow out the donor cache
   Cache->Flush(obj,FLUSH_ALL_TRAITS); 
   IObjectQuery* query = Query(obj,kTraitQueryAllDescendents); 
   Cache->FlushObjSet(query,FLUSH_ALL_TRAITS); 
   SafeRelease(query); 

   // Now you can send your messages

   sHierarchyMsg msg = { kind, obj, donor}; 
   for (int i = 0; i < Listeners.Size(); i++)
   {
      sListener& listener = Listeners[i]; 
      listener.func(&msg,listener.data); 
   }
}



//
// Remove pri-zero metaprop links from obj that don't match arch 
//

BOOL cTraitManager::RemoveArchetypeLinks(ObjID obj, ObjID arch)
{
   BOOL found = FALSE; 

   ILinkQuery* query = MetaProps->Query(obj,LINKOBJ_WILDCARD); 
   for (; !query->Done(); query->Next())
   {
      LinkID id = query->ID(); 
      int pri = *(tMetaPropertyPriority*)query->Data(); 
      if (pri == 0)
      {
         if (!found && obj != arch)
         {
            sLink link;
            MetaProps->Get(id,&link); 

            if (link.source == obj && link.dest == arch) // no need to change
            {
               found = TRUE;  
               continue;
            }
         }
         MetaProps->Remove(id); 
      }
   }
   SafeRelease(query); 
   return found;
}


void cTraitManager::AddArchetypeLink(ObjID obj, ObjID arch)
{
   // archetype is the zeroeth metaproperty, so add it.
   tMetaPropertyPriority archpri = 0;
   MetaProps->AddFull(obj,arch,&archpri);
}


void cTraitManager::MetaPropListener(sRelationListenMsg* msg, RelationListenerData data)
{
   cTraitManager* us = (cTraitManager*)data;
   
   int kind = (msg->type & kListenLinkPostMortem) ? kDonorRemoved : kDonorAdded; 
   us->SendListenMessage(kind,msg->link.source,msg->link.dest); 
}

////////////////////////////////////////////////////////////
// CREATE THE TRAIT MANAGER
// 

tResult LGAPI TraitManagerCreate(void)
{
   IUnknown* outer = AppGetObj(IUnknown);
   cTraitManager* man = new cTraitManager(outer);
   return (man != NULL) ? NOERROR : E_FAIL;
}










