/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __DPCAIWND_H
#define __DPCAIWND_H

#include <ainoncbt.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWander
//

class cAIWander : public cAINonCombatAbility
{
public:
   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Ability flow
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__DPCAIWND_H */
