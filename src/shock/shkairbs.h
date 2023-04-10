/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkairbs.h,v 1.2 1999/03/31 18:50:59 JON Exp $
//
//
//

#ifndef __SHKAIRBS_H
#define __SHKAIRBS_H

#include <shkaisbs.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockRangedMeleeBehaviorSet
//

class cAIShockRangedMeleeBehaviorSet : public cAIShockBehaviorSet
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
#endif /* !__SHKAIRBS_H */






