/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiinvest.h,v 1.11 1998/11/01 01:51:33 TOML Exp $
//
//
//

#ifndef __AIINVEST_H
#define __AIINVEST_H

#include <ainoncbt.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

class cAIInvestAction;

///////////////////////////////////////////////////////////////////////////////

void AIInitInvestigateAbility();
void AITermInvestigateAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIInvestigate
//

class cAIInvestigate : public cAINonCombatAbility
{
public:
   cAIInvestigate();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();
   
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction *);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

   STDMETHOD_(void, OnProperty)(IProperty *, const sPropertyListenMsg *);
   STDMETHOD_(void, OnDefend)(const sAIDefendPoint *);
   
   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestMode)   (sAIModeSuggestion *);
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   virtual void BroadcastAlertness();

private:
   ObjID GetAlertnessSource();
   void SetAlertnessSource(ObjID);

   cAIRandomTimer             m_MaxSenseResyncTimer;
   cAIRandomTimer             m_MaxResyncTimer;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIINVEST_H */
