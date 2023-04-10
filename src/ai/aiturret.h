/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiturret.h,v 1.6 1999/05/15 13:14:20 JON Exp $
//
// AI Turret Ability
//

#ifndef __AITURRET_H
#define __AITURRET_H

#include <aidev.h>

#pragma once
#pragma pack(4)

class cAITurret;

typedef cAIAction*(*GunActionFactoryFn)(cAITurret *pTurret, ObjID targetObj, int subModel); 

class cAITurret : public cAIDevice
{
public:
   cAITurret();

   STDMETHOD_(void, Set) (GunActionFactoryFn gunActionFactoryFn) {m_gunActionFactoryFn = gunActionFactoryFn;};

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);

   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   ObjID GetRangedWeapon(void);
   BOOL CheckPitch(ObjID targetID);
   eAIPriority CheckRange(ObjID targetID);
   virtual eAIPriority TargetPriority(ObjID targetID);

   BOOL CheckLOF(ObjID targetID, int* pSubModel);
   BOOL CheckLOF(ObjID targetID, int subModel);

   cAITimer m_fireTimer;
   GunActionFactoryFn m_gunActionFactoryFn;
};

#pragma pack()

#endif /* !__AITURRET_H */
