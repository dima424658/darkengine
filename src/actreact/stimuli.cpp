/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


#include <comtools.h>
#include <appagg.h>

#include <stimuli_.h>
#include <linkman.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <string.h>
#include <dbasemsg.h>
#include <dbfile.h>
#include <objquery.h>
#include <traitbas.h>
#include <trait.h>
#include <osysbase.h>
#include <propag8n.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// cStimuli IMPLEMENTATION
//

static sRelativeConstraint Constraints[] = 
{
   { kConstrainAfter, &IID_ILinkManager},
   { kConstrainAfter, &IID_ITraitManager},
   { kConstrainAfter, &IID_IObjectSystem},
   { kNullConstraint}
};


cStimuli::cStimuli(IUnknown* pOuter)
   : StimRoot(OBJ_NULL),
     pObjSys(NULL),
     pTraitMan(NULL),
     pStimDonors(NULL),
     pIsStimTrait(NULL)
{
   MI_INIT_AGGREGATION_1(pOuter,IStimuli,kPriorityNormal,Constraints); 
}

cStimuli::~cStimuli()
{

}

//------------------------------------------------------------
// IsStimTrait
//

static sTraitDesc stimtraitdesc = 
{
   "IsStimulus"
};

BOOL cStimuli::IsStimPred(ObjID obj, TraitPredicateData data)
{
   cStimuli* us = (cStimuli*)data;
   return (obj == us->StimRoot);
}

void cStimuli::create_is_stim_trait()
{
   sTraitPredicate pred = { IsStimPred, this}; 

   pIsStimTrait = pTraitMan->CreateTrait(&stimtraitdesc,&pred);
}
//------------------------------------------------------------
// STIM DONORS RELATION
//

//
// @TODO: Change this to be prioritized, perhaps breaking out into 
// its own abstraction.  Then change queryheirs and querydonors to use it.
//


static sRelationDesc donor_rel = 
{
   "StimDonor",
   kRelationTransient|kRelationNoEdit,
};

static sRelationDataDesc donor_rel_data = 
{
   "None", 0
};

static IRelation* create_donors_relation(void)
{
   return CreateStandardRelation(&donor_rel,&donor_rel_data,
                                 kQCaseSetSourceKnown
                                 |kQCaseSetDestKnown
                                 |kQCaseSetBothKnown);
}

//------------------------------------------------------------
// AGGREGATE PROTOCOL 
//

#define ROOT_STIMULUS_NAME "Stimulus"

STDMETHODIMP cStimuli::Init()
{
   pObjSys = AppGetObj(IObjectSystem);
   pTraitMan = AppGetObj(ITraitManager);
   pPropagation = AppGetObj(IPropagation); 

#ifdef PRIORITIZED_DONORS
   pStimDonors = create_donors_relation();
#endif 
   create_is_stim_trait();
   return S_OK;
}

STDMETHODIMP cStimuli::End()
{
   SafeRelease(pObjSys);
   SafeRelease(pTraitMan);
   SafeRelease(pStimDonors);
   SafeRelease(pPropagation); 
   return S_OK;
}


//------------------------------------------------------------
// IStimuli METHODS
//

STDMETHODIMP_(StimID) cStimuli::GetRootStimulus()
{
   return (StimID)StimRoot;
}

////////////////////////////////////////

STDMETHODIMP_(StimID) cStimuli::CreateStimulus(const sStimDesc* desc, StimID parent)
{
   char name[32]; 
   strncpy(name,desc->name,sizeof(name));
   name[sizeof(name)-1] = '\0';
   if (parent == OBJ_NULL)
      parent = StimRoot;
   else if (!IsStimulus(parent))
      return OBJ_NULL;
   
   ObjID result = pTraitMan->CreateArchetype(name,(ObjID)parent);
   CompileDonors(result);
   pPropagation->AddStimulus(result); 
   return (StimID)result;
}

////////////////////////////////////////

STDMETHODIMP_(StimID) cStimuli::AddStimulus(ObjID obj)
{
   if (!pTraitMan->ObjHasDonor(obj,StimRoot))
      pTraitMan->SetArchetype(obj,StimRoot); 
   CompileDonors(obj); 
   pPropagation->AddStimulus(obj); 
   return obj; 

}

////////////////////////////////////////

STDMETHODIMP_(StimID) cStimuli::GetStimulusNamed(const char* strname)
{
   ObjID result = pObjSys->GetObjectNamed(strname);
   if (!IsStimulus(result))
      return OBJ_NULL;

   return (StimID)result;
}

////////////////////////////////////////

STDMETHODIMP_(const char* ) cStimuli::GetStimulusName(StimID id)
{
   if (!IsStimulus(id))
      return NULL;
   
   return pObjSys->GetName(id);
}

////////////////////////////////////////

STDMETHODIMP cStimuli::DatabaseNotify(tStimDatabaseMsg msg, IUnknown* dbfile)
{
   switch(DB_MSG(msg))
   {
      case kDatabasePostLoad:
         if (!(msg & kObjPartAbstract))
            break;
         // else fall through
      case kDatabaseDefault:
         // create/find  the root stimulus
         pTraitMan->CreateBaseArchetype(ROOT_STIMULUS_NAME,&StimRoot);
         // detach it from the root archetype
         pTraitMan->SetArchetype(StimRoot,OBJ_NULL);
         CompileAllDonors();
         AddAllStimuli(); 
         break;
   }
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cStimuli::IsStimulus(ObjID obj)
{
   return pIsStimTrait->PossessedBy(obj);
}

////////////////////////////////////////

STDMETHODIMP_(IObjectQuery*) cStimuli::QueryDonors(StimID stim)
{

#ifdef PRIORITIZED_DONORS
   cAutoIPtr<ILinkQuery> q = pStimDonors->Query(stim,LINKOBJ_WILDCARD);
   IObjectQuery* donors = CreateLinkDestObjectQuery(q);
#else
   IObjectQuery* donors = pTraitMan->Query(stim,kTraitQueryAllDonors);
#endif 
   return donors;
}

////////////////////////////////////////

STDMETHODIMP_(IObjectQuery*) cStimuli::QueryHeirs(StimID stim)
{
#ifdef PRIORITIZED_DONORS
   cAutoIPtr<ILinkQuery> q(pStimDonors->Query(LINKOBJ_WILDCARD,stim));
   cAutoIPtr<ILinkQuery> rq(q->Inverse());
   return CreateLinkDestObjectQuery(rq);
#else
   return pTraitMan->Query(stim,kTraitQueryAllDescendents);
#endif 

}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cStimuli::InheritsFrom(StimID stimulus, ObjID donor)
{
#ifdef PRIORITIZED_DONORS
   cAutoIPtr<ILinkQuery> q(pStimDonors->Query(stimulus,donor));
   return !q->Done();
#else 
   return pTraitMan->ObjHasDonor(stimulus,donor);
#endif 
}


//------------------------------------------------------------
// Internal methods
//

void cStimuli::CompileDonors(ObjID obj)
{
#ifdef PRIORITIZED_DONORS
   // remove all old links
   ILinkQuery* oldq = pStimDonors->Query(obj,LINKOBJ_WILDCARD);
   for (; !oldq->Done(); oldq->Next())
   {
      pStimDonors->Remove(oldq->ID());
   }
   SafeRelease(oldq);

   // add the new donors
   IObjectQuery* objq = pTraitMan->Query(obj,kTraitQueryAllDonors);
   for (; !objq->Done(); objq->Next())
   {
      // add the link
      pStimDonors->Add(obj,objq->Object());
   }
   SafeRelease(objq);
#endif 
}

void cStimuli::CompileAllDonors()
{
#ifdef PRIORITIZED_DONORS
   IObjectQuery* q = pTraitMan->Query(StimRoot,kTraitQueryAllDescendents);
   for (; !q->Done(); q->Next())
   {
      ObjID obj = q->Object();
      if (OBJ_IS_ABSTRACT(obj))
         CompileDonors(obj);
   }
   SafeRelease(q);
#endif 
}


void cStimuli::AddAllStimuli()
{
   cAutoIPtr<IObjectQuery> query (pTraitMan->Query(StimRoot,kTraitQueryAllDescendents));
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object(); 
      Assert_(OBJ_IS_ABSTRACT(obj)); 
      pPropagation->AddStimulus(obj); 
   }
      
}


//------------------------------------------------------------
// Factory function
//

void StimuliCreate(void)
{
   AutoAppIPtr_(Unknown,pUnk);
   new cStimuli(pUnk);
}



