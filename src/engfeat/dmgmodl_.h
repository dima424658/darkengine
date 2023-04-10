/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/dmgmodl_.h,v 1.11 2000/02/05 19:37:55 adurant Exp $
#pragma once

#ifndef __DMGMODL__H
#define __DMGMODL__H

#include <aggmemb.h>
#include <dmgmodel.h>
#include <dmgbase.h>
#include <dynarray.h>

F_DECLARE_INTERFACE(IIntProperty);
F_DECLARE_INTERFACE(IScriptMan); 
F_DECLARE_INTERFACE(IObjectSystem); 

#define PROP_DEATH_STAGE_NAME "DeathStage"

////////////////////////////////////////////////////////////
// BASE DAMAGE MODEL IMPLEMENTATION CLASS
//

class cBaseDamageModel : public cCTDelegating<IDamageModel>,
                         public cCTAggregateMemberControl<kCTU_Default>
{
public:
   cBaseDamageModel(IUnknown* outer, struct sRelativeConstraint* constraints = NULL);
   virtual ~cBaseDamageModel();
   
   //
   // IDamageModel methods
   //
   STDMETHOD_(eDamageResult,HandleImpact)(ObjID hittee, ObjID hitter, sImpact* impact, sChainedEvent* cause);
   STDMETHOD_(eDamageResult,DamageObject)(ObjID victim, ObjID culprit, sDamage* damage, sChainedEvent* cause, BOOL allowzero=FALSE); 
   STDMETHOD_(eDamageResult,SlayObject)(ObjID victim, ObjID culprit, sChainedEvent* cause);
   STDMETHOD_(eDamageResult,TerminateObject)(ObjID victim, sChainedEvent* cause);
   STDMETHOD_(eDamageResult,ResurrectObject)(ObjID victim, ObjID culprit, sChainedEvent* cause);
   STDMETHOD(Filter)(tDamageFilterFunc func, tDamageCallbackData data);
   STDMETHOD(Listen)(tDamageMsgMask interests, tDamageListenerFunc func, tDamageCallbackData data);

   //
   // Aggregate protocol
   //
   STDMETHOD(Init)(); 
   STDMETHOD(End)(); 

protected:
   eDamageResult SendMessage(const sDamageMsg* msg);
   eDamageResult ApplyResult(ObjID victim, ObjID culprit, eDamageResult result, sChainedEvent* cause = NULL); 
   eDamageResult ApplyFilters(ObjID victim, ObjID culprit, sDamage* damage);

   BOOL AlreadySentMsg(ulong msg,ObjID obj); 

   IScriptMan* mpScriptMan; 
   IObjectSystem* mpObjSys; 
   
private:
   void SendScriptMessage(const sDamageMsg * msg ); 

   

   struct sListener
   {
      tDamageMsgMask interests;
      tDamageListenerFunc func;
      tDamageCallbackData data;
   };

   struct sFilter
   {
      tDamageFilterFunc func; 
      tDamageCallbackData data;
   };
   
   cDynArray<sListener> mListeners;
   cDynArray<sFilter>   mFilters;
   IIntProperty* death_stage;
};

#endif // __DMGMODL__H






