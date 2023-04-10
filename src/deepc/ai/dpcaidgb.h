/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCAIDGB_H
#define __DPCAIDGB_H

#include <ai.h>
#include <aibasbhv.h>
#include <aibascmp.h>

///////////////////////////////////////////////////////////////////////////////
//
// Dog behavior set
//

class cAIDogBehaviorSet : public cAIBehaviorSet
{ 
public: 
   STDMETHOD_(const char *, GetName)()
   {
      return "Dog";
   }
   
protected:
   virtual void CreateNonCombatAbilities(cAIComponentPtrs   * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs      * pComponents);
   virtual void CreateGenericAbilities(cAIComponentPtrs     * pComponents);
   virtual void CreateNonAbilityComponents(cAIComponentPtrs * pComponents);
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__DPCAIDGB_H */
