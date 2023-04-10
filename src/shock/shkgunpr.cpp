/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkgunpr.cpp,v 1.42 1999/11/23 13:24:46 adurant Exp $


This file has been moved to gunprop.cpp AMSD


#include <shkgunpr.h>

#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <shkgnapi.h>
#include <shkprutl.h>
 
// must be last header
#include <dbmem.h>

// Base gun description
// the all new improved property system...

typedef cSpecificProperty<IBaseGunDescProperty, &IID_IBaseGunDescProperty, cBaseGunDescs*, cHashPropertyStore<cNoZeroDataOps <cBaseGunDescs> > > cBaseGunDescPropertyBase;

// property implementation class
class cBaseGunDescProperty: public cBaseGunDescPropertyBase
{
public:
   cBaseGunDescProperty(const sPropertyDesc* desc)
      : cBaseGunDescPropertyBase(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(cBaseGunDescs); 
};

IBaseGunDescProperty *g_baseGunDescProperty;

sBaseGunDesc g_defaultBaseGunDesc = {1, 10, 1, 1, 100, 200, 1, 1, 0};

cBaseGunDescs::cBaseGunDescs()
{
   for (int i=0; i<kNumGunSettings; i++)
      m_desc[i] = g_defaultBaseGunDesc;
}

static char *apBaseGunDescFlagNames[] = {"Continuous Fire",};

#define BASE_GUN_FIELD(i) \
   {"Setting " #i ": Burst", kFieldTypeInt, FieldLocation(cBaseGunDescs, m_desc[i].m_burst),}, \
   {"Setting " #i ": Clip", kFieldTypeInt, FieldLocation(cBaseGunDescs, m_desc[i].m_clip),}, \
   {"Setting " #i ": Spray", kFieldTypeInt, FieldLocation(cBaseGunDescs, m_desc[i].m_spray),}, \
   {"Setting " #i ": Stim Mult", kFieldTypeFloat, FieldLocation(cBaseGunDescs, m_desc[i].m_stimModifier),}, \
   {"Setting " #i ": Burst Interval", kFieldTypeInt, FieldLocation(cBaseGunDescs, m_desc[i].m_burstInterval),}, \
   {"Setting " #i ": Shot Interval", kFieldTypeInt, FieldLocation(cBaseGunDescs, m_desc[i].m_shotInterval),}, \
   {"Setting " #i ": Ammo Usage", kFieldTypeInt, FieldLocation(cBaseGunDescs, m_desc[i].m_ammoUsage),}, \
   {"Setting " #i ": Speed Mult", kFieldTypeFloat, FieldLocation(cBaseGunDescs, m_desc[i].m_speedModifier),}, \
   {"Setting " #i ": Reload Time", kFieldTypeInt, FieldLocation(cBaseGunDescs, m_desc[i].m_reloadTime),}

// Must keep this is synch with kNumGunSettings
static sFieldDesc BaseGunDescFields[] = 
{
   BASE_GUN_FIELD(0), 
   BASE_GUN_FIELD(1),
   BASE_GUN_FIELD(2),
};

static sStructDesc BaseGunDescStructDesc = 
   StructDescBuild(cBaseGunDescs, kStructFlagNone, BaseGunDescFields);

IBaseGunDescProperty *CreateBaseGunDescProperty(sPropertyDesc *desc, 
                                                          ePropertyImpl impl)
{
   StructDescRegister(&BaseGunDescStructDesc);
   return new cBaseGunDescProperty(desc);
}

static sPropertyDesc BaseGunDescDesc = 
{
   PROP_BASE_GUN_DESC, 
   0,
   NULL, 
   1,    // version
   0,
   {"Gun", "Base Gun Description"},
};

void BaseGunDescPropertyInit()
{
   g_baseGunDescProperty = CreateBaseGunDescProperty(&BaseGunDescDesc, kPropertyImplDense);
}

BOOL BaseGunDescGet(ObjID objID, sBaseGunDesc **ppBaseGunDesc)
{
   cBaseGunDescs *pBaseGunDescs;

   if (g_baseGunDescProperty->Get(objID, &pBaseGunDescs))
   {
      *ppBaseGunDesc = &(pBaseGunDescs->m_desc[GunStateGetSetting(objID)]);
      return TRUE;
   }
   return FALSE;
}

void BaseGunDescGetSafe(ObjID objID, sBaseGunDesc **ppBaseGunDesc)
{
   cBaseGunDescs *pBaseGunDescs;

   if (g_baseGunDescProperty->Get(objID, &pBaseGunDescs))
      *ppBaseGunDesc = &(pBaseGunDescs->m_desc[GunStateGetSetting(objID)]);
   else
      *ppBaseGunDesc = &g_defaultBaseGunDesc;
}

int BaseGunDescGetBurst(ObjID objID)
{
   cBaseGunDescs *pBaseGunDesc;

   if (g_baseGunDescProperty->Get(objID, &pBaseGunDesc))
      return pBaseGunDesc->m_desc[GunStateGetSetting(objID)].m_burst;
   return 1;
}

int BaseGunDescGetClip(ObjID objID)
{
   cBaseGunDescs *pBaseGunDesc;

   if (g_baseGunDescProperty->Get(objID, &pBaseGunDesc))
      return pBaseGunDesc->m_desc[GunStateGetSetting(objID)].m_clip;
   return 1;
}

/////////////////////////////////////////////////////////////
// AI gun desc

// data ops
class cAIGunDescDataOps: public cClassDataOps<sAIGunDesc>
{
};

// storage class
class cAIGunDescStore: public cHashPropertyStore<cAIGunDescDataOps>
{
};

// property implementation class
class cAIGunDescProperty: public cSpecificProperty<IAIGunDescProperty, &IID_IAIGunDescProperty, sAIGunDesc*, cAIGunDescStore>
{
   typedef cSpecificProperty<IAIGunDescProperty, &IID_IAIGunDescProperty, sAIGunDesc*, cAIGunDescStore> cParent; 

public:
   cAIGunDescProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sAIGunDesc); 

};

IAIGunDescProperty *g_aiGunDescProperty;

static sFieldDesc AIGunDescFields[] = 
{
   {"AI Max Range (Not used)", kFieldTypeFloat, FieldLocation(sAIGunDesc, m_maxRange),},
   {"Fire Offset", kFieldTypeVector, FieldLocation(sAIGunDesc, m_fireOffset),},
   {"Start Lag", kFieldTypeInt, FieldLocation(sAIGunDesc, m_startTime),},
   {"Burst Lag", kFieldTypeInt, FieldLocation(sAIGunDesc, m_burstTime),},
   {"End Lag", kFieldTypeInt, FieldLocation(sAIGunDesc, m_endTime),},
   {"Aim Error", kFieldTypeInt, FieldLocation(sAIGunDesc, m_aimError), kFieldFlagHex},
};

static sStructDesc AIGunDescStructDesc = 
   StructDescBuild(sAIGunDesc, kStructFlagNone, AIGunDescFields);

IAIGunDescProperty *CreateAIGunDescProperty(sPropertyDesc *desc, 
                                                          ePropertyImpl impl)
{
   StructDescRegister(&AIGunDescStructDesc);
   return new cAIGunDescProperty(desc);
}

static sPropertyDesc AIGunDescDesc = 
{
   PROP_AI_GUN_DESC, 
   0,
   NULL, 
   1,    // version
   0,
   {"Gun", "AI Gun Description"},
};

void AIGunDescPropertyInit()
{
   g_aiGunDescProperty = CreateAIGunDescProperty(&AIGunDescDesc, kPropertyImplDense);
}

BOOL AIGunDescGet(ObjID objID, sAIGunDesc **ppAIGunDesc)
{
   return g_aiGunDescProperty->Get(objID, ppAIGunDesc);
}

float AIGunDescGetMaxRange(ObjID objID)
{
   sAIGunDesc *pAIGunDesc;

   if (g_aiGunDescProperty->Get(objID, &pAIGunDesc))
      return pAIGunDesc->m_maxRange;
   return kShockGunDefaultMaxRange;
}

BOOL AIGunDescGetFireOffset(ObjID objID, mxs_vector **ppOffset)
{
   sAIGunDesc *pAIGunDesc;

   if (g_aiGunDescProperty->Get(objID, &pAIGunDesc))
   {
      *ppOffset = &(pAIGunDesc->m_fireOffset);
      return TRUE;
   }
   return FALSE;
}

//////////////////////////////////////////////////
// Player gun desc
//

typedef cSpecificProperty<IPlayerGunDescProperty, &IID_IPlayerGunDescProperty, sPlayerGunDesc*, cHashPropertyStore<cClassDataOps<sPlayerGunDesc> > > cPlayerGunDescPropertyBase;

// property implementation class
class cPlayerGunDescProperty: public cPlayerGunDescPropertyBase
{
public:
   cPlayerGunDescProperty(const sPropertyDesc* desc)
      : cPlayerGunDescPropertyBase(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sPlayerGunDesc); 

};

IPlayerGunDescProperty *g_playerGunDescProperty;

static char *flagBits[] = 
{
   "KUp",
   "KDown", 
   "KLeft",
   "KRight", 
   "JUp",
   "JDown", 
   "JLeft",
   "JRight", 
   "Delayed Burst", 
};

static char *handednessTypes[] = 
{ 
   "OneHanded",
   "TwoHanded",
};
 
static sFieldDesc PlayerGunDescFields[] = 
{
   {"Flags", kFieldTypeBits, FieldLocation(sPlayerGunDesc, m_flags), kFieldFlagNone, 0, 9, 9, flagBits,},
   {"Icon Name", kFieldTypeString, FieldLocation(sPlayerGunDesc, m_iconName),},
   {"Hand Model", kFieldTypeString, FieldLocation(sPlayerGunDesc, m_handModel),},
   {"Model Offset", kFieldTypeVector, FieldLocation(sPlayerGunDesc, m_posOffset),},
   {"Fire Offset", kFieldTypeVector, FieldLocation(sPlayerGunDesc, m_fireOffset),},
   {"Model Heading", kFieldTypeInt, FieldLocation(sPlayerGunDesc, m_headingOffset), kFieldFlagHex},
   {"Reload Pitch", kFieldTypeInt, FieldLocation(sPlayerGunDesc, m_reloadPitch), kFieldFlagHex},
   {"Reload Rate", kFieldTypeInt, FieldLocation(sPlayerGunDesc, m_reloadRate), kFieldFlagHex},
   { "", kFieldTypeEnum, FieldLocation(sPlayerGunDesc, m_handedness), kFieldFlagUnsigned, 0, 2, 2, handednessTypes},
};

static sStructDesc PlayerGunDescStructDesc = 
   StructDescBuild(sPlayerGunDesc, kStructFlagNone, PlayerGunDescFields);

IPlayerGunDescProperty *CreatePlayerGunDescProperty(sPropertyDesc *desc, 
                                                          ePropertyImpl impl)
{
   StructDescRegister(&PlayerGunDescStructDesc);
   return new cPlayerGunDescProperty(desc);
}

static sPropertyConstraint PlayerGunDescConstraints[] = 
{
   {
      kPropertyAutoCreate, 
      PROP_GUN_STATE,
   },
   NULL,
};

static sPropertyDesc PlayerGunDescDesc = 
{
   PROP_PLAYER_GUN_DESC, 
   0,
   PlayerGunDescConstraints, 
   1,    // version
   0,
   {"Gun", "Player Gun Description"},
};

void PlayerGunDescPropertyInit()
{
   g_playerGunDescProperty = CreatePlayerGunDescProperty(&PlayerGunDescDesc, kPropertyImplDense);
}

BOOL IsPlayerGun(ObjID objID)
{
   sPlayerGunDesc *pPlayerGunDesc;

   return g_playerGunDescProperty->Get(objID, &pPlayerGunDesc);
}

BOOL PlayerGunDescGet(ObjID objID, sPlayerGunDesc **ppPlayerGunDesc)
{
   return g_playerGunDescProperty->Get(objID, ppPlayerGunDesc);
}

BOOL PlayerGunDescGetHandModel(ObjID objID, Label **ppLabel)
{
   sPlayerGunDesc *pPlayerGunDesc;

   if (g_playerGunDescProperty->Get(objID, &pPlayerGunDesc))
   {
      *ppLabel = &(pPlayerGunDesc->m_handModel);
      return TRUE;
   }
   return FALSE;
}

BOOL PlayerGunDescGetIconName(ObjID objID, Label **ppLabel)
{
   sPlayerGunDesc *pPlayerGunDesc;

   if (g_playerGunDescProperty->Get(objID, &pPlayerGunDesc))
   {
      *ppLabel = &(pPlayerGunDesc->m_iconName);
      return TRUE;
   }
   return FALSE;
}

BOOL PlayerGunDescGetModelOffset(ObjID objID, mxs_vector **ppOffset)
{
   sPlayerGunDesc *pPlayerGunDesc;

   if (g_playerGunDescProperty->Get(objID, &pPlayerGunDesc))
   {
      *ppOffset = &(pPlayerGunDesc->m_posOffset);
      return TRUE;
   }
   return FALSE;
}

BOOL PlayerGunDescGetFireOffset(ObjID objID, mxs_vector **ppOffset)
{
   sPlayerGunDesc *pPlayerGunDesc;

   if (g_playerGunDescProperty->Get(objID, &pPlayerGunDesc))
   {
      *ppOffset = &(pPlayerGunDesc->m_fireOffset);
      return TRUE;
   }
   return FALSE;
}

mxs_ang PlayerGunDescGetModelHeading(ObjID objID)
{
   sPlayerGunDesc *pPlayerGunDesc;

   if (g_playerGunDescProperty->Get(objID, &pPlayerGunDesc))
      return pPlayerGunDesc->m_headingOffset;
   return 0;
}

//////////////////////////////////////////////////
// Player gun kickback
//

typedef cSpecificProperty<IGunKickProperty, &IID_IGunKickProperty, cGunKicks*, cHashPropertyStore<cNoZeroDataOps<cGunKicks> > > cGunKickPropertyBase; 

   // property implementation class
class cGunKickProperty: public cGunKickPropertyBase
{

public:
   cGunKickProperty(const sPropertyDesc* desc)
      : cGunKickPropertyBase(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(cGunKicks); 

};

// some reasonable values for a kick
sGunKick g_defaultGunKick = {1000, 1000, 1000, 0, -0.2, -0.2, 1, 1.0, 200, 200, 1};

cGunKicks::cGunKicks(void)
{
   for (int i=0; i<kNumGunSettings; i++)
      m_kick[i] = g_defaultGunKick;
}

IGunKickProperty *g_pGunKickProperty;

#define PLAYER_GUN_KICK_FIELD(i) \
   {"Setting " #i ": Kickback Pitch", kFieldTypeInt, FieldLocation(cGunKicks, m_kick[i].m_kickPitch)}, \
   {"Setting " #i ": Kickback Pitch Max", kFieldTypeInt, FieldLocation(cGunKicks, m_kick[i].m_kickPitchMax)}, \
   {"Setting " #i ": Kick Angle Return", kFieldTypeInt, FieldLocation(cGunKicks, m_kick[i].m_kickAngularReturnRate)}, \
   {"Setting " #i ": Kickback Heading", kFieldTypeInt, FieldLocation(cGunKicks, m_kick[i].m_kickHeading)}, \
   {"Setting " #i ": Kickback", kFieldTypeFloat, FieldLocation(cGunKicks, m_kick[i].m_kickBack),}, \
   {"Setting " #i ": Kickback Max", kFieldTypeFloat, FieldLocation(cGunKicks, m_kick[i].m_kickBackMax),}, \
   {"Setting " #i ": Kickback Return", kFieldTypeFloat, FieldLocation(cGunKicks, m_kick[i].m_kickBackReturnRate),}, \
   {"Setting " #i ": Pre-Shot Pct", kFieldTypeFloat, FieldLocation(cGunKicks, m_kick[i].m_preKickPct),}, \
   {"Setting " #i ": Jolt Pitch", kFieldTypeInt, FieldLocation(cGunKicks, m_kick[i].m_joltPitch)}, \
   {"Setting " #i ": Jolt Heading", kFieldTypeInt, FieldLocation(cGunKicks, m_kick[i].m_joltHeading)}, \
   {"Setting " #i ": Jolt Back", kFieldTypeFloat, FieldLocation(cGunKicks, m_kick[i].m_joltBack),}

static sFieldDesc GunKickFields[] = 
{
   PLAYER_GUN_KICK_FIELD(0),
   PLAYER_GUN_KICK_FIELD(1),
   PLAYER_GUN_KICK_FIELD(2),
};

static sStructDesc GunKickStructDesc = 
   StructDescBuild(cGunKicks, kStructFlagNone, GunKickFields);

IGunKickProperty *CreateGunKickProperty(sPropertyDesc *desc, ePropertyImpl impl)
{
   StructDescRegister(&GunKickStructDesc);
   return new cGunKickProperty(desc);
}

static sPropertyDesc GunKickDesc = 
{
   PROP_PLAYER_GUN_KICK, 
   0,
   NULL, 
   1,    // version
   0,
   {"Gun", "Kickback"},
};

void GunKickPropertyInit(void)
{
   g_pGunKickProperty = CreateGunKickProperty(&GunKickDesc, kPropertyImplDense);
}

BOOL GunKickGet(ObjID objID, sGunKick **ppGunKick)
{
   cGunKicks *pGunKicks;

   if (g_pGunKickProperty->Get(objID, &pGunKicks))
   {
      *ppGunKick = &(pGunKicks->m_kick[GunGetSetting(objID)]);
      return TRUE;
   }
   return FALSE;
}

void GunKickGetSafe(ObjID objID, sGunKick **ppGunKick)
{
   cGunKicks *pGunKicks;

   if (g_pGunKickProperty->Get(objID, &pGunKicks))
      *ppGunKick = &(pGunKicks->m_kick[GunGetSetting(objID)]);
   else 
      *ppGunKick = &g_defaultGunKick;
}

//////////////////////////////////////////////////
// Gun reliability desc
//

// data ops
class cGunReliabilityDataOps: public cClassDataOps<sGunReliability>
{
};

// storage class
class cGunReliabilityStore: public cHashPropertyStore<cGunReliabilityDataOps>
{
};

// property implementation class
class cGunReliabilityProperty: public cSpecificProperty<IGunReliabilityProperty, &IID_IGunReliabilityProperty, sGunReliability*, cGunReliabilityStore>
{
   typedef cSpecificProperty<IGunReliabilityProperty, &IID_IGunReliabilityProperty, sGunReliability*, cGunReliabilityStore> cParent; 

public:
   cGunReliabilityProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sGunReliability); 

};

IGunReliabilityProperty *g_pGunReliabilityProperty;


//below changed to use degraderate[0] AMSD 11/22/99

static sFieldDesc GunReliabilityFields[] = 
{
   {"Min Breakage (%)", kFieldTypeFloat, FieldLocation(sGunReliability, m_minBreak), kFieldFlagNone,},
   {"Max Breakage (%)", kFieldTypeFloat, FieldLocation(sGunReliability, m_maxBreak), kFieldFlagNone,},
#ifndef SHOCK
   {"Degrade Rate (%)", kFieldTypeFloat, FieldLocation(sGunReliability, m_degradeRate[0]), kFieldFlagNone,},
#endif
#ifdef SHOCK
   {"Degrade Rate (%)", kFieldTypeFloat, FieldLocation(sGunReliability, m_degradeRate, kFieldFlagNone,},
#endif
   {"Break Threshold (%)", kFieldTypeFloat, FieldLocation(sGunReliability, m_threshBreak), kFieldFlagNone,},
};

static sStructDesc GunReliabilityStructDesc = 
   StructDescBuild(sGunReliability, kStructFlagNone, GunReliabilityFields);

static sPropertyDesc GunReliabilityDesc = 
{
   PROP_GUN_RELIABILITY, 
   0, 
   NULL,
   1,    // version
   0,
   {"Gun", "Reliability"},
};

void GunReliabilityPropertyInit(void)
{
   StructDescRegister(&GunReliabilityStructDesc);
   g_pGunReliabilityProperty = new cGunReliabilityProperty(&GunReliabilityDesc);
}

////////////////////////////////////////////////
// Gun State property
// the all new improved property system...

// data ops
class cGunStateDataOps: public cClassDataOps<sGunState>
{
};

// storage class
class cGunStateStore: public cHashPropertyStore<cGunStateDataOps>
{
};

// property implementation class
class cGunStateProperty: public cSpecificProperty<IGunStateProperty, &IID_IGunStateProperty, sGunState*, cGunStateStore>
{
   typedef cSpecificProperty<IGunStateProperty, &IID_IGunStateProperty, sGunState*, cGunStateStore> cParent; 

public:
   cGunStateProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sGunState); 

};

static sFieldDesc GunStateFields[] = 
{
   {"Ammo", kFieldTypeInt, FieldLocation(sGunState, m_ammoCount),},
   {"Condition (%)", kFieldTypeFloat, FieldLocation(sGunState, m_condition),},
   {"Setting", kFieldTypeInt, FieldLocation(sGunState, m_setting),},
   {"Modification", kFieldTypeInt, FieldLocation(sGunState, m_modification),},
};

IGunStateProperty *g_pGunStateProperty;

static sStructDesc GunStateStructDesc = 
   StructDescBuild(sGunState, kStructFlagNone, GunStateFields);

IGunStateProperty *CreateGunStateProperty(sPropertyDesc *desc, ePropertyImpl impl)
{
   StructDescRegister(&GunStateStructDesc);
   return new cGunStateProperty(desc);
}

static sPropertyDesc GunStateDesc = 
{
   PROP_GUN_STATE, 
   kPropertyInstantiate,
   NULL, 
   1,    // version
   0,
   {"Gun", "Gun State"},
};

void GunStatePropertyInit()
{
   g_pGunStateProperty = CreateGunStateProperty(&GunStateDesc, kPropertyImplDense);
}

int GunStateGetAmmo(ObjID objID)
{
   sGunState *pGunState;

   if (g_pGunStateProperty->Get(objID, &pGunState))
      return pGunState->m_ammoCount;
   return 0;
}

void GunStateSetAmmo(ObjID objID, int ammo)
{
   sGunState *pGunState;

   if (g_pGunStateProperty->Get(objID, &pGunState))
   {
      pGunState->m_ammoCount = ammo;
      g_pGunStateProperty->Set(objID, pGunState);
   }
   else
      Warning(("GunStateSetAmmo: Gun %d has no gun state\n"));
}

BOOL ObjHasGunState(ObjID objID)
{
   return g_pGunStateProperty->IsRelevant(objID);
}

BOOL GunStateGet(ObjID objID, sGunState **ppGunState)
{
   return g_pGunStateProperty->Get(objID, ppGunState);
}

int GunStateGetSetting(ObjID objID)
{
   sGunState *pGunState;

   if (g_pGunStateProperty->Get(objID, &pGunState))
      return pGunState->m_setting;
   return 0;
}

/////////////////////////////////////////////////////////////
// AI Gun Name Property

ILabelProperty* gPropAIGunName;

static sPropertyDesc AIGunNameDesc =
{
   PROP_AI_GUN_NAME, 0, 
};

#define AI_GUN_NAME_IMPL kPropertyImplDense

void AIGunNamePropInit()
{
   gPropAIGunName = CreateLabelProperty(&AIGunNameDesc,AI_GUN_NAME_IMPL);
}

BOOL ObjGetAIGunName(ObjID obj, Label **ppName)
{
   Assert_(gPropAIGunName);
   return gPropAIGunName->Get(obj, ppName);
}

void ObjSetAIGunName(ObjID obj, Label *pName)
{
   Assert_(gPropAIGunName);
   gPropAIGunName->Set(obj, pName);
}

/////////////////////////////////////////////////////////////
// Cannister property

// the all new improved property system...
// data ops
class cCannisterDataOps: public cClassDataOps<sCannister>
{
};

// storage class
class cCannisterStore: public cHashPropertyStore<cCannisterDataOps>
{
};

// property implementation class
class cCannisterProperty: public cSpecificProperty<ICannisterProperty, &IID_ICannisterProperty, sCannister*, cCannisterStore>
{
   typedef cSpecificProperty<ICannisterProperty, &IID_ICannisterProperty, sCannister*, cCannisterStore> cParent; 

public:
   cCannisterProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sCannister); 

};

ICannisterProperty *g_CannisterProperty;

static sFieldDesc CannisterFields[] = 
{
   {"Number", kFieldTypeInt, FieldLocation(sCannister, m_num),},
   {"Speed", kFieldTypeFloat, FieldLocation(sCannister, m_speed),},
   {"Generations", kFieldTypeInt, FieldLocation(sCannister, m_generations),},
};

static sStructDesc CannisterStructDesc = 
   StructDescBuild(sCannister, kStructFlagNone, CannisterFields);

ICannisterProperty *CreateCannisterProperty(sPropertyDesc *desc, 
                                                          ePropertyImpl impl)
{
   StructDescRegister(&CannisterStructDesc);
   return new cCannisterProperty(desc);
}

static sPropertyDesc CannisterDesc = 
{
   PROP_CANNISTER, 0,
   NULL, 0, 0,
   {"Gun", "Cannister"},
};

void CannisterPropertyInit()
{
   g_CannisterProperty = CreateCannisterProperty(&CannisterDesc, kPropertyImplDense);
}

BOOL ObjHasCannister(ObjID objID)
{
   sCannister *pCannister;
   return g_CannisterProperty->Get(objID, &pCannister);
}

BOOL CannisterGet(ObjID objID, sCannister **ppCannister)
{
   return g_CannisterProperty->Get(objID, ppCannister);
}

int CannisterGetNum(ObjID objID)
{
   sCannister *pCannister;

   if (g_CannisterProperty->Get(objID, &pCannister))
      return pCannister->m_num;
   return 0;
}

float CannisterGetProjName(ObjID objID)
{
   sCannister *pCannister;

   if (g_CannisterProperty->Get(objID, &pCannister))
   {
      return pCannister->m_speed;
   }
   return kCannisterDefaultSpeed;
}

/////////////////////////////////////////////////////////////
// Weapon Type Property

IIntProperty* g_pWeaponTypeProperty;

static sPropertyDesc weaponTypePropDesc =
{
   PROP_SHOCK_WEAPON_TYPE_NAME,
   0, NULL, 0, 0,
   { "Gun", "Weapon Type" }
};

// It sucks that this needs to manually stay in synch with the weapon skills
// in shkplcst.h, but I think I'm going to leave it like this for the moment...
static char *weaponTypeBits[] = 
{ 
   "Conventional", 
   "Energy", 
   "Heavy",
   "Annelid",
   "Psi Amp",
};
 
static sFieldDesc weaponTypeFieldDesc[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 5, 5, weaponTypeBits},
}; 

static sStructDesc weaponTypeSDesc = 
{
   PROP_SHOCK_WEAPON_TYPE_NAME, 
   sizeof(int),
   kStructFlagNone,
   sizeof(weaponTypeFieldDesc)/sizeof(weaponTypeFieldDesc[0]),
   weaponTypeFieldDesc,
}; 

static sPropertyTypeDesc weaponTypeTypeDesc = {PROP_SHOCK_WEAPON_TYPE_NAME, sizeof(int)}; 

// Init the property
void WeaponTypePropertyInit(void)
{
   StructDescRegister(&weaponTypeSDesc);
   g_pWeaponTypeProperty = CreateIntegralProperty(&weaponTypePropDesc, &weaponTypeTypeDesc, kPropertyImplDense);
}
