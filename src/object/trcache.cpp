/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/trcache.cpp,v 1.11 1999/05/19 16:06:33 mahk Exp $
#include <appagg.h>

#include <osystype.h>
#include <trcache_.h>
#include <hshsttem.h>
#include <dlisttem.h>
#include <objquery.h>
#include <osysbase.h>

#include <traitlnk.h>
#include <traitman.h>
#include <traitbas.h>
#include <linkman.h>
#include <relation.h>
#include <linkbase.h>

#include <string.h>
#include <mprintf.h>
#include <config.h>
#include <dynarray.h>
#include <hshpptem.h>

//
// INCLUDE THESE ABSOLUTLY LAST
//
#include <initguid.h>
#include <tcachiid.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// cDonorCache Implementation
//
//
//  clear_status clears the zeroth element of TraitDescriptions and DonorStats dynamic arrays.
//  A new element is appended to these arrays which start at element one in this application.
//
static void clear_stats(TraitDescriptions* Descriptions,
						DonorStats* IDStats)
{
   // Clear the trait descriptions status array.
   Descriptions->Append(NULL);  // Append a NULL pointer to the Descriptions array.
   Descriptions->SetSize(1);    // Set the Descriptions array size to one.
   // Clear the ID status array.
   sDonorCacheStats NullEntry;  // Create a dummy array entry into IDStats.
   memset(&NullEntry,NULL,sizeof(sDonorCacheStats));  // Clear the dummy entry.
   IDStats->Append(NullEntry); // Append the cleared entry to the IDStats array.
   IDStats->SetSize(1);        // Set the array size to one.
};

static sDonorCacheParams default_params =
{
   1024,    // max_entries
   0,       // flags
};

#ifdef DBG_ON
#define trSpew(x) do { if (Params.flags & kDonorCacheSpew) mprintf x; } while(0)
#else
#define trSpew(x)
#endif 


//------------------------------------------------------------
// Construction and Deconstruction
//

static sRelativeConstraint Constraints[] = 
{
   { kNullConstraint}
};


IMPLEMENT_AGGREGATION_SELF_DELETE(cDonorCache);


cDonorCache::cDonorCache(IUnknown* pOuter)
   : Params(default_params), 
     StatEntries(0), // Clear the universal cache status counter. 
     NextID(1)
{
   clear_stats(&TraitDescs, &StatsByID); // Clear the Trait description and ID stats arrays.
   INIT_AGGREGATION_1(pOuter,IID_IDonorCache,this,kPriorityNormal,Constraints);
	
};

cDonorCache::~cDonorCache()
{
   Clear();
}

//------------------------------------------------------------
// IDonorCache methods
//

//
// This method takes the trait description structure input 
// and attaches it to the new Trait ID. 
//
STDMETHODIMP_(TraitID) cDonorCache::NewTrait(const sTraitDesc* tDesc)
{
    StatsByID.Grow(1);  // Increase the statistics dynamic array by a cleared element
	ulong NextID = TraitDescs.Append(tDesc);  // Add the trait description to trait array
 	Assert_(TraitDescs.Size() == StatsByID.Size());  // Check for equivalent sizes
	return NextID;
}


////////////////////////////////////////

STDMETHODIMP cDonorCache::SetDonor(ObjID obj, TraitID id, ObjID donor, ObjID thru)
{
   if (Params.flags & kDonorCacheLoading)
      return S_FALSE; 

   if (OBJ_IS_CONCRETE(obj))
   {
      if (Params.flags & kDonorCachePermitConcrete)
         trSpew(("Permitting Concrete Object %d\n",obj));
      else
         return S_FALSE;
   }

   sCacheKey key(obj,id);
   cDonorCacheEntry* e = Table.Search(&key);


   if (e == NULL) // new entry
   {
      e = new cDonorCacheEntry(obj,id,donor,thru);
      Add(e);
   }
   else
   {
	  Assert_(id < StatsByID.Size());  // Check for valid id
	  StatsByID[id].overwrites++;  // Add overwrites count to the particular ID
      e->donor = (short)donor;
      e->through = (short)thru;
      Touch(e);
   }
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cDonorCache::GetDonor(ObjID obj, TraitID trait, ObjID* donor, ObjID* thru)
{
   // quick reject
   if (Params.flags & kDonorCacheLoading)
      return FALSE; 

   if (OBJ_IS_CONCRETE(obj) && !(Params.flags & kDonorCachePermitConcrete))
      return FALSE;

   sCacheKey key(obj,trait);
   cDonorCacheEntry* e = Table.Search(&key);
   
   if (e != NULL)
   {
      *donor = e->donor;
      *thru = e->through;
      Assert_(trait < StatsByID.Size());  // Check for valid id
      StatsByID[trait].hits++;  // Add hit count to the particular ID
      Touch(e);
      return TRUE;
   }
   else
   {
      trSpew(("Trait cached missed obj %d trait %d\n",obj,trait));
	  Assert_(trait < StatsByID.Size());  // Check for valid id
      StatsByID[trait].misses++;
      return FALSE;
   }
}

////////////////////////////////////////

STDMETHODIMP cDonorCache::Clear()
{
   trSpew(("Clearing Donor Cache\n"));
   Table.Clear(Params.max_entries); 
   LRU.DestroyAll();
   
   cEntriesByObj::cIter iter; 
   for (iter = ByObj.Iter(); !iter.Done(); iter.Next())
      delete iter.Value(); 

   ByObj.Clear(Params.max_entries/4); 

   StatEntries = 0; 
   // Do we really want to clear this?  
   sDonorCacheStats zeroes; 
   memset(&zeroes,0,sizeof(zeroes)); 
   for (int i = 0; i < StatsByID.Size(); i++)
	   StatsByID[i] = zeroes; 

   return S_OK;
}

////////////////////////////////////////


STDMETHODIMP cDonorCache::Flush(ObjID obj, TraitID trait)
{

   // single entry case
   if (obj != FLUSH_ALL_OBJS)
   {
      if (trait != FLUSH_ALL_TRAITS)
      {
         sCacheKey key(obj,trait);
         Assert_(trait < StatsByID.Size());  // Check for valid id
         StatsByID[trait].flushes++;
         cDonorCacheEntry* node = Table.Search(&key);
         if (node != NULL)
            Drop(node);
      }
      else 
      {
         cObjEntryList* objlist = ByObj.Search(obj); 

         if (!objlist)
            return S_OK; 

         cDonorCacheEntry* node = objlist->GetFirst(); 
         cDonorCacheEntry* next = NULL; 

         for (; node != NULL; node = next)
         {
            // we have to use the correct type, so we get the correct GetNext(); 
            cObjEntryListNode* listnode = node; 
            next = listnode->GetNext();

            if (trait != FLUSH_ALL_TRAITS && node->key.trait != trait)
            {  // Flush all traits
#ifndef SHIP 
               for (int traitflush=1;traitflush<=trait;traitflush++)
                  StatsByID[traitflush].flushes++;
#endif 
               continue;
            }

            Drop(node);
         }

      
      }
   }
   else 
   {
      cDonorCacheEntry* node = (cDonorCacheEntry*)LRU.GetFirst();
      cDonorCacheEntry* next = NULL;
   
      for (; node != NULL; node = next)
      {
         // we have to use the correct type, so we get the correct GetNext(); 
         cDonorLRUNode* listnode = node; 
         next = listnode->GetNext();

         if (trait != FLUSH_ALL_TRAITS && node->key.trait != trait)
         {  // Flush all traits
#ifndef SHIP 
            for (int traitflush=1;traitflush<=trait;traitflush++)
               StatsByID[traitflush].flushes++;
#endif 
            continue;
         }

         Drop(node);
      }
   }
   return S_OK;
}

////////////////////////////////////////

// @OPTIMIZE: !!!!! This is gratuitously slow in the ALL_TRAITS case
//            A hashed "object set" abstraction would be of use here. 

STDMETHODIMP cDonorCache::FlushObjSet(IObjectQuery* q, TraitID trait)
{
#ifndef SHIP
   BOOL all = trait == FLUSH_ALL_TRAITS; 
   if (all)
   {  // Gather stats for flushing all traits
		for (int traitflush=1;traitflush < StatsByID.Size();traitflush++)
			StatsByID[traitflush].flushes++;
   }
   else
      StatsByID[trait].flushes++; 
#endif 

   
   for (; !q->Done(); q->Next())
   {
      Flush(q->Object(),trait);

#ifndef SHIP
      if (!all)
      {
         Assert_(trait < StatsByID.Size());  // Check for valid id
         StatsByID[trait].flushes--;
      }
#endif 
   }
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cDonorCache::SetParams(const sDonorCacheParams* params)
{
   BOOL was_loading = Params.flags & kDonorCacheLoading; 
   Params = *params;

   // enforce max_entries
   Assert_(Params.max_entries >= 0);
   while (StatEntries > Params.max_entries && StatEntries > 0)
      Drop((cDonorCacheEntry*)LRU.GetFirst());

   if (!was_loading && (Params.flags & kDonorCacheLoading))
      Flush(FLUSH_ALL_OBJS,FLUSH_ALL_TRAITS); 

   return S_OK;
}

STDMETHODIMP cDonorCache::GetParams(sDonorCacheParams* params)
{
   *params = Params;
   return S_OK;
}

//////////////////////////////////////////////////////////
//  This method sums all statstics that have been accumulated
//  separately into the trait statistics array and returns the total.
//////////////////////////////////////////////////////////
STDMETHODIMP cDonorCache::GetTotalStats(sDonorCacheStats* stats)
{
   int StatsSize = StatsByID.Size()-1;  // Get the ID status array size
   // Clear the stats output structure.
   memset(stats,0,sizeof(sDonorCacheStats));

   if (StatsSize)  // Get totals only if there are totals to get.
   {
	   for (int StatsIndex = 1; StatsIndex <= StatsSize; StatsIndex++)
	   {
			stats->adds+=StatsByID[StatsIndex].adds;
			stats->drops+=StatsByID[StatsIndex].drops;
			stats->hits+=StatsByID[StatsIndex].hits;
			stats->misses+=StatsByID[StatsIndex].misses;
			stats->overwrites+=StatsByID[StatsIndex].overwrites;
			stats->flushes+=StatsByID[StatsIndex].flushes;
	   }
   }

   return S_OK;
}

///////////////////////////////////////////////////////////
//  This method GetStatusByTrait uses the trait ID to index
//  into the DonorStats array. If the ID is valid, cache
//  statistics are copied into the stats location and S_OK
//  is returned.  S_FALSE is returned if the trait ID is invalid.
///////////////////////////////////////////////////////////

STDMETHODIMP cDonorCache::GetStatsByTrait(TraitID trait, sDonorCacheStats* stats)
{
   // Return a valid sDonorCacheStats value if the trait ID is valid.
   if (trait < StatsByID.Size()) // Check for an out of bounds trait ID.
   {
       memcpy(stats,&StatsByID[trait],sizeof(sDonorCacheStats));  // Save the match.
       return S_OK;
   }
   else
	   return S_FALSE;
}

////////////////////////////////////////////////////////////
//   This method GetByTraitName uses the name input parameter
//   as a search key into the TraitDescription dynamic array.
//   Once a match is found between the key and a name in the
//   array, the resulting array position is used as an index
//   into the DonorStats array where a valid stats item is
//   copied into the stats location.
////////////////////////////////////////////////////////////
STDMETHODIMP cDonorCache::GetByTraitName(const char* name, sDonorCacheStats* stats)
{
    // Get the size of the TraitDescriptions array.
    int DescSize = TraitDescs.Size()-1;
	if (DescSize)  // Search only if there are elements to be found.
	{
	    // Search for a name match between the key and TraitDescriptions array item.
	    for (int DynarrayIndex = 1; DynarrayIndex <= DescSize; DynarrayIndex++)
		{
		    if (!(strcmp(name, TraitDescs[DynarrayIndex]->name))) {
                memcpy(stats,&StatsByID[DynarrayIndex],sizeof(sDonorCacheStats));  // save the match.
			    return S_OK;
			}
		}
	}
   return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////
// This method GetMaxID retrieves the current size of the trait status array.
// Assume that a zero size can be returned.
////////////////////////////////////////////////////////////////////////////
STDMETHODIMP cDonorCache::GetMaxID()
{
    ulong MaxID = StatsByID.Size()-1;  // get the size of the status array
	mprintf( "Largest ID is: %d\n",MaxID);
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
// This method GetStatEntries prints the current entry count in the cache.
////////////////////////////////////////////////////////////////////////////
STDMETHODIMP cDonorCache::GetCacheEntries()
{
	mprintf( "Current cache entries: %d\n",StatEntries);
    return S_OK;
};


//------------------------------------------------------------
// Internals
//

void cDonorCache::Add(cDonorCacheEntry* e)
{
   trSpew(("Donor Cache Add: %d gets %d from  %d through %d\n",e->key.obj,e->key.trait,e->donor,e->through));
   Table.Insert(e);
   LRU.Append(e);
   cObjEntryList* objlist = ByObj.Search(e->key.obj); 
   if (!objlist)
   {
      objlist = new cObjEntryList; 
      ByObj.Insert(e->key.obj,objlist); 
   }
   objlist->Append(e); 

   StatEntries++;
   Assert_(e->key.trait < StatsByID.Size());  // Check for valid id
   StatsByID[e->key.trait].adds++;

   if (StatEntries > Params.max_entries)
      Drop((cDonorCacheEntry*)LRU.GetFirst());
}

void cDonorCache::Drop(cDonorCacheEntry* e)
{
   trSpew(("Donor Cache Drop: %d gets %d from  %d through %d\n",e->key.obj,e->key.trait,e->donor,e->through));
   LRU.Remove(e);
   Table.Remove(e);

   cObjEntryList* objlist = ByObj.Search(e->key.obj); 
   Assert_(objlist); 
   objlist->Remove(e); 
   if (objlist->GetFirst() == NULL)
   {
      ByObj.Delete(e->key.obj); 
      delete objlist; 
   }

   StatEntries--;  
   Assert_(e->key.trait < StatsByID.Size());  // Check for valid id
   StatsByID[e->key.trait].drops++;
   delete e;
}


void cDonorCache::Touch(cDonorCacheEntry* e)
{
   trSpew(("Donor Cache Touch: %d gets %d from  %d through %d\n",e->key.obj,e->key.trait,e->donor,e->through));

   LRU.Remove(e);
   LRU.Append(e);
}



//------------------------------------------------------------
// Aggregate protocol
//

HRESULT cDonorCache::Init()
{
#ifdef LISTEN_TO_LINKS

   AutoAppIPtr_(LinkManager,LinkMan);
   cAutoIPtr<IRelation> meta (LinkMan->GetRelationNamed(LINK_METAPROP_NAME));
   cAutoIPtr<IRelation> arch (LinkMan->GetRelationNamed(LINK_ARCHETYPE_NAME));

   meta->Listen(kListenLinkBirth|kListenLinkDeath,HierarchyListener,this);
   arch->Listen(kListenLinkBirth|kListenLinkDeath,HierarchyListener,this);
#endif 

   // use config to initialize params
   sDonorCacheParams params;
   GetParams(&params);
   config_get_int("trait_cache_size",&params.max_entries);
   config_get_int("trait_cache_flags",&params.flags);
   SetParams(&params);

   return S_OK;
}

HRESULT cDonorCache::End()
{
   return S_OK;
}

////////////////////////////////////////////////////////////

void DonorCacheCreate(void)
{
   AutoAppIPtr_(Unknown,pAgg);
   new cDonorCache(pAgg); 
}
