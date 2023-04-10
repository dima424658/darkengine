/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/simpdmg_.h,v 1.13 2000/02/05 19:37:56 adurant Exp $
#pragma once

#ifndef __SIMPDMG__H
#define __SIMPDMG__H

#include <dmgmodl_.h>
#include <simpldmg.h>
#include <appagg.h>
#include <linktype.h>
#include <culpable.h>

#include <propface.h>

F_DECLARE_INTERFACE(ITraitManager); 
F_DECLARE_INTERFACE(IRelation); 

////////////////////////////////////////////////////////////
// THE SIMPLE DAMAGE MODEL IMPLEMENTATION CLASS
//

class cSimpleDamageModel : public cBaseDamageModel
{

public:
   cSimpleDamageModel(IUnknown* pOuter);
   ~cSimpleDamageModel();

   STDMETHOD_(eDamageResult,HandleImpact)(ObjID hittee, ObjID hitter, sImpact* impact, sChainedEvent* cause);
   STDMETHOD_(eDamageResult,DamageObject)(ObjID victim, ObjID culprit, sDamage* damage, sChainedEvent* cause, BOOL allowzero DEFAULT_TO(FALSE)); 
   STDMETHOD_(eDamageResult,SlayObject)(ObjID victim, ObjID culprit, sChainedEvent* cause);
   STDMETHOD_(eDamageResult,TerminateObject)(ObjID victim, sChainedEvent* cause);
   STDMETHOD_(eDamageResult,ResurrectObject)(ObjID victim, ObjID culprit, sChainedEvent* cause);

protected:
   STDMETHOD(Init)(); 
   STDMETHOD(End)(); 

   void CreateCorpseRelation(); 
   void CreateFlinderizeRelation();

   //
   // Culpability stuff
   //

   void CreateCulpability(); 

public:
   void PropagateCulpability(ObjID from, ObjID to, ulong flags); 
   ObjID GetRealCulprit(ObjID culprit); 

   static cSimpleDamageModel* TheDamageModel; 

protected:

   //
   // Data members
   //

   IRelation* mpCorpses; 
   IRelation* mpFlinders;
   ITraitManager* mpTraitMan; 
   IRelation*     mpCulpableFor;
   IBoolProperty* mpCulpable; 
}; 

////////////////////////////////////////
//
// Corpse iterator - so we can have multiple corpses/victim
//

class cCorpseIter
{
public:
   cCorpseIter(ObjID objID, IRelation *pRelation);
   ~cCorpseIter(void);

   ObjID Get(void);
   void *GetData(void);
   BOOL  Finished(void);
   void  Next(void);

private:
   ILinkQuery *m_pQuery;
   ObjID m_startObj;    // we store off the first obj we find links from
};


#endif // __SIMPDMG__H
