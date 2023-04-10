/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapprop.cpp,v 1.13 1999/12/14 19:20:31 porges Exp $
//
// core weapon (hand-to-hand-combat) routines
//

#include <lg.h>

#include <appagg.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>

#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>

#include <iobjsys.h>
#include <bintrait.h>

#include <weapprop.h>

#ifdef DBG_ON
#include <mprintf.h>
#include <objedit.h>
#include <config.h>
#endif

// Must be last header 
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

#define WEAPON_RELATION_QCASES (kQCaseSetBothKnown)
static sRelationDesc     weaponRelDesc  = { "CurWeapon", kRelationNetworkLocalOnly };
static sRelationDataDesc weaponDataDesc = LINK_DATA_DESC(int);

IRelation *g_pWeaponRel = NULL;

#ifdef DBG_ON
#define LISTENTO (kListenLinkModify|kListenLinkBirth|kListenLinkDeath)

void LGAPI WeaponPainListener(sRelationListenMsg *msg, RelationListenerData data)
{
   mprintf("val %x f %s t %s type %d id %x\n",*((int *)g_pWeaponRel->GetData(msg->id)),
           ObjWarnName(msg->link.source),ObjWarnName(msg->link.dest),msg->type,msg->id);
}
#endif

void InitWeaponRelation()
{  // Init weapon ownership relation
   g_pWeaponRel = CreateStandardRelation(&weaponRelDesc, &weaponDataDesc, WEAPON_RELATION_QCASES);
   AssertMsg(g_pWeaponRel != NULL, "Unable to create weapon link relation");
#ifdef DBG_ON
   if (config_is_defined("weaponrel_listen"))
      g_pWeaponRel->Listen(LISTENTO,WeaponPainListener,NULL);
#endif
}

void TermWeaponRelation()
{
   SafeRelease(g_pWeaponRel);
}

////////////////////////////////////////////////////////////////////////////////

static IIntProperty *g_pBaseWeaponDamageProp = NULL;

static sPropertyDesc BaseWeaponDamageDesc = 
{
   PROP_BASE_WEAPON_DAMAGE_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   NULL,
   1,
   0,
   { "Weapon", "BaseDamage"},
};

#define BASE_WEAPON_DAMAGE_IMPL kPropertyImplDense

void InitBaseWeaponDamageProp()
{
   g_pBaseWeaponDamageProp = CreateIntProperty(&BaseWeaponDamageDesc, BASE_WEAPON_DAMAGE_IMPL);
}

void TermBaseWeaponDamageProp()
{
   SafeRelease(g_pBaseWeaponDamageProp);
}

////////////////////////////////////////

void SetBaseWeaponDamage(ObjID weapon, int damage)
{  // @TODO: range-check
   g_pBaseWeaponDamageProp->Set(weapon, damage);
}

int  GetBaseWeaponDamage(ObjID weapon)
{
   int damage=-1;  // default value
   g_pBaseWeaponDamageProp->Get(weapon, &damage);
   return damage;
}

//////////////////////////////
// weapon exposure props, for how much a weapon increases exposure

static IIntProperty *g_pWeaponExposureProp = NULL;

static sPropertyDesc WeaponExposureDesc = 
{
   PROP_WEAPON_EXPOSURE_NAME,
   0, // kPropertyNoInherit | kPropertyInstantiate,
   NULL,
   1,
   0,
   { "Weapon", "Exposure"},
};

#define WEAPON_EXPOSURE_IMPL kPropertyImplVerySparse

void InitWeaponExposureProp()
{
   g_pWeaponExposureProp = CreateIntProperty(&WeaponExposureDesc, WEAPON_EXPOSURE_IMPL);
}

void TermWeaponExposureProp()
{
   SafeRelease(g_pWeaponExposureProp);
}

////////////////////////////////////////

void SetWeaponExposure(ObjID weapon, int exposure)
{
   g_pWeaponExposureProp->Set(weapon, exposure);
}

int  GetWeaponExposure(ObjID weapon)
{
   int exposure=0;  // default value
   g_pWeaponExposureProp->Get(weapon, &exposure);
   return exposure;
}

//////////////////////////////
// weapon exposure props, for how much a weapon increases exposure

static IIntProperty *g_pSwingExposeProp = NULL;

static sPropertyDesc SwingExposeDesc = 
{
   PROP_SWING_EXPOSURE_NAME,
   0, // kPropertyNoInherit | kPropertyInstantiate,
   NULL,
   1,
   0,
   { "Weapon", "SwingExpose"},
};

#define WEAPON_EXPOSURE_IMPL kPropertyImplVerySparse

void InitWeapSwingExposureProp()
{
   g_pSwingExposeProp = CreateIntProperty(&SwingExposeDesc, WEAPON_EXPOSURE_IMPL);
}

void TermWeapSwingExposureProp()
{
   SafeRelease(g_pSwingExposeProp);
}

////////////////////////////////////////

void SetWeapSwingExposure(ObjID weapon, int exposure)
{
   g_pSwingExposeProp->Set(weapon, exposure);
}

int  GetWeapSwingExposure(ObjID weapon)
{
   int exposure=0;  // default value
   g_pSwingExposeProp->Get(weapon, &exposure);
   return exposure;
}

////////////////////////////////////////////////////////////////////////////////

// Howdy howdy, we're hacks
// This stuff will end up in the weapon (sword?) propagation, whenever one exists...

static IIntProperty *g_pCurWeaponDamageProp = NULL;

static sPropertyDesc CurWeaponDamageDesc = 
{
   PROP_CUR_WEAPON_DAMAGE_NAME,
   kPropertyConcrete | kPropertyNoEdit,
   NULL,
   1,
   0,
   { "Weapon", "CurDamage"},
};

#define CUR_WEAPON_DAMAGE_IMPL kPropertyImplDense

void InitCurWeaponDamageProp()
{
   g_pCurWeaponDamageProp = CreateIntProperty(&CurWeaponDamageDesc, CUR_WEAPON_DAMAGE_IMPL);
}

void TermCurWeaponDamageProp()
{
   SafeRelease(g_pCurWeaponDamageProp);
}

////////////////////////////////////////

void SetWeaponDamageFactor(ObjID weapon, int factor)
{
   int base_damage;

   // @TODO: range-check
   if ((base_damage = GetBaseWeaponDamage(weapon)) == -1)
   {
      Warning(("no base weapon damage on %d, aborting\n", weapon));
      return;
   }

   g_pCurWeaponDamageProp->Set(weapon, factor + base_damage);
}

void ClearWeaponDamageFactor(ObjID weapon)
{
   g_pCurWeaponDamageProp->Delete(weapon);
}

////////////////////////////////////////

int  GetWeaponDamage(ObjID weapon)
{
   int damage=-1;  // default value
   g_pCurWeaponDamageProp->Get(weapon, &damage);
   return damage;
}


//////////////////////////////
// weapon terrain collide, for whether a weapon collides with terrain

IBoolProperty *g_pWeaponTerrainCollisionProp = NULL;

static sPropertyDesc WeaponTerrainCollisionDesc = 
{
   PROP_WEAPON_TERRAIN_COLLISON_NAME,
   0,
   NULL,
   0,
   0,
   { "Weapon", "Collides With Terrain"},
};

void InitWeaponTerrainCollisionProp()
{
   g_pWeaponTerrainCollisionProp = CreateBoolProperty(&WeaponTerrainCollisionDesc,kPropertyImplSparseHash);
}

void TermWeaponTerrainCollisionProp()
{
   SafeRelease(g_pWeaponTerrainCollisionProp);
}

////////////////////////////////////////

void SetWeaponTerrainCollision(ObjID weapon, BOOL collisionval)
{
   g_pWeaponTerrainCollisionProp->Set(weapon, collisionval);
}

BOOL  GetWeaponTerrainCollision(ObjID weapon)
{
   BOOL collisionval=FALSE;  // default value
   g_pWeaponTerrainCollisionProp->Get(weapon, &collisionval);
   return collisionval;
}

////////////////////////////////////////////////////////////////////////////////

// This is the name of the metaprop to apply when changing modes

static IStringProperty *g_pWeaponModeChangeMetaProp = NULL;

static sPropertyDesc WeaponModeChangeMetaPropDesc = 
{
   PROP_WEAPON_MODE_CHANGE_METAPROP_NAME,
   kPropertyInstantiate,
   NULL,
   1,
   0,
   { "Weapon", "Mode Change Metaproperty"},
};

#define WEAPON_MODE_CHANGE_METAPROP_IMPL kPropertyImplDense

void InitWeaponModeChangeMetaProp()
{
   g_pWeaponModeChangeMetaProp = CreateStringProperty(&WeaponModeChangeMetaPropDesc, WEAPON_MODE_CHANGE_METAPROP_IMPL);
}

void TermWeaponModeChangeMetaProp()
{
   SafeRelease(g_pWeaponModeChangeMetaProp);
}

const char *GetWeaponModeChangeMetaProp(ObjID weapon)
{
   const char *name = NULL;
   g_pWeaponModeChangeMetaProp->Get(weapon, &name);
   return name;
}

////////////////////////////////////////////////////////////////////////////////
// This is the name of a metaproperty to UNAPPLY when changing modes

static IStringProperty *g_pWeaponModeUnchangeMetaProp = NULL;

static sPropertyDesc WeaponModeUnchangeMetaPropDesc = 
{
   PROP_WEAPON_MODE_UNCHANGE_METAPROP_NAME,
   kPropertyInstantiate,
   NULL,
   1,
   0,
   { "Weapon", "Mode Change-back Metaproperty"},
};

#define WEAPON_MODE_UNCHANGE_METAPROP_IMPL kPropertyImplDense

void InitWeaponModeUnchangeMetaProp()
{
   g_pWeaponModeUnchangeMetaProp = CreateStringProperty(&WeaponModeUnchangeMetaPropDesc, WEAPON_MODE_UNCHANGE_METAPROP_IMPL);
}

void TermWeaponModeUnchangeMetaProp()
{
   SafeRelease(g_pWeaponModeUnchangeMetaProp);
}

const char *GetWeaponModeUnchangeMetaProp(ObjID weapon)
{
   const char *name = NULL;
   g_pWeaponModeUnchangeMetaProp->Get(weapon, &name);
   return name;
}

