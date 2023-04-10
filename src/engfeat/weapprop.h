/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapprop.h,v 1.7 2000/01/31 09:45:56 adurant Exp $
//
// Weapon (hand-to-hand-combat) property-related functions
//
#pragma once

#ifndef __WEAPPROP_H
#define __WEAPPROP_H

#include <relation.h>

////////////////////////////////////////

EXTERN void InitWeaponRelation(void);
EXTERN void TermWeaponRelation(void);

////////////////////////////////////////

#define PROP_BASE_WEAPON_DAMAGE_NAME "BaseWpnDmg"

EXTERN void InitBaseWeaponDamageProp(void);
EXTERN void TermBaseWeaponDamageProp(void);

////////////////////////////////////////

#define PROP_WEAPON_MODE_CHANGE_METAPROP_NAME "ModeChangeMeta"

EXTERN void InitWeaponModeChangeMetaProp(void);
EXTERN void TermWeaponModeChangeMetaProp(void);
EXTERN const char *GetWeaponModeChangeMetaProp(ObjID weapon);

////////////////////////////////////////

#define PROP_WEAPON_MODE_UNCHANGE_METAPROP_NAME "ModeUnchngeMeta"

EXTERN void InitWeaponModeUnchangeMetaProp(void);
EXTERN void TermWeaponModeUnchangeMetaProp(void);
EXTERN const char *GetWeaponModeUnchangeMetaProp(ObjID weapon);

////////////////////////////////////////

#define PROP_WEAPON_EXPOSURE_NAME "WpnExposure"

EXTERN void InitWeaponExposureProp(void);
EXTERN void TermWeaponExposureProp(void);

EXTERN void SetWeaponExposure(ObjID weapon, int exposure);
EXTERN int  GetWeaponExposure(ObjID weapon);

////////////////////////////////////////

#define PROP_SWING_EXPOSURE_NAME    "SwingExpose"

EXTERN void InitWeapSwingExposureProp();
EXTERN void TermWeapSwingExposureProp();

EXTERN void SetWeapSwingExposure(ObjID weapon, int exposure);
EXTERN int  GetWeapSwingExposure(ObjID weapon);
   
////////////////////////////////////////

#define PROP_CUR_WEAPON_DAMAGE_NAME "CurWpnDmg"

EXTERN void InitCurWeaponDamageProp(void);
EXTERN void TermCurWeaponDamageProp(void);

////////////////////////////////////////

#define PROP_WEAPON_TERRAIN_COLLISON_NAME "WpnTerrColl"

EXTERN void InitWeaponTerrainCollisionProp(void);
EXTERN void TermWeaponTerrainCollisionProp(void);

EXTERN void SetWeaponTerrainCollision(ObjID weapon, BOOL exposure);
EXTERN BOOL GetWeaponTerrainCollision(ObjID weapon);

////////////////////////////////////////

// Set/Get the weapon attack damage
EXTERN void SetBaseWeaponDamage(ObjID weapon, int damage);
EXTERN int  GetBaseWeaponDamage(ObjID weapon);

EXTERN void SetWeaponDamageFactor(ObjID weapon, int factor);
EXTERN void ClearWeaponDamageFactor(ObjID weapon);

EXTERN int  GetWeaponDamage(ObjID weapon);

////////////////////////////////////////

EXTERN IRelation *g_pWeaponRel;

#endif
