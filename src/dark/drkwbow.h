/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/drkwbow.h,v 1.6 2000/01/31 09:40:07 adurant Exp $
//
// Weapon (hand-to-hand-combat) related functions
//
#pragma once

#ifndef __DRKWBOW_H
#define __DRKWBOW_H

#include <objtype.h>

EXTERN BOOL EquipBow();
EXTERN BOOL UnEquipBow();
EXTERN BOOL IsBowEquipped();
EXTERN BOOL IsBowArrowKnocked();

EXTERN void StartBowAttack();
EXTERN BOOL FinishBowAttack();
EXTERN BOOL AbortBowAttack();
EXTERN void ForceFinishBowAttack(BOOL abort);
EXTERN void UpdateBowAttack(long dt);

EXTERN BOOL SetCurrentArrow(ObjID arrow);

EXTERN ObjID PlayerArrow();

EXTERN void SetBowZoomActive (BOOL active);

EXTERN void BowInit();
EXTERN void BowTerm();


#endif
