/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkairnb.h,v 1.1 1999/03/31 18:51:26 JON Exp $
//
//
//

#ifndef __SHKAIRNB_H
#define __SHKAIRNB_H

#include <shkaisbs.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockRangedBehaviorSet
//
// Basic ranged combat 
//

class cAIShockRangedBehaviorSet : public cAIShockBehaviorSet
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
#endif /* !__SHKAIRNB_H */






