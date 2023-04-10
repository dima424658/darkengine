/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/edittool.cpp,v 1.9 1999/11/01 18:07:28 henrys Exp $
#include <appagg.h>
#include <aggmemb.h>
#include <edittul_.h>

#include <objedit.h>
#include <sdesc.h>
#include <isdesced.h>
#include <isdescst.h>

#include <config.h>

//
// Include these last
//
#include <initguid.h>
#include <etooliid.h>

// must be last header
#include <dbmem.h>

#ifdef EDITOR
    IStructEditor* NewWatchObjDialog (void* data);
#endif

IMPLEMENT_AGGREGATION_SELF_DELETE(cEditTools);

static sRelativeConstraint Constraints[] = 
{
   { kNullConstraint}
};

cEditTools::cEditTools(IUnknown* pOuter)
{
   INIT_AGGREGATION_1(pOuter,IID_IEditTools,this,kPriorityNormal,Constraints);
}

cEditTools::~cEditTools()
{
}

STDMETHODIMP_(const char*) cEditTools::ObjName(ObjID obj)
{
   return ObjEditName(obj);
}

STDMETHODIMP_(ObjID) cEditTools::GetObjNamed(const char* name)
{
   return EditGetObjNamed(name);
}

STDMETHODIMP cEditTools::EditTypedData(const char* title, const char* type, void* data)
{
   AutoAppIPtr_(StructDescTools,pSdescTools); 
   const sStructDesc* desc = pSdescTools->Lookup(type);
   if (desc)
   {
      sStructEditorDesc eddesc = { "", kStructEditNoApplyButton};
      strncpy(eddesc.title,title,sizeof(eddesc.title));
      eddesc.title[sizeof(eddesc.title)-1] = '\0'; 

      BOOL result;
      IStructEditor* ed;

#ifdef EDITOR
      if (strcmp(type, "sAIWatchPoint") == 0 && config_is_defined("hens_changes"))
      {
          //  Special case for AIWatchObj
          //  Repeatedly lets you edit the steps from a list
          //
          while (true)
          {
              ed = NewWatchObjDialog (data);
              if (ed == NULL)  break;
                  
              result = ed->Go(kStructEdModal);
              SafeRelease(ed);
          }
      }
      else
      {
          ed = CreateStructEditor(&eddesc,(sStructDesc*)desc,data);
          result = ed->Go(kStructEdModal);
          SafeRelease(ed);
      }
#else
      ed = CreateStructEditor(&eddesc,(sStructDesc*)desc,data);
      result = ed->Go(kStructEdModal);
      SafeRelease(ed);
#endif
     
      return (result) ? S_OK : S_FALSE;
   }
   return S_FALSE;
   
}


HRESULT cEditTools::Init()
{
   return S_OK;
}

HRESULT cEditTools::End()
{
   return S_OK;
}

void EditToolsCreate(void)
{
   AutoAppIPtr_(Unknown,pUnk);
   new cEditTools(pUnk);
}


