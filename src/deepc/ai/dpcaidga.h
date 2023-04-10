/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __DPCAIDGA_H
#define __DPCAIDGA_H

#include <aicombat.h>
#include <ctagset.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

EXTERN void DPCAIInitDogCombatAbility(void);

EXTERN void DPCAITermDogCombatAbility(void);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDogCombat
//

enum eAIDogCombatState {kAIDogCombatNothing, kAIDogCombatCharge, kAIDogCombatRandom, kAIDogCombatLeap,};

class cAIDogCombat : public cAICombat
{
public:   
   cAIDogCombat(void);
   ~cAIDogCombat(void);

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(void, Init)();

   STDMETHOD_(void, OnSimStart)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnCollision)(const sPhysListenMsg *);
   
   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   eAIDogCombatState m_state;
   cAIRandomTimer    m_leapTimer; 
   float             m_biteDist;
   float             m_leapDist;

   void SetState(eAIDogCombatState state);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__DPCAIDGA_H */
