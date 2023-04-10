/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//

#ifndef __DPCAIRBS_H
#define __DPCAIRBS_H

#include <dpcaisbs.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCRangedMeleeBehaviorSet
//

class cAIDPCRangedMeleeBehaviorSet : public cAIDPCBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
};

#pragma pack()
#endif /* !__DPCAIRBS_H */
