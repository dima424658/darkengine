/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/dmgprop.h,v 1.4 2000/01/29 13:19:29 adurant Exp $
#pragma once

#ifndef __DMGPROP_H
#define __DMGPROP_H
#include <objtype.h>

enum eSlayResult
{
   kSlayNormal,     // Do the default thing
   kSlayNoEffect,   // Do the "Walking dead"
   kSlayTerminate,  // Corpsify me
   kSlayDestroy,    // Destroy me 

   kNumSlayResults, 

   kSlayResultPad = 0xFFFFFFFF, 
};

typedef enum eSlayResult eSlayResult; 

#define PROP_WEAPON_DAMAGE_NAME "WeaponDamage"
#define PROP_WEAPON_TYPE_NAME "WeaponType"
#define PROP_SLAY_RESULT_NAME "SlayResult" 

EXTERN void ObjSetWeaponDamage(ObjID obj, int damage);
EXTERN int ObjGetWeaponDamage(ObjID obj); // default == 0
EXTERN BOOL ObjHasWeaponDamage(ObjID obj);
EXTERN void ObjUnsetWeaponDamage(ObjID obj);

EXTERN BOOL ObjGetWeaponType(ObjID obj, int* type);
EXTERN void ObjSetWeaponType(ObjID obj, int type);
EXTERN void ObjUnsetWeaponType(ObjID obj);

EXTERN eSlayResult ObjGetSlayResult(ObjID obj); 
EXTERN void ObjSetSlayResult(ObjID obj, eSlayResult result); 


EXTERN void InitDamageProps(void);
EXTERN void TermDamageProps(void); 


#endif // __DMGPROP_H
