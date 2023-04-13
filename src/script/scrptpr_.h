/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/scrptpr_.h,v 1.3 2000/02/22 20:01:46 toml Exp $
#pragma once  
#ifndef __SCRPTPR__H
#define __SCRPTPR__H
#include <propbase.h>
#include <propdef.h>
#include <dataops_.h>
#include <propsprs.h>
#include <propert_.h>
#include <listset.h>
#include <traittyp.h>

#include <dbmem.h>

F_DECLARE_INTERFACE(IScriptMan); 
//
// Structure def
//

#define SCRIPTS_PER_PROP 4

struct sScriptProp
{
   char script[SCRIPTS_PER_PROP][32];  // Names of four scripts
   BOOL no_inherit;        // Should I not inherit past here?
};


//------------------------------------------------------------
// Class: cScriptProp
//

#undef INTERFACE
#define INTERFACE IScriptProperty
DECLARE_PROPERTY_INTERFACE(IScriptProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sScriptProp*); 
}; 


class cScriptOps : public cClassDataOps<sScriptProp>
{
}; 

class cScriptStore : public cSparseHashPropertyStore<cScriptOps>
{
};


class cScriptProp: public cSpecificProperty<IScriptProperty,&IID_IScriptProperty,sScriptProp*,cScriptStore>
{
   typedef cSpecificProperty<IScriptProperty,&IID_IScriptProperty,sScriptProp*,cScriptStore> cBaseScriptProp; 

   class cArray: public cDynArray<const char*>
   {
   };

public:
   struct sModule
   {
      char* name;

      sModule() { name = NULL; };
      sModule(const char* s) :name(new char[strlen(s)+1]) { strcpy(name,s); };
      sModule(const sModule& s) : name( new char[strlen(s.name)+1]) { strcpy(name,s.name); };
      ~sModule() { delete name;};
      operator const char*() const { return name;};
      bool operator ==(const sModule& m) const { return strcmp(name,m.name) == 0;};
   };

   class cModuleSet : public cSimpleListSet<sModule>
   {
   };

   STANDARD_DESCRIBE_TYPE(sScriptProp); 

protected:
   //
   // Helpers
   //

   void rebuild_obj_scripts(ObjID obj);
   BOOL add_scripts_to_array(ObjID obj, cArray& array);
   void rebuild_all_scripts(ObjID arch);
   void add_listener(void);

   static void LGAPI HierarchyListener(const sHierarchyMsg* msg, 
                                       HierarchyListenerData data);

   void LoadModules(ITagFile* file);
   void SaveModules(ITagFile* file);

public:

   static cScriptProp* TheProp;

   cScriptProp(const sPropertyDesc* desc)
      : cBaseScriptProp(desc),
        mpScriptMan(AppGetObj(IScriptMan))
   {
      TheProp = this;
      add_listener();
   }


   ~cScriptProp()
   {
      // The script man is gone before we are...
      //      SafeRelease(mpScriptMan);
      TheProp = NULL;
   }

   //
   // Property methods
   //
   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data);
   STDMETHOD(Delete)(ObjID obj);

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue value);

   //
   // Other ops
   //
   HRESULT AddModule(const char* module);
   void DropModule(const char* module);
   void DropAllModules();
   cModuleSet::cIter Modules() { return mModules.Iter(); };

protected:
   IScriptMan* mpScriptMan;
   cModuleSet mModules;
};

#include <undbmem.h>

#endif // __SCRPTPR__H
