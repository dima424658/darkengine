/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimsen_.h,v 1.10 2000/01/29 12:44:49 adurant Exp $
#pragma once

#ifndef __STIMSEN__H
#define __STIMSEN__H

#include <stimsens.h>
#include <sensbase.h>
#include <aggmemb.h>
#include <relation.h>

#include <propag8n.h>
#include <stimuli.h>

#include <traitman.h>
#include <traitbas.h>

#include <proptype.h>
#include <propbase.h>

#include <dlistsim.h>
#include <hashpp.h>

class cStimSensors : public IStimSensors
{
   DECLARE_AGGREGATION(cStimSensors);

public: 
   cStimSensors(IUnknown* pOuter); 
   virtual ~cStimSensors();

   //------------------------------------------------------------
   // IStimSensors methods
   //

   STDMETHOD_(ReceptronID,AddReceptron)(ObjID obj, StimID stimulus, const sReceptron* tron);
   STDMETHOD(RemoveReceptron)(ReceptronID);
   STDMETHOD(GetReceptron)(ReceptronID, sReceptron* tron);
   STDMETHOD(SetReceptron)(ReceptronID, const sReceptron* tron);
   STDMETHOD_(sObjStimPair,GetReceptronElems)(ReceptronID);

   STDMETHOD_(IReceptronQuery*,QueryReceptrons)(ObjID obj, StimID stim);
   STDMETHOD_(IReceptronQuery*,QueryInheritedReceptrons)(ObjID obj, StimID stim);
   STDMETHOD_(StimSensorID, LookupSensor)(ObjID obj, StimID stim);
   STDMETHOD_(sObjStimPair,GetSensorElems)(StimSensorID id);
   STDMETHOD(DatabaseNotify)(tStimDatabaseMsg msg, IUnknown* file);
   STDMETHOD(ObjectNotify)(eObjNotifyMsg msg, ObjNotifyData data);
   STDMETHOD_(IStimSensorQuery*,QuerySensors)(ObjID obj, StimID stim);
   STDMETHOD_(ObjID,ObjParam)(eReactObjParam which);

protected: 
   //------------------------------------------------------------
   // Helper functions
   //

   void AddSensorLink(ObjID obj, ObjID stim); 
   void AddSensor(ObjID obj, ObjID stim);
   void AddAllSensors(ObjID arch, ObjID stim);
   void RemoveSensorLink(ObjID obj, ObjID stim); 
   void RemoveSensor(ObjID obj, ObjID stim);
   void RemoveAllSensors(ObjID arch, ObjID stim);
   void AddObjParams(LinkID tronlink, ObjID obj);
   void RemObjParams(LinkID tronlink, ObjID obj);
   void AddInheritanceLink(ObjID obj, ObjID newdonor);
   void RemoveInheritanceLink(ObjID obj, ObjID olddonor);
   void RecomputeSensors(void);
   void ResetOrder(void);
   void FillObjParams(void);

   //
   // Hierarchy listener
   //
   
   static void LGAPI HierarchyListener(const sHierarchyMsg* msg, HierarchyListenerData data); 


   //
   // ID mappings
   //

public:
   StimSensorID Link2Sensor(LinkID link);
   LinkID Sensor2Link(StimSensorID id);
   StimSensorID Link2Receptron(LinkID link);
   LinkID Receptron2Link(StimSensorID id);

   //
   // Internal relations
   //

private:
   static void LGAPI SensorListener(sRelationListenMsg* msg, RelationListenerData data);
   static void LGAPI ReceptronListener(sRelationListenMsg* msg, RelationListenerData data);
   static void LGAPI ObjParamListener(sRelationListenMsg* msg, RelationListenerData data);

   void CreateReceptronsRelation(); 
   void CreateSensorsRelation(); 
   void CreateObjParamsRelation();

   //
   // Helper classes
   //

   typedef cScalarHashFunctions<StimID>  cStimHashFuncs;

   class cTraitTable : public cHashTable<StimID,ITrait*,cStimHashFuncs>
   {
   };

   struct sOneStimTraitParam // given to the single-stim trait predicate
   {
      cStimSensors* us; // pointer to sensors
      StimID stim;      // which stim are we
   };

   class cTraitParamSet : public cSimpleDList<sOneStimTraitParam>
   {
   public:
      ~cTraitParamSet() { DestroyAll(); }; 
   };

   
   //------------------------------------------------------------
   // Aggregate protocol
   //

   HRESULT Init();
   HRESULT End();

   //
   // Data Members
   //

   // Convenience interface refs
   IStimuli*   pStimuli;
   IPropagation* pPropagation;
   ITraitManager* pTraitMan;

   IRelation*  pReceptrons;   // The receptron set
   IRelation*  pSensors;      // The sensor set
   IRelation*  pObjParams;    // Links that track the life of receptron object params
   ulong NextOrder;           // next receptron in order
   ObjID SpecialObjParams[kNumSpecialObjParams]; // ObjID's for special params

   ITrait*  pAnyStimTrait;    // Any receptron trait

};



#endif // __STIMSEN__H
