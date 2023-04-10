/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  AI Multi Combat implementation.
//

#include <dpcaimuc.h>

#include <aitrginf.h>

#include <mprintf.h>

#include <dbmem.h>

const eAITimerPeriod kAIDPCFrustrationTime = kAIT_10Sec;

extern sHtoHModeSelections g_defaultDPCModeSelections;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCMultiCombat
//

cAIDPCMultiCombat::cAIDPCMultiCombat():
   m_HtoHSubcombat(&g_defaultDPCModeSelections),
   m_HtoHFrustrationTimer(kAIDPCFrustrationTime),
   m_rangedFrustrationTimer(kAIDPCFrustrationTime)
{
   m_HtoHSubcombat.InitSubability(this);
   m_rangedSubcombat.InitSubability(this);
   m_rangedSubcombat.SetPhyscastCallback(tPhyscastObjTest(DPCRangedPhyscastCallback));
   SetSubcombat(&m_rangedSubcombat);
   m_HtoHFrustrationTimer.Force();
   m_rangedFrustrationTimer.Force();
}
   
///////////////////////////////////////

cAIDPCMultiCombat::~cAIDPCMultiCombat()
{
   SafeRelease(m_pActiveSubcombat);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIDPCMultiCombat::GetSubComponents(cAIComponentPtrs * pResult)
{
   m_HtoHSubcombat.AddRef();
   pResult->Append(&m_HtoHSubcombat);
   m_rangedSubcombat.AddRef();
   pResult->Append(&m_rangedSubcombat);
}

///////////////////////////////////////////////////////////////////////////////

cAISubcombat* cAIDPCMultiCombat::SelectSubcombat(cAIGoal * pGoal, const cAIActions & previous)
{
   const sAITargetInfo* pTargetInfo = GetTargetInfo();

   if (!m_HtoHFrustrationTimer.Expired())
   {
      if (m_rangedFrustrationTimer.Expired())
         return &m_rangedSubcombat;
   }
   else if (!m_rangedFrustrationTimer.Expired() && m_HtoHFrustrationTimer.Expired())
      return &m_HtoHSubcombat;

   // @TODO: make more sensible, propertize
   if (pTargetInfo->distSq<100)
      return &m_HtoHSubcombat;
   else
      return &m_rangedSubcombat;
}

///////////////////////////////////////////////////////////////////////////////

void cAIDPCMultiCombat::SignalFrustration()
{
   if (m_pActiveSubcombat == &m_HtoHSubcombat)
      m_HtoHFrustrationTimer.Reset();
   else
      m_rangedFrustrationTimer.Reset();
}

///////////////////////////////////////////////////////////////////////////////

sHtoHModeSelections g_defaultDPCModeSelections =
{
   //////////////////////////
   //
   // Responses
   //
   {
      // OpponentAttack
      {
         0,                                      // no flags
         {
            {kSwingQuick, 3},
            {kSwingNormal, 1},
            {kBlock, 9},
            {kBackoff, 6},
            {kUndecided, 7},  // dont respond at all
         }
      },

      // LowHitpoints
      {
         0,                                      // no flags
         {
            {kSwingQuick, 1},
            {kBlock, 6},
            {kBackoff, 3},
            {kDodge, 6},
            {kUndecided, 7},  // dont respond at all
         }
      },

      // Opening
      {
         0,                                      // no flags
         {
            {kSwingQuick, 2},
            {kSwingNormal, 4},
            {kSwingMassive, 6},
            {kSwingSpecial, 1},
            {kUndecided, 8},  // dont respond at all
         }
      },

      // TooCloseToOpponent
      {
         0,                                      // no flags
         {
            {kJumpback, 1}
         }
      },

      // RemainingTooCloseToOpponent
      {
         0,                                      // no flags
         {
            {kSwingQuick, 3},
            {kCircleCCW, 1},
            {kCircleCW, 1},
            {kDodge, 1},
         }
      },

      // FailedBackup
      {
         0,                                      // no flags
         {
            {kSwingQuick, 2},
            {kSwingNormal, 3},
            {kCircleCCW, 1},
            {kCircleCW, 1},
            {kBlock, 1},
            {kAdvance, 1},
         }
      },

      // Cornered
      {
         0,                                      // no flags
         {
            {kSwingNormal, 2},
            {kSwingMassive, 2},
            {kAdvance, 3},
            {kBlock, 2},
            {kIdle, 1},
         }
      },
   },

   //////////////////////////
   //
   // Attacks
   //
   {
      // NormalWhileIdle
      {
         0,                                      // no flags
         {
            {kSwingQuick,     2},
            {kSwingNormal,    4},
            {kSwingMassive,   6},
            {kSwingSpecial,   1},
            {kBlock,          1},
            {kCircleCW,       1},
            {kCircleCCW,      1},
         }
      },

      // NormalWhileActive
      {
         0,                                      // no flags
         {
            {kSwingQuick,     2},
            {kSwingNormal,    2},
            {kSwingMassive,   2},
            {kCircleCW,       2},
            {kCircleCCW,      2},
         }
      },

      // OpponentAttacking
      {
         0,                                      // no flags
         {
            {kSwingQuick,     1},
            {kSwingNormal,    1},
            {kBlock,          8},
            {kDodge,          8},
            {kCircleCW,       2},
            {kCircleCCW,      2},
         }
      },

      // OpponentBlocking
      {
         0,                                      // no flags
         {
            {kSwingMassive,   2},
            {kDodge,          2},
            {kIdle,           2},
            {kCircleCW,       2},
            {kCircleCCW,      2},
         }
      },

      // OpponentUnarmed
      {
         0,                                      // no flags
         {
            {kSwingQuick,     2},
            {kSwingNormal,    4},
            {kSwingMassive,   8},
            {kSwingSpecial,   2},
         }
      }
   }
};
