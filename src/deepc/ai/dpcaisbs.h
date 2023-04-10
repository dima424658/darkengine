/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//

#ifndef __DPCAISBS_H
#define __DPCAISBS_H

#include <aibasbhv.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCBehaviorSet
//

class cAIDPCBehaviorSet : public cAIBehaviorSet
{
 public:
    //
    // Find out the behavior set name
    //
    STDMETHOD_(const char *, GetName)();

 protected:
    virtual void CreateNonCombatAbilities(cAIComponentPtrs   * pComponents);
    virtual void CreateCombatAbilities(cAIComponentPtrs      * pComponents);
    virtual void CreateNonAbilityComponents(cAIComponentPtrs * pComponents);
};

#pragma pack()
#endif /* !__DPCAISBS_H */






