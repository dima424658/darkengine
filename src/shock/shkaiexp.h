/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaiexp.h,v 1.1 1999/03/04 12:09:00 JON Exp $
//
// AI Action - explosion
//

#ifndef __SHKAIEXP_H
#define __SHKAIEXP_H

#include <aiapiiai.h>
#include <aibasact.h>
#include <aiactmot.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIExplodeAction
//
// Attack a target with a ranged attack
//

class cAIExplodeAction : public cAIMotionAction
{
public:
   cAIExplodeAction(IAIActor * pOwner, DWORD data = 0);

   // Set up action parameters
   void Set(ObjID projectileID);

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:
   ObjID m_projectileID;
};

////////////////////////////////////////

inline cAIExplodeAction::cAIExplodeAction(IAIActor * pOwner, DWORD data)
 : cAIMotionAction(pOwner, data)
{
   // @HACK, hack
   type = kAIAT_Explode;
   AddTags("RangedCombat");
}

#pragma pack()

#endif /* !__SHKAIEXP_H */









