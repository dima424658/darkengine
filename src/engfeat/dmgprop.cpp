/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/dmgprop.cpp,v 1.6 1998/10/19 13:03:30 mahk Exp $
#include <appagg.h>
#include <dmgprop.h>
#include <property.h>
#include <propface.h>
#include <propbase.h>

#include <sdesbase.h>
#include <sdestool.h>

// must be last header
#include <dbmem.h>


static sPropertyDesc WeapDesc = 
{
   PROP_WEAPON_DAMAGE_NAME, 
   0 , // flags 
   NULL, // constraints 
   0, 0, // Version
   { "Game: Damage Model", "Weapon Damage" }, 
};

static sPropertyDesc WeapTypeDesc = 
{
   PROP_WEAPON_TYPE_NAME, 
   0 , // flags 
   NULL, // constraints 
   0, 0, // Version
   { "Game: Damage Model", "Weapon Type" }, 
};

static sPropertyDesc SlayResultDesc = 
{
   PROP_SLAY_RESULT_NAME,
   0,
   NULL,
   0, 0,
   { "Game: Damage Model", "Slay Result"}, 
};

static sPropertyTypeDesc SlayResultType = {   "eSlayResult", sizeof(int) }; 
void init_slay_sdesc(); 

static IIntProperty* WeapProp = NULL;
static IIntProperty* WeapTypeProp = NULL;
static IIntProperty* SlayResultProp = NULL; 

#define WEAPPROP_IMPL kPropertyImplSparseHash
#define WEAPTYPEPROP_IMPL kPropertyImplSparseHash
#define SLAYRESULT_IMPL   kPropertyImplSparseHash

void InitDamageProps(void)
{
   WeapProp = CreateIntProperty(&WeapDesc,WEAPPROP_IMPL);
   WeapTypeProp = CreateIntProperty(&WeapTypeDesc,WEAPTYPEPROP_IMPL);
   SlayResultProp = CreateIntegralProperty(&SlayResultDesc,&SlayResultType,SLAYRESULT_IMPL); 
   init_slay_sdesc(); 
}

void TermDamageProps(void)
{
   SafeRelease(WeapProp); 
   SafeRelease(WeapTypeProp); 
}

void ObjSetWeaponDamage(ObjID obj, int damage)
{
   Assert_(WeapProp);
   WeapProp->Set(obj,damage);
}

int ObjGetWeaponDamage(ObjID obj)
{
   Assert_(WeapProp);
   int damage = 0;
   WeapProp->Get(obj,&damage);
   return damage;
}

BOOL ObjHasWeaponDamage(ObjID obj)
{
   Assert_(WeapProp);
   return WeapProp->IsRelevant(obj);
}

void ObjUnsetWeaponDamage(ObjID obj)
{
   Assert_(WeapProp);
   WeapProp->Delete(obj);
}

BOOL ObjGetWeaponType(ObjID obj, int* type)
{
   Assert_(WeapTypeProp);
   return WeapTypeProp->Get(obj,type);
}

void ObjSetWeaponType(ObjID obj, int type)
{
   Assert_(WeapTypeProp);
   WeapTypeProp->Set(obj, type);
}

void ObjUnsetWeaponType(ObjID obj)
{
   Assert_(WeapTypeProp);
   WeapTypeProp->Delete(obj);
}


eSlayResult ObjGetSlayResult(ObjID obj)
{
   Assert_(SlayResultProp); 
   eSlayResult result = kSlayNormal; 
   SlayResultProp->Get(obj,(int*)&result); 
   return result; 
}

void ObjSetSlayResult(ObjID obj, eSlayResult result) 
{
   Assert_(SlayResultProp); 
   SlayResultProp->Set(obj,(int)result); 
}

//
// Slay result sdesc
//

static const char* slay_strings[] = 
{ "Normal", "No Effect", "Terminate", "Destroy" }; 

#define NUM_SLAY_STRINGS (sizeof(slay_strings)/sizeof(slay_strings[0]))

static sFieldDesc slay_field[] = 
{
   { "Effect", kFieldTypeEnum, sizeof(eSlayResult), 0, kFieldFlagUnsigned, 0, kNumSlayResults, NUM_SLAY_STRINGS, slay_strings }
};

static sStructDesc slay_sdesc = StructDescBuild(eSlayResult, kStructFlagNone, slay_field); 

static void init_slay_sdesc()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&slay_sdesc); 
}

