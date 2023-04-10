/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objremap.cpp,v 1.11 1999/10/06 15:59:44 BFarquha Exp $

#include <appagg.h>
#include <objremap.h>

#include <hashpp.h>
#include <hshpptem.h>

#include <tagfile.h>
#include <vernum.h>

#include <iobjsys.h>
#include <objquery.h>
#include <osysbase.h>

#include <traitman.h>

#include <netman.h>
#include <iobjnet.h>

// Must be last header
#include <dbmem.h>

//
// The Obj Mapping table
//

typedef cScalarHashFunctions<ObjID> cObjHashFns;
typedef cHashTable<ObjID, ObjID, cObjHashFns> cObjMap;

static cObjMap* pObjMap = NULL;


//
// SAVE
//

//
// Tag file tag & version
//

static TagFileTag ObjMapTag  = { "OBJ_MAP" };
static TagVersion ObjMapVer = { 0, 2};

//
// An single mapping pair
//


//----------------------------------------


EXTERN void SaveObjMappingTable(ITagFile* pFile)
{
   TagVersion v = ObjMapVer;


   if (SUCCEEDED(pFile->OpenBlock(&ObjMapTag,&v)))
   {
      AutoAppIPtr_(ObjectSystem, pObjSys);  // grab the object system

      // iterate over all abstract objects, write down their names.
      IObjectQuery* q = pObjSys->Iter(kObjectAbstract);
      for (; !q->Done(); q->Next())
      {
         ObjID obj = q->Object();
         const char* name = pObjSys->GetName(obj);
         if (name != NULL)
         {
            ulong size = strlen(name)+1;
            pFile->Write((char*)&obj,sizeof(obj));
            pFile->Write((char*)&size,sizeof(size));
            pFile->Write(name,size);

         }
      }
      q->Release();

      pFile->CloseBlock();
   }
}

//------------------------------------------------------------
// Load in the mapping table.
//

void LoadObjMappingTable(ITagFile* pFile)
{
   // toast the old objmap, and build a new one
   delete pObjMap;
   pObjMap = new cObjMap;

   AutoAppIPtr_(TraitManager,pTraitMan);
   AutoAppIPtr_(ObjectSystem,pObjSys); // grab the object system

   TagVersion v = ObjMapVer;
   if (SUCCEEDED(pFile->OpenBlock(&ObjMapTag,&v)))
   {
      BOOL fixed_size = !VersionNumsEqual(&ObjMapVer,&v);
      BOOL warned = FALSE;

      while (pFile->TellFromEnd() > 0)
      {
         ObjID loadobj;

         // read in an objid
         ulong len = pFile->Read((char*)&loadobj,sizeof(loadobj));
         Assert_(len == sizeof(loadobj));

         ulong size;
         if (fixed_size)
            size = sizeof(Label);
         else
         {
            len = pFile->Read((char*)&size,sizeof(size));
            Assert_(len == sizeof(size));
         }

         char* buf = new char[size];
         len = pFile->Read(buf,size);
         Assert_(len == size);
         buf[size-1] = '\0';

         // Find the object with its name.
         ObjID obj = pObjSys->GetObjectNamed(buf);

         if (obj == OBJ_NULL)
         {
            // lazily create the parent for all "missing" archetypes
            ObjID missing;
            const char* name = "Missing";
            pTraitMan->CreateBaseArchetype(name,&missing);

            if (!warned)
               Warning(("There are objects missing from the gamesys\n"));
            warned = TRUE;

            obj = pTraitMan->CreateArchetype(buf,missing);
         }

         if (obj != loadobj && obj != OBJ_NULL) // don't bother with identity mappings
            pObjMap->Insert(loadobj,obj);

         delete [] buf;
      }
      pFile->CloseBlock();
   }
}

//------------------------------------------------------------

void ClearObjMappingTable(void)
{
   delete pObjMap;
   pObjMap = NULL;
}

//------------------------------------------------------------


void AddObjMappingToTable(ObjID targ, ObjID src)
{
   if (pObjMap) {
      pObjMap->Set(src,targ);
#ifdef NEW_NETWORK_ENABLED
      AutoAppIPtr(NetManager);
      if (pNetManager->IsNetworkGame()) {
         AutoAppIPtr(ObjectNetworking);
         pObjectNetworking->NotifyObjRemapped(targ, src);
      }
#endif
   }
}


//------------------------------------------------------------
// Remap an object
//

ObjID ObjRemapOnLoad(ObjID obj)
{
   ObjID result = obj; // default to identity mapping
   if (pObjMap != NULL)
      pObjMap->Lookup(obj,&result);
   return result;
}
