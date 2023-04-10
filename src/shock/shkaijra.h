/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaijra.h,v 1.2 1998/07/14 11:13:00 JON Exp $
//
// AI joint rotate
//

#ifndef __SHKAIJRA_H
#define __SHKAIJRA_H

#include <aiapiiai.h>
#include <aibasact.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointRotateAction
//
// Rotate a joint (don't move the object)
//

class cAIJointRotateAction : public cAIAction
{
public:
   cAIJointRotateAction(IAIAbility * pOwner, DWORD data = 0);

   // Set up action parameters
   void Set(int jointID, floatang targetAng, floatang rotateSpeed);
   void Set(int jointID, ObjID targetObj, floatang rotateSpeed);

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:
   int m_jointID;
   ObjID m_targetID;
   floatang m_targetAng;
   floatang m_rotateSpeed;
};

////////////////////////////////////////

inline cAIJointRotateAction::cAIJointRotateAction(IAIAbility * pOwner, DWORD data)
 : cAIAction(kAIAT_JointRotate, pOwner, data),
   m_targetID(OBJ_NULL)
{
}

#pragma pack()

#endif /* !__SHKAIJRA_H */









