/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactinv.h,v 1.8 1999/11/29 12:27:12 BFarquha Exp $
//
//

#ifndef __AIACTINV_H
#define __AIACTINV_H

#include <aiactloc.h>
#include <aibasact.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

class cAIMotionAction;
class cAILocoAction;
class cAIOrientAction;
class cAIWanderAction;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIInvestAction
//

#define kAIIA_InfiniteDur INT_MAX

enum eAIInvestStyleFlags
{
   kAIIS_InitOrient = 0x01,
   kAIIS_InitScan   = 0x02,
   kAIIS_EndWander  = 0x04,
   kAIIS_VarSpeed   = 0x08,

   kAIAF_All        = 0xffffffff
};

///////////////////////////////////////

class cAIInvestAction : public cAIAction
{
public:
   cAIInvestAction(IAIActor * pOwner, DWORD data = 0);
   ~cAIInvestAction();

   //
   // Investigate a point, using an object as a hint for determining
   // the destination cell
   //
   void Set(const cMxsVector & dest,
            ObjID              hintObject,
            eAISpeed           speed      = kAIS_Num, // defaults based on distance
            float              accuracySq = kAILA_DefaultAccuracy,
            float              accuracyZ  = kAILA_DefaultAccuracyZ);

   //
   // Investigate an object
   //
   void Set(ObjID    object,
            eAISpeed speed      = kAIS_Num,     // defaults based on distance
            float    accuracySq = kAILA_DefaultAccuracy,
            float    accuracyZ  = kAILA_DefaultAccuracyZ);

   //
   // Investigate a point
   //
   void Set(const cMxsVector & dest,
            eAISpeed           speed      = kAIS_Num, // defaults based on distance
            float              accuracySq = kAILA_DefaultAccuracy,
            float              accuracyZ  = kAILA_DefaultAccuracyZ);


   // Path near object if actually can't path to. Should occur before Set.
   void SetNearPath(BOOL bPathNear, float fDist);

   void SetCheckPathDist(BOOL bCheck);

   //
   // Flags
   //
   void SetStyle(unsigned flags);
   void SetDuration(unsigned time); // set to kAIIA_InfiniteDur if want to never expire

   //
   // Scan settings
   //
   void SetScan(unsigned minScanDelay,
                unsigned maxScanDelay);
   void ResetScan();
   void ForceScan();

   BOOL IsScanning() const;
   BOOL ReachedPoint() const;

   const mxs_vector & GetDest() const;

   float ComputePathDist();

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:
   enum eState
   {
      kNotStarted,
      kInitOrient,
      kEnRoute,
      kScanEnRoute,
      kScanAtLoc,

      kIntMax = 0xffffffff
   };

   cAILocoAction *   m_pLocoAction;
   cAIMotionAction * m_pMotionAction;
   cAIOrientAction * m_pOrientAction;
   cAIWanderAction * m_pWanderAction;

   cAITimer          m_expiration;
   unsigned          m_StyleFlags;
   cAIRandomTimer    m_ScanTimer;
   cAITimer          m_CheckDistTimer;

   eState            m_state;

   float fPathNearDist;
   BOOL m_bDoCheckDist;
};

///////////////////////////////////////

// Path near object if actually can't path to.
inline void cAIInvestAction::SetNearPath(BOOL bPathNear, float fDist)
{
   if (bPathNear)
      fPathNearDist = fDist;
   else
      fPathNearDist = -1;
}


inline void cAIInvestAction::SetCheckPathDist(BOOL bCheck)
{
   m_bDoCheckDist = bCheck;
}


///////////////////////////////////////

inline BOOL cAIInvestAction::IsScanning() const
{
   return !!m_pMotionAction;
}

///////////////////////////////////////

inline BOOL cAIInvestAction::ReachedPoint() const
{
   return (m_state == kScanAtLoc);
}

///////////////////////////////////////

inline void cAIInvestAction::SetStyle(unsigned flags)
{
   m_StyleFlags |= flags;
}

///////////////////////////////////////

inline void cAIInvestAction::SetDuration(unsigned time)
{
   m_expiration.Set(AICustomTime(time));
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTINV_H */
