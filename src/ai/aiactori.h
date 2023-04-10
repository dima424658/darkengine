/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactori.h,v 1.4 1999/03/02 17:42:01 TOML Exp $
//
//
//

#ifndef __AIACTORI_H
#define __AIACTORI_H

#include <aibasact.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIOrientAction
//

enum eAIOrientType
{
   kAIOT_Absolute,
   kAIOT_Toward,
   kAIOT_Relative,
   
   kAIOT_TypeMax = 0xffffffff
};

///////////////////////////////////////

class cAIOrientAction : public cAIAction
{
public:
   cAIOrientAction(IAIActor * pOwner, DWORD data = 0);
   ~cAIOrientAction();
   
   void Set(ObjID toward);
   void Set(floatang angle, ObjID relativeTo);
   void Set(floatang angle);
   void Set(floatang angle, ObjID relativeTo, eAIOrientType);
   
   // Update the action.
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:
   floatang        m_angle;
   ObjID           m_object;
   eAIOrientType   m_type;
   floatang        m_lastAngle;

   cAIMoveAction * m_pMoveAction;
   cAITimer        m_Timeout;
};

///////////////////////////////////////

inline cAIOrientAction::cAIOrientAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Orient, pOwner, data),
   m_angle(0.0),
   m_object(OBJ_NULL),
   m_pMoveAction(NULL),
   m_Timeout(AICustomTime(500))
{
}

///////////////////////////////////////

inline void cAIOrientAction::Set(ObjID toward)
{
   m_type   = kAIOT_Toward;
   m_object = toward;
}

///////////////////////////////////////

inline void cAIOrientAction::Set(floatang angle, ObjID relativeTo)
{
   m_type   = kAIOT_Relative;
   m_object = relativeTo;
   m_angle  = angle;
}

///////////////////////////////////////

inline void cAIOrientAction::Set(floatang angle)
{
   m_type  = kAIOT_Absolute;
   m_angle = angle;
}

///////////////////////////////////////

inline void cAIOrientAction::Set(floatang angle, ObjID object, eAIOrientType type)
{
   m_type   = type;
   m_object = object;
   m_angle  = angle;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTORI_H */
