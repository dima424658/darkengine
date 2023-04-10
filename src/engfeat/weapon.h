/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapon.h,v 1.14 2000/01/31 09:45:55 adurant Exp $
//
// Weapon (hand-to-hand-combat) related functions
//
#pragma once

#ifndef __WEAPON_H
#define __WEAPON_H

#include <objtype.h>
#include <config.h>
#include <relatio_.h>

#ifndef __MATRIXS_H
#include <matrixs.h>
#endif // !__MATRIXS_H

////////////////////////////////////////////////////////////////////////////////

// Initializes the weapons
EXTERN void InitWeapon();
EXTERN void TermWeapon();

// Sets the weapon for the specified owner (can be either object or archetype)
EXTERN ObjID SetWeapon(ObjID owner, ObjID sword, int mode);

// Remove the weapon from active duty
EXTERN BOOL  UnSetWeapon(ObjID owner);

// for actual swings, to set current state of the swing
EXTERN BOOL StartWeaponSwing(ObjID owner, ObjID weapon, int val);
EXTERN BOOL FinishWeaponSwing(ObjID owner, ObjID weapon);
// returns 0 if weapon isnt right weapon, or if it isnt swinging
// else return weapon value
EXTERN int IsWeaponSwinging(ObjID weapon);

// Check if the specified object is a weapon
EXTERN BOOL  IsWeapon(ObjID weapon);

// Get the object ID of the weapon of the specified owner, and vice-versa
// (returns OBJ_NULL on fail)
EXTERN ObjID GetWeaponObjID(ObjID owner);
EXTERN ObjID GetWeaponOwnerObjID(ObjID weapon);

#define WEAPON_RELATION_NAME "Weapon"

EXTERN IRelation *g_pWeaponRelation;

////////////////////////////////////////////////////////////////////////////////

// AI-specific

EXTERN BOOL  EquipAIWeapon(ObjID owner, ObjID *pWeapon, int mode);
EXTERN BOOL  UnEquipAIWeapon(ObjID owner, ObjID weapon);

EXTERN BOOL  MakeAIWeaponPhysical(ObjID owner, ObjID weapon);
EXTERN void  MakeAIWeaponNonPhysical(ObjID owner, ObjID weapon);

////////////////////////////////////////////////////////////////////////////////

// Weapon Offset Link.
// Intention:  Provide a property for attaching weapons to creatures.
//             This should extend the player weapon code introduced in Shock to all creatures.

//             (Though this could be in the weapprop.h, it had some macro was expanding in a wacky way.
//              It still kinda makes sense to be here too).

// The actual structure
struct sWeaponOffset
{
   int        m_mode;            // When to use the weapon offset.
   mxs_vector m_transforms[4];   // Transform matrix.
   int        m_joint;           // Source joint.
};

#define RELATION_WEAPONOFFSET "WeaponOffset"

EXTERN void        InitWeaponOffsetRelation(void);
EXTERN void        TermWeaponOffsetRelation(void);
// WeaponOffset accessors
EXTERN BOOL        WeaponOffsetGet(ObjID objID, int mode, sWeaponOffset **ppWeaponOffsetResult);
EXTERN ObjID       WeaponOffsetGetDest        (ObjID objID);
EXTERN int         WeaponOffsetGetMode        (ObjID objID);
EXTERN mxs_vector* WeaponOffsetGetTransform   (ObjID objID);
EXTERN int         WeaponOffsetGetJoint       (ObjID objID);

// WeaponOffsetSetActiveOffset: Sets the active WeaponOffset for a given mode.
// TRUE:  An appropriate weapon offset exists for this mode.
EXTERN BOOL        WeaponOffsetSetActive(ObjID objID, int mode);

EXTERN BOOL        ObjHasWeaponOffset(ObjID objID);

#define WEAPONOFFSET_SET(objID, ppParams) \
   PROPERTY_SET(g_WeaponOffsetProperty, objID, (sWeaponOffset**)ppParams)
#define WEAPONOFFSET_DELETE(objID) \
   IProperty_Delete(g_WeaponOffsetProperty, objID)

#endif

