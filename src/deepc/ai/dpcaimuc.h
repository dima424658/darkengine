/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//

#ifndef __DPCAIMUC_H
#define __DPCAIMUC_H

#include <aimultcb.h>
#include <aicbhtoh.h>

#include <aicbrngd.h>

#include <dpcaicbr.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCMultiCombat
//

class cAIDPCMultiCombat : public cAIMultiCombat
{
public:
   cAIDPCMultiCombat();
   ~cAIDPCMultiCombat();

   STDMETHOD_(void, GetSubComponents)(cAIComponentPtrs *);

   virtual cAISubcombat * SelectSubcombat(cAIGoal * pGoal, const cAIActions & previous);

   void SignalFrustration(void);

private:
   cAIDPCRangedSubcombat m_rangedSubcombat;
   cAIHtoHSubcombat m_HtoHSubcombat;

   cAITimer m_rangedFrustrationTimer;
   cAITimer m_HtoHFrustrationTimer;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif // !__DPCAIMUC_H


