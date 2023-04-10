/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaisbs.h,v 1.1 1999/03/31 18:52:58 JON Exp $
//
//
//

#ifndef __SHKAISBS_H
#define __SHKAISBS_H

#include <aibasbhv.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockBehaviorSet
//

class cAIShockBehaviorSet : public cAIBehaviorSet
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
#endif /* !__SHKAISBS_H */






