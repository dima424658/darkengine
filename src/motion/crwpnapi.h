/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef _CRWPNAPI_H
#define _CRWPNAPI_H

typedef struct 
{
   ObjID obj;           // Creature's ID.
   ObjID weaponObj;     // Creature's weapon ID.
   int   nWeapon;       // Current Creature's weapon.
} sCreatureWeapon;

void CreatureWeaponInit(void);
void CreatureWeaponTerm(void);

EXTERN int  (*CreatureWeaponObjSwizzle) (ObjID weapon, int mode);
EXTERN BOOL (*CreatureWeaponObjCreate)  (ObjID id, int mode);
EXTERN void (*CreatureWeaponObjDestroy) (ObjID id);
EXTERN void (*CreatureWeaponObjDrop)    (ObjID id);

// WeaponModes - used for picking appropriate weapon for mode.
enum
{
   kWeaponModeInvalid = -1,
   kWeaponModeBlock   = (1<<0),
   kWeaponModeMelee   = (1<<1),
   kWeaponModeRanged  = (1<<2),
   kWeaponModeIdle    = (1<<3),
   kNumWeaponModes = 4
};

// Internal API
// ClearWeaponOffsetCache:  Make sure we don't have anything dangling about.
EXTERN void ClearWeaponOffsetCache(void);
#endif // _CRWPNAPI_H
