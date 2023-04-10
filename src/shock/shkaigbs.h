/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaigbs.h,v 1.3 1999/03/25 17:32:41 JON Exp $
//
//
//

#ifndef __SHKAIGBS_H
#define __SHKAIGBS_H

#include <aibasbhv.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGruntBehaviorSet
//

class cAIGruntBehaviorSet : public cAIBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateNonAbilityComponents(cAIComponentPtrs * pComponents);
};

#pragma pack()
#endif /* !__SHKAIGBS_H */






