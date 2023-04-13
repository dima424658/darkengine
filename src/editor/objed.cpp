/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/objed.cpp,v 1.4 1998/06/16 14:59:21 mahk Exp $

#include <wtypes.h>
#include <dynfunc.h>

#include <comtools.h>

#include <appagg.h>
#include <aggmemb.h>

#include <iobjed.h>
#include <objedbas.h>

#include <edtrait.h>
#include <edtraist.h>

#include <dynarray.h>

#include <proped.h>



// Include these last
#include <initguid.h>
#include <objediid.h>

// must be last header
#include <dbmem.h>


////////////////////////////////////////////////////////////
// IMPLEMENTATION CLASS FOR IObjEditors Agg Member


class cObjEditors : public cCTDelegating<IObjEditors>,
                   public cCTAggregateMemberControl<kCTU_Default>
{
public: 
   cObjEditors(IUnknown* pouter);

   virtual ~cObjEditors(); 

   STDMETHOD(AddTrait)(IEditTrait* trait); 
   STDMETHOD(RemoveTrait)(IEditTrait* trait); 
   STDMETHOD_(IEditTrait*, AddProperty)(IProperty* prop); 
   STDMETHOD_(IEditTrait*,FirstTrait)(sEditTraitIter* iterstate);
   STDMETHOD_(IEditTrait*,NextTrait)(sEditTraitIter* iterstate); 
   STDMETHOD_(IObjEditor*,Create)(sObjEditorDesc* desc, ObjID obj);

   STDMETHOD(End)();

protected:
   class cTraits : public cDynArray<IEditTrait*>
   {

   }; 


   cTraits Traits; 
}; 

////////////////////////////////////////////////////////////


static sRelativeConstraint Constraints[] = 
{
   { kNullConstraint } 
};

F_DECLARE_INTERFACE(IObjEditors); 

cObjEditors::cObjEditors(IUnknown* pOuter)
{
   MI_INIT_AGGREGATION_1(pOuter, IObjEditors, kPriorityNormal, Constraints);
} 

cObjEditors::~cObjEditors()
{
}

////////////////////////////////////////////////////////////

STDMETHODIMP cObjEditors::AddTrait(IEditTrait* trait)
{
   Assert_(trait); 
   
   // Search for the trait
   for (int i = 0; i < Traits.Size(); i++)
   {
      if (Traits[i] == trait)
      {
         Warning(("Added EditTrait %s twice.\n",trait->Describe()->strings.name)); 
         return S_FALSE;
      }
      if (Traits[i] == NULL)
      {
         Traits[i] = trait;
         trait->AddRef();
         return S_OK; 
      }
   }

   Traits.Append(trait); 
   trait->AddRef();
   return S_OK; 
}

STDMETHODIMP cObjEditors::RemoveTrait(IEditTrait* trait)
{
   Assert_(trait); 

   // Search for the trait
   for (int i = 0; i < Traits.Size(); i++)
   {
      if (Traits[i] == trait)
      {
         Traits[i] = NULL;
         SafeRelease(trait);
         return S_OK;
      }
   }

   Warning(("Could not remove EditTrait %s.\n",trait->Describe()->strings.name)); 

   return S_OK; 
}


STDMETHODIMP_(IEditTrait*) cObjEditors::AddProperty(IProperty* prop)
{
   IEditTrait* trait = new cSdescPropEditTrait(prop); 
   AddTrait(trait);
   return trait; 
}

////////////////////////////////////////////////////////////

STDMETHODIMP_(IEditTrait*) cObjEditors::FirstTrait(sEditTraitIter* iter)
{
   Assert_(iter); 

   int i;
   for (i = 0; i < Traits.Size() ; i++)
      if (Traits[i] != NULL)
      {
         iter->idx = i;
         Traits[i]->AddRef();
         return Traits[i]; 
      }

   // No luck
   iter->idx = i; 
   return NULL;
}


STDMETHODIMP_(IEditTrait*) cObjEditors::NextTrait(sEditTraitIter* iter)
{
   Assert_(iter); 

   int i;
   for (i = iter->idx+1; i < Traits.Size() ; i++)
      if (Traits[i] != NULL)
      {
         iter->idx = i;
         Traits[i]->AddRef();
         return Traits[i]; 
      }

   // No luck
   iter->idx = i; 
   return NULL;
}

////////////////////////////////////////////////////////////


static IObjEditor* obj_woe(const sObjEditorDesc* , ObjID obj)
{
   CriticalMsg("Could not load dialog!");
   return NULL;
}


DeclDynFunc_(IObjEditor*, LGAPI, ConstructObjEditor, (const sObjEditorDesc* , ObjID ));
ImplDynFunc(ConstructObjEditor, "darkdlgs.dll", "_ConstructObjEditor@8", obj_woe);

#define ObjEdit (DynFunc(ConstructObjEditor).GetProcAddress())

STDMETHODIMP_(IObjEditor*) cObjEditors::Create(sObjEditorDesc* desc, ObjID obj)
{
   return ObjEdit(desc,obj);  
}

STDMETHODIMP cObjEditors::End()
{
   for (int i = 0; i < Traits.Size(); i++)
   {
      SafeRelease(Traits[i]); 
   }
   return S_OK; 
}


void ObjEditorsCreate(void)
{
   AutoAppIPtr(Unknown);
   new cObjEditors(pUnknown); 
}

