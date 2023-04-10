/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkmelee.h,v 1.6 2000/01/31 09:57:56 adurant Exp $
// Melee weapons
#pragma once

#ifndef __SHKMELEE_H
#define __SHKMELEE_H

#include <shkmelpr.h>

extern void ShockMeleeInit(void);
extern void ShockMeleeTerm(void);

extern BOOL IsMelee(ObjID weapon);
extern BOOL EquipMelee(ObjID owner, ObjID weapon, int type);
extern BOOL UnEquipMelee(ObjID owner, ObjID weapon);
extern BOOL DestroyMelee(ObjID owner, ObjID weapon);
extern BOOL IsCurrentPlayerArm(ObjID weapon);

extern void StartMeleeAttack(ObjID owner, ObjID weapon);
extern void FinishMeleeAction(ObjID owner, ObjID weapon);

// player melee action types
enum {
   kPlayerMeleeActionSwingMediumLeft,
   kPlayerMeleeActionSwingLong,
   kPlayerMeleeNumActions,
};

enum eShockWeaponType
{
   kSWT_Wrench,
};

#endif
