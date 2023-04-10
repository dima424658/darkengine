/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimuli_.h,v 1.3 2000/01/29 12:44:53 adurant Exp $
#pragma once

#ifndef __STIMULI__H
#define __STIMULI__H
#include <stimuli.h>
#include <aggmemb.h>
#include <stimbase.h>
#include <iobjsys.h>
#include <traitman.h>
#include <traitbas.h>
#include <relation.h>

F_DECLARE_INTERFACE(IPropagation); 

////////////////////////////////////////////////////////////
// IMPLEMENTATION CLASS FOR IStimuli
//


class cStimuli : public cCTDelegating<IStimuli>,
                 public cCTAggregateMemberControl<kCTU_Default>
{

public: 
   cStimuli(IUnknown* pOuter);
   virtual ~cStimuli();

   //----------------------------------------
   // IStimuli methods
   //

   STDMETHOD_(StimID,GetRootStimulus)();
   STDMETHOD_(StimID,CreateStimulus)(const sStimDesc* desc, StimID parent);
   STDMETHOD_(StimID,AddStimulus)(ObjID obj); 
   STDMETHOD_(const char*,GetStimulusName)(StimID id);
   STDMETHOD_(StimID,GetStimulusNamed)(const char* name);
   STDMETHOD(DatabaseNotify)(tStimDatabaseMsg msg, IUnknown* dbfile);
   STDMETHOD_(BOOL,IsStimulus)(ObjID obj); 
   STDMETHOD_(IObjectQuery*,QueryDonors)(StimID stim);
   STDMETHOD_(IObjectQuery*,QueryHeirs)(StimID stim);
   STDMETHOD_(BOOL,InheritsFrom)(StimID stimulus, ObjID donor);


protected:

   void CompileDonors(ObjID obj);
   void CompileAllDonors();
   void AddAllStimuli(); 
   static BOOL IsStimPred(ObjID obj, TraitPredicateData data); 
   void create_is_stim_trait(void);

   //----------------------------------------
   // Aggregate Protocol
   //
 
   STDMETHOD(Init)();
   STDMETHOD(End)();

   //
   // Data members
   //

   // interface refs, for convenience
   IObjectSystem* pObjSys;  
   ITraitManager* pTraitMan; 
   IPropagation* pPropagation; 

   ITrait* pIsStimTrait;
   IRelation* pStimDonors;   // transitive closure of donor relationship
   ObjID StimRoot;           // root stimulus
};



#endif // __STIMULI__H


