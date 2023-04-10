/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaijsa.h,v 1.1 1998/07/14 11:18:20 JON Exp $
//
// AI joint slide
//

#ifndef __SHKAIJSA_H
#define __SHKAIJSA_H

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
   cAIJointSlideAction(IAIAbility * pOwner, DWORD data = 0);

   // Set up action parameters
   void Set(int jointID, float target, float speed);

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

   float GetTarget(void) const;

private:
   int m_jointID;
   float m_target;
   float m_speed;
};

////////////////////////////////////////

inline cAIJointSlideAction::cAIJointSlideAction(IAIAbility * pOwner, DWORD data)
 : cAIAction(kAIAT_JointSlide, pOwner, data)
{
}

////////////////////////////////////////

inline float cAIJointSlideAction::GetTarget(void) const {return m_target;}

////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAIJSA_H */









