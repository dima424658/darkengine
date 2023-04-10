/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/scrptprp.cpp,v 1.25 1999/05/19 16:12:30 mahk Exp $
#include <scrptapi.h>
#include <scrptprp.h>

#include <iobjsys.h>
#include <osysbase.h>
#include <property.h>
#include <propert_.h>
#include <dataops_.h>
#include <propsprs.h>

#include <objnotif.h>
#include <dbasemsg.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <dynarray.h>

#include <traitman.h>
#include <trait.h>
#include <objquery.h>

#include <edittool.h>
#include <simpwrap.h>

#include <listset.h>
#include <lststtem.h>

#include <vernum.h>
#include <tagfile.h>

#include <mprintf.h>

#include <traitman.h>
#include <traitbas.h>

// For script datapath
#include <datapath.h>
#include <config.h>

// Command stuff
#include <ctype.h>
#include <command.h>
#include <status.h>

#include <scrptpr_.h>
#include <scrpted.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// SCRIPT PROPERTY
//


//
// Structure desc
//

#define SCRIPT_LINE(N) \
  { "Script "#N, kFieldTypeString, FieldLocation(sScriptProp,script[N]), }


static sFieldDesc script_fields[] =
{
   SCRIPT_LINE(0),
   SCRIPT_LINE(1),
   SCRIPT_LINE(2),
   SCRIPT_LINE(3),
  { "Don't Inherit", kFieldTypeBool, FieldLocation(sScriptProp,no_inherit), }

};

static sStructDesc script_sdesc = StructDescBuild(sScriptProp, kStructFlagNone, script_fields);

//
// Property desc
//

static sPropertyDesc script_pdesc =
{
   "Scripts",
   kPropertySendEndCreate, // flags
   NULL, // Constraints
   1, 1,  // version
};


cScriptProp* cScriptProp::TheProp = NULL;

////////////////////////////////////////

BOOL cScriptProp::add_scripts_to_array(ObjID obj, cArray& array)
{
   sScriptProp* scrs;
   if (Get(obj,&scrs))
   {
      for (int i = 0; i < SCRIPTS_PER_PROP; i++)
      {
         if (scrs->script[i][0] != '\0')
            array.Append(scrs->script[i]);
      }
      return scrs->no_inherit;
   }
   return FALSE;
}

////////////////////////////////////////

void cScriptProp::rebuild_obj_scripts(ObjID obj)
{
   cArray array;

   IObjectQuery* query = mpExemplars->GetAllDonors(obj);
   for (; !query->Done(); query->Next())
      if (add_scripts_to_array(query->Object(),array))
         break;
   SafeRelease(query);

   if (array.Size() > 0)
      mpScriptMan->SetObjScripts(obj,array,array.Size());
   else
      mpScriptMan->ForgetObj(obj);
}

////////////////////////////////////////

void cScriptProp::rebuild_all_scripts(ObjID obj)
{
   if (OBJ_IS_CONCRETE(obj))
      rebuild_obj_scripts(obj);
   IObjectQuery* query = mpExemplars->GetAllHeirs(obj,kObjectConcrete);
   for (; !query->Done(); query->Next())
      rebuild_obj_scripts(query->Object());
   SafeRelease(query);
}

////////////////////////////////////////

HRESULT cScriptProp::AddModule(const char* name)
{
   sModule mod(name);
   mModules.AddElem(mod);

   return mpScriptMan->AddModule(mod);
}

////////////////////////////////////////

void cScriptProp::DropModule(const char* name)
{
   sModule mod(name);
   mModules.RemoveElem(mod);

   mpScriptMan->RemoveModule(mod);
}

////////////////////////////////////////

void cScriptProp::DropAllModules()
{
   mModules.RemoveAll();
   mpScriptMan->ClearModules();
}

////////////////////////////////////////

static const TagFileTag ScrTag = { "ScrModules" };
static const TagVersion ScrVer = { 1, 1};

static BOOL setup_file(ITagFile* file)
{
   TagVersion v = ScrVer;

   HRESULT retval = file->OpenBlock(&ScrTag,&v);
   if (!VersionNumsEqual(&ScrVer,&v))
   {
      file->CloseBlock();
      return FALSE;
   }
   // If we wanted to to version back compatibility stuff, we would do it here.

   return SUCCEEDED(retval);
}

static void cleanup_file(ITagFile* file)
{
   file->CloseBlock();
}


void cScriptProp::LoadModules(ITagFile* file)
{
   if (setup_file(file))
   {
      while(file->TellFromEnd() > 0)
      {
         char name[128];
         Verify(file->Read(name,sizeof(name)) == sizeof(name));

         sModule mod(name);
         mModules.AddElem(mod);
         mpScriptMan->AddModule(mod);
      }
      cleanup_file(file);
   }
}

void cScriptProp::SaveModules(ITagFile* file)
{
   if (setup_file(file))
   {
      cModuleSet::cIter iter;
      for (iter = mModules.Iter(); !iter.Done(); iter.Next())
      {
         char name[128];
         const sModule& mod = iter.Value();
         strncpy(name,mod.name,sizeof(name));
         name[sizeof(name)-1] = '\0';
         Verify(file->Write(name,sizeof(name)) == sizeof(name));
      }
      cleanup_file(file);
   }
}

void LGAPI cScriptProp::HierarchyListener(const sHierarchyMsg* msg, HierarchyListenerData data)
{
   ((cScriptProp*)data)->rebuild_all_scripts(msg->obj);
}

void cScriptProp::add_listener(void)
{
   AutoAppIPtr_(TraitManager,pTraitMan);
   pTraitMan->Listen(HierarchyListener,this);
}

//------------------------------------------------------------
// IProperty methods
//


void cScriptProp::OnListenMsg(ePropertyListenMsg msg, ObjID obj, 
                              uPropListenerValue value)
{
   rebuild_all_scripts(obj);
   cBaseScriptProp::OnListenMsg(msg,obj,value);
   if (msg == kListenPropUnset)
      mpScriptMan->ForgetObj(obj);
}

STDMETHODIMP cScriptProp::Delete(ObjID obj)
{
   HRESULT retval = cBaseScriptProp::Delete(obj);
   rebuild_all_scripts(obj);
   return retval;
}

STDMETHODIMP_(void) cScriptProp::Notify(ePropertyNotifyMsg msg, PropNotifyData data)
{
   ObjID obj = (ObjID)data;

   switch (NOTIFY_MSG(msg))
   {
      case kObjNotifyCreate:
      {
         sScrMsg msg(obj,"Create"); 
         mpScriptMan->SendMessage(&msg); 
      }
      break;

      case kObjNotifyDelete:
      {
         sScrMsg msg(obj,"Destroy"); 
         mpScriptMan->SendMessage(&msg);          
      }
      break;

      case kObjNotifyReset:
         mModules.RemoveAll();
         mpScriptMan->ForgetAllObjs();
         mpScriptMan->ClearModules();
         mpScriptMan->EndScripts(); 
         break;

      case kObjNotifySave:
         if (msg & kDBMission)
            SaveModules((ITagFile*)data);
         break;

      case kObjNotifyLoad:
         if (msg & kDBMission)
            LoadModules((ITagFile*)data);
         break;

   }
   cBaseScriptProp::Notify(msg,data);
}

////////////////////////////////////////////////////////////
// SCRIPT COMMANDS
//
// Script commands not involving the script property are in scrptcmd.
//

#include <filemenu.h>

// filter for save load fun
static sFileMenuFilter scr_file_filters[] = 
{
   { "Script Files (*.osm)", "*.osm"},
   { "All Files (*.*)", "*.*"},
   { NULL }
};

// remove any directories and such from a file path, leaving only the
// filename (also clobbers trailing whitespace)
static void prune_path(char *str)
{
   int length = strlen(str);
   for (char* s = str + length - 1; s >= str && isspace(*s); s--)
      *s = '\0';

   int search = length - 1;
   while (str[search] != '\\'
       && str[search] != '/'
       && str[search] != ':') {
      if (!search)
         return;
      --search;
   }
   Warning(("Script property should not have full path: %s\n", str));
   memmove(str, str + search + 1, length - search + 1);
}

static char _fname_buf[256]="";
static char *get_real_fname(char *str, eFileMenuMode fMode)
{
  if (str != NULL)
      while (isspace(*str)) str++;

#ifdef DIALOG
   if (str == NULL || *str == '\0')
   {
      str = FileDialog(fMode, scr_file_filters, "OSM", _fname_buf, 
                       sizeof(_fname_buf));
      if (str == NULL || *str == '\0')
         return NULL;
   }
#endif

   prune_path(str);

   return str;
}

static void script_load(char* arg)
{
   arg=get_real_fname(arg,kFileMenuLoad);
   if (arg && *arg)
   {
      if (SUCCEEDED(cScriptProp::TheProp->AddModule(arg)))
         Status("Module Loaded");
      else
         Status("Failed to load module!");
   }
}

// If no script is specified we pop up a menu.
static void script_drop(char* arg)
{
   if (arg && *arg) 
   {
      prune_path(arg);
      cScriptProp::TheProp->DropModule(arg);
      Status ("Module dropped.");
   } 
   else 
   {
      if (PickAndDropScript())
         Status ("Module dropped.");
   } 
   Status ("No script modules dropped.");
}

static void script_drop_all()
{
   cScriptProp::TheProp->DropAllModules();
   Status ("All modules dropped.");
}

static void dump_modules(void)
{
   cScriptProp::cModuleSet::cIter iter;
   mprintf("Script modules in mission:\n");
   for (iter = cScriptProp::TheProp->Modules(); !iter.Done(); iter.Next())
   {
      mprintf("'%s'\n",iter.Value().name);
   }
}


static Command commands[] =
{
   // module handling
   { "script_load", FUNC_STRING, script_load,
     "Load a script file, add to mission", HK_ALL },
   { "script_drop", FUNC_STRING, script_drop,
     "drop a script file from mission.", HK_ALL },
   { "script_drop_all", FUNC_VOID, script_drop_all,
     "drop all script files from mission.", HK_ALL },
   { "script_dump_files", FUNC_STRING, dump_modules,
     "List mission script files on mono\n", HK_ALL },
};

static Datapath script_path;

EXTERN void InitScriptReactions(); 


void CreateScriptProp(void)
{
   COMMANDS(commands,HK_ALL);
   StructDescRegister(&script_sdesc);

   IScriptProperty* prop = new cScriptProp(&script_pdesc);
   SafeRelease(prop);

   // @TODO: move this someplace appropriate
   InitScriptReactions(); 

   // Set up the load path
   char pathbuf[256] = "";
   config_get_raw("script_module_path", pathbuf,sizeof(pathbuf));
   DatapathAdd(&script_path,pathbuf);

   AutoAppIPtr(ScriptMan);
   pScriptMan->SetModuleDatapath(&script_path);
}
