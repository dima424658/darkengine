/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gamestr.cpp,v 1.7 1998/09/29 10:40:25 TOML Exp $
#include <stdio.h>
#include <gamestr.h>
#include <propface.h>
#include <propbase.h>
#include <string.h>
#include <hashset.h>
#include <hshsttem.h>
#include <appagg.h>
#include <aggmemb.h>

#include <resapi.h>
#include <resistr.h>
#include <strrstyp.h>
#include <filespec.h>
#include <iobjsys.h>
#include <traitman.h>

#include <command.h>
#include <ctype.h>

// include these last 
#include <dbmem.h>
#include <initguid.h>
#include <gstrguid.h>

//
// Hash set elem for game string properties
//
struct sTablePropElem
{
   cStr table; 
   IStringProperty* prop; 
   IStringRes* res; 

   sTablePropElem(const char* t = "", IStringProperty* p = NULL, IStringRes* r = NULL)
      : table(t), prop(p), res(r)
   {
      if (prop)
         prop->AddRef(); 
      if (res)
         res->AddRef(); 
   }

   ~sTablePropElem()
   {
      SafeRelease(prop);
      SafeRelease(res); 
   }
}; 

//
// The hash set 
//
class cStrPropTable : public cStrIHashSet<sTablePropElem*>
{
   virtual tHashSetKey GetKey(tHashSetNode p) const 
   {
      return (tHashSetKey)(const char*)((sTablePropElem*)p)->table; 
   }
};

static cStr table_basepath = "strings/";

// utility functions

// convert name to valid string lookup key
void SafetyPupSays(cStr *pName)
{
   BOOL safe = FALSE;
   char ch;

   // now take out any invalid characters
   while (!safe)
   {
      ch = pName->Find('#');
      if (ch != -1)
         pName->SetAt(ch,'p');
      else
      {
         ch = pName->Find('-');
         if (ch != -1)
            pName->SetAt(ch,'n');
         else
         {
            ch = pName->Find(' ');
            if (ch != -1)
               pName->SetAt(ch,'_');
            else
               safe = TRUE;
         }
      }
   }
}

//
// IGameStrings implementation
//

class cGameStrings: public cCTDelegating<IGameStrings>,
                    public cCTAggregateMemberControl<kCTU_Default> 
{
   static sRelativeConstraint gConstraints[]; 

public: 
   cGameStrings(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter,IGameStrings,kPriorityNormal, gConstraints);   
   }

protected:
   cStrPropTable mTable; 
   IResMan* mpResMan; 

   STDMETHOD(Init)()
   {
      mpResMan = AppGetObj(IResMan); 
      return S_OK; 
   }

   STDMETHOD(End)()
   {
      mTable.DestroyAll(); 
      SafeRelease(mpResMan); 
      return S_OK; 
   }

   void ParsePropString(const char* in, cStr* name, cStr* text)
   {
      // look for colon 
      const char* s = strchr(in,':');
      if (!s)
      {
         *name = in;
         *text = ""; 
         return; 
      }

      // Peril! mutate in place
      //*s = '\0';
      // save off name
      *name = in; 
      // now mutate back 
      //*s = ':'; 

      s++; 
      // Now find the quote
      s = strchr(s,'\"'); 
      if (s)
      {
         s++;  // skip past quote
         const char* q = strchr(s,'\"'); 
         
         if (q)
         {
            // Mutate in place again 
            //*q = '\0'; 
            *text = s; 
            //*q = '\"'; 
         }
         else
            *text = ""; 
      }
      else
         *text = s; 

   }


public: 

   STDMETHOD_(cStr,FetchString)(const char* table, const char* name, const char* default_val, const char* relpath)
   {
      cStr result = default_val; 
      if (name == NULL || *name == '\0')
         return result; 

      cAutoIPtr<IRes> res = mpResMan->Bind(table,
                                           RESTYPE_STRING,
                                           NULL,
                                           relpath); 
      if (!res)
      {
         Warning(("FetchString(%s, %s) failed.\n", table, name));
         return result;
      }

      cAutoIPtr<IStringRes> strres (IID_IStringRes,res); 
      
      const char* s = strres->StringLock(name); 
      if (s)
      {
         result = s; 
         strres->StringUnlock(name); 
      }
      return result; 
   }

   void DoResPrep(sTablePropElem* elem)
   {
      if (elem->res) 
         return; 

      // Look up the string table resource 
      cAutoIPtr<IRes> res = mpResMan->Bind(elem->table,
                                           RESTYPE_STRING,
                                           NULL,
                                           table_basepath); 
      if (!!res)
         Verify(SUCCEEDED(res->QueryInterface(IID_IStringRes,(void**)&elem->res))); 
   }

   STDMETHOD(RegisterProp)(const char* table, IStringProperty* prop)
   {
      AssertMsg1(!mTable.Search(table),"String table %s is already registered.",table); 

      sTablePropElem* elem = new sTablePropElem(table,prop);

      DoResPrep(elem);
      mTable.Insert(elem); 
      return S_OK; 
   }

   STDMETHOD_(cStr,FetchObjString)(ObjID obj, const char* table)
   {
      sTablePropElem* elem = mTable.Search(table); 
      AssertMsg1(elem,"No registered string table named %s",table);
      const char* raw; 
      const char* objname;
      cStr name,text(""); 

      DoResPrep(elem); 

      if (!elem->prop->Get(obj,&raw))
      {
         AutoAppIPtr(ObjectSystem);
         AutoAppIPtr(TraitManager);

         objname = pObjectSystem->GetName(obj);
         // since most concrete objs aren't named, go and get the 
         // name from our parent.
         if (objname == NULL)
         {
            ObjID arch;
            arch = pTraitManager->GetArchetype(obj);
            if (arch != OBJ_NULL)
               objname = pObjectSystem->GetName(arch);
            else
               return(""); // this should never happen
         }
         name.FmtStr("%s",objname);
         SafetyPupSays(&name);
      }
      else
      {
         ParsePropString(raw,&name,&text); 
      }
      if (elem->res)
      {
         const char* s = elem->res->StringLock(name); 
         if (s)
         {
            text = s; 
            elem->res->StringUnlock(name); 
         }
      }
      return text;          
   }

   void DoDumpProp(sTablePropElem* elem)
   {
      cFileSpec fname = cFileSpec(table_basepath + elem->table);
      fname.SetFileExtension("STR"); 

      cStr fnamestr; 
      fname.GetNameString(fnamestr); 
      FILE* file = fopen(fnamestr,"wt"); 
      if (file == NULL)
      {
         Warning(("DoDumpProp: Could not open fname %s!\n",fnamestr));
         return;
      }

      sPropertyObjIter iter; 
      IStringProperty* prop = elem->prop; 
      ObjID obj; 
      const char* str; 
      prop->IterStart(&iter); 
      while (prop->IterNextValue(&iter,&obj,&str))
      {
         cStr name,text; 
         ParsePropString(str,&name,&text); 
         fprintf(file,"%s: \"%s\"\n",(const char*)name,(const char*)text); 
      }
      prop->IterStop(&iter); 
      fclose(file); 
   }

   STDMETHOD(DumpProp)(const char* table)
   {
      if (table)
      {
         sTablePropElem* elem = mTable.Search(table); 
         AssertMsg1(elem,"No registered string table named %s",table);
         DoDumpProp(elem); 
      }
      else
      {
         tHashSetHandle handle; 
         sTablePropElem* elem; 

         for (elem = mTable.GetFirst(handle); elem != NULL; elem = mTable.GetNext(handle))
            DoDumpProp(elem); 
      }
      return S_OK; 

   }

   void DoLoadProp(sTablePropElem* elem)
   {
      sPropertyObjIter iter; 
      IStringProperty* prop = elem->prop; 
      ObjID obj; 
      const char* str; 
      prop->IterStart(&iter); 
      while (prop->IterNextValue(&iter,&obj,&str))
      {
         cStr name,text; 
         ParsePropString(str,&name,&text); 
         DoResPrep(elem); 
         if (elem->res)
         {
            char* s = elem->res->StringLock(name); 
            if (s)
            {
                prop->Set(obj, cStr{ static_cast<const char*>(name) } + ": \"" + s + "\"");
               elem->res->StringUnlock(name); 
            }
         }
      }
      prop->IterStop(&iter); 
   }

   STDMETHOD(LoadProp)(const char* table)
   {
      if (table)
      {
         sTablePropElem* elem = mTable.Search(table); 
         AssertMsg1(elem,"No registered string table named %s",table);
         DoLoadProp(elem); 
      }
      else
      {
         tHashSetHandle handle; 
         sTablePropElem* elem; 

         for (elem = mTable.GetFirst(handle); elem != NULL; elem = mTable.GetNext(handle))
            DoLoadProp(elem); 

      }
      return S_OK; 
   }

   void DoStripProp(sTablePropElem* elem) 
   {
      sPropertyObjIter iter; 
      IStringProperty* prop = elem->prop; 
      ObjID obj; 
      const char* str; 
      prop->IterStart(&iter); 
      while (prop->IterNextValue(&iter,&obj,&str))
      {
         cStr name,text; 
         ParsePropString(str,&name,&text); 
         prop->Set(obj,name); 
      }
      prop->IterStop(&iter); 
   }

   STDMETHOD(StripProp)(const char* table)
   {
      if (table)
      {
         sTablePropElem* elem = mTable.Search(table); 
         AssertMsg1(elem,"No registered string table named %s",table);
         DoStripProp(elem); 
      }
      else
      {
         tHashSetHandle handle; 
         sTablePropElem* elem; 

         for (elem = mTable.GetFirst(handle); elem != NULL; elem = mTable.GetNext(handle))
            DoStripProp(elem); 

      }
      return S_OK; 
   }

   void DoModernizeProp(sTablePropElem* elem)
   {
      sPropertyObjIter iter; 
      IStringProperty* prop = elem->prop; 
      ObjID obj; 
      const char* str; 
      AutoAppIPtr(ObjectSystem);
      prop->IterStart(&iter); 
      while (prop->IterNextValue(&iter,&obj,&str))
      {
         // assumes the entire str is the text
         // transforms it to "name:text"
         cStr val,name;

         name.FmtStr("%s_%d",pObjectSystem->GetName(obj),obj);
         SafetyPupSays(&name);
         val.FmtStr("%s: \"%s\"",(const char *)name,str);
         
         prop->Set(obj,val);
      }
      prop->IterStop(&iter); 
   }
      
   STDMETHOD(ModernizeProp)(const char *table)
   {
      if (table)
      {
         sTablePropElem* elem = mTable.Search(table); 
         AssertMsg1(elem,"No registered string table named %s",table);
         DoModernizeProp(elem); 
      }
      else
      {
         tHashSetHandle handle; 
         sTablePropElem* elem; 

         for (elem = mTable.GetFirst(handle); elem != NULL; elem = mTable.GetNext(handle))
            DoModernizeProp(elem); 
      }
      return(S_OK);
   }
}; 

F_DECLARE_INTERFACE(IPropertyManager); 

sRelativeConstraint cGameStrings::gConstraints[] = 
{
   { kConstrainAfter, &IID_IPropertyManager }, 
   { kNullConstraint }
}; 

void init_commands(); 

EXTERN void GameStringsCreate()
{
   AutoAppIPtr(Unknown); 
   new cGameStrings(pUnknown); 
   init_commands(); 
}


#ifdef EDITOR 

static void do_load(char* arg)
{
   // eat trailing whitespace
   for (char* s = arg +strlen(arg) - 1; s >= arg && isspace(*s); s--)
      *s = '\0'; 

   if (*arg == '\0') arg = NULL; 

   AutoAppIPtr(GameStrings); 
   pGameStrings->LoadProp(arg); 
}

static void do_dump(char* arg)
{
   // eat trailing whitespace
   for (char* s = arg +strlen(arg) - 1; s >= arg && isspace(*s); s--)
      *s = '\0'; 

   if (*arg == '\0') arg = NULL; 

   AutoAppIPtr(GameStrings); 
   pGameStrings->DumpProp(arg); 
}


static void do_strip(char* arg)
{
   // eat trailing whitespace
   for (char* s = arg +strlen(arg) - 1; s >= arg && isspace(*s); s--)
      *s = '\0'; 

   if (*arg == '\0') arg = NULL; 

   AutoAppIPtr(GameStrings); 
   pGameStrings->StripProp(arg); 
}

static void do_modernize(char* arg)
{
   // eat trailing whitespace
   for (char* s = arg +strlen(arg) - 1; s >= arg && isspace(*s); s--)
      *s = '\0'; 

   if (*arg == '\0') arg = NULL; 

   AutoAppIPtr(GameStrings); 
   pGameStrings->ModernizeProp(arg); 
}

static Command commands[] = 
{
   { "game_strings_load", FUNC_STRING, do_load, "load [<table>]: load the specified string table(s) into its property", HK_EDITOR }, 
   { "game_strings_dump", FUNC_STRING, do_dump, "dump [<table>]: dump the specified string table(s) from its property", HK_EDITOR }, 
   { "game_strings_strip", FUNC_STRING, do_strip, "strip [<table>]: strip text values from the property corresponding to the table.", HK_EDITOR }, 
   { "game_strings_modernize", FUNC_STRING, do_modernize, "modernize [<table>]: converts the property corresponding to the table to the hip gamestrings format.", HK_EDITOR }, 
}; 

static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}

#else 

static void init_commands()
{
}

#endif // !EDITOR 



