/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//
//

#ifndef __AITURBS_H
#define __AITURBS_H

#include <aibasbhv.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITurretBehaviorSet
//

class cAITurretBehaviorSet : public cAIBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateNonAbilityComponents(cAIComponentPtrs * pComponents);
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
};

#pragma pack()
#endif /* !__AITURBS_H */






