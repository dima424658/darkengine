/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/trcache.h,v 1.6 2000/01/29 13:25:21 adurant Exp $
#pragma once
#ifndef __TRCACHE_H
#define __TRCACHE_H

#include <comtools.h>
#include <objtype.h>
#include <traitbas.h>

////////////////////////////////////////////////////////////
// DONOR CACHE INTERFACE
//

//
// Base types
//

typedef ulong TraitID; 
#define FLUSH_ALL_TRAITS ((TraitID)-1)
#define FLUSH_ALL_OBJS OBJ_NULL

F_DECLARE_INTERFACE(IDonorCache);
F_DECLARE_INTERFACE(IObjectQuery);

//
// Caching flags
// 

enum eDonorCacheFlags 
{
   kDonorCacheSpew            = (1 << 0),
   kDonorCachePermitConcrete  = (1 << 1), 
   kDonorCacheLoading         = (1 << 2), 
};

//
// Caching parameters
//

typedef ulong ulStatEntries;  // Current number of entries in the cache.

struct sDonorCacheParams
{
   ulong max_entries;  // cache size limit
   ulong flags;        // eDonorCacheFlags
};

typedef struct sDonorCacheParams sDonorCacheParams;

//
// Cache statistics 
//

struct sDonorCacheStats
{
   ulong adds;          // entries added
   ulong drops;         // entries dropped
   ulong hits;          // Get donors hit
   ulong misses;        // get donors missed
   ulong overwrites;    // adds of existing entries
   ulong flushes;       // times flushed
}; 


#undef INTERFACE
#define INTEFACE IDonorCache
DECLARE_INTERFACE_(IDonorCache,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Get a new trait ID number
   // 
   STDMETHOD_(TraitID,NewTrait)(THIS_ const sTraitDesc* tDesc) PURE;

   //
   // Add a cache entry 
   //
   STDMETHOD(SetDonor)(THIS_ ObjID obj, TraitID trait, ObjID donor, ObjID through) PURE;
   
   //
   // Lookup a cache entry
   // 
   STDMETHOD_(BOOL,GetDonor)(THIS_ ObjID obj, TraitID trait, ObjID* donor, ObjID* through) PURE;
   
   //
   // Clear the cache and all statistics
   //
   STDMETHOD(Clear)(THIS) PURE;

   //
   // Invalidate an object, or a set of objects.  FLUSH_ALL_TRAITS is a valid 
   // trait argument.  FLUSH_ALL_OBJS is a valid wildcard object for Flush
   //
   STDMETHOD(Flush)(THIS_ ObjID obj, TraitID trait) PURE;  
   STDMETHOD(FlushObjSet)(THIS_ IObjectQuery* objs, TraitID traits) PURE;

   //
   // Set/Get parameters (cache size, etc)
   //
   STDMETHOD(SetParams)(THIS_ const sDonorCacheParams* params) PURE;
   STDMETHOD(GetParams)(THIS_ sDonorCacheParams* params) PURE;

   //
   // Get performace statistics
   //
   STDMETHOD(GetTotalStats)(THIS_ sDonorCacheStats* stats) PURE;
   STDMETHOD(GetStatsByTrait)(THIS_ TraitID trait, sDonorCacheStats* stats) PURE;
   STDMETHOD(GetByTraitName)(THIS_ const char* name, sDonorCacheStats* stats) PURE;
   STDMETHOD(GetMaxID)() PURE;
   STDMETHOD(GetCacheEntries)() PURE;
};
//
// Create the donor cache
//

EXTERN void DonorCacheCreate(void);


#endif // __TRCACHE_H
