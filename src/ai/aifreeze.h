/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aifreeze.h,v 1.1 1998/09/29 12:02:59 JON Exp $
//
//
//

#ifndef __AIFREEZE_H
#define __AIFREEZE_H

#include <aibasabl.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

void AIInitFreezeAbility(IAIManager *pAIManager);
void AITermFreezeAbility(void);
void AIClearFrozen(ObjID objID);
void AISetFrozen(ObjID obj, tSimTime duration);
BOOL AIIsFrozen(ObjID obj);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFreeze
//

class cAIFreeze : public cAIAbility
{
public:
   cAIFreeze();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();
   
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   
   STDMETHOD_(void, OnProperty)(IProperty *pProp, const sPropertyListenMsg *pMsg);

   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIFREEZE_H */
