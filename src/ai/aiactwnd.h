/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactwnd.h,v 1.3 1999/03/02 17:42:12 TOML Exp $
//
//
//

#ifndef __AIACTWND_H
#define __AIACTWND_H

#include <aibasact.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

#define kAIWA_InfiniteDur INT_MAX

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWanderAction
//

class cAIWanderAction : public cAIAction
{
public:

   cAIWanderAction(IAIActor * pOwner, DWORD data = 0);
   ~cAIWanderAction();
   
   //
   // Set the center of wandering. Uses the current AI location by default
   //
   void SetCenter(const mxs_vector &);
   
   //
   // Set the range of wandering
   //
   void SetRange(float max, float min = 0.0);
   
   //
   // Specify a motion to play on arrival to a wander point
   //
   void SetPauseMotion(const cTagSet &);
   
   //
   // Set an expiration time
   //
   void SetDuration(ulong);
   
   //
   // Set chance that the wandering will recenter on original point (pct)
   //
   void SetOddsRecenter(int pctChance);
   
   //
   // Set the minimum front-facing a point must have to be chosen
   //
   void SetMinFrontClearance(float);
  
   //
   // Set chance that the pause motion, if specified, will fire at waypoint
   //
   void SetOddsPauseMotion(int pctChance);
   

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:

   cAILocoAction * PickLoco();

   mxs_vector     m_Center;
   float          m_MinRadius;
   float          m_MaxRadius;
   cTagSet        m_PauseMotionTags;
   unsigned       m_OddsMotion;
   cAITimer       m_Timer;
   unsigned       m_OddsRecenter;
   float          m_MinFrontClearance;
   BOOL           m_fLastWasCenter;

   cAILocoAction *   m_pLocoAction;
   cAIMotionAction * m_pMotionAction;
};

///////////////////////////////////////

inline void cAIWanderAction::SetCenter(const mxs_vector & center)
{
   m_Center = center;
}

///////////////////////////////////////

inline void cAIWanderAction::SetRange(float max, float min)
{
   m_MinRadius = min;
   m_MaxRadius = max;
}

///////////////////////////////////////

inline void cAIWanderAction::SetPauseMotion(const cTagSet & tags)
{
   m_PauseMotionTags = tags;
}

///////////////////////////////////////

inline void cAIWanderAction::SetDuration(ulong time)
{
   m_Timer.Set(AICustomTime(time));
}

///////////////////////////////////////

inline void cAIWanderAction::SetOddsRecenter(int pctChance)
{
   m_OddsRecenter = pctChance;
}

///////////////////////////////////////

inline void cAIWanderAction::SetMinFrontClearance(float clearance)
{
   m_MinFrontClearance = clearance;
}

///////////////////////////////////////

inline void cAIWanderAction::SetOddsPauseMotion(int pctChance)
{
   m_OddsMotion = pctChance;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTWND_H */
