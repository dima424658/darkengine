/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaicha.h,v 1.3 1999/02/09 09:43:17 JON Exp $
//
//
//

#ifndef __SHKAICHA_H
#define __SHKAICHA_H

#include <aiactloc.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIChargeAction
//
// A simple loco action that tries to follow a moving object
//

///////////////////////////////////////

class cAIChargeAction : public cAILocoAction
{
public:
   cAIChargeAction(IAIAbility* pOwner, DWORD data);

   void Set(ObjID object, eAITimerPeriod rePathPeriod);
   void Set(const cMxsVector & dest, eAITimerPeriod rePathPeriod);

   BOOL ToObject(void) const {return m_toObj;};  // are we heading to an object?

   STDMETHOD_(eAIResult, Update)();

   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

private:
   BOOL m_toObj;                  // heading to obj (or loc)?
   cAITimer m_rePathTimer;       // frequency of re-checking path
};

///////////////////////////////////////

inline cAIChargeAction::cAIChargeAction(IAIAbility * pOwner, DWORD data)
 : cAILocoAction(pOwner, data),
   m_rePathTimer(kAIT_1Sec)
{
   // @HACK, hack
   type = kAIAT_Charge;
}

///////////////////////////////////////

inline void cAIChargeAction::Set(ObjID object, eAITimerPeriod rePathPeriod)
{
   m_toObj = TRUE;
   m_rePathTimer.Set(rePathPeriod);
   cAILocoAction::Set(object, kAIS_Fast, 0);
}

///////////////////////////////////////

inline void cAIChargeAction::Set(const cMxsVector & dest, eAITimerPeriod rePathPeriod)
{
   m_toObj = FALSE;
   m_rePathTimer.Set(rePathPeriod);
   cAILocoAction::Set(dest, kAIS_Fast, 0);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAICHA_H */
