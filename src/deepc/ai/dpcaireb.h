/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//

#ifndef __DPCAIREB_H
#define __DPCAIREB_H

#include <dpcaisbs.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCRangedExplodeBehaviorSet
//
// Basic ranged combat 
//

class cAIDPCRangedExplodeBehaviorSet : public cAIDPCBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
};

#pragma pack()
#endif /* !__DPCAIREB_H */
