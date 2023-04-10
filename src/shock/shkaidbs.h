/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaidbs.h,v 1.2 1999/03/31 18:48:40 JON Exp $
//
//
//

#ifndef __SHKAIDBS_H
#define __SHKAIDBS_H

#include <shkaisbs.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDroidBehaviorSet
//

class cAIDroidBehaviorSet : public cAIShockBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

protected:
   virtual void CreateNonCombatAbilities(cAIComponentPtrs * pComponents);
   virtual void CreateCombatAbilities(cAIComponentPtrs * pComponents);
};

#pragma pack()
#endif /* !__SHKAIDBS_H */






