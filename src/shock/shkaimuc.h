/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaimuc.h,v 1.3 1999/05/23 17:05:24 JON Exp $
//
//
//

#ifndef __SHKAIMUC_H
#define __SHKAIMUC_H

#include <aimultcb.h>
#include <aicbhtoh.h>

#include <aicbrngd.h>

#include <shkaicbr.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockMultiCombat
//

class cAIShockMultiCombat : public cAIMultiCombat
{
public:
   cAIShockMultiCombat();
   ~cAIShockMultiCombat();

   STDMETHOD_(void, GetSubComponents)(cAIComponentPtrs *);

   virtual cAISubcombat * SelectSubcombat(cAIGoal * pGoal, const cAIActions & previous);

   void SignalFrustration(void);

private:
   cAIShockRangedSubcombat m_rangedSubcombat;
   cAIHtoHSubcombat m_HtoHSubcombat;

   cAITimer m_rangedFrustrationTimer;
   cAITimer m_HtoHFrustrationTimer;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAIMUC_H */


