/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/proped.cpp,v 1.16 1999/11/22 17:55:20 henrys Exp $

#include <proped.h>
#include <property.h>
#include <propbase.h>
#include <string.h>
#include <dataops.h>
#include <propstor.h>

#include <sdesc.h>
#include <isdesced.h>
#include <isdescst.h>

#include <config.h>

// must be last header
#include <dbmem.h>

IStructEditor* NewAlertResponseDialog (void* data);
IStructEditor* NewBodyResponseDialog (void* data);
IStructEditor* NewCombatResponseDialog (void* data);
IStructEditor* NewSignalResponseDialog (void* data);
IStructEditor* NewThreatResponseDialog (void* data);
IStructEditor* NewVisionDescDialog (void* data);

typedef IStructEditor* (*psDialogFuncType)(void*);

////////////////////////////////////////////////////////////
// cBasePropEditTrait implementation 
//

static sEditTraitCaps default_caps = 
{
   kTraitCanAdd|kTraitCanRemove, 
}; 

cBasePropEditTrait::cBasePropEditTrait(IProperty* prop)
   : Prop(prop),
     Caps(default_caps)
{
   Prop->AddRef(); 

   const sPropertyDesc* pdesc = Prop->Describe(); 


   // Build the descriptor
   memset(&Desc,0,sizeof(Desc));
   Desc.kind = kPropertyTrait;
   strncpy(Desc.strings.name,pdesc->name,sizeof(Desc.strings.name));
   // check to see if we have a friendly name
   if (pdesc->ui.friendly_name != NULL)
      strncpy(Desc.strings.friendly_name,pdesc->ui.friendly_name,sizeof(Desc.strings.friendly_name)); 
   else // default to name
      strcpy(Desc.strings.friendly_name,Desc.strings.name); 
   // copy category
   if (pdesc->ui.category != NULL)
      strncpy(Desc.strings.category,pdesc->ui.category,sizeof(Desc.strings.category));

   if (pdesc->flags & kPropertyNoEdit)
      Desc.flags |= kTraitInvisible;
}


cBasePropEditTrait::~cBasePropEditTrait()
{
   // @HACK: we know the prop is going to get blasted anyway, so don't bother
   Prop = NULL; 
   //   SafeRelease(Prop); 
}


//------------------------------------------------------------

STDMETHODIMP_(const sEditTraitDesc*) cBasePropEditTrait::Describe()
{
   return &Desc;
}

STDMETHODIMP_(const sEditTraitCaps*) cBasePropEditTrait::Capabilities()
{
   return &Caps;
}

//------------------------------------------------------------

STDMETHODIMP_(BOOL) cBasePropEditTrait::IntrinsicTo(ObjID obj)
{
   return Prop->IsSimplyRelevant(obj); 
}

STDMETHODIMP cBasePropEditTrait::Add(ObjID obj)
{
   Prop->Create(obj);
   return S_OK;
}

STDMETHODIMP cBasePropEditTrait::Remove(ObjID obj)
{
   Prop->Delete(obj);
   return S_OK;
}


////////////////////////////////////////////////////////////
// cSdescPropEditTrait implementation 
//

cSdescPropEditTrait::cSdescPropEditTrait(IProperty* property)
   : cBasePropEditTrait(property), Sdesc(NULL)
{


}



cSdescPropEditTrait::~cSdescPropEditTrait()
{
}

STDMETHODIMP_(const sEditTraitCaps*) cSdescPropEditTrait::Capabilities()
{
   if (Sdesc == NULL)
   {
      const sPropertyTypeDesc* tdesc = Prop->DescribeType(); 
      AutoAppIPtr_(StructDescTools,pTools); // @TODO: hold on to this
      Sdesc = pTools->Lookup(tdesc->type);
      if (Sdesc)
      {
         Caps.flags |= kTraitCanEdit|kTraitCanUnparse; 
         if (pTools->IsSimple(Sdesc))
            Caps.flags |= kTraitCanParse; 
      }   
   }
   return &Caps;
}


STDMETHODIMP cSdescPropEditTrait::Edit(ObjID obj)
{
   if (Sdesc != NULL)
   {
      HRESULT retval = S_FALSE; 
      IPropertyStore* store;
      Verify(SUCCEEDED(Prop->QueryInterface(IID_IPropertyStore,(void**)&store)));

      // copy property data
      sDatum dat;
      store->GetCopy(obj,&dat);
      sStructEditorDesc eddesc = { "" , kStructEditAllButtons };
      strncpy(eddesc.title,Desc.strings.friendly_name,sizeof(eddesc.title)); 

      
      IDataOps* ops = store->GetOps(); 
      // decide whether dat is used as a pointer or a value...
      void* struc = (ops && ops->BlockSize(dat) > 0) ? dat.value : &dat; 

      // Check for the new pseudo-script dialog
      //
      psDialogFuncType AlternateDialogFunction = NULL;
      if (config_is_defined("hens_changes"))
      {
         const char* candidates [] =
         {
            "Alert response",
            "Body response",
            "Sense combat response",
            "Signal response",
            "Threat response",
            "Vision description"
         };

         psDialogFuncType funcs [] =
         {
            &NewAlertResponseDialog,
            &NewBodyResponseDialog,
            &NewCombatResponseDialog,
            &NewSignalResponseDialog,
            &NewThreatResponseDialog,
            &NewVisionDescDialog,
         };

         for (short i = 0; i < (sizeof(candidates) / sizeof(char*)); i++)
         {
            if (strcmp(Desc.strings.friendly_name, candidates[i]) == 0)
            {
               AlternateDialogFunction = funcs[i];
               break;
            }
         }
      }

      IStructEditor* sed;
      if (AlternateDialogFunction != NULL)
      {
          while (true)
          {
              sed = AlternateDialogFunction (struc);
              if (sed == NULL)  break;
                  
              if (sed->Go(kStructEdModal))
              {
                 store->Set(obj,dat); 
                 Prop->Touch(obj); 
                 retval = S_OK; 
              }
              SafeRelease(sed);
          }
      }
      else
      {
          sed = CreateStructEditor(&eddesc,Sdesc,struc);
          
          if (sed->Go(kStructEdModal))
          {
             store->Set(obj,dat); 
             Prop->Touch(obj); 
             retval = S_OK; 
          }
          SafeRelease(sed);
      }
      SafeRelease(ops); 

      store->ReleaseCopy(obj,dat);
      SafeRelease(store); 

      return retval;
   }

   return E_FAIL; 
}


STDMETHODIMP cSdescPropEditTrait::Parse(ObjID obj, const char* val) 
{
   if (Sdesc != NULL)
   {
      IPropertyStore* store;
      Verify(SUCCEEDED(Prop->QueryInterface(IID_IPropertyStore,(void**)&store)));

      BOOL already_present = store->Relevant(obj); 
      sDatum dat = store->Create(obj); 
      IDataOps* ops = store->GetOps(); 
      // Use BlockSize to determine the whether we're a pointer or a scalar
      void* struc = (ops && ops->BlockSize(dat) >= 0) ? dat.value : &dat; 

      SafeRelease(ops); 
      
      AutoAppIPtr_(StructDescTools,pTools); 

      if (pTools->ParseSimple(Sdesc,val,struc))
      {
         store->Set(obj,dat);
         Prop->Touch(obj); 
      }
      else if (!already_present)
         Prop->Delete(obj);

      SafeRelease(store); 

      return S_OK;
   }

   return E_FAIL;    
}

#define BUF_SIZE(x) ((x) < 1024 ? 1024 : (x))
 
STDMETHODIMP cSdescPropEditTrait::Unparse(ObjID obj, char* buf, int buflen)
{
   if (Sdesc != NULL)
   {
      IPropertyStore* store;
      Verify(SUCCEEDED(Prop->QueryInterface(IID_IPropertyStore,(void**)&store)));

      sDatum dat;
      store->Get(obj,&dat); 

      IDataOps* ops = store->GetOps(); 

      // Determine whether dat is a pointer 
      BOOL isPtr = (ops && ops->BlockSize(dat) >= 0);

      if (dat || !isPtr)
      {
         void* struc = isPtr ? dat.value : &dat; 
         AutoAppIPtr_(StructDescTools,pTools); 
         pTools->UnparseFull(Sdesc,struc,buf,buflen); 
      
         buf[buflen-1] = '\0';

         SafeRelease(store); 

         return S_OK;
      }

      buf[0] = '\0';
   }

   return E_FAIL;
}




