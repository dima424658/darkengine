/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaimc.h,v 1.1 1999/05/23 15:31:15 JON Exp $
//

#ifndef __SHKAIMC_H
#define __SHKAIMC_H

#include <aimultcb.h>
#include <aicbhtoh.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockMeleeMultiCombat
//

class cAIShockMeleeMultiCombat : public cAIMultiCombat
{
public:
   cAIShockMeleeMultiCombat();
   ~cAIShockMeleeMultiCombat();
   
   STDMETHOD_(void, GetSubComponents)(cAIComponentPtrs *);

   cAISubcombat * SelectSubcombat(cAIGoal * pGoal, const cAIActions & previous);

protected:

   virtual BOOL IsValidTarget(ObjID object);
};

#pragma pack()

#endif /* !__SHKAIMC_H */
