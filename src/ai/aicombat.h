/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicombat.h,v 1.26 1999/08/05 16:45:47 Justin Exp $
//
//

#ifndef __AICOMBAT_H
#define __AICOMBAT_H

#include <dynarray.h>

#include <aibasabl.h>
#include <aicblink.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

class cAISubCombat;

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitCombatAbility(IAIManager *);
BOOL AITermCombatAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICombat
//

class cAICombat : public cAIAbility
{
public:
   cAICombat();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();
   STDMETHOD_(void, Term)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);
   STDMETHOD_(void, OnDamage)(const sDamageMsg *pMsg, ObjID realCulpritID);
   STDMETHOD_(void, OnWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit);
   STDMETHOD_(void, OnProperty)(IProperty *, const sPropertyListenMsg *);
   STDMETHOD_(void, OnSound)(const sSoundInfo *, const sAISoundType * pType);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
// @TBD (toml 07-27-98): need onmode

   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestMode)   (sAIModeSuggestion *);
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   // misc
   STDMETHOD_(void, SetTargetAlertnessThreshold)(eAIAwareLevel threshold) {m_targetAlertnessThreshold = threshold;}

public:
   // Functions potentially useful to both descendants and contained classes
   ObjID GetTarget();
   const mxs_vector & GetTargetLoc();

   BOOL CanPlayReactCharge();
   void SetPlayedReactCharge();

protected:
   ObjID SeekCombat();
   ObjID SelectTarget();
   void SetTarget(ObjID);
   
   void CheckLostContact();   // See if we need to play the LostContact concept.
   
   BOOL  IsHostileToward(ObjID);
   eAIAwareLevel GetTargetAlertnessThreshold(void) const {return m_targetAlertnessThreshold;}

   // over-ride these for ability specific targetting behavior
   virtual BOOL IsValidTarget(ObjID object);                         // Is this valid for acquiring or retaining?
   virtual BOOL IsAcquirableTarget(const sAIAwareness& awareness);   // Is this valid to acquire?
   virtual BOOL IsRetainableTarget(const sAIAwareness& awareness);   // Is this valid to retain?
   virtual eAIPriority TargetPriority(ObjID object);                 // How good is this target?

private:
   void SignalSubGoals() {}

   void  UpdateTargets();
   ObjID TargetScan();

   enum
   {
      kWasDamaged    = kBaseFirstAvailFlag,
      kWasThreatened = (kWasDamaged << 1),

      kCombatFirstAvailFlag =  (kWasThreatened << 1)
   };

   cAITimer       m_TargetRefreshTimer;
   cAIRandomTimer m_ReactChargeTimer;
   cAIRandomTimer m_LostContactTimer;
   ObjID          m_LostContactWith;
   eAIAwareLevel  m_targetAlertnessThreshold;   // level at which identifying a combat target becomes easier

   int            m_lastTargetSwitch;
};

///////////////////////////////////////

inline BOOL cAICombat::CanPlayReactCharge()
{
   return m_ReactChargeTimer.Expired();
}

///////////////////////////////////////

inline void cAICombat::SetPlayedReactCharge()
{
   m_ReactChargeTimer.Reset();
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()
#endif /* !__AICOMBAT_H */
