/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/trcachui.cpp,v 1.6 1998/09/28 11:32:20 CMONTER Exp $

#include <trcachui.h>
#include <trcache.h>
#include <sdesc.h>
#include <isdesced.h>
#include <sdesbase.h>
#include <isdescst.h>
#include <appagg.h>
#include <mprintf.h>

// Must be last header
#include <dbmem.h>

//////////////////////////////////////////////////////////////
// DONOR CACHE PARAMETER EDITOR  
//

//
// Parameter sdesc
//


char* bit_names[] = { "Spew", "Permit Concretes" };

static sFieldDesc param_fields [] = 
{
   { "Max entries", kFieldTypeInt, FieldLocation(sDonorCacheParams,max_entries), kFieldFlagUnsigned },
   { "Flags", kFieldTypeBits, FieldLocation(sDonorCacheParams,flags), kFieldFlagUnsigned , 0, 2, 2, bit_names},
};

static sStructDesc param_desc = StructDescBuild(sDonorCacheParams,kStructFlagNone,param_fields); 

//
// Parameter editor callback
//

static void LGAPI param_edit_CB(sStructEditEvent* event, StructEditCBData )
{
   BOOL freeit = FALSE;
   sDonorCacheParams* p = (sDonorCacheParams*)event->ed->Struct();
   AutoAppIPtr_(DonorCache,pCache);

   switch (event->kind)
   {
      case kStructEdApply:
         pCache->SetParams(p);
         pCache->GetParams(p); // re-get it
         break;
   }


}

//
// Edit the darn thing
//
static sStructEditorDesc param_edit_desc = { "Trait Cache Params", kStructEditAllButtons }; 

void EditDonorCacheParams(void)
{
   AutoAppIPtr_(DonorCache,pCache);
   sDonorCacheParams edit_me;
   pCache->GetParams(&edit_me);

   IStructEditor* ed = CreateStructEditor(&param_edit_desc,&param_desc,&edit_me);
   ed->SetCallback(param_edit_CB,NULL);
   if (ed->Go(kStructEdModal))
   {
      pCache->SetParams(&edit_me);
   }
   SafeRelease(ed);
}

////////////////////////////////////////////////////////////
// TRAIT CACHE STATS DISPLAY
//

//
// stats sdesc
//

static sFieldDesc stats_fields [] = 
{
   { "Adds", kFieldTypeInt, FieldLocation(sDonorCacheStats,adds), kFieldFlagUnsigned },
   { "Drops", kFieldTypeInt, FieldLocation(sDonorCacheStats,drops), kFieldFlagUnsigned },
   { "Hits", kFieldTypeInt, FieldLocation(sDonorCacheStats,hits), kFieldFlagUnsigned },
   { "Misses", kFieldTypeInt, FieldLocation(sDonorCacheStats,misses), kFieldFlagUnsigned },
   { "Overwrites", kFieldTypeInt, FieldLocation(sDonorCacheStats,overwrites), kFieldFlagUnsigned },
   { "Flushes", kFieldTypeInt, FieldLocation(sDonorCacheStats,flushes), kFieldFlagUnsigned },

};

static sStructDesc stats_desc = StructDescBuild(sDonorCacheStats,kStructFlagNotEdit,stats_fields); 

//
// Dump the stats
// 

void DumpDonorCacheStats(void)
{
   AutoAppIPtr_(DonorCache,pCache);
   sDonorCacheStats stats;
   pCache->GetTotalStats(&stats);  // Get the totals of all trait statistics.
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Dump(&stats_desc,&stats);
}

///////////////////////////////////////////////////////////
//  This routine gets a donor status by trait ID.
//  If the trait is valid, cache statistics are displayed.
///////////////////////////////////////////////////////////
void GetTraitStats(TraitID trait)
{
   AutoAppIPtr_(DonorCache,pCache);
   sDonorCacheStats stats;
   bool result = pCache->GetStatsByTrait(trait,&stats);  // Get the totals of all trait statistics.
   if (result == S_OK) {
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Dump(&stats_desc,&stats);
   }
   else
	   mprintf("Donor status not found for trait ID %d\n",trait);
}

////////////////////////////////////////////////////////////////////
//  This routine gets trait statistics using the trait name
//  If the trait name is valid, cache statistics are displayed.
////////////////////////////////////////////////////////////////////
void GetNamedStats(const char * name)
{
   AutoAppIPtr_(DonorCache,pCache);
   sDonorCacheStats stats;
   bool result = pCache->GetByTraitName(name, &stats);
   if (result == S_OK) {
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Dump(&stats_desc,&stats);
   }
   else
	   mprintf("Trait status not found for name %s\n",name);
}


////////////////////////////////////////////////////////////////////
//  This routine displays the largest trait ID currently available.
////////////////////////////////////////////////////////////////////
void GetMaxID(void)
{
   AutoAppIPtr_(DonorCache,pCache);
   pCache->GetMaxID();
}

////////////////////////////////////////////////////////////////////////////
// This routine gets and prints the current entry count in the cache.
////////////////////////////////////////////////////////////////////////////
void GetCacheEntryCount(void)
{
   AutoAppIPtr_(DonorCache,pCache);
   pCache->GetCacheEntries();
}

