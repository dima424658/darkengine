/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitma_.h,v 1.11 2000/01/29 13:25:11 adurant Exp $
#pragma once
#ifndef __TRAITMA__H
#define __TRAITMA__H

#include <aggmemb.h>
#include <nameprop.h>
#include <traitman.h>
#include <dynarray.h>

#include <proptype.h>
#include <linktype.h>

////////////////////////////////////////////////////////////
// TRAIT MANAGER IMPLEMENTATION CLASS
//

F_DECLARE_INTERFACE(IProperty);
F_DECLARE_INTERFACE(IRelation);
F_DECLARE_INTERFACE(IIntProperty);
F_DECLARE_INTERFACE(IObjectSystem);
F_DECLARE_INTERFACE(IDonorCache);

class cTraitManager : public ITraitManager 
{
   DECLARE_AGGREGATION(cTraitManager); 

   struct sListener 
   {
      HierarchyListenFunc func;
      HierarchyListenerData data;
   };

public:
   cTraitManager(IUnknown* outer); 
   virtual ~cTraitManager(); 

   STDMETHOD(CreateBaseArchetype)(const char* name, ObjID* pArchetypeID);
   STDMETHOD_(ObjID,CreateArchetype)(const char* name, ObjID parent);
   STDMETHOD_(ObjID,CreateConcreteArchetype)(const char* name, ObjID parent);
   STDMETHOD_(ObjID,CreateMetaProperty)(const char* name, ObjID parent); 
   STDMETHOD(AddObject)(ObjID obj, ObjID archetype);
   STDMETHOD(RemoveObject)(ObjID obj); 
   STDMETHOD(AddObjMetaProperty)(ObjID obj, ObjID meta);
   STDMETHOD(AddObjMetaPropertyPrioritized)(ObjID obj, ObjID meta, tMetaPropertyPriority);
   STDMETHOD(RemoveObjMetaProperty)(ObjID obj, ObjID meta);
   STDMETHOD(RemoveObjMetaPropertyPrioritized)(ObjID obj, ObjID meta, tMetaPropertyPriority);
   STDMETHOD_(BOOL,ObjHasDonor)(ObjID obj, ObjID meta);
   STDMETHOD_(BOOL,ObjHasDonorIntrinsically)(ObjID obj, ObjID meta);
   STDMETHOD_(BOOL,ObjIsDonor)(ObjID donor);
   STDMETHOD_(IObjectQuery*, Query)(ObjID obj, eTraitQueryType type);
   STDMETHOD_(ITrait*,CreateTrait)(const sTraitDesc* desc, const sTraitPredicate* pred);   
   STDMETHOD(Listen)(HierarchyListenFunc func, HierarchyListenerData data);
   STDMETHOD(Notify)(eObjNotifyMsg msg, ObjNotifyData data);
   STDMETHOD_(ObjID,GetArchetype)(ObjID obj);
   STDMETHOD(SetArchetype)(ObjID obj,ObjID arch);

   STDMETHOD_(BOOL,IsMetaProperty)(ObjID obj);
   STDMETHOD_(BOOL,IsArchetype)(ObjID obj);

   STDMETHOD_(ObjID,RootMetaProperty)() { return MetaPropArchetype;};

protected:
   void SendListenMessage(int kind, ObjID obj, ObjID donor);
   static void LGAPI MetaPropListener(sRelationListenMsg* msg, RelationListenerData data); 

   //   void SynchArchetypes(); 
   BOOL RemoveArchetypeLinks(ObjID obj, ObjID arch); 
   void AddArchetypeLink(ObjID obj, ObjID arch); 

protected:
   cDynArray<ITrait*> Traits;
   cDynArray<sListener> Listeners; 
   IIntProperty* DonorType;
   IInvStringProperty* SymName;

   ObjID MetaPropArchetype;  // the archetype that metaprops belong to

   // Convenience interfaces
   IRelation* MetaProps;
   IObjectSystem* ObjSys;
   IDonorCache* Cache;

   //
   // Aggregate Protocol
   //
   HRESULT Init();
   HRESULT End();

   
}; 



#endif // __TRAITMA__H
