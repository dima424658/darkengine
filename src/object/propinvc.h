/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

 // $Header: r:/t2repos/thief2/src/object/propinvc.h,v 1.7 2000/01/29 13:24:33 adurant Exp $
#pragma once
#ifndef __PROPINVC_H
#define __PROPINVC_H
#include <propert_.h>
#include <dataops_.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <hashfns.h>
#include <objnotif.h>
#include <propbase.h>

#define COMP_INV_CLASS_PARENT(TYPE,NAME,HASH) \
       cComplexInvProperty<TYPE,I##NAME##Property,&IID_I##NAME##Property, \
       IInv##NAME##Property, &IID_IInv##NAME##Property,HASH>

#define COMP_INV_BY_PTR_CONSTRUCTOR(CLASS,TYPE,NAME,HASH) \
  CLASS(const sPropertyDesc* d, IPropertyStore* s) \
: COMP_INV_CLASS_PARENT(TYPE,NAME,HASH)(d,s) {}

#define COMP_INV_BY_ENUM_CONSTRUCTOR(CLASS,TYPE,NAME,HASH) \
  CLASS(const sPropertyDesc* d, ePropertyImpl i) \
: COMP_INV_CLASS_PARENT(TYPE,NAME,HASH)(d,CreateGenericPropertyStore(i)) {}

//////////////////////////////////////////////////////////////////
// MACRO FOR INSTANTIATING YOUR COMPLEX INVERTIBLE IMPLEMENTATION
//



//
// SPECIFY THE HASH FUNCTIONS
//

#define DECLARE_COMPLEX_INVERTIBLE_PROPERTY_CLASS_HASHED(CLASS,TYPE,NAME,HASH) \
class CLASS : public COMP_INV_CLASS_PARENT(TYPE,NAME,HASH) \
{ public: \
  COMP_INV_BY_PTR_CONSTRUCTOR(CLASS,TYPE,NAME,HASH); \
  COMP_INV_BY_ENUM_CONSTRUCTOR(CLASS,TYPE,NAME,HASH); \
  STANDARD_DESCRIBE_TYPE(TYPE); \
};

//
// USE STANDARD HASH FUNCTIONS
//

#define DECLARE_COMPLEX_INVERTIBLE_PROPERTY_CLASS(CLASS,TYPE,NAME) \
   DECLARE_COMPLEX_INVERTIBLE_PROPERTY_CLASS_HASHED(CLASS,TYPE,NAME,cInderectedHashFuncions<TYPE>)

////////////////////////////////////////////////////////////
// COMPLEX INVERTIBLE PROPERTY IMPLEMENTATION
//

template <class TYPE> class cInderectedHashFuncs
{
   static unsigned Hash(const TYPE* key) { return HashThing(key,sizeof(*key));};
   static unsigned IsEqual(const TYPE* k1, const TYPE* k2) { return *k1 == *k2;}; 
};

template <class TYPE, class TYPEFACE, const GUID* TYPEGUID, class INVFACE, const GUID* INVGUID, class HASHFUNCS>
class cComplexInvProperty : public cGenericProperty<INVFACE,INVGUID,TYPE*>
{
   typedef cHashTable<TYPE*,ObjID,HASHFUNCS> ValTable;
   typedef cGenericProperty<INVFACE,INVGUID,TYPE*> Parent;
   typedef cComplexInvProperty<TYPE,TYPEFACE,TYPEGUID,INVFACE,INVGUID,HASHFUNCS> Thistype;

   //
   // Rebuild the table from scratch
   //
   void Rebuild();

   //
   // Try to generate a unique value
   //
   virtual BOOL Uniquify(ObjID obj, TYPE* val)
   {
      return FALSE; 
   }


   //
   // Listener that maintains the 
   // table based on changes.
   //
   static void LGAPI ListenFunc(sPropertyListenMsg* msg, PropListenerData data)
   {
      Thistype* prop = (Thistype*)data;
      if (msg->type & (kListenPropModify|kListenPropSet))
      {
         // Actually get the real value, so that you have it 
         // identically.
         TYPE* value;
         Verify(prop->Get(msg->obj,&value));

         ObjID oldobj; 
         if (prop->GetObj(value,&oldobj) && msg->obj != oldobj)  // If someone already has this value
         {
            if (prop->Uniquify(msg->obj,value))
               prop->Set(msg->obj,value); 
            else
               prop->mpStore->Delete(msg->obj);   // prohibit from changing 
         }
         else
            prop->ByVal.Set(value,msg->obj);
      }
      if (msg->type & kListenPropUnset)
      {
         prop->ByVal.Delete((TYPE*)msg->value.ptrval);
      }
   }

   void Init()
   {
      Listen(kListenPropModify|kListenPropUnset|kListenPropSet, ListenFunc,(PropListenerData)this);
      SetOps(&Ops); 
   }

public:
   cComplexInvProperty(const sPropertyDesc* desc, IPropertyStore* impl)
      : Parent(desc,impl) { Init();}

   STDMETHOD(QueryInterface)(REFIID id, void** ppI);
   STDMETHOD_(BOOL,GetObj)(const TYPE* val, ObjID* objp) const;
   STDMETHOD(Set)(ObjID obj, TYPE* val);

   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data);

private:
   ValTable ByVal;
   cClassDataOps<TYPE> Ops; 
   
};

////////////////////////////////////////////////////////////
// METHOD IMPLEMENTATIONS
//

#define PROP_TEMPLATE template <class TYPE, class TYPEFACE, const GUID* TYPEGUID, class INVFACE, const GUID* INVGUID, class HASH>
#define PROPCLASS cComplexInvProperty<TYPE,TYPEFACE,TYPEGUID,INVFACE,INVGUID,HASH> 

PROP_TEMPLATE STDMETHODIMP PROPCLASS::QueryInterface(REFIID id, void** ppI)
{
   if (id == *TYPEGUID)
   {
      AddRef();
      *ppI = this;
      return S_OK;
   }
   return Parent::QueryInterface(id,ppI);
}

PROP_TEMPLATE STDMETHODIMP_(BOOL) PROPCLASS::GetObj(const TYPE* val, ObjID* objp) const
{
   if (ByVal.Lookup((TYPE*)val,objp))
   {
      // This is dumb, but verify whether this is the correct value.  
      // This is to deal with the fact that out-of-date entries *could* be in the table
      // The *right* thing to do is to write our own IPropertyRaw interface, or to 
      // have a "set" callback that gives the old value. 

      TYPE* objval;
      if (GetSimple(*objp,&objval))
      {
         if (HASH::IsEqual(val,objval))
            return TRUE;
         else  // we've found a spurious entry.  Delete it.
            // This yucky cast is because we're performing a benevolent side-effect  
            // inside a const member function. 
            ((PROPCLASS*)this)->ByVal.Delete((TYPE*)val);   
                  
      }
   }
   return FALSE;
}

PROP_TEMPLATE STDMETHODIMP PROPCLASS::Set(ObjID obj, TYPE* val)
{
   TYPE* old;

   if (GetSimple(obj,&old))
   {
      ByVal.Delete(old);
   }
   return Parent::Set(obj,val);
}

PROP_TEMPLATE STDMETHODIMP_(void) PROPCLASS::Notify(ePropertyNotifyMsg msg, PropNotifyData data)
{
   switch(NOTIFY_MSG(msg))
   {
      case kObjNotifyReset:
         ByVal.Clear();
         break;
      case kObjNotifyPostLoad:
         Rebuild();
         break;
   }
   Parent::Notify(msg,data);
}

PROP_TEMPLATE void PROPCLASS::Rebuild()
{
   sPropertyObjIter iter;
   TYPE* val;
   ObjID obj;

   ByVal.Clear();
   IterStart(&iter);
   while (IterNextValue(&iter,&obj,&val))
   {
      ByVal.Set(val,obj);
   }
   IterStop(&iter);
}

#undef PROP_TEMPLATE
#undef PROPCLASS


#endif // __PROPINVC_H




