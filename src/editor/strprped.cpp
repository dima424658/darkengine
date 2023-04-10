/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/strprped.cpp,v 1.2 2000/02/19 13:11:28 toml Exp $
#include <string.h>
#include <strprop_.h>
#include <proped.h>
#include <iobjed.h>

#include <sdesc.h>
#include <isdesced.h>
#include <isdescst.h>
#include <sdesbase.h>
#include <sdestool.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// STRING PROPERTY EDITING 

//
// For now, we'll just use a really big sdesc
//
struct sBuf
{
   char buf[2048]; 

   sBuf(const char* s = NULL) 
   { 
      if (s)
      {
         strncpy(buf,s,sizeof(buf)); 
         buf[sizeof(buf)-1] = '\0'; 
      }
      else
         memset(buf,0,sizeof(buf)); 
   };

   operator const char*() { return buf; }; 
}; 

StructDescDefineSingleton(string_sdesc,sBuf,kFieldTypeString,0); 

// mostly for other things to use
static void register_sdesc()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&string_sdesc); 
}



class cStringEditTrait: public cBasePropEditTrait
{
public:
   cStringEditTrait(IStringProperty* prop)
      : cBasePropEditTrait((IProperty*)prop)
   {
      Caps.flags |= kTraitCanParse|kTraitCanUnparse|kTraitCanEdit; 
   }

   STDMETHOD(Edit)(ObjID obj)
   {
      const char* str; 
      IStringProperty* prop = (IStringProperty*)Prop; 
      if (!prop->Get(obj,&str))
         return E_FAIL; 
      sBuf buf(str); 

      sStructEditorDesc eddesc = { "" , kStructEditAllButtons };
      strncpy(eddesc.title,Desc.strings.friendly_name,sizeof(eddesc.title)); 
      IStructEditor* sed = CreateStructEditor(&eddesc,&string_sdesc,&buf); 

      HRESULT retval = S_FALSE; 
      if (sed->Go(kStructEdModal))
      {
         prop->Set(obj,buf); 
         retval = S_OK; 
      }
      SafeRelease(sed);
      return retval; 
   }


   STDMETHOD(Parse)(ObjID obj, const char* val)
   {
      IStringProperty* prop = (IStringProperty*)Prop; 
      return prop->Set(obj,val); 
   }


   STDMETHOD(Unparse)(ObjID obj, char* buf, int buflen)
   {
      IStringProperty* prop = (IStringProperty*)Prop; 
      const char* str; 
      if (prop->Get(obj,&str))
      {
         strncpy(buf,str,buflen);
         buf[buflen-1] = '\0'; 
         return S_OK; 
      }
      return S_FALSE; 
   }

};

void CreateStringPropEditor(IStringProperty* prop)
{
   register_sdesc();

   IObjEditors* pEditors = AppGetObj(IObjEditors);
   if (pEditors)
   {
      IEditTrait* trait = new cStringEditTrait(prop); 
      pEditors->AddTrait(trait);
      SafeRelease(trait); 
      SafeRelease(pEditors); 
   }
}
