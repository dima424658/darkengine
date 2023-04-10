/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprabil.cpp,v 1.10 2000/02/19 12:44:57 toml Exp $
//
//
//

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <proplist.h>
#include <prophash.h>
#include <propert_.h>
#include <dataops_.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <aiprabil.h>
#include <aiprrngd.h>
#include <aiinvtyp.h>
#include <ainonhst.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////

IBoolProperty *         g_pAIPatrolProperty;
IBoolProperty *         g_pAIPatrolRandomProperty;
IBoolProperty *         g_pAIFidgetProperty;

IAIInvestKindProperty * g_pAIInvestKindProperty;
IAINonHostilityProperty * g_pAINonHostilityProperty;

IAINonCombatDmgRespProperty* g_pAINonCombatDmgRespProperty;

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Patrol", BOOL
//

static sPropertyDesc g_AIPatrolPropertyDesc =
{
  PROP_AI_PATROL, 
  0,
  NULL, 0, 0, 
  { AI_ABILITY_CAT, "Patrol: Does patrol" }, 
  kPropertyChangeLocally,
};

static sPropertyDesc g_AIPatrolRandomPropertyDesc =
{
  PROP_AI_PATROL_RANDOM, 
  0,
  NULL, 0, 0, 
  { AI_ABILITY_CAT, "Patrol: Random sequence" }, 
  kPropertyChangeLocally,
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Fidget", BOOL
//

static sPropertyDesc g_AIFidgetPropertyDesc =
{
  PROP_AI_FIDGET, 
  0,
  NULL, 0, 0, 
  { AI_ABILITY_CAT, "Idling: Should fidget" }, 
  kPropertyChangeLocally,
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIInvestKindProperty, "AI_InvKnd"
//

static sPropertyDesc g_AIInvestKindPropertyDesc =
{
   PROP_AI_INVESTKIND,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Investigation: Style" },
   kPropertyChangeLocally,
};

///////////////////////////////////////

static const char * _g_ppszAIInvestKinds[kAIIK_Num] = 
{
   "Normal",
   "Don't investigate",
};

///////////////////////////////////////

static sFieldDesc _g_AIInvestKindFieldDesc[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, kAIIK_Num, kAIIK_Num, _g_ppszAIInvestKinds },
}; 

static sStructDesc _g_AIInvestKindStructDesc = 
{
   "eAIInvestKind",
   sizeof(int),
   kStructFlagNone,
   sizeof(_g_AIInvestKindFieldDesc)/sizeof(_g_AIInvestKindFieldDesc[0]),
   _g_AIInvestKindFieldDesc,
}; 

///////////////////////////////////////

typedef cSpecificProperty<IAIInvestKindProperty, &IID_IAIInvestKindProperty, eAIInvestKind, cHashPropertyStore< cSimpleDataOps > > cAIInvestKindPropertyBase;

class cAIInvestKindProperty : public cAIInvestKindPropertyBase
{
public:

   cAIInvestKindProperty()
      : cAIInvestKindPropertyBase(&g_AIInvestKindPropertyDesc)
   {
   }

   STANDARD_DESCRIBE_TYPE(eAIInvestKind); 
};


///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAINonHostilityProperty, "AI_NonHst"
//

static sPropertyDesc g_AINonHostilityPropertyDesc =
{
   PROP_AI_NONHOSTILITY,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Combat: Non-hostile" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////////////////////////

static const char * _g_ppszAINonHostilities[kAINH_Num] = 
{
   "Never",
   "Always to player",
   "To player until damaged",
   "To player until threatened",
   "Until damaged",
   "Until threatened",
   "Always",
};

///////////////////////////////////////

static sFieldDesc _g_AINonHostilityFieldDesc[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, kAINH_Num, kAINH_Num, _g_ppszAINonHostilities },
}; 

static sStructDesc _g_AINonHostilityStructDesc = 
{
   "eAINonHostility",
   sizeof(int),
   kStructFlagNone,
   sizeof(_g_AINonHostilityFieldDesc)/sizeof(_g_AINonHostilityFieldDesc[0]),
   _g_AINonHostilityFieldDesc,
}; 

///////////////////////////////////////

typedef cSpecificProperty<IAINonHostilityProperty, &IID_IAINonHostilityProperty, eAINonHostility, cHashPropertyStore< cSimpleDataOps > > cAINonHostilityPropertyBase;

class cAINonHostilityProperty : public cAINonHostilityPropertyBase
{
public:

   cAINonHostilityProperty()
      : cAINonHostilityPropertyBase(&g_AINonHostilityPropertyDesc)
   {
   }

   STANDARD_DESCRIBE_TYPE(eAINonHostility); 
};

////////////////////////////////////////
//
// PROPERTY: IAINonCombatDmgRespProperty, "AI_NCDmgRsp"
//

static sFieldDesc NonCombatDmgRespPropertyFields[] = 
{
   { "Wound Threshold", kFieldTypeInt, FieldLocation(sAINonCombatDmgResp, woundThreshold),  kFieldFlagNone },
   { "Severe Threshold", kFieldTypeInt, FieldLocation(sAINonCombatDmgResp, severeThreshold),  kFieldFlagNone },
   { "Response Chance (%)", kFieldTypeInt, FieldLocation(sAINonCombatDmgResp, responseChance),  kFieldFlagNone },
};

////////////////////////////////////////

sAINonCombatDmgResp _g_AIDefaultNonCombatDmgResp = {1, 10, 100};

class cAINonCombatDmgResp : public sAINonCombatDmgResp
{
public:
   cAINonCombatDmgResp() { *this = *(cAINonCombatDmgResp *)&_g_AIDefaultNonCombatDmgResp; }
};

static sStructDesc NonCombatDmgRespDesc = StructDescBuild(cAINonCombatDmgResp, kStructFlagNone, NonCombatDmgRespPropertyFields);

////////////////////////////////////////

class cNonCombatDmgRespOps : public cClassDataOps<cAINonCombatDmgResp>
{
public:
   cNonCombatDmgRespOps() : cClassDataOps<cAINonCombatDmgResp>(kNoFlags) {};
};

typedef cHashPropertyStore<cNonCombatDmgRespOps> cNonCombatDmgRespPropertyStore;

typedef cSpecificProperty<IAINonCombatDmgRespProperty, &IID_IAINonCombatDmgRespProperty, sAINonCombatDmgResp *, cNonCombatDmgRespPropertyStore> cBaseNonCombatDmgRespProp;

class cAINonCombatDmgRespProp : public cBaseNonCombatDmgRespProp
{
public:
   cAINonCombatDmgRespProp(const sPropertyDesc* desc) : cBaseNonCombatDmgRespProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAINonCombatDmgResp);
};

/////////////////////////////////

static sPropertyDesc _g_AINonCombatDmgRespPropertyDesc = 
{
   PROP_AI_NONCOMBATDMGRESP,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Non-combat: Dmg Response Params" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void AIInitAbilProps()
{
   g_pAIPatrolProperty = CreateBoolProperty(&g_AIPatrolPropertyDesc, kPropertyImplDense);
   g_pAIPatrolRandomProperty= CreateBoolProperty(&g_AIPatrolRandomPropertyDesc, kPropertyImplDense);
   g_pAIFidgetProperty = CreateBoolProperty(&g_AIFidgetPropertyDesc, kPropertyImplDense);

   g_pAIInvestKindProperty = new cAIInvestKindProperty;
   StructDescRegister(&_g_AIInvestKindStructDesc);
   
   g_pAINonHostilityProperty = new cAINonHostilityProperty;
   StructDescRegister(&_g_AINonHostilityStructDesc);

   g_pAINonCombatDmgRespProperty = new cAINonCombatDmgRespProp(&_g_AINonCombatDmgRespPropertyDesc);
   StructDescRegister(&NonCombatDmgRespDesc);

   InitAIProjectileRelation();
   InitRangedCombatProperty();
}

///////////////////////////////////////

void AITermAbilProps()
{
   SafeRelease(g_pAIPatrolProperty);
   SafeRelease(g_pAIFidgetProperty);

   SafeRelease(g_pAIInvestKindProperty);

   SafeRelease(g_pAINonHostilityProperty);

   SafeRelease(g_pAINonCombatDmgRespProperty);

   TermAIProjectileRelation();
   TermRangedCombatProperty();
}

///////////////////////////////////////////////////////////////////////////////




