/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactwt.h,v 1.3 1999/03/02 17:42:12 TOML Exp $
//
//
//

#ifndef __AIACTWT_H
#define __AIACTWT_H

#include <aibasact.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

class cAIMotionAction;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWaitAction
//

class cAIWaitAction : public cAIAction
{
public:
   cAIWaitAction(IAIActor * pOwner, DWORD data = 0);
   ~cAIWaitAction();

   // Add to the sequence
   void Set(unsigned waitTime);
   
   // Update the action.
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

   // Save/Load the action.  Assumes pTagFile cursor is positioned for this action.
   STDMETHOD_(BOOL, Save)(ITagFile * pTagFile);
   STDMETHOD_(BOOL, Load)(ITagFile * pTagFile);

private:
   // Time when done waiting
   unsigned          m_TimeDone;
   
   // Motion to play, if any (if tags set)
   cAIMotionAction * m_pMotion;
   
   // Switch timer, if not playing a motion
   cAITimer          m_Timer;

};

///////////////////////////////////////

inline cAIWaitAction::cAIWaitAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Wait, pOwner, data),
   m_TimeDone(0), 
   m_pMotion(NULL),
   m_Timer(kAIT_2Hz)
{
}

///////////////////////////////////////

inline void cAIWaitAction::Set(unsigned waitTime)
{
   m_TimeDone = AIGetTime() + waitTime;
   m_Timer.Reset();
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTWT_H */
