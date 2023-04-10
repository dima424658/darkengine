/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aigests.h,v 1.2 2000/01/29 12:45:18 adurant Exp $
//
//
//
#pragma once

#ifndef __AIGESTS_H
#define __AIGESTS_H

///////////////////////////////////////////////////////////////////////////////
//
// Conceptual gestures
//

typedef unsigned eAIGestureType;

enum eAIGestureTypeEnum
{
   kAIGT_CoreBase = 0,
   
   kAIGT_None = kAIGT_CoreBase,
   kAIGT_Calibration,
   kAIGT_Rest,
   kAIGT_Fidget,

   kAIGT_GameBase = 10000,

#if 0
   kAIGT_AlertLevelOne,
   kAIGT_AlertLevelTwo,
   kAIGT_AlertLevelThreeFront,
   kAIGT_AlertLevelThreeRear,
   kAIGT_CallingForHelp,
   kAIGT_EnteringAttackMode,
   kAIGT_SwordAttack,
   kAIGT_SwordAttShort,
   kAIGT_SwordAttMedium,
   kAIGT_SwordAttLong,
   kAIGT_SwordParry,
   kAIGT_SwordDodge,
   kAIGT_SideStepRight,
   kAIGT_SideStepLeft,
   kAIGT_Retreat,
   kAIGT_RetreatFast,   
   kAIGT_Advance,
   kAIGT_Death,
   kAIGT_Walk,
   kAIGT_Awaken,
   kAIGT_Hurt,
   kAIGT_HurtLight,
   kAIGT_HurtLots,
   kAIGT_CombatIdle,
   kAIGT_Stun,
#endif
   
   kAIGType_TypeMax=0xffffffff // force it use an int
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIGESTS_H */
