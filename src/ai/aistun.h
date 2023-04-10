/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aistun.h,v 1.6 1999/04/23 12:23:25 JON Exp $
//

#ifndef __AISTUN_H
#define __AISTUN_H

#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICombatDummy
//

class cAIStun : public cAIAbility
{
public:
   // construct
   cAIStun();
   ~cAIStun();

   // this is the get me from outside and do stuff
   static cAIStun * AccessAIStun(ObjID id);

   // setup our notifications
   STDMETHOD_(void, Init)(void);
   
   // Standard component methods
   STDMETHOD_(const char *, GetName)(void);

   // yea, we got alert, are we suprised?
   STDMETHOD_(void, OnAlertness)(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw);
   
   // Update the status of the current goal
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);

   // Inform scripts when we stun/unstun
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);

   // Save/load
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Ability flow
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   // ways to set that you are stunned
   BOOL SetStunned(char    *beginTagsStr, char    *loopTagsStr, int ms);
   BOOL SetStunned(cTagSet *beginTags,    cTagSet *loopTags,    int ms);
   BOOL GetStunned() { return m_Stunned; }; 

   BOOL UnsetStunned(void);

private:
   BOOL     m_Stunned;
   
   cTagSet  m_StunBegin;
   cTagSet  m_StunLoop;
   int      m_StunMs;
   BOOL     m_SingleStun;
   BOOL     m_AtLoop;
   cAITimer m_Timer;

   BOOL     m_NewStun;

   static cDynArray<cAIStun *> cAIStun::gm_StunAbilities;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

void AIInitStunAbility(void);
void AITermStunAbility(void);

#endif /* !__AISTUN_H */
