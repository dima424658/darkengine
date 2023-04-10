/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaiabs.h,v 1.1 1999/04/02 13:16:55 JON Exp $
//
//
//

#ifndef __SHKAIABS_H
#define __SHKAIABS_H

#include <shkaisbs.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockBehaviorSet
//

class cAIActorBehaviorSet : public cAIShockBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateGenericAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
};

#pragma pack()
#endif /* !__SHKAIABS_H */






