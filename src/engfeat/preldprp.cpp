/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/preldprp.cpp,v 1.4 2000/02/19 13:15:08 toml Exp $
#include <preldprp.h>
#include <proptype.h>
#include <propbase.h>
#include <propfac_.h>

#include <objtype.h>
#include <objdef.h>
#include <traitman.h>
#include <traitbas.h>
#include <iobjsys.h>
#include <objquery.h>
#include <objnotif.h>

#include <dlist.h>
#include <dlisttem.h>

#ifndef SHIP
#include <config.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
#endif

////////////////////////////////////////////////////////////
// "PRELOAD" property.  Lets you set archetypes 
// that are to be "pre-loaded."  Basically we just create
// them when the gamesys post-loads 
//

static sPropertyDesc preload_desc = 
{
   "Preload", 
   0, // flags
   NULL, 0, 0,  // constraints, version 
   {
      "Engine Features",
      "Preload"
   }
};

//
// The property class 
//

class cPreloadProp: public cGenericBoolProperty
{
public:
   cPreloadProp()
      : cGenericBoolProperty(&preload_desc,kPropertyImplLlist)
   {
   }

}; 

static cPreloadProp* gpPreloadProp = NULL; 

void PreloadPropInit(void)
{
   gpPreloadProp = new cPreloadProp; 
}

void PreloadPropTerm(void)
{
   SafeRelease(gpPreloadProp); 
}


class cObjList: public cSimpleDList<ObjID>
{
}; 

void PreloadObjects(void)
{
   AutoAppIPtr_(TraitManager,pTraitMan); 
   AutoAppIPtr_(ObjectSystem,pObjSys);

   cObjList load_us; 
               
   sPropertyObjIter iter; 
   ObjID obj; 
   BOOL val; 

#ifndef SHIP
   if (config_is_defined("no_preload"))
      return;
#endif

   gpPreloadProp->IterStart(&iter); 
   //   Warning(("Start preload\n")); 
   while(gpPreloadProp->IterNextValue(&iter,&obj,&val))
      if (val && OBJ_IS_ABSTRACT(obj))
      {
         cAutoIPtr<IObjectQuery> q = pTraitMan->Query(obj,kTraitQueryAllDescendents); 
         for (; !q->Done(); q->Next())
         {
            ObjID arch = q->Object(); 
            BOOL preload = FALSE; 
            if (pTraitMan->IsArchetype(arch) 
                && gpPreloadProp->Get(arch,&preload) 
                && preload)
            {
               // Just add to the list 
               load_us.Append(arch); 
            }

         }
      }
   gpPreloadProp->IterStop(&iter); 
   //   Warning(("Stop preload\n")); 

   cObjList::cIter list_iter; 
   for (list_iter = load_us.Iter(); !list_iter.Done(); list_iter.Next())
   {
      ObjID arch = list_iter.Value(); 

      // Just create one, and then destroy it
      // That should preload the cache, as well
      // as doing any app-specific rebuild concreting.  
      ObjID temp = pObjSys->Create(arch,kObjectConcrete); 
      pObjSys->Destroy(temp); 
   }

}




