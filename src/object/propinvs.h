/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propinvs.h,v 1.6 2000/01/29 13:24:34 adurant Exp $
#pragma once
#ifndef __PROPINVS_H
#define __PROPINVS_H
#include <propsimp.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <hashfns.h>
#include <objnotif.h>
#include <propbase.h>

#define SIMP_INV_CLASS_PARENT(TYPE,NAME) \
       cSimpleInvProperty<TYPE,I##NAME##Property,&IID_I##NAME##Property, \
       IInv##NAME##Property, &IID_IInv##NAME##Property>

#define SIMP_INV_BY_PTR_CONSTRUCTOR(CLASS,TYPE,NAME) \
  CLASS(const sPropertyDesc* d, IPropertyStore* i) \
: SIMP_INV_CLASS_PARENT(TYPE,NAME)(d,i) {}

#define SIMP_INV_BY_ENUM_CONSTRUCTOR(CLASS,TYPE,NAME) \
  CLASS(const sPropertyDesc* d, ePropertyImpl i) \
: SIMP_INV_CLASS_PARENT(TYPE,NAME)(d,CreateGenericPropertyStore(i)) {}

//////////////////////////////////////////////////////////////////
// MACRO FOR INSTANTIATING YOUR SIMPLE INVERTIBLE IMPLEMENTATION
//

#define DECLARE_SIMPLE_INVERTIBLE_PROPERTY_CLASS(CLASS,TYPE,NAME) \
class CLASS : public SIMP_INV_CLASS_PARENT(TYPE,NAME) \
{ public: \
  SIMP_INV_BY_PTR_CONSTRUCTOR(CLASS,TYPE,NAME); \
  SIMP_INV_BY_ENUM_CONSTRUCTOR(CLASS,TYPE,NAME); \
  STDMETHOD_(const sPropertyTypeDesc*,DescribeType)() const \
   { static sPropertyTypeDesc desc = { #NAME, sizeof(TYPE)}; return &desc; }; \
};

////////////////////////////////////////////////////////////
// SIMPLE INVERTIBLE PROPERTY IMPLEMENTATION
//

template <class TYPE, class TYPEFACE, const GUID* TYPEGUID, class INVFACE, const GUID* INVGUID>
class cSimpleInvProperty : public cSimpleProperty<INVFACE,INVGUID,TYPE>
{
   typedef cHashTable<TYPE,ObjID,cGenericHashFunctions<TYPE>> ValTable;
   typedef cSimpleProperty<INVFACE,INVGUID,TYPE> Parent;
   typedef cSimpleInvProperty<TYPE,TYPEFACE,TYPEGUID,INVFACE,INVGUID> Thistype;

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
      TYPE* value = (TYPE*)msg->value;

      if (msg->type & (kListenPropModify|kListenPropSet))
      {
         ObjID oldobj; 
         if (prop->GetObj(*value,&oldobj && msg->obj != oldobj))  // If someone already has this value
         {
            if (prop->Uniquify(msg->obj,value))
               prop->Set(msg->obj,value); 
            else
               prop->mpStore->Delete(msg->obj);   // prohibit from changing 
         }
         else
            prop->ByVal.Set(*value,msg->obj);

      }
      if (msg->type & kListenPropUnset)
      {
         prop->ByVal.Delete(*value);
      }
   }

   void Init()
   {
      Listen(kListenPropModify|kListenPropUnset|kListenPropSet, ListenFunc,(PropListenerData)this);
   }

public:
   cSimpleInvProperty(const sPropertyDesc* desc, IPropertyStore* impl)
      : Parent(desc,impl) { Init();}
   cSimpleInvProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : Parent(desc,impl) { Init();}

   STDMETHOD(QueryInterface)(REFIID id, void** ppI);
   STDMETHOD_(BOOL,GetObj)(TYPE val, ObjID* objp) const;
   STDMETHOD(Set)(ObjID obj, TYPE val);
   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data);

private:
   ValTable ByVal;
   
};

////////////////////////////////////////////////////////////
// METHOD IMPLEMENTATIONS
//

#define PROP_TEMPLATE template <class TYPE, class TYPEFACE, const GUID* TYPEGUID, class INVFACE, const GUID* INVGUID>
#define PROPCLASS cSimpleInvProperty<TYPE,TYPEFACE,TYPEGUID,INVFACE,INVGUID> 

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

PROP_TEMPLATE STDMETHODIMP_(BOOL) PROPCLASS::GetObj(TYPE val, ObjID* objp) const
{
   if (ByVal.Lookup(val,objp))
   {
      // This is dumb, but verify whether this is the correct value.  
      // This is to deal with the fact that out-of-date entries *could* be in the table
      // The *right* thing to do is to write our own IPropertyRaw interface, or to 
      // have a "set" callback that gives the old value. 

      TYPE objval;
      if (GetSimple(*objp,&objval))
      {
         if (HASH::IsEqual(val,objval))
            return TRUE;
         else  // we've found a spurious entry.  Delete it.
            // This yucky cast is because we're performing a benevolent side-effect  
            // inside a const member function. 
            ((PROPCLASS*)this)->ByVal.Delete(val);   
      }
   }
}

PROP_TEMPLATE STDMETHODIMP_(void) PROPCLASS::Set(ObjID obj, TYPE val)
{
   TYPE old;

   if (GetSimple(obj,&old))
   {
      ByVal.Delete(old);
   }
   return Parent::Set(obj,val);
}

PROP_TEMPLATE STDMETHODIMP_(void) PROPCLASS::Notify(ePropertyNotifyMsg msg, PropNotifyData data)
{
   switch(msg)
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
   TYPE val;
   ObjID obj;

   ByVal.Clear();
   IterStart(&iter);
   while (IterNextValue(&next,&obj,&val))
   {
      ByVal.Set(val,obj);
   }
   IterStop(&iter);
}

#undef PROP_TEMPLATE
#undef PROPCLASS


#endif // __PROPINVS_H




