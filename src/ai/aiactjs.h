/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactjs.h,v 1.3 1999/03/02 17:41:59 TOML Exp $
//
// AI joint slide
//

#ifndef __AIACTJS_H
#define __AIACTJS_H

#include <aiapiiai.h>
#include <aibasact.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointSlideAction
//
// Slide a joint (don't move the object)
//

class cAIJointSlideAction : public cAIAction
{
public:
   cAIJointSlideAction(IAI *pAI, void *netmsg);
   cAIJointSlideAction(IAIActor * pOwner, DWORD data = 0);
   virtual ~cAIJointSlideAction();

   // Set up action parameters
   void Set(int jointID, float target, float speed);

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

   float GetTarget(void) const;

protected:
   // Broadcast the action in a multi-player game.
   void BroadcastAction(ulong deltaTime);

private:
   int m_jointID;
   float m_target;
   float m_speed;
};

////////////////////////////////////////

inline cAIJointSlideAction::cAIJointSlideAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_JointSlide, pOwner, data)
{
}

////////////////////////////////////////

inline float cAIJointSlideAction::GetTarget(void) const {return m_target;}

////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTJS_H */









