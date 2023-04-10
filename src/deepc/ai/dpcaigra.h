/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//

#ifndef __DPCAIGRA_H
#define __DPCAIGRA_H

#ifndef __AICOMBAT_H
#include <aicombat.h>
#endif // !__AICOMBAT_H

#ifndef _CTAGSET_H
#include <ctagset.h>
#endif // !_CTAGSET_H

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

EXTERN void DPCAIInitGrubCombatAbility(void);

EXTERN void DPCAITermGrubCombatAbility(void);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGrubCombat
//

enum eAIGrubCombatState {kAIGrubCombatNothing, kAIGrubCombatCharge, kAIGrubCombatRandom, kAIGrubCombatLeap,};

class cAIGrubCombat : public cAICombat
{
public:   
   cAIGrubCombat();
   ~cAIGrubCombat();

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
   eAIGrubCombatState m_state;
   cAIRandomTimer m_leapTimer; 
   float m_biteDist;
   float m_leapDist;

   void SetState(eAIGrubCombatState state);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__DPCAIGRA_H */
