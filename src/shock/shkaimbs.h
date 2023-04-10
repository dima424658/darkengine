/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaimbs.h,v 1.2 1999/03/31 18:50:15 JON Exp $
//
//
//

#ifndef __SHKAIMBS_H
#define __SHKAIMBS_H

#include <shkaisbs.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMeleeBehaviorSet
//

class cAIMeleeBehaviorSet : public cAIShockBehaviorSet
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
#endif /* !__SHKAIMBS_H */






