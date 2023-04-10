/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/trait.cpp,v 1.16 1999/05/19 16:05:39 mahk Exp $
#include <trait_.h>
#include <objquery.h>
#include <traisrch.h>
#include <trcache.h>
#include <osysbase.h>
#include <config.h>
#include <cfgdbg.h>
#include <mprintf.h>
#include <dlistsim.h>
#include <dlisttem.h>
#include <traitln_.h>
#include <relation.h>
#include <linkman.h>
#include <linkbase.h>

// Must be last header
#include <dbmem.h>

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseTrait,ITrait);

////////////////////////////////////////////////////////////
//
// cUninheritedTrait
//

STDMETHODIMP_(BOOL) cUninheritedTrait::PossessedBy(ObjID obj)
{
   return IntrinsicTo(obj);
}

////////////////////////////////////////

STDMETHODIMP_(ObjID) cUninheritedTrait::GetDonor(ObjID )
{
   return OBJ_NULL;
}

////////////////////////////////////////

STDMETHODIMP_(IObjectQuery*) cUninheritedTrait::GetAllDonors(ObjID obj)
{
   if (IntrinsicTo(obj))
      return CreateSingleObjectQuery(obj);
   else
      return CreateEmptyObjectQuery(); 
}

////////////////////////////////////////

STDMETHODIMP_(IObjectQuery*) cUninheritedTrait::GetAllHeirs(ObjID , eObjConcreteness)
{
   return CreateEmptyObjectQuery(); 
}

//------------------------------------------------------------
// FILTER QUERIES
//

class cTraitFilterQuery : public cFilterObjectQuery
{
   ITrait* Trait;
protected:
   BOOL Filter(ObjID obj) { return Trait->IntrinsicTo(obj);};

public:
   cTraitFilterQuery(IObjectQuery* q, ITrait* trait)
      : cFilterObjectQuery(q),Trait(trait)
   {
      Trait->AddRef();
      Skip();
   }

   ~cTraitFilterQuery() { SafeRelease(Trait);};
};

////////////////////////////////////////////////////////////
//
// cInheritedTrait
//

STDMETHODIMP_(BOOL) cInheritedTrait::PossessedBy(ObjID obj)
{
   if (IntrinsicTo(obj))
      return TRUE;
   else
      return GetDonor(obj) != OBJ_NULL;
}

////////////////////////////////////////

STDMETHODIMP_(ObjID) cInheritedTrait::GetDonor(ObjID obj)
{
   ObjID result = OBJ_NULL;
   IObjectQuery* query = GetAllDonors(obj);
   if (!query->Done()) result = query->Object();
   if (result == obj)
   {
      query->Next();
      result = (query->Done()) ? OBJ_NULL :query->Object();
   }
   SafeRelease(query);
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(IObjectQuery*) cInheritedTrait::GetAllDonors(ObjID obj)
{
   IObjectQuery* inner = TraitMan->Query(obj,kTraitQueryAllDonors);
   IObjectQuery* outer = new cTraitFilterQuery(inner,this);
   SafeRelease(inner);
   return outer;
}

////////////////////////////////////////

STDMETHODIMP_(IObjectQuery*) cInheritedTrait::GetAllHeirs(ObjID obj , eObjConcreteness which)
{
   // @TODO: make this actually stop when another donor overshadows the property
   IObjectQuery* inner = TraitMan->Query(obj,kTraitQueryAllDescendents); 
   IObjectQuery* outer = new cConcretenessFilterQuery(inner,which);
   SafeRelease(inner);
   return outer;
}


////////////////////////////////////////////////////////////
//
// Cached trait query
//
// A total rework of the depth first query 
//

//
// Helper class for iterating across metaprop links
//

class cChildren : public MetaPropLinks::cIter
{
public: 
   typedef MetaPropLinks::cIter cParent; 
   cChildren(const cParent& p)
      : cParent(p)
   {
   }

   cChildren() {}; 

   cChildren(ObjID obj) 
   {
      MetaPropLinks* links = GetObjectMetaPropLinks(obj); 
      if (links)
         *this = links->Iter(); 
   }

   ObjID Object()
   {
      return Value().dest; 
   }
   
}; 


//
// Search stack frame
//



struct sStackFrame
{
   ObjID node;
   BOOL intrinsic; 
   cChildren children;
   ObjID donor; // current opinion of donor
   ObjID through; // where we get donor through

   sStackFrame(ObjID n = OBJ_NULL, 
               BOOL i = FALSE,
               IObjectQuery* c = NULL, 
               ObjID d = OBJ_NULL,
               ObjID t = OBJ_NULL)
      : node(n),
        donor(d),
        through(t),
        intrinsic(i)
   {
   }


};

//
// Search stack
//

class cSearchStack : public cSimpleStack<sStackFrame>
{

};




class cCachedTraitQuery : public cBaseObjectQuery 
{
   TraitID ID;
   cAutoIPtr<ITrait> Trait; 
   cSearchStack Stack;
   BOOL do_spew;
   ObjID YieldMe; 
   ObjID SkipMe; 

#ifndef SHIP
#define trSpew(x) if (do_spew) mprintf x
#else
#define trSpew(x) 
#endif
   
   static IDonorCache* Cache;
   static ITraitManager* TraitMan;

   typedef cBaseObjectQuery cParent;

protected: 

   //
   // Given an object, build a query of successors, using cache where possible.
   //
   BOOL Expand(sStackFrame* frame)
   {
      ObjID obj = frame->node;
      ObjID donor;   
      ObjID through;

      if (obj == SkipMe)
      {
         SkipMe = OBJ_NULL; 
         trSpew(("Expanding past %d\n",obj)); 
      }
      
      YieldMe = OBJ_NULL; 

      // Look for us in the cache
      if (Cache->GetDonor(obj,ID,&donor,&through))
      {
         trSpew(("Cache: %d has %d through %d\n",obj,donor,through));
         if (donor == OBJ_NULL)
         {
            frame->children = cChildren(); 
            return FALSE;
         }

         frame->donor = donor;
         frame->through = through;

         if (frame->donor != SkipMe)
         {
            YieldMe = donor; 
            SkipMe = donor; 
         }

         cChildren rest(obj); 
         // fast forward the donor query past our "through" 

         for (; !rest.Done(); rest.Next())
            if (rest.Object() == through)
               break; 

         frame->children = rest; 
            
         return TRUE; 
      }
      else 
      {
         frame->children = cChildren(obj); 

         return !frame->children.Done();
      }
   }



   //
   // Open a stack frame for an object
   //
   void Open(ObjID obj)
   {
      sStackFrame frame(obj);
      frame.intrinsic = Trait->IntrinsicTo(obj);
      trSpew(("Opening %d intrinsic %d\n",obj,frame.intrinsic));

      Stack.Push(frame);
   }

   void PropagateDonor(const sStackFrame& finished)
   {
      // Now, set the donor of the current top, based on what we already knew.
      if (Stack.Top().donor == OBJ_NULL) // if we don't already know
      {
         ObjID donor = finished.donor;
         if (finished.intrinsic) 
            donor = finished.node;
         Stack.Top().donor = donor;
         Stack.Top().through = finished.node;
      }
   }

   //
   // Unwind the stack to the next upward branch
   //

   BOOL Contract()
   {
      if (Stack.Top().node == OBJ_NULL)
         return FALSE;

      while (Stack.Top().children.Done())
      {
         sStackFrame& finished = Stack.Top();
         // This node is finished.  Cache it.



         trSpew(("Closing %d  Donor: %d through %d Intrinsic: %d\n",
                 finished.node,
                 finished.donor,
                 finished.through,
                 finished.intrinsic));


         Cache->SetDonor(finished.node,ID,finished.donor,finished.through);

         Stack.Pop(); 

         // If we hit bottom, we're done.
         if (Stack.Top().node == OBJ_NULL)
            return FALSE;

         // propagate the donor backward
         PropagateDonor(finished);
         Stack.Top().children.Next();
      }
      return TRUE;
   }

   //
   // Go to the next object in the search, before filtering out non-donors
   //
   void NextObj()
   {
      if (!Expand(&Stack.Top()))
      {
         if (!Contract())
            return;
      }
      
      Open(Stack.Top().children.Object());
   }
   
   void Skip()
   {
      while(!Done() 
            && YieldMe == OBJ_NULL 
            && !(Stack.Top().intrinsic && Stack.Top().node != SkipMe))
      {
         trSpew(("Skipping %d\n",Stack.Top().node)); 

         NextObj();
      }
   }

   // @TODO: Release these interfaces one day
   void GrabIFaces()
   {
      if (Cache == NULL)
         Cache = AppGetObj(IDonorCache);

      if (TraitMan == NULL)
         TraitMan = AppGetObj(ITraitManager);
   }



public:
   cCachedTraitQuery(ObjID obj, ITrait* trait, TraitID id)
      : ID(id),   
        Trait(trait),
        do_spew(FALSE),
        YieldMe(OBJ_NULL),
        SkipMe(OBJ_NULL)
   {
#ifndef SHIP
      do_spew = config_is_defined("trait_cache_spew");
      trSpew(("Start %s search on %d\n",Trait->Describe()->name,obj));
#endif 
      Trait->AddRef();
      
      GrabIFaces();

      Open(obj);  
      //      NextObj();
      Skip();
   };

   ~cCachedTraitQuery() 
   {
      while (Stack.Top().node != OBJ_NULL)
      {
         sStackFrame frame = Stack.Pop();

         // if we got valid cache data, store it.
         trSpew(("Unwinding %d.  ",frame.node));
         if (frame.donor != OBJ_NULL) // valid donor data
         {
            trSpew(("Caching donor %d through %d",frame.donor,frame.through));
            Cache->SetDonor(frame.node,ID,frame.donor,frame.through);
         }
         PropagateDonor(frame);
         trSpew(("\n"));
      }

      trSpew(("End search.\n"));
   };


   STDMETHOD(Next)()
   {
      NextObj();
      Skip();
      return S_OK;
   }

   STDMETHOD_(BOOL,Done)() { return Stack.Top().node == OBJ_NULL;};
   STDMETHOD_(ObjID,Object)()  
   { 
      if (YieldMe != OBJ_NULL)
         return YieldMe; 
      return Stack.Top().node;
   };

};
#undef trSpew

ITraitManager* cCachedTraitQuery::TraitMan = NULL;
IDonorCache* cCachedTraitQuery::Cache    = NULL;



////////////////////////////////////////////////////////////
//
// cCachedTrait
//
//


cCachedTrait::cCachedTrait(const sTraitDesc& desc, const sTraitPredicate& pred)
   : cInheritedTrait(desc,pred),
   pCache(AppGetObj(IDonorCache))
{
   ID = pCache->NewTrait(&Desc);

   ConfigSpew("trait_id_spew",("Trait %s has id %d\n",desc.name,ID));

      
}

cCachedTrait::~cCachedTrait()
{
   SafeRelease(pCache);
}

//
// Go ahead and do as much of the first level of the search as you 
// can, using the cache.  This is particularly useful in the case
// of a concrete that is not in the cache
//
// TRUE means the out param is the donor you're looking for
//
// FALSE means donor is the first donor for search
//

BOOL cCachedTrait::get_cached_donor(ObjID obj, ObjID* donor)
{
   BOOL retval = FALSE;

   MetaPropLinks* links = GetObjectMetaPropLinks(obj); 
   if (!links)
      return FALSE; 

   MetaPropLinks::cIter iter;
   for (iter = links->Iter(); !iter.Done(); iter.Next())
   {
      ObjID qobj = iter.Value().dest; 

      ObjID cachedonor;
      ObjID through; 
      
      if (IntrinsicTo(qobj))
      {
         pCache->SetDonor(obj,ID,qobj,qobj);
         *donor = qobj;
         return TRUE;
      }
      
      if (pCache->GetDonor(qobj,ID,&cachedonor,&through))
      {
         if (cachedonor == OBJ_NULL)
            continue;
         else
         {
            *donor = cachedonor; 
            pCache->SetDonor(obj,ID,*donor,qobj);
            return TRUE;
         }
      }
      
      *donor = qobj;
      return FALSE;
   }
   // we got no donors
   *donor = OBJ_NULL;
   pCache->SetDonor(obj,ID,OBJ_NULL,OBJ_NULL);
   return TRUE;
}

STDMETHODIMP_(ObjID) cCachedTrait::GetDonor(ObjID obj)
{
   // check the cache first
   ObjID cachedonor,through;
   if (pCache->GetDonor(obj,ID,&cachedonor,&through))
      return cachedonor;
   
   // Now check the first level
   ObjID donor;
   if (get_cached_donor(obj,&donor))
      return donor;
   
   // If that fails, do the search
   ObjID result = OBJ_NULL;

   IObjectQuery* query = GetAllDonors(obj);
   if (!query->Done()) result = query->Object();
   if (result == obj)
   {
      query->Next();
      result = (query->Done()) ? OBJ_NULL :query->Object();
   }
   SafeRelease(query);
   return result;
}

STDMETHODIMP_(IObjectQuery*) cCachedTrait::GetAllDonors(ObjID obj)
{
   return new cCachedTraitQuery(obj,this,ID);
}

STDMETHODIMP cCachedTrait::Touch(ObjID obj)
{
   if (obj == OBJ_NULL) 
      pCache->Flush(FLUSH_ALL_OBJS,ID);
   else
   {
      IObjectQuery* objs = TraitMan->Query(obj,kTraitQueryAllDescendents);
      pCache->FlushObjSet(objs,ID);
      SafeRelease(objs);
   }

   return S_OK;
}







