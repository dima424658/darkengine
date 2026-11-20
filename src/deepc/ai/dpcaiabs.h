/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//

#ifndef __DPCAIABS_H
#define __DPCAIABS_H

#include <dpcaisbs.h>

#pragma once
#pragma pack(push, 4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCBehaviorSet
//

class cAIActorBehaviorSet : public cAIDPCBehaviorSet
{
 public:
    //
    // Find out the behavior set name
    //
    STDMETHOD_(const char *, GetName)();

 protected:
    virtual void CreateGenericAbilities(cAIComponentPtrs   * pComponents);
    virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
    virtual void CreateCombatAbilities(cAIComponentPtrs    * pComponents);
};
#pragma pack(pop)
#endif /* !__DPCAIABS_H */
