/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/daifrog.h,v 1.1 1998/10/21 22:19:02 TOML Exp $
//
//
//

#ifndef __DAIFROG_H
#define __DAIFROG_H

#include <aicombat.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

EXTERN void DarkAIInitFrogBeast();
EXTERN void DarkAITermFrogBeast();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFrogCombat
//

class cAIFrogCombat : public cAICombat
{
public: 
   
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__DAIFROG_H */
