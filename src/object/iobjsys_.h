/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/iobjsys_.h,v 1.19 2000/02/24 23:40:59 mahk Exp $
#pragma once
#ifndef __IOBJSYS__H
#define __IOBJSYS__H
#include <iobjsys.h>
#include <objdef.h>
#include <aggmemb.h>
#include <nameprop.h>
#include <dlistsim.h>
#include <dynarray.h>
#include <objspace.h>

#include <objproxy.h>

F_DECLARE_INTERFACE(IPropertyManager);
F_DECLARE_INTERFACE(ILinkManager);
F_DECLARE_INTERFACE(ITraitManager);
F_DECLARE_INTERFACE(ITagFile);
F_DECLARE_INTERFACE(INetManager);
F_DECLARE_INTERFACE(IObjectNetworking);

class cObjList: public cSimpleDList<ObjID>
{
}; 

class cListeners : public cDynArray<sObjListenerDesc>
{
}; 

class cSinks: public cDynArray<IObjIDSink*>
{
}; 

//
// OBJECT SYSTEM IMPLEMENTATION CLASS
// 

class cObjectSystem :  public cCTDelegating<IObjectSystem>,
                       public cCTAggregateMemberControl<kCTU_Default>
{

protected: 

   static cObjectSystem* TheObjSys;

   // Helper functions
   void notify_obj(ObjID id,ulong msg);
   void notify_obj_all(eObjConcreteness which,ulong msg); 
   void send_post_load(eObjPartition part); 
   
   void remove_obj(ObjID obj); 

   ObjID get_next_obj(eObjConcreteness concrete);
   void build_obj_lists(); 
   void SaveActiveArray(ITagFile* file, eObjPartition partition);
   HRESULT LoadActiveArray(ITagFile* file, eObjPartition partition);

public:
   cObjectSystem(IUnknown* pOuter);
   virtual ~cObjectSystem();

   //
   // IObjectSystem methods
   //
   STDMETHOD_(ObjID,Create)(ObjID archetype, eObjConcreteness concrete);
   STDMETHOD_(ObjID,BeginCreate)(ObjID archetype, eObjConcreteness concrete);
   STDMETHOD(EndCreate)(ObjID obj);
   STDMETHOD(Destroy)(ObjID obj);
   STDMETHOD(Reset)();
   STDMETHOD_(BOOL,Exists)(ObjID obj);
   STDMETHOD_(IObjectQuery*,Iter)(eObjConcreteness which);
   STDMETHOD(DatabaseNotify)(eObjNotifyMsg msg, ObjNotifyData data);
   STDMETHOD(NameObject)(ObjID obj, const char* name);
   STDMETHOD_(const char*,GetName)(ObjID obj);
   STDMETHOD_(ObjID,GetObjectNamed)(const char* name);
   STDMETHOD_(IProperty*,GetCoreProperty)(eObjSysProperty which);
   STDMETHOD(SetObjTransience)(ObjID obj, BOOL transient);
   STDMETHOD_(BOOL,ObjIsTransient)(ObjID obj);
   STDMETHOD(SetObjImmobility)(ObjID obj, BOOL immobile);
   STDMETHOD_(BOOL,ObjIsImmobile)(ObjID obj);
   STDMETHOD(CloneObject)(ObjID to, ObjID from);

   STDMETHOD(PurgeObjects)(); 

   STDMETHOD_(tObjListenerHandle,Listen)(sObjListenerDesc* desc) ; 
   STDMETHOD(Unlisten)(tObjListenerHandle handle) ; 

   STDMETHOD_(ObjID,MaxObjID)() ;
   STDMETHOD_(ObjID,MinObjID)() ;
   STDMETHOD_(int,ActiveObjects)() ;

   STDMETHOD(Lock)(); 
   STDMETHOD(Unlock)(); 

   STDMETHOD_(BOOL,IsObjSavePartition)(ObjID obj, eObjPartition part); 
   STDMETHOD_(BOOL,IsObjLoadPartition)(ObjID obj, eObjPartition part); 
   STDMETHOD_(ObjID,RemapOnLoad)(ObjID src);
   STDMETHOD(SetSubPartitionFilter)(eObjPartition subpart, const sObjPartitionFilter* filter); 

   STDMETHOD_(eObjPartition,ObjDefaultPartition)(ObjID obj); 

   //----------------------------------------
   // IObjIDManagerm methods

   sObjBounds GetObjIDBounds(); 
   tResult ResizeObjIDSpace(const sObjBounds& bounds); 
   tResult Connect(IObjIDSink* pSink); 
   tResult Disconnect(IObjIDSink* pSink); 


protected: 

   enum { kNumSubPartitions = 8, }; 

   struct sFilter : public sObjPartitionFilter
   {
      sFilter(const sObjPartitionFilter& f) { *(sObjPartitionFilter*)this = f; }; 
      sFilter() { func = NULL; } ;
   };
   

   //
   // Aggregate Protocol
   //
   STDMETHOD(Init)();
   STDMETHOD(End)();

   IPropertyManager* PropMan;
   ILinkManager* LinkMan;
   ITraitManager* TraitMan;
   INetManager* NetMan;
   IObjectNetworking* ObjNet;
   IInvStringProperty* SymName;
   IBoolProperty* Transient;
   IBoolProperty* Immobile;

   cObjList mActiveObjs;
   cObjList mFreeObjs; 
   cObjList mLoadedObjs; 
   cObjList mDeletedObjs;
   
   int mLockCount; 

   cListeners mListeners; 
   sFilter mSubPartFilters[kNumSubPartitions]; 

   cSinks  m_Sinks; 
   

};


//------------------------------------------------------------
// ID manager proxy object 

class cObjIDMan : public cCTDelegating<IObjIDManager>,
                   public cCTAggregateMemberControl<kCTU_Default>
{
   cObjectSystem* m_pObjSys; 
public:

   cObjIDMan(IUnknown* pOuter, cObjectSystem* pObjSys); 
   
   STDMETHOD(End)() { ((IObjectSystem*)m_pObjSys)->Release(); return S_OK; }; 

   virtual sObjBounds GetObjIDBounds() { return m_pObjSys->GetObjIDBounds(); }; 
   virtual tResult ResizeObjIDSpace(const sObjBounds& bounds) { return m_pObjSys->ResizeObjIDSpace(bounds); };
   virtual tResult Connect(IObjIDSink* pSink) { return m_pObjSys->Connect(pSink); }; 
   virtual tResult Disconnect(IObjIDSink* pSink) { return m_pObjSys->Disconnect(pSink); }; 
};




#endif // __IOBJSYS__H
