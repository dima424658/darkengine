/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weaphit.h,v 1.2 2000/01/31 09:45:54 adurant Exp $
//
// Weapon (hand-to-hand-combat) logic header 
//
#pragma once

#ifndef __WEAPHIT_H
#define __WEAPHIT_H

#include <objtype.h>

//
// Handles the pre-damage elements of weapons hitting things 
// (i.e. blocking)
// Returns weather the "hit" should do damage
//
EXTERN BOOL HandleWeaponHit(ObjID hitee, ObjID weapon);
EXTERN BOOL HandleWeaponBlock(ObjID owner1, ObjID weapon1, ObjID owner2, ObjID weapon2); 

// de-physicalize post hit, pass OBJ_NULL if you don't know the owner
EXTERN void WeaponPostHit(ObjID hitee, ObjID weapon, ObjID owner);

//
// Resolve a full weapon attack, including damage propagation.
// Magnitude is in momentum units, I think.  
//

EXTERN BOOL ResolveWeaponAttack(ObjID weapon, ObjID victim, float magnitude, const struct sChainedEvent* cause); 

#endif // __WEAPHIT_H



