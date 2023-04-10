/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/nameprop.cpp,v 1.5 1998/05/13 16:30:33 mahk Exp $
#include <nameprop.h>
#include <objnotif.h>
#include <propstor.h>
#include <string.h>
#include <strprop_.h>
#include <hashpp.h>
#include <hshpptem.h>

// Must be last header
#include <dbmem.h>

typedef cGenericProperty<IInvStringProperty,&IID_IInvStringProperty,cStr*> cBaseInvStringProp; 

typedef cStringPropertyExtensions<cBaseInvStringProp>  cGenericInvStringProp; 


class cInvStringProp : public cGenericInvStringProp
{
   typedef cHashTable<const char*,ObjID,cCaselessStringHashFuncs> cValTable;
   typedef cGenericInvStringProp cParent; 

public:

   cInvStringProp(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store) 
   {
   }

protected:

   STDMETHODIMP QueryInterface(REFIID id, void** ppI)
   {
      if (id == IID_IStringProperty)
      {
         AddRef();
         *ppI = this;
         return S_OK;
      }
      return cParent::QueryInterface(id,ppI);
   }

   STDMETHOD_(BOOL,GetObj)(const char* val, ObjID* objp) const
   {
      return mByVal.Lookup(val,objp); 
   }

   STDMETHOD(Set)(ObjID obj, const char* val)
   {
      const char* old;

      if (GetSimple(obj,&old))
      {
         mByVal.Delete(old);
      }
      return cParent::Set(obj,val);
   }


   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data)
   {
      switch(NOTIFY_MSG(msg))
      {
         case kObjNotifyReset:
            mByVal.Clear();
            break;
         case kObjNotifyPostLoad:
            Rebuild();
            break;
      }
      cParent::Notify(msg,data);
   }

   void OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue v)
   {
      if (type & (kListenPropModify|kListenPropSet))
      {
         // Actually get the real value, so that you have it 
         // identically.
         const char* value;
         Verify(Get(obj,&value));

         ObjID oldobj; 
         if (GetObj(value,&oldobj) && obj != oldobj)  // If someone already has this value
         {
            char buf[64]; 
            sprintf(buf,"Object#%d",obj); 
            Set(obj,buf); 
         }
         else
            mByVal.Set(value,obj);
      }
      if (type & kListenPropUnset)
      {
         mByVal.Delete(*(cStr*)v.ptrval);
      }

      cParent::OnListenMsg(type,obj,v); 
   }

   void Rebuild()
   {
      sPropertyObjIter iter;
      const char* val;
      ObjID obj;

      mByVal.Clear();
      IterStart(&iter);
      while (IterNextValue(&iter,&obj,&val))
      {
         mByVal.Set(val,obj);
      }
      IterStop(&iter);
   }

   cValTable mByVal; 

}; 


IInvStringProperty* CreateInvStringProperty(const sPropertyDesc* desc, ePropertyImpl impl)
{
   cAutoIPtr<IPropertyStore> store( CreateGenericPropertyStore(impl)); 
   return new cInvStringProp(desc,store);
}

IInvStringProperty* CreateInvStringPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* impl)
{
   return new cInvStringProp(desc,impl);
}









