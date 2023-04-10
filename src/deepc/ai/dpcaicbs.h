/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Camera behavior set
//

#ifndef __DPCAICBS_H
#define __DPCAICBS_H

#include <aibasbhv.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICameraBehaviorSet
//

class cAICameraBehaviorSet : public cAIBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected: 
   virtual void CreateNonAbilityComponents(cAIComponentPtrs * pComponents);
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
};

#pragma pack()
#endif /* !__DPCAICBS_H */
