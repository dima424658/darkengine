/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaiwnd.h,v 1.1 1998/11/23 09:16:31 JON Exp $
//
//
//

#ifndef __SHKAIWND_H
#define __SHKAIWND_H

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

#endif /* !__SHKAIWND_H */
