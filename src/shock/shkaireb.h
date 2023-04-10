/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaireb.h,v 1.1 1999/06/04 18:55:10 JON Exp $
//
//
//

#ifndef __SHKAIREB_H
#define __SHKAIREB_H

#include <shkaisbs.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockRangedExplodeBehaviorSet
//
// Basic ranged combat 
//

class cAIShockRangedExplodeBehaviorSet : public cAIShockBehaviorSet
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
#endif /* !__SHKAIREB_H */






