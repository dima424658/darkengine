/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/proptest.cpp,v 1.38 2000/01/18 12:35:24 PATMAC Exp $

#include <comtools.h>
#include <io.h>

#include <str.h>
#include <dynarray.h>

extern "C"
{
#include <editbr.h>
#include <brlist.h>
#include <status.h>
}
#include <ctype.h>
#include <appagg.h>
#include <propbase.h>
#include <propman.h>
#include <proptest.h>
#include <propguid.h>
#include <propface.h>
#include <propraw.h>
#include <proparry.h>
#include <command.h>
#include <mprintf.h>
#include <traitprp.h>
#include <iobjsys.h>
#include <objedit.h>
#include <traitman.h>
#include <traitbas.h>
#include <osysbase.h>
#include <objlpars.h>
#include <objquery.h>
#include <editobj.h>
#include <editbr_.h>
#include <vbrush.h>
#include <brinfo.h>
#include <trcachui.h>
#include <trcache.h>
#include <linkint.h>
#include <lnkquery.h>
#include <autolink.h>
#include <report.h>

#include <objnotif.h>
#include <dbfile.h>

// for texture search and destroy
#include <config.h>
#include <cfgtool.h>

// For tree edit
#include <otreeed.h>

// For obj EDIT
#include <iobjed.h>
#include <objedbas.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////
static ObjID parse_obj_arg(char* arg)
{
   // Remove trailing whitespace;
   int len = strlen(arg); 
   for (char* s = arg + len - 1; s > arg && isspace(*s); s--)
      *s = '\0';
   ObjID focus = EditGetObjNamed(arg);
   if (focus == OBJ_NULL)
   {
      editBrush* cur = vBrush_GetSel(); 
      if (brushGetType(cur) == brType_OBJECT)
         return brObj_ID(cur); 
   }

   return focus; 
}


////////////////////////////////////////

static void lookup_obj(char* symname)
{
   char buf[256];
   char* s = symname;

   while (!isspace(*s) && *s != '\0') s++;
   *s = '\0';
   
   ObjID obj = EditGetObjNamed(symname);

   AutoAppIPtr_(ObjectSystem,pObjSys); 

   if (obj != OBJ_NULL && pObjSys->Exists(obj))
   {
      sprintf(buf,"Found: %s",ObjEditName(obj));

      if (OBJ_IS_ABSTRACT(obj))
      {
         EditObjHierarchy(obj); 
      }
      else
      {
         editBrush* br = editObjGetBrushFromObj(obj);
         if (br != NULL)
            vBrush_SelectBrush(br);
      }
         
   }
   else
      sprintf(buf,"There is no obj named %s",symname);
   Status(buf);
}


////////////////////////////////////////

static void edit_obj_tree(char* arg)
{
   ObjID focus = parse_obj_arg(arg); 
   if (OBJ_IS_CONCRETE(focus))
   {
      AutoAppIPtr_(TraitManager,pTraitMan);
      focus = pTraitMan->GetArchetype(focus); 
   }

   EditObjHierarchy(focus); 
}


////////////////////////////////////////

static sObjEditorDesc objeddesc = 
{
   "",
   kObjEdShowAll
};

static void obj_edit(char* arg)
{
   ObjID obj = parse_obj_arg(arg); 

   AutoAppIPtr_(ObjEditors,pEditors);
   IObjEditor* ed = pEditors->Create(&objeddesc,obj); 
   ed->Go(kObjEdModal);
   SafeRelease(ed); 

}


////////////////////////////////////////

static void clear_trait_cache()
{
   AutoAppIPtr_(DonorCache,pCache);
   pCache->Clear();
}



////////////////////////////////////////

static void link_lock_count(void)
{
   AutoAppIPtr_(LinkManagerInternal,LinkMan);
   long count = LinkMan->GetLockCount(); 
   mprintf("Link lock count is %d\n",count);
}

////////////////////////////////////////

#define STRINGARG(x) ((x) == NULL ? "" : (x))

static void list_props(void)
{
   AutoAppIPtr_(PropertyManager,pMan); 
   sPropertyIter iter; 
   pMan->BeginIter(&iter); 
   IProperty* prop; 
   for (prop = pMan->NextIter(&iter); prop != NULL; prop = pMan->NextIter(&iter))
   {
      const sPropertyDesc* desc = prop->Describe(); 
      if (desc->ui.friendly_name != NULL)
         mprintf("%s\\%s = %s\n",STRINGARG(desc->ui.category),desc->ui.friendly_name,desc->name); 
      else
         mprintf("%s\n",desc->name);
      SafeRelease(prop); 
   } 
}

int count_links(ILinkQuery* query)
{
   int count;
   for (count = 0; !query->Done(); query->Next(), count++)
      ;
   return count;
}

static void link_dump_stats(void)
{
   AutoAppIPtr_(LinkManager,pLinkMan);
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   IObjectQuery* objects = pObjSys->Iter(kObjectAll); 
   for (; !objects->Done(); objects->Next())
   {
      ObjID obj = objects->Object(); 
      mprintf("Obj %s: ",ObjEditName(obj)); 
      ILinkQuery* from = pLinkMan->Query(obj,LINKOBJ_WILDCARD,RELID_WILDCARD);
      mprintf("links from: %d ",count_links(from));
      SafeRelease(from); 
      ILinkQuery* to = pLinkMan->Query(LINKOBJ_WILDCARD,obj,RELID_WILDCARD);
      mprintf("links to: %d\n",count_links(to)); 
      SafeRelease(to); 
   }
   SafeRelease(objects);

   mprintf("Total links: "); 
   ILinkQuery* query = pLinkMan->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD,RELID_WILDCARD); 

   mprintf("%d\n",count_links(query));
   SafeRelease(query); 
}


////////////////////////////////////////

static void destroy_obj(char* symname)
{
   ObjID obj = parse_obj_arg(symname); 
   if (obj != OBJ_NULL)
   {
      AutoAppIPtr_(ObjectSystem,pObjSys); 
      pObjSys->Destroy(obj); 
      Status ("Object Destroyed"); 
   }
   Status("No such object"); 
}

////////////////////////////////////////

// @HACK: here we have secret gnosis of the set of object hierarchy trees 

static const char* obj_roots [] = 
{
   "Object",
   "Metaproperty", 
   "Stimulus",
   "Texture",
   "Flow Group", 
   "Base Room",
   "This Source",
   "This Sensor", 
}; 

#define NUM_ROOTS (sizeof(obj_roots)/sizeof(obj_roots[0]))

static void find_lost_objects(void)
{
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   AutoAppIPtr_(TraitManager,pTraitMan);
   
   IObjectQuery* query = pObjSys->Iter(kObjectAll); 
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object();

      BOOL found = FALSE; 
      for (int i = 0; i < NUM_ROOTS; i++)
      {
         ObjID root = pObjSys->GetObjectNamed(obj_roots[i]);
         if (obj == root || pTraitMan->ObjHasDonor(obj,root))
         {
            found = TRUE; 
            break; 
         }
      }

      if (found)
         continue; 

      // Named objects can be lost 
      //      if (pObjSys->GetName(obj) != NULL) 
      //         continue; 

      ObjID missing;
      pTraitMan->CreateBaseArchetype("Missing",&missing); 
      //      pObjSys->SetObjTransience(missing,TRUE); 

      pTraitMan->SetArchetype(obj,missing); 
      //      pObjSys->SetObjTransience(obj,TRUE); 
   }
   SafeRelease(query); 
}


static void report_lost_objects(int WarnLevel, void *data, char *buffer)
{
   char *p=buffer;
   int numLost = 0;
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   AutoAppIPtr_(TraitManager,pTraitMan);

   rsprintf(&p,"\nChecking for Lost Objects\n");

   IObjectQuery* query = pObjSys->Iter(kObjectAll); 
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object();

      BOOL found = FALSE; 
      for (int i = 0; i < NUM_ROOTS; i++)
      {
         ObjID root = pObjSys->GetObjectNamed(obj_roots[i]);
         if (obj == root || pTraitMan->ObjHasDonor(obj,root))
         {
            found = TRUE; 
            break; 
         }
      }

      if (found)
         continue; 

      numLost++;
      rsprintf(&p, "ERROR: Object %s is lost!\n", ObjWarnName(obj) );

   }
   if ( numLost ) {
      rsprintf(&p,"\n%d Lost Objects\n", numLost );
   }
   SafeRelease(query); 
}


static void purge_missing_objs(void)
{
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   AutoAppIPtr_(TraitManager,pTraitMan); 
   ObjID missing = pObjSys->GetObjectNamed("Missing"); 
   ObjID metaprop = pTraitMan->RootMetaProperty(); 
   if (missing == OBJ_NULL)
   {
      Status("No 'Missing' Archetype"); 
      return; 
   }

   // remove "missing" from the tree 
   pTraitMan->SetArchetype(missing,OBJ_NULL); 

   pObjSys->Lock(); 
   cAutoIPtr<IObjectQuery> query (pTraitMan->Query(missing,kTraitQueryAllDescendents));
   for (;!query->Done(); query->Next())
   {
      ObjID obj = query->Object(); 
      if (!pTraitMan->ObjHasDonor(obj,ROOT_ARCHETYPE) 
          && !pTraitMan->ObjHasDonor(obj,metaprop))
         pObjSys->Destroy(query->Object()); 
   }
   pObjSys->Unlock(); 

   pObjSys->Destroy(missing); 
}

static void lookup_gamesys_textures(void)
{
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   AutoAppIPtr_(TraitManager,pTraitMan);
   
   //query just the abstract objects.
   IObjectQuery* query = pObjSys->Iter(kObjectAbstract); 
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object();

      BOOL found = FALSE; 
//hack, here I know that the root Texture archetype is called "Texture"
      ObjID root = pObjSys->GetObjectNamed("Texture");
      if ((obj != root) && (pTraitMan->ObjHasDonor(obj,root)) )
	found = TRUE;

      if (!found)
	continue;

      //DANGER! hack ahead.  Assume textures start with t_fam if they
      //are from files, and assume all are pcx files.
      char texname[256];
      strcpy(texname,pObjSys->GetName(obj));
      char texsubpath[256];
      char texpath[256];
      if (sscanf(texname,"t_fam%s",texsubpath))
	{
	  sprintf(texpath,"fam%s.pcx",texsubpath);
	  mprintf("%s",texpath);
	  char filename[256];
	  if (find_file_in_config_path(filename,texpath,"resname_base"))
	    mprintf("  Found\n");
	  else
	    mprintf(" NOT FOUND\n");
	}
      else
	mprintf("%s\n",texname);
   }
   SafeRelease(query); 
}

static void kill_old_gamesys_textures(void)
{
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   AutoAppIPtr_(TraitManager,pTraitMan);
   
   //query just the abstract objects.
   IObjectQuery* query = pObjSys->Iter(kObjectAbstract); 
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object();

      BOOL found = FALSE; 
//hack, here I know that the root Texture archetype is called "Texture"
      ObjID root = pObjSys->GetObjectNamed("Texture");
      if ((obj != root) && (pTraitMan->ObjHasDonor(obj,root)) )
	found = TRUE;

      if (!found)
	continue;

      //DANGER! hack ahead.  Assume textures start with t_fam if they
      //are from files, and assume all are pcx files.
      char texname[256];
      strcpy(texname,pObjSys->GetName(obj));
      char texsubpath[256];
      char texpath[256];
      if (sscanf(texname,"t_fam%s",texsubpath))
	{
	  sprintf(texpath,"fam%s.pcx",texsubpath);
 	  char filename[256];
	  if (!find_file_in_config_path(filename,texpath,"resname_base"))
	    {
	      mprintf("KILLING %s\n",texpath);
	      pObjSys->Destroy(obj);
	    }
	}
   }
   SafeRelease(query); 
}


////////////////////////////////////////

static void link_many(char* arg)
{
   cStr argStr(arg);
   cStr args[3];
   cStr fromStr;
   cStr toStr;
   int  len;
   
   for (int i = 0; i < 3; i++)
   {
      len = argStr.SpanExcluding(",");
      if (!len)
         return;
      argStr.Mid(args[i], 0, len);
      if (argStr[len])
         argStr.Remove(0, len + 1);
      else
         argStr.Remove(0, len);
      args[i].Trim();
   }

   cDynArray<ObjID> sources, destinations;
   cObjListParser   objListParser;

   if (args[1] == "*")
      destinations.Append(LINKOBJ_WILDCARD);
   else
      objListParser.Parse(args[1], &sources);

   if (args[2] == "*")
      destinations.Append(LINKOBJ_WILDCARD);
   else
      objListParser.Parse(args[2], &destinations);

   if (!sources.Size() || !destinations.Size())
      return;

   AutoAppIPtr(LinkManager);
   IRelation * pRelation = pLinkManager->GetRelationNamed(args[0]);
   
   if (!pRelation)
      return;
      
   for (int i = 0; i < sources.Size(); i++)
   {
      if (sources[i] != LINKOBJ_WILDCARD)
      {
         for (int j = 0; j < destinations.Size(); j++)
         {
            if (destinations[j] != LINKOBJ_WILDCARD && sources[i] != destinations[j])
               pRelation->Add(sources[i], destinations[j]);
         }
      }
   }

   pRelation->Release();
}

static void unlink_many(char* arg)
{
   cStr argStr(arg);
   cStr args[3];
   cStr fromStr;
   cStr toStr;
   int  len;
   
   for (int i = 0; i < 3; i++)
   {
      len = argStr.SpanExcluding(",");
      if (!len)
         return;
      argStr.Mid(args[i], 0, len);
      if (argStr[len])
         argStr.Remove(0, len + 1);
      else
         argStr.Remove(0, len);
      args[i].Trim();
   }

   cDynArray<ObjID> sources, destinations;
   cObjListParser   objListParser;

   if (args[1] == "*")
      destinations.Append(LINKOBJ_WILDCARD);
   else
      objListParser.Parse(args[1], &sources);

   if (args[2] == "*")
      destinations.Append(LINKOBJ_WILDCARD);
   else
      objListParser.Parse(args[2], &destinations);

   if (!sources.Size() || !destinations.Size())
      return;

   AutoAppIPtr(LinkManager);
   IRelation * pRelation = pLinkManager->GetRelationNamed(args[0]);
   
   if (!pRelation)
      return;
      
   for (int i = 0; i < sources.Size(); i++)
   {
      for (int j = 0; j < destinations.Size(); j++)
      {
         if (sources[i] != destinations[j])
         {
            ILinkQuery * pQuery;
            pQuery = pRelation->Query(sources[i], destinations[j]);
            for (; !pQuery->Done(); pQuery->Next())
               pRelation->Remove(pQuery->ID());
               
            pQuery->Release();
         }
      }
   }

   pRelation->Release();
}

void make_archetype(char* arg)
{
   ObjID objID = parse_obj_arg(arg); 

   AutoAppIPtr_(TraitManager, pTraitMan);
   AutoAppIPtr_(ObjectSystem, pObjSys);

   if (objID == OBJ_NULL)
   {
      Status("No object specified"); 
      return; 
   }

   char name[256];
   sprintf(name,"Archetype copy of %s", ObjEditName(objID)); 
   ObjID old_arch = pTraitMan->GetArchetype(objID); 
   if (old_arch == OBJ_NULL)
      old_arch = ROOT_ARCHETYPE; 

   ObjID new_arch = pTraitMan->CreateArchetype(name, old_arch);
  
   pObjSys->CloneObject(new_arch, objID);
   Status("Archetype Created"); 
}

////////////////////////////////////////

static BOOL LGAPI test_partition_func(ObjID obj, const sObjPartitionFilter* )
{
   // even numbered objects go in the briefcase :)
   return (obj % 2) == 0; 
}


static void test_briefcase_save(void)
{
   AutoAppIPtr_(ObjectSystem,pObjSys);

   // register the filter that describes the "briefcase" sub-partition 
   sObjPartitionFilter filter = { test_partition_func}; 
   pObjSys->SetSubPartitionFilter(kObjPartBriefcase,&filter); 

   // save the "briefcase" sub-partition 
   dbSave("briefcase.tmp",kObjPartConcrete|kObjPartBriefcase); 
}

static void test_briefcase_load(void)
{
   // merge load the "briefcase" subpartition 
   dbMergeLoad("briefcase.tmp",kObjPartConcrete|kObjPartBriefcase); 
}

////////////////////////////////////////

static void copy_props_to(char* arg)
{
   // Remove trailing whitespace;
   int len = strlen(arg); 
   for (char* s = arg + len - 1; s > arg && isspace(*s); s--)
      *s = '\0';
 
   ObjID targ = EditGetObjNamed(arg);
   if (targ == NULL)
   {
      Status("No such object");
      return ; 
   }

   editBrush* cur = vBrush_GetSel(); 
   if (brushGetType(cur) != brType_OBJECT)
   {
      Status("Selection is not an object");
      return ; 
   }

   ObjID src = brObj_ID(cur); 
      
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   pObjSys->CloneObject(targ,src); 
}

////////////////////////////////////////

static Command commands[] = 
{

   { "find_obj", FUNC_STRING, lookup_obj, "Look up an object by name or number",HK_EDITOR},
   { "destroy_obj", FUNC_STRING, destroy_obj, "Destroy an object by name or number",HK_EDITOR},
   { "trait_cache_params", FUNC_VOID, EditDonorCacheParams, "Edit trait cache parameters", HK_EDITOR },
   { "trait_cache_clear", FUNC_VOID, clear_trait_cache, "Empty the trait cache", HK_ALL },
   { "link_lock_count", FUNC_VOID, link_lock_count, "Report the number active link queries", HK_ALL },
   { "obj_tree", FUNC_STRING, edit_obj_tree, "Object Hierarchy editor", HK_EDITOR },
   { "edit_obj", FUNC_STRING, obj_edit, "edit an object", HK_EDITOR },
   { "copy_props_to", FUNC_STRING, copy_props_to, "Copy all props from selection to arg", HK_EDITOR },

   { "list_props", FUNC_VOID, list_props, "List properties on mono", HK_ALL },
   { "link_dump_stats", FUNC_VOID, link_dump_stats, "Dump tons of stats on links", HK_ALL },
   { "find_lost_objs", FUNC_VOID, find_lost_objects, "Find lost objects and set their archetypes to 'missing' ", HK_ALL },
   { "purge_missing_objs", FUNC_VOID, purge_missing_objs, "Delete all objects that inherit from 'Missing'", HK_ALL },

   { "briefcase_save", FUNC_VOID, test_briefcase_save, "Try to save the briefcase", HK_ALL },
   { "briefcase_load", FUNC_VOID, test_briefcase_load, "Try to load the briefcase", HK_ALL },

   { "link_many", FUNC_STRING, link_many, "Link many concrete objects", HK_EDITOR },
   { "unlink_many", FUNC_STRING, unlink_many, "Unlink many concrete objects", HK_EDITOR },

   { "make_archetype", FUNC_STRING, make_archetype, "Clone a concrete into an archetype", HK_EDITOR },
   { "print_gamesys_textures", FUNC_STRING, lookup_gamesys_textures, "Print to monolog all textures in the gamesys, and whether they are in path or not",HK_EDITOR},
   { "X99_kill_old_gamesys_textures", FUNC_STRING, kill_old_gamesys_textures, "Delete all textures in gamesys associated with files which aren't in the resource path.  DANGEROUS.",HK_EDITOR},
};


////////////////////////////////////////

void proptest_init_real(void)
{
   COMMANDS(commands,HK_EDITOR);
   ReportRegisterGenCallback( report_lost_objects, kReportAllObj, "Lost Objects", NULL );
}

////////////////////////////////////////

void proptest_term_real(void)
{
   ReportUnRegisterGenCallback( report_lost_objects, kReportAllObj, NULL );
}
 

