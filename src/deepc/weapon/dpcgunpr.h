/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover gun related properties
#pragma once

#ifndef __DPCGUNPR_H
#define __DPCGUNPR_H

#ifndef PROPFACE_H
#include <propface.h>
#endif // !PROPFACE_H

#ifndef __GUNBASE_H
#include <gunbase.h>
#endif // !__GUNBASE_H

#ifndef __DPCGUNAN_H
#include <dpcgunan.h>
#endif // !__DPCGUNAN_H

/////////////////////////////////////
// Gun description property
// This is a property meant to be attached to an archetype - used to figure out what
// projectile etc the gun uses

F_DECLARE_INTERFACE(IBaseGunDescProperty);

// Structure for mode specific descriptions
class cBaseGunDescs
{
public:
   cBaseGunDescs();

   sBaseGunDesc m_desc[kNumGunSettings];
};

#undef INTERFACE
#define INTERFACE IBaseGunDescProperty

DECLARE_PROPERTY_INTERFACE(IBaseGunDescProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(cBaseGunDescs*);  // Type-specific accessors, by reference
};

#define PROP_BASE_GUN_DESC "BaseGunDesc"

EXTERN IBaseGunDescProperty *g_baseGunDescProperty;

EXTERN void BaseGunDescPropertyInit();
EXTERN BOOL BaseGunDescGet(ObjID objID, sBaseGunDesc **ppBaseGunDescs);
EXTERN void BaseGunDescGetSafe(ObjID objID, sBaseGunDesc **ppBaseGunDescs);
EXTERN int BaseGunDescGetClip(ObjID objID);
EXTERN int BaseGunDescGetBurst(ObjID objID);
#define BASE_GUN_DESC_SET(objID, ppParams) \
   PROPERTY_SET(g_baseGunDescProperty, objID, (sBaseGunDescs*)ppParams)
#define BASE_GUN_DESC_DELETE(objID) \
   IProperty_Delete(g_baseGunDescProperty, objID)

/////////////////////////////////////
// AI gun description

F_DECLARE_INTERFACE(IAIGunDescProperty);

#undef INTERFACE
#define INTERFACE IAIGunDescProperty

DECLARE_PROPERTY_INTERFACE(IAIGunDescProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sAIGunDesc*);  // Type-specific accessors, by reference
};

#define PROP_AI_GUN_DESC "AIGunDesc"

EXTERN IAIGunDescProperty *g_aiGunDescProperty;

EXTERN void AIGunDescPropertyInit();
EXTERN BOOL AIGunDescGet(ObjID objID, sAIGunDesc **ppAIGunDesc);
EXTERN float AIGunDescGetMaxRange(ObjID objID);
EXTERN BOOL AIGunDescGetFireOffset(ObjID objID, mxs_vector **ppOffset);

/////////////////////////////////////
// Player gun desc
//

F_DECLARE_INTERFACE(IPlayerGunDescProperty);

#undef INTERFACE
#define INTERFACE IPlayerGunDescProperty

DECLARE_PROPERTY_INTERFACE(IPlayerGunDescProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sPlayerGunDesc*);  // Type-specific accessors, by reference
};

#define PROP_PLAYER_GUN_DESC "PlayerGunDesc"

EXTERN IPlayerGunDescProperty *g_playerGunDescProperty;

EXTERN void PlayerGunDescPropertyInit();
EXTERN BOOL IsPlayerGun(ObjID objID);
EXTERN BOOL PlayerGunDescGet(ObjID objID, sPlayerGunDesc **ppPlayerGunDesc);
EXTERN BOOL PlayerGunDescGetHandModel(ObjID objID, Label **ppLabel);
EXTERN BOOL PlayerGunDescGetIconName(ObjID objID, Label **ppLabel);
EXTERN BOOL PlayerGunDescGetModelOffset(ObjID objID, mxs_vector **ppOffset);
EXTERN BOOL PlayerGunDescGetFireOffset(ObjID objID, mxs_vector **ppOffset);
EXTERN mxs_ang PlayerGunDescGetModelHeading(ObjID objID);

/////////////////////////////////////
// Player gun kick
//

F_DECLARE_INTERFACE(IGunKickProperty);

// settings specific info
// we can add a constructor if we want defaults
class cGunKicks
{
public:
   cGunKicks(void);

   sGunKick m_kick[kNumGunSettings];
};

#undef INTERFACE
#define INTERFACE IGunKickProperty

DECLARE_PROPERTY_INTERFACE(IGunKickProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(cGunKicks*);  // Type-specific accessors, by reference
};

#define PROP_PLAYER_GUN_KICK "GunKick"

EXTERN IGunKickProperty *g_pGunKickProperty;
void GunKickPropertyInit(void);
EXTERN BOOL GunKickGet(ObjID objID, sGunKick **ppGunKick);
EXTERN void GunKickGetSafe(ObjID objID, sGunKick **ppGunKick);

/////////////////////////////////////
// Gun reliability
//

F_DECLARE_INTERFACE(IGunReliabilityDescProperty);

#undef INTERFACE
#define INTERFACE IGunReliabilityProperty

DECLARE_PROPERTY_INTERFACE(IGunReliabilityProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sGunReliability*);  // Type-specific accessors, by reference
};

#define PROP_GUN_RELIABILITY "GunReliability"

EXTERN IGunReliabilityProperty *g_pGunReliabilityProperty;

EXTERN void GunReliabilityPropertyInit(void);

/////////////////////////////////////
// Gun state property
// This is a property meant to be attached to an instance of a gun

F_DECLARE_INTERFACE(IGunStateProperty);

#undef INTERFACE
#define INTERFACE IGunStateProperty

DECLARE_PROPERTY_INTERFACE(IGunStateProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sGunState*);  // Type-specific accessors, by reference
};

#define PROP_GUN_STATE "GunState"

EXTERN IGunStateProperty *g_pGunStateProperty;

EXTERN void GunStatePropertyInit();
EXTERN BOOL ObjHasGunState(ObjID objID);
EXTERN BOOL GunStateGet(ObjID objID, sGunState **ppGunState);
EXTERN int GunStateGetAmmo(ObjID objID);
EXTERN void GunStateSetAmmo(ObjID objID, int ammo);
EXTERN int GunStateGetSetting(ObjID objID);

/////////////////////////////////////
// AI Gun Name Property
// this is the property that goes on an AI to tell it which gun it should use

#define PROP_AI_GUN_NAME "AIGun"
EXTERN ILabelProperty* gPropAIGunName;
EXTERN void AIGunNamePropInit();
#define ObjHasAIGunName(obj) IProperty_IsRelevant(gPropAIGunName,obj)
EXTERN BOOL ObjGetAIGunName(ObjID obj, Label **ppName);
EXTERN void ObjSetAIGunName(ObjID obj, Label *pName);

/////////////////////////////////////
// Cannister Property
// this property is attached to projectiles or particles to tell them how many 
// objects they explode into

F_DECLARE_INTERFACE(ICannisterProperty);

#undef INTERFACE
#define INTERFACE ICannisterProperty

DECLARE_PROPERTY_INTERFACE(ICannisterProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sCannister*);  // Type-specific accessors, by reference
};

#define PROP_CANNISTER "Cannister"

EXTERN ICannisterProperty *g_CannisterProperty;

EXTERN void CannisterPropertyInit();
EXTERN BOOL ObjHasCannister(ObjID objID);
EXTERN BOOL CannisterGet(ObjID objID, sCannister **ppCannister);
EXTERN int CannisterGetNum(ObjID objID);
EXTERN float CannisterGetSpeed(ObjID objID);
#define CANNISTER_SET(objID, pCannister) \
   PROPERTY_SET(g_CannisterProperty, objID, pCannister)

#define PROP_DPC_WEAPON_TYPE_NAME "DPCWeaponType"
EXTERN IIntProperty* g_pWeaponTypeProperty;
EXTERN void WeaponTypePropertyInit();

#endif  // __DPCGUNPR_H
