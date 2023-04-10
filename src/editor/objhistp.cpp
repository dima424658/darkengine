/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/objhistp.cpp,v 1.5 2000/02/19 13:11:08 toml Exp $

#include <appagg.h>
#include <aggmemb.h>

#include <hashpp.h>
#include <hashfunc.h>
#include <hshpptem.h>
#include <mprintf.h>

#include <traitman.h>
#include <iobjsys.h>
#include <objquery.h>
#include <simpwrap.h>
#include <objdef.h>
#include <editobj.h>

#include <objhistp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define STRLEN    64

typedef struct {
   int count;
   ObjID arch;
} ObjHistData;

typedef cHashTableFunctions<ObjID> ObjNameHashFunctions;
typedef cHashTable<ObjID, int, ObjNameHashFunctions> cObjHistCBTable;

#ifdef _MSC_VER
template cObjHistCBTable;
#endif

static cObjHistCBTable objs_used;

int ObjHistCompare(const void *p, const void *q)
{
   ObjHistData *a = (ObjHistData *) p;
   ObjHistData *b = (ObjHistData *) q;

   if (a->count > b->count)
      return -1;
   else
      return a->count < b->count;
}

static int BuildObjData(char ***objstrings, ObjHistData **histdata)
{
   int entries;
   IObjectQuery* pQuery;
   IObjectSystem* pOS = AppGetObj(IObjectSystem);
   ITraitManager* traitman = AppGetObj(ITraitManager);
   int i,count;
   int total = 0;
   ObjID obj,arch;
   cObjHistCBTable::cIter hashiter;

   objs_used.Clear();

   // okay, sift through every object in the world and collect stats
   pQuery = pOS->Iter(kObjectConcrete);
   while (!pQuery->Done())
   {
      obj = pQuery->Object();

      // reject transients, like texture concretes
      if (pOS->ObjIsTransient(obj))
      {
         pQuery->Next();
         continue;
      }

      arch = traitman->GetArchetype(obj);
      if ((obj != OBJ_NULL) && (arch != OBJ_NULL))
      {
         count = objs_used.Search(arch);
         objs_used.Set(arch,count+1);
         ++total;
      }
      pQuery->Next();
   }

   // take those stats and generate a list
   entries = objs_used.nElems();
   // extra entry for total 
   *objstrings = (char **)Malloc(sizeof(char *) * (entries+1));
   for (i=0; i < entries+1; i++)
      (*objstrings)[i] = (char *)Malloc(sizeof(char) * STRLEN);

   *histdata = (ObjHistData *)Malloc(sizeof(ObjHistData) * entries);

   hashiter = objs_used.Iter();
   count = 0;
   while (!hashiter.Done())
   {
      (*histdata)[count].arch = hashiter.Key();
      (*histdata)[count].count = hashiter.Value();
      count++;
      hashiter.Next();
   }

   SafeRelease(pQuery);
   SafeRelease(pOS);
   return total;
}


// handy dandy object histogram palette
void popup_obj_histogram(void)
{
   int count;
   char **objstrings;
   ObjHistData *histdata;
   int entries;
   int i,choice;
   ObjID arch;
   IObjectSystem* pOS = AppGetObj(IObjectSystem);
   int total;

   total = BuildObjData(&objstrings,&histdata);

   entries = objs_used.nElems();

   // sort the list
   qsort(histdata, entries, sizeof(ObjHistData), ObjHistCompare);

   sprintf(objstrings[0],"Total %d", total);
   // form the stringlist
   for (count=0; count < entries; count++)
   {
      sprintf(objstrings[count+1],"(%02d) %03d %s",count, histdata[count].count,pOS->GetName(histdata[count].arch));
   }
   // make the selection
   choice = PickFromStringList("Object Histogram", (const char* const*)objstrings, entries+1);

   // now set the default object
   if (choice >= 0)
   {
      arch = histdata[choice].arch;
      editObjSetDefaultArchetype(arch);
   }

   // some cleanup
   for (i=0; i < entries; i++)
      Free(objstrings[i]);
   Free(objstrings);
   Free(histdata);

   SafeRelease(pOS);
}

// simple alphabetical list of placed objects
void popup_obj_alpha(void)
{
   int count;
   char **objstrings;
   ObjHistData *histdata;
   int entries;
   int i,choice;
   ObjID arch;
   IObjectSystem* pOS = AppGetObj(IObjectSystem);

   BuildObjData(&objstrings,&histdata);
   entries = objs_used.nElems();
 
   // form the stringlist
   for (count=0; count < entries; count++)
   {
      sprintf(objstrings[count],"%s %03d",pOS->GetName(histdata[count].arch),histdata[count].count);
   }

   // make the selection
   choice = PickFromStringList("Object Alpha List", (const char* const*)objstrings, entries);

   // now set the default object
   if (choice != -1)
   {
      arch = histdata[choice].arch;
      editObjSetDefaultArchetype(arch);
   }

   // some cleanup
   for (i=0; i < entries; i++)
      Free(objstrings[i]);
   Free(objstrings);
   Free(histdata);

   SafeRelease(pOS);
}
