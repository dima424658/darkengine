/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Melee weapons
#pragma once

#ifndef __DPCMELEE_H
#define __DPCMELEE_H

#ifndef __DPCMELPR_H
#include <dpcmelpr.h>
#endif // !__DPCMELPR_H

extern void DPCMeleeInit(void);
extern void DPCMeleeTerm(void);

extern BOOL IsMelee(ObjID weapon);
extern BOOL EquipMelee(ObjID owner, ObjID weapon, int type);
extern BOOL UnEquipMelee(ObjID owner, ObjID weapon);
extern BOOL DestroyMelee(ObjID owner, ObjID weapon);
extern BOOL IsCurrentPlayerArm(ObjID weapon);

extern void StartMeleeAttack(ObjID owner, ObjID weapon);
extern void FinishMeleeAction(ObjID owner, ObjID weapon);

// player melee action types
enum
{
   kPlayerMeleeActionSwingMediumLeft,
   kPlayerMeleeActionSwingLong,
   kPlayerMeleeNumActions,
};

enum eDPCWeaponType
{
   kSWT_Wrench,
};

#endif  // __DPCMELEE_H
