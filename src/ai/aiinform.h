/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiinform.h,v 1.4 1999/11/29 12:29:25 BFarquha Exp $
//
//
//

#ifndef __AIINFORM_H
#define __AIINFORM_H

#include <aiapiinf.h>
#include <aiapicmp.h>
#include <aibasabl.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitInformAbility(IAIManager *);
BOOL AITermInformAbility();
BOOL AIHasInformResponse(ObjID id);

///////////////////////////////////////////////////////////////////////////////

class cAIInform  : public cAIAbility, cCTDelegating<IAIInform>
{
public:
   cAIInform();
   ~cAIInform();

   STDMETHOD (QueryInterface)(REFIID id, void ** ppI);

   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Notifications
   STDMETHOD_(void, OnSound)(const sSoundInfo *, const sAISoundType * pType);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

   // Ability flow
   STDMETHOD (SuggestGoal)(cAIGoal *, cAIGoal **);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   //
   // IAIInform members
   //
   STDMETHOD_(BOOL, WantsInforms)();
   STDMETHOD (Post)(const sAIInform *);
   STDMETHOD (Receive)(const sAIInform *);

private:
   cAITimer  m_Timer;
   sAIInform m_Current;
   sAIInform m_Received;
   cAITimer  m_RetellDelay;

   cDynArray<ObjID> m_Passes;

   BOOL      m_fTriggered;

   static cDynArray<cAIInform *> gm_InformAbilities;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIINFORM_H */
