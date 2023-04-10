/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimsrc_.h,v 1.5 1998/01/27 18:55:55 mahk Exp $
#pragma once
#ifndef __STIMSRC__H
#define __STIMSRC__H

#include <comtools.h>
#include <stimsrc.h>
#include <linktype.h>
#include <proptype.h>
#include <aggmemb.h>
#include <traitbas.h>


F_DECLARE_INTERFACE(IRelation);
F_DECLARE_INTERFACE(ITraitManager);
F_DECLARE_INTERFACE(IStimuli);
F_DECLARE_INTERFACE(IPropagation);

//------------------------------------------------------------
// Base stim source query 
//

class cBaseStimSourceQuery : public IStimSourceQuery
{
public:
   DECLARE_UNAGGREGATABLE();

   virtual ~cBaseStimSourceQuery() {};
};



////////////////////////////////////////////////////////////
// STIM SOURCES IMPLEMENTATION CLASS
//

class cStimSources : public IStimSources 
{
   DECLARE_AGGREGATION(cStimSources);

public:
   cStimSources(IUnknown* pOuter);
   virtual ~cStimSources(); 

   //------------------------------------------------------------
   // IStimSources Methods
   //

   STDMETHOD_(StimSourceID, AddSource)(ObjID obj, StimID stimulus, const sStimSourceDesc* desc); 
   STDMETHOD(RemoveSource)(StimSourceID stim);

   STDMETHOD_(sObjStimPair,GetSourceElems)(StimSourceID id);
   STDMETHOD_(tStimTimeStamp,GetSourceBirthDate)(StimSourceID id);
   STDMETHOD(DescribeSource)(StimSourceID id, sStimSourceDesc* desc);

   STDMETHOD_(IStimSourceQuery*,QuerySources)(ObjID obj, StimID stimulus);
   STDMETHOD(DatabaseNotify)(tStimDatabaseMsg msg, IUnknown* file);
   STDMETHOD(ObjectNotify)(eObjNotifyMsg msg, ObjNotifyData data);

protected: 
   //------------------------------------------------------------
   // Helper functions
   //

   void AddInheritanceLink(ObjID obj, ObjID newdonor);
   void RemoveInheritanceLink(ObjID obj, ObjID olddonor);

   void AddSource(StimSourceID descid, ObjID obj, StimID stim); 
   void AddAllSources(StimSourceID descid, ObjID obj, StimID stim); 

   void RemoveSource(StimSourceID descid, ObjID obj, StimID stim);
   void RemoveAllSources(StimSourceID descid, ObjID obj, StimID stim); 
   void DestroyAllSources(StimSourceID descid, ObjID obj, StimID stim);

   void RecomputeSources(void);

   //
   // LInk ID remappers
   //

   StimSourceID Link2Source(LinkID id);
   LinkID       Source2Link(StimSourceID id); 

   //
   // Link listeners for external relations
   //
   static void LGAPI HierarchyListener(const sHierarchyMsg* msg, HierarchyListenerData data);

   //
   // Trait function
   // 
   static BOOL SourceTraitPredicate(ObjID obj, TraitPredicateData data);

   //
   // Internal Relations
   //

   void CreateSourceDescRelation();
   static void LGAPI SourceDescListener(sRelationListenMsg* msg, RelationListenerData data); 
   
   void CreateSourceRelation();
   static void LGAPI SourceListener(sRelationListenMsg* msg, RelationListenerData data); 
   
   // 
   // Source query class
   //
   
   class cSourceQuery : public cBaseStimSourceQuery 
   {
      ILinkQuery* Query;
      cStimSources* Sources; 

   public: 
      cSourceQuery(ILinkQuery* q, cStimSources* s);
      ~cSourceQuery();

      STDMETHOD(Start)();
      STDMETHOD_(BOOL,Done)();
      STDMETHOD(Next)();

      STDMETHOD_(StimSourceID,ID)();
      STDMETHOD_(sObjStimPair,Elems)();
      STDMETHOD_(const sStimSourceDesc*, Source)();
   
   };

   friend class cSourceQuery;

   //------------------------------------------------------------
   // Aggregate Protocol
   //

   HRESULT Init();
   HRESULT End();
   
   //
   // Data Members
   //

   // Convenience interface refs
   IStimuli* pStimuli;  
   IPropagation* pPropagation;
   ITraitManager* pTraitMan;
   ILinkManager* pLinkMan; 

   IRelation* pSourceDescs;  // descriptions of sources
   IRelation* pSources;      // the sources themselves 
   
   ITrait* pSourceDescTrait;  // Any source desc links? 

   enum RelIDIDXs
   {
      kSourceIDX,
      kSourceDescIDX,
      kNumIDXs
   };

   RelationID RelIDs[kNumIDXs]; 
};



#endif // __STIMSRC__H





