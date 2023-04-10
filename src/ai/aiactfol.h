/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactfol.h,v 1.4 1999/03/02 17:41:58 TOML Exp $
//
//
//

#ifndef __AIACTFOL_H
#define __AIACTFOL_H

#include <dynarray.h>

#include <aibasact.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFollowAction
//

#define kAIF_UpdateTime  AICustomTime(250)

class cAIFollowAction : public cAIAction
{
public:
   cAIFollowAction(IAIActor * pOwner, DWORD data = 0);
   virtual ~cAIFollowAction();
   
   void SetObject(ObjID object);
   void AddVector(floatang angle, float distance);

   // Update the action.
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:
   ObjID                m_object;
   cDynArray<floatang>  m_angles;
   cDynArray<float>     m_distances;
   cAITimer             m_timer;

   cAIMoveAction *      m_pMoveAction;
};

///////////////////////////////////////

inline cAIFollowAction::cAIFollowAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Follow, pOwner, data),
   m_object(OBJ_NULL),
   m_timer(kAIF_UpdateTime),
   m_pMoveAction(NULL)
{
}

///////////////////////////////////////

inline void cAIFollowAction::SetObject(ObjID object)
{
   m_object = object;
}

///////////////////////////////////////

inline void cAIFollowAction::AddVector(floatang angle, float distance)
{
   m_angles.Append(angle);
   m_distances.Append(distance);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTFOL_H */
