/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapbow.h,v 1.4 2000/01/31 09:45:49 adurant Exp $
//
// Weapon (hand-to-hand-combat) related functions
//
#pragma once

#ifndef __WEAPBOW_H
#define __WEAPBOW_H

#include <objtype.h>

EXTERN BOOL  EquipBow();
EXTERN BOOL  UnEquipBow();
EXTERN BOOL  IsBowEquipped();

EXTERN void  StartBowAttack();
EXTERN void  FinishBowAttack();

EXTERN BOOL  SetCurrentArrow(ObjID arrow);

EXTERN ObjID PlayerArrow();

EXTERN void BowInit();
EXTERN void BowTerm(); 

#endif
