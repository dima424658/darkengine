/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ainoncbt.h,v 1.3 1998/06/19 14:32:10 TOML Exp $
//
// Base class for general non-combat abilities. Mostly takes care of default
// broadcast right now. (toml 05-20-98)
//

#ifndef __AINONCBT_H
#define __AINONCBT_H

#include <aibasabl.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAINonCombatAbility
//

class cAINonCombatAbility : public cAIAbility
{
public:
   cAINonCombatAbility();

   // On begin frame, if in control broadcast alertness
   STDMETHOD_(void, OnBeginFrame)();

   // On alertness, broadcast transitions
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

   virtual void BroadcastAlertness();
   
protected:   
   cAITimer m_BroadcastTimer;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AINONCBT_H */
