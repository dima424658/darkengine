/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propert_.h,v 1.31 2000/01/29 13:24:24 adurant Exp $
#pragma once

#ifndef PROPERT__H
#define PROPERT__H

#include <propstor.h>
#include <propman.h>
#include <propknow.h>
#include <dbtype.h>
#include <propbase.h>
#include <dynarray.h>
#include <pstordel.h>
#include <traittyp.h>
#include <prpstats.h>
#include <propnet.h>
#include <propinst.h>

F_DECLARE_INTERFACE(ITrait);
F_DECLARE_INTERFACE(IPropertyStore); 
F_DECLARE_INTERFACE(INetManager);
F_DECLARE_INTERFACE(IObjectNetworking);

template <class T> class cSimpleDList;

////////////////////////////////////////////////////////////
//
// cPropertyBase
//
// Basic support for constraints, listeners with no property store.  
//
// Not a COM object 
//
////////////////////////////////////////////////////////////

class cPropertyBase: public cPropertyManagerKnower 
{
   struct Listener
   {
      static PropListenerHandle gNextHandle; 

      PropertyListenMsgSet interests; 
      PropertyListenFunc func;
      PropListenerData data; 
      PropListenerHandle handle; 

      Listener(PropertyListenMsgSet i, PropertyListenFunc f, 
                   PropListenerData d) 
         :interests(i),func(f),data(d),handle(gNextHandle++) {};
   }; 


   typedef cSimpleDList<PropertyID> PropIDList;


public:
   // Destructor
   virtual ~cPropertyBase ();

   // Describe myself
   const sPropertyDesc*  Describe () const {return &mDesc;};

   // Return my id
   PropertyID GetID() const { return mID; } ;

   //  Add a listener
   PropListenerHandle Listen(PropertyListenMsgSet interests, PropertyListenFunc func, PropListenerData data);
   
   void Unlisten(PropListenerHandle handle); 
  

   // Notify the property system that the object no longer exists
   void Notify (ePropertyNotifyMsg msg, PropNotifyData data);


   // Call property listeners
   void CallListeners(ePropertyListenMsg msg, ObjID obj, sDatum value, ObjID donor = OBJ_NULL);

   // Recompile constraints 
   void CompileConstraints(void);

   // Enforce constraints
   void EnforceRequirements(ObjID obj);
   void EnforceImplications(ObjID obj);

   void CreateEditor(IProperty* prop); 
      
   //
   // Overridables
   // 

   // A built-in listener for the property's own use
   virtual void OnListenMsg(ePropertyListenMsg , ObjID , uPropListenerValue ) {};

   // Create the property's editor 

protected:
   // Constructor; tell it the name of your property and the implementation.
   // pimpl must be constructed first, but the cProperty then takes over
   // management of it. 
   cPropertyBase (const sPropertyDesc *desc);
   
   sPropertyDesc  mDesc;           // Descriptor
   PropertyID     mID;             // unique to each cProperty
   cDynArray<Listener> mListeners; // listener set
   PropIDList* mpImplied;             // Properties I imply
   PropIDList* mpRequired;            // Properties I require
};

////////////////////////////////////////////////////////////
//
// cStoredProperty
//
// Common code for properties that use IPropertyStore, and 
// Have ITraits for inheritance/instantiation.
// 
// Not a COM object.
//
////////////////////////////////////////////////////////////

// Forward decl of our classes used for side interfaces.
class cStoredPropertyStats; 
class cStoredPropertyNetworking;

// Stored property instrumentation macros
#define STOREDPROP_AUTO_BLAME() PROP_AUTO_BLAME(mAllocName)
#define STOREDPROP_TIMER(x) PROP_TIMER_STATS_IDX(mStoreStats,kProp##x##Time)
#define STOREDPROP_TIMER_RESTART(x) PROP_TIMER_RESTART(mStoreStats,kProp##x##Time)

class cStoredProperty: public cPropertyBase
{
public:

	// Query Interface 
   HRESULT QI(IUnknown* me, REFIID id, void ** ppI);

   // Make the property relevant for this object
   HRESULT Create(ObjID obj);

   // Decide that this property is irrelevant for the object
   HRESULT Delete (ObjID obj);

   // Copy the property
   HRESULT Copy(ObjID targ, ObjID src);

   // Notify the property of database changes
   void Notify (ePropertyNotifyMsg msg, PropNotifyData data);

   // Is this property relevant to the given object?
   BOOL IsSimplyRelevant(ObjID obj) const 
   {
      STOREDPROP_TIMER(Relevant); 
      return mpStore->Relevant (obj);
   } ;

   // Is this property relevant to the given object?
   BOOL IsRelevant(ObjID obj) const
   {
      STOREDPROP_TIMER(Relevant);
      if (mpStore->Relevant(obj)) 
         return TRUE;

      PROP_TIMER_STOP(); 
      obj = GetDonor(obj);
      PROP_TIMER_START(); 

      return mpStore->Relevant(obj);
   }
   
   HRESULT Set(ObjID obj,sDatum val); 

   // modify a property in place, call listeners
   BOOL Touch(ObjID obj,sDatum* val); 
   
   // Iteration
   void  IterStart (sPropertyObjIter* iter) const
   { 
      STOREDPROP_TIMER(IterStart); 

		mpStore->IterStart(iter); 
   }; 

   BOOL IterNext (sPropertyObjIter* iter, ObjID* next) const
   { 
	   sDatum dummy; 
      STOREDPROP_TIMER(IterNext); 
		return mpStore->IterNext(iter,next,&dummy);
   }; 

   void IterStop(sPropertyObjIter* iter) const
   { 
      STOREDPROP_TIMER(IterStop); 
		mpStore->IterStop(iter);
   };

   // Find the object an object inherits from
   ObjID GetDonor(ObjID obj) const;

   // Find the object that the property would be copied from on create
   ObjID GetExemplar(ObjID obj) const;

   PropListenerHandle Listen(PropertyListenMsgSet interests, PropertyListenFunc func, PropListenerData data);

   ~cStoredProperty(); 

   // Swap out for a new store 
   void SetStore(IPropertyStore* store) ;

   void SetOps(IDataOps* ops) { mpStore->SetOps(ops); }; 

   // Concrete requirements enforcement
   void SetRebuildConcretes(BOOL fRebuild);
   void RebuildConcretes(ObjID obj);
   void RebuildOneConcrete(ObjID obj); 
   static void LGAPI RebuildHierarchyListener(const sHierarchyMsg* msg, 
                                              HierarchyListenerData data);

   // Return the mpStats structure pointer.
   sPropTimeStats* GetStoreTimeStats(){return &mStoreStats;};
   
   //
   // Overridables
   // 

   // A built-in listener for the property's own use
   virtual void OnListenMsg(ePropertyListenMsg , ObjID , uPropListenerValue );

   // 
   virtual void RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue, ObjID donor) {};
   
protected:
   friend class cStoredPropertyNetworking;

   enum eFlags
   {
      kListeningHierarchy = 0x01,
      kRebuildConcretes   = 0x02
   };

   // Call property listeners, possibly doing our own updating 
   void CallListeners(ePropertyListenMsg msg, ObjID obj, sDatum value, ObjID donor = OBJ_NULL);


   cStoredProperty (const sPropertyDesc *desc, IPropertyStore * pStore);


   void InitTraits(void); 
   void Save(ITagFile* file, edbFiletype filetype);
   void Load(ITagFile* file, edbFiletype filetype);
   void write_obj(ObjID obj, IDataOpsFile* file, edbFiletype filetype);
   void read_obj(IDataOpsFile* file, edbFiletype filetype, uint version);
   
   IPropertyStore *mpStore;         // Property store 
   ITrait* mpDonors;                // My trait for inheritance
   ITrait* mpExemplars;             // My trait for creation
   unsigned mFlags;

   // This is for the IPropertyNetworking interface, which handles network messages.
   // NULL for properties that don't require networking (kPropertyChangeLocally).
   cStoredPropertyNetworking* mpNetProp;

   // This should be #Ifndef SHIP or something 
   cStoredPropertyStats* mpStats;   // our stat interface 
   sPropTimeStats mStoreStats;   // actual gather stats
   const char* mAllocName;       // our name for lgalloc blame

   // Interfaces that are needed by & shared by all properties.
   static INetManager *gmNetMan;
   static IObjectNetworking *gmObjNet;
};


////////////////////////////////////////////////////////////
//
// cUnknownProperty
//
// Has only IUnknown methods 
//
////////////////////////////////////////////////////////////

template <class IFACE, const GUID* pIID> 
class cUnknownProperty : public cCTUnaggregated<IFACE,pIID,kCTU_Default>
{
public:

   virtual ~cUnknownProperty() {}; 

   
}; 

////////////////////////////////////////////////////////////
//
// cProperty
//
// Templatized on interface, but only implements IProperty methods
// If you want to roll your own (slow) accessors, use this 
//
////////////////////////////////////////////////////////////

template <class IFACE, const GUID *pIID>
class cProperty: public cUnknownProperty<IFACE,pIID>,
                 public cStoredProperty
{
protected:
    using cBase = cStoredProperty;

public:
   // Describe myself
   STDMETHOD_(const sPropertyDesc*, Describe) () const { return cBase::Describe(); }; 

   // Return my id
   STDMETHOD_(PropertyID,GetID)() const { return cBase::GetID(); } ;

   // Make this property relevant for this object
   STDMETHOD(Create) (ObjID obj) { return cBase::Create(obj); } ; 

   // Decide that this property is irrelevant for the object
   STDMETHOD(Delete) (ObjID obj) { return cBase::Delete(obj); } ; 

   // Copy the prop from one obj to another
   STDMETHOD(Copy) (ObjID targ,ObjID src) { return cBase::Copy(targ,src); } ; 

   // Notify the property system that the object no longer exists
   STDMETHOD_(void,Notify) (ePropertyNotifyMsg msg, PropNotifyData data) 
   {
      // @TODO: move this someplace more appropriate!
      if (!mCalledCreateEditor)
      {
         CreateEditor();
         mCalledCreateEditor = TRUE; 
      }
      cBase::Notify(msg,data); 
   }; 

   // Is this property relevant to the given object?
   STDMETHOD_(BOOL,IsRelevant)(ObjID obj) const {return cBase::IsRelevant(obj); } ; 
   STDMETHOD_(BOOL,IsSimplyRelevant)(ObjID obj) const {return cBase::IsSimplyRelevant(obj); } ;    

   // modify an object in place
   STDMETHOD_(BOOL,Touch)(ObjID obj)  {return cBase::Touch(obj,NULL); } ; 

   //  Add a listener
   STDMETHOD_(PropListenerHandle, Listen)(PropertyListenMsgSet interests, PropertyListenFunc func, PropListenerData data) { return cBase::Listen(interests,func,data); } ; 

   STDMETHOD(Unlisten)(PropListenerHandle handle) { cBase::Unlisten(handle); return S_OK; }; 

   STDMETHOD_(void, IterStart) (sPropertyObjIter* iter) const { cBase::IterStart(iter);} ; 
   STDMETHOD_(BOOL, IterNext)  (sPropertyObjIter* iter, ObjID* next) const 
   { return cBase::IterNext(iter,next);};
   STDMETHOD_(void, IterStop) (sPropertyObjIter* iter) const { cBase::IterStop(iter);} ; 

   STDMETHOD_(const sPropertyTypeDesc*, DescribeType)() const
   { static sPropertyTypeDesc desc = { "Unknown", 0}; return &desc; };

   STDMETHOD(QueryInterface)(REFIID id, void ** ppI)
   {
      if (id == *pIID)
      {
         *ppI = this;
         AddRef();
         return S_OK;
      }
      if (id == IID_IPropertyStore)
      {
         *ppI = &mStoreDel; 
         AddRef(); 
         return S_OK; 
      }
      else 
         return cBase::QI((IUnknown*)this,id,ppI);
   }

   void SetStore(IPropertyStore* store) 
   {
      cBase::SetStore(store); 
      mStoreDel.SetStore(mpStore); 
   }

protected:
   // Create the property editor 
   virtual void CreateEditor() { cPropertyBase::CreateEditor(this); }; 

   // constructor
   cProperty(const sPropertyDesc *desc, IPropertyStore *store)
      : cBase(desc,store),
        mStoreDel(mpStore,this), 
        mCalledCreateEditor(FALSE)
   {
      GetManager()->AddProperty (this, &mID);
      if (mpStore)
         mpStore->SetID(mID); 
   };

   // Destructor
   virtual ~cProperty () { GetManager()->DelProperty(this); };


   cDelegatingPropertyStore mStoreDel; // the store I hand out when QI'd for one. 
   BOOL mCalledCreateEditor;  // have I created an editor yet?  (sigh)
};

////////////////////////////////////////////////////////////
//
// cGenericProperty 
//
// Generic property, with (slow) accessors, templatized on type 
// Accessors must be by 32-bit (or smaller) value. 
//
////////////////////////////////////////////////////////////

template <class IFACE, const GUID* IID, class TYPE> 
class cGenericProperty : public cProperty<IFACE,IID> 
{
protected:
   // We use this to do type-conversion 
   union uPropVal
   {
      TYPE t;
      void* d; 

      uPropVal(const TYPE& tt) : t(tt) {}; 
   }; 

public:
   cGenericProperty(const sPropertyDesc* desc, IPropertyStore* store = NULL, IDataOps* ops = NULL)
      : cProperty<IFACE,IID>(desc,store)
   {
      AssertMsg(sizeof(TYPE) == sizeof(sDatum),"Can't make property accessors for non-32 bit type");  
      if (ops)
         store->SetOps(ops); 
   } 

   STDMETHOD_(BOOL,Get)(ObjID obj, TYPE (*pval)) const 
   {
      STOREDPROP_TIMER(Get); 

      sDatum* pdat = (sDatum*)pval; 
      BOOL result = mpStore->Get(obj,pdat);
      if (!result)
      {
         PROP_TIMER_STOP(); 
         ObjID donor = GetDonor(obj);
         PROP_TIMER_START(); 
         if (donor != OBJ_NULL)
            result = mpStore->Get(donor,pdat);
      }
      return result; 
   }

   STDMETHOD_ (BOOL, GetSimple) (ObjID obj, TYPE (*ptr)) const 
   {
      STOREDPROP_TIMER(Get); 
      sDatum* pdat = (sDatum*)ptr; 
      return mpStore->Get(obj,pdat);
   }

   STDMETHOD(Set) (ObjID obj, TYPE val) 
   {
      uPropVal dat = val;
      return cBase::Set(obj, dat.d);
   }

   STDMETHOD_ (BOOL, IterNextValue) (sPropertyObjIter* iter,ObjID* next, TYPE (*val)) const
   {
      STOREDPROP_TIMER(IterNext); 
      sDatum* pdat = (sDatum*)val; 
      return mpStore->IterNext(iter,next,pdat); 
   }

   STDMETHOD_(BOOL,TouchValue)(ObjID obj, TYPE val)  
   {
      uPropVal dat = val;
      sDatum d = dat.d; 
      return cBase::Touch(obj,&d); 
   }; 
}; 
 

////////////////////////////////////////////////////////////
//
// cSpecificProperty
//
// Generic property, templatized on type and on store type for speed 
// Accessors must be by 32-bit value. 
//
////////////////////////////////////////////////////////////

//
// Keep my store from deleting itself
//

template<class STORE>
class cNonDeletingStore : public STORE
{
   void OnFinalRelease() {}; 

};

template <class IFACE, const GUID* IID, class TYPE, class STORE> 
class cSpecificProperty : public cProperty<IFACE,IID> 
{
protected:
   // We use this to do type-conversion 
   union uPropVal
   {
      TYPE t;
      void* d; 

      uPropVal(const TYPE& tt) : t(tt) {}; 
   }; 

public:


   cSpecificProperty(const sPropertyDesc* desc)
      : cProperty<IFACE,IID>(desc,NULL)
   {
      AssertMsg(sizeof(TYPE) == sizeof(sDatum),"Can't make property accessors for non-32 bit type");  

      // Since we circumvent the mpStore a lot, we can't deal with the auto-mixing
      // done by kpropertyconcrete
      Assert_(!(desc->flags & kPropertyConcrete)); 
      
      // We can't pass &mStore into the constructor, because it hasn't been initialized 
      // yet, so we set it up here instead.  
      SetStore(&mStore);  
   } 

   ~cSpecificProperty()
   {
      mStore.Reset(); 
      // unset the store before it goes away
      SetStore(NULL); 
   }

   // 
   // Store accessor
   // 

   STORE& Store() { return mStore; }; 

   //
   // METHODS
   // 

   STDMETHOD_(BOOL,Get)(ObjID obj, TYPE (*pval)) const 
   {
      STOREDPROP_TIMER(Get); 
      sDatum* pdat = (sDatum*)pval; 
      BOOL result = mStore.Get(obj,pdat);
      if (!result)
      {
         PROP_TIMER_STOP(); 
         ObjID donor = GetDonor(obj);
         PROP_TIMER_START(); 

         if (donor != OBJ_NULL)
            result = mStore.Get(donor,pdat);
      }
      return result; 
   }

   STDMETHOD_ (BOOL, GetSimple) (ObjID obj, TYPE (*ptr)) const 
   {
      STOREDPROP_TIMER(Get); 
      sDatum* pdat = (sDatum*)ptr; 
      return mStore.Get(obj,pdat);
   }

   STDMETHOD(Set) (ObjID obj, TYPE val) 
   {
      uPropVal dat = val;
      return cBase::Set(obj, dat.d);
   }

   STDMETHOD_ (BOOL, IterNextValue) (sPropertyObjIter* iter,ObjID* next, TYPE (*val)) const
   {
      STOREDPROP_TIMER(IterNext); 
      sDatum* pdat = (sDatum*)val; 
      return mStore.IterNext(iter,next,pdat); 
   }

   STDMETHOD_(BOOL,TouchValue)(ObjID obj, TYPE val)  
   {
      uPropVal dat = val;
      sDatum d = dat.d; 
      return cBase::Touch(obj,&d); 
   } ; 

protected:
   cNonDeletingStore<STORE> mStore; 
}; 




//
// Use this macro to create the standard DescribeType method for a type
//

#define STANDARD_DESCRIBE_TYPE(type) \
   STDMETHOD_(const sPropertyTypeDesc*, DescribeType)() const \
   { static sPropertyTypeDesc desc = { #type, sizeof(type)}; return &desc; }


#endif // PROPERT__H

