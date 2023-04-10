/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aideath.h,v 1.4 1998/10/06 15:03:04 TOML Exp $
//
//

#ifndef __AIDEATH_H
#define __AIDEATH_H

#include <aibasabl.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// Enums and constants
//

enum eAIDeathCauses
{
   kAIDC_Invalid,
   kAIDC_Damage,
   kAIDC_Knockout,
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDeath
//

class cAIDeath : public cAIAbility
{
public:   
   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnDeath)(const sDamageMsg * pMsg);

   // Ability flow
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   // Cause of death
   int m_DeathCause;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIDEATH_H */
