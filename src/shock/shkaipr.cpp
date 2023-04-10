/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaipr.cpp,v 1.5 2000/02/19 12:36:28 toml Exp $
//
// AI properties
//

#include <shkaipr.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <proplist.h>
#include <propert_.h>
#include <dataops_.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////

static sPropertyDesc g_AINotMeleeTargetPropertyDesc =
{
   "AI_NotMelee",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Not Melee Target" },
   kPropertyChangeLocally, // net_flags
};

IBoolProperty* g_pAINotMeleeTargetProperty;

void ShockAIInitMeleeTargetProp(void)
{
   g_pAINotMeleeTargetProperty = CreateBoolProperty(&g_AINotMeleeTargetPropertyDesc, kPropertyImplVerySparse);
}

void ShockAITermMeleeTargetProp(void)
{
   SafeRelease(g_pAINotMeleeTargetProperty);
}

///////////////////////////////////////////////////////////////////////////////

IAISwarmProperty * g_pAISwarmProperty;

////////////////////////////////////////

const sAISwarmParams g_AIDefaultSwarmParams =
{
   1,    // close distance
   10,   // back off distance
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Swarm", Complex
//

static sFieldDesc SwarmPropertyFields[] = 
{
   { "Close Distance", kFieldTypeFloat, FieldLocation(sAISwarmParams, m_closeDist),  kFieldFlagNone },
   { "Back-Off Distance", kFieldTypeFloat, FieldLocation(sAISwarmParams, m_backOffDist),  kFieldFlagNone },
};

////////////////////////////////////////

static sPropertyDesc g_AISwarmPropertyDesc = 
{
   PROP_AI_SWARM,
   0,
   NULL, 0, 0,
   { AI_ABILITY_CAT, "Swarm: parameters" },
};

////////////////////////////////////////

class cAISwarmParams : public sAISwarmParams
{
public:
   cAISwarmParams() { *this = *(cAISwarmParams *)&g_AIDefaultSwarmParams; }
};

static sStructDesc SwarmDesc = StructDescBuild(cAISwarmParams, kStructFlagNone, SwarmPropertyFields);

////////////////////////////////////////

class cSwarmOps : public cClassDataOps<cAISwarmParams>
{
public:
   cSwarmOps() : cClassDataOps<cAISwarmParams>(kNoFlags) {};
};

typedef cListPropertyStore<cSwarmOps> cSwarmPropertyStore;

typedef cSpecificProperty<IAISwarmProperty, &IID_IAISwarmProperty, sAISwarmParams *, cSwarmPropertyStore> cBaseSwarmProp;

class cAISwarmProp : public cBaseSwarmProp
{
public:
   cAISwarmProp(const sPropertyDesc* desc) : cBaseSwarmProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAISwarmParams);
};

float ShockAIGetCloseDist(ObjID objID)
{
   sAISwarmParams *pParams = AIGetProperty(g_pAISwarmProperty, objID, (sAISwarmParams *)&g_AIDefaultSwarmParams);
   return pParams->m_closeDist;
}

float ShockAIGetBackOffDist(ObjID objID)
{
   sAISwarmParams *pParams = AIGetProperty(g_pAISwarmProperty, objID, (sAISwarmParams *)&g_AIDefaultSwarmParams);
   return pParams->m_backOffDist;
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void ShockAIInitSwarmProp(void)
{
   g_pAISwarmProperty = new cAISwarmProp(&g_AISwarmPropertyDesc);
   StructDescRegister(&SwarmDesc);
}

///////////////////////////////////////

void ShockAITermSwarmProp(void)
{
   SafeRelease(g_pAISwarmProperty);
}

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Grub_Combat", Complex
//

IAIGrubCombatProperty * g_pAIGrubCombatProperty;

////////////////////////////////////////

const sAIGrubCombatParams g_AIDefaultGrubCombatParams =
{
   10, 
   5,
   "Venom",
   1,
   5,
   25,
   3, 
   5,
};

///////////////////////////////////////////////////////////////////////////////

static sFieldDesc GrubCombatPropertyFields[] = 
{
   { "Leap Distance", kFieldTypeFloat, FieldLocation(sAIGrubCombatParams, m_leapDist),  kFieldFlagNone },
   { "Bite Distance", kFieldTypeFloat, FieldLocation(sAIGrubCombatParams, m_biteDist),  kFieldFlagNone },
   { "Stimulus", kFieldTypeString, FieldLocation(sAIGrubCombatParams, m_stimulus),  kFieldFlagNone },
   { "Intensity", kFieldTypeFloat, FieldLocation(sAIGrubCombatParams, m_intensity),  kFieldFlagNone },
   { "Leap Speed X", kFieldTypeFloat, FieldLocation(sAIGrubCombatParams, m_leapSpeedX),  kFieldFlagNone },
   { "Leap Speed Z", kFieldTypeFloat, FieldLocation(sAIGrubCombatParams, m_leapSpeedZ),  kFieldFlagNone },
   { "Min Leap Time", kFieldTypeInt, FieldLocation(sAIGrubCombatParams, m_minLeapTime),  kFieldFlagUnsigned },
   { "Max Leap Time", kFieldTypeInt, FieldLocation(sAIGrubCombatParams, m_maxLeapTime),  kFieldFlagUnsigned },
};

////////////////////////////////////////

static sPropertyDesc g_AIGrubCombatPropertyDesc = 
{
   PROP_AI_GRUB_COMBAT,
   0,
   NULL, 0, 0,
   { AI_ABILITY_CAT, "GrubCombat: parameters" },
};

////////////////////////////////////////

class cAIGrubCombatParams : public sAIGrubCombatParams
{
public:
   cAIGrubCombatParams() { *this = *(cAIGrubCombatParams *)&g_AIDefaultGrubCombatParams; }
};

static sStructDesc GrubCombatDesc = StructDescBuild(cAIGrubCombatParams, kStructFlagNone, GrubCombatPropertyFields);

////////////////////////////////////////

class cGrubCombatOps : public cClassDataOps<cAIGrubCombatParams>
{
public:
   cGrubCombatOps() : cClassDataOps<cAIGrubCombatParams>(kNoFlags) {};
};

typedef cListPropertyStore<cGrubCombatOps> cGrubCombatPropertyStore;

typedef cSpecificProperty<IAIGrubCombatProperty, &IID_IAIGrubCombatProperty, sAIGrubCombatParams *, cGrubCombatPropertyStore> cBaseGrubCombatProp;

class cAIGrubCombatProp : public cBaseGrubCombatProp
{
public:
   cAIGrubCombatProp(const sPropertyDesc* desc) : cBaseGrubCombatProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAIGrubCombatParams);
};

float ShockAIGetBiteDist(ObjID objID)
{
   sAIGrubCombatParams *pParams = AIGetProperty(g_pAIGrubCombatProperty, objID, (sAIGrubCombatParams *)&g_AIDefaultGrubCombatParams);
   return pParams->m_biteDist;
}

const char* ShockAIGetStimulus(ObjID objID)
{
   sAIGrubCombatParams *pParams = AIGetProperty(g_pAIGrubCombatProperty, objID, (sAIGrubCombatParams *)&g_AIDefaultGrubCombatParams);
   return pParams->m_stimulus;
}

float ShockAIGetIntensity(ObjID objID)
{
   sAIGrubCombatParams *pParams = AIGetProperty(g_pAIGrubCombatProperty, objID, (sAIGrubCombatParams *)&g_AIDefaultGrubCombatParams);
   return pParams->m_intensity;
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void ShockAIInitGrubCombatProp(void)
{
   g_pAIGrubCombatProperty = new cAIGrubCombatProp(&g_AIGrubCombatPropertyDesc);
   StructDescRegister(&GrubCombatDesc);
}

///////////////////////////////////////

void ShockAITermGrubCombatProp(void)
{
   SafeRelease(g_pAIGrubCombatProperty);
}

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Wander", Complex
//

IAIWanderProperty * g_pAIWanderProperty;

////////////////////////////////////////

const sAIWanderParams g_AIDefaultWanderParams =
{
   10,    // wander distance
};

///////////////////////////////////////////////////////////////////////////////

static sFieldDesc WanderPropertyFields[] = 
{
   { "Wander Distance", kFieldTypeFloat, FieldLocation(sAIWanderParams, m_wanderDist),  kFieldFlagNone },
};

////////////////////////////////////////

static sPropertyDesc g_AIWanderPropertyDesc = 
{
   PROP_AI_Wander,
   0,
   NULL, 0, 0,
   { AI_ABILITY_CAT, "Wander: parameters" },
};

////////////////////////////////////////

class cAIWanderParams : public sAIWanderParams
{
public:
   cAIWanderParams() { *this = *(cAIWanderParams *)&g_AIDefaultWanderParams; }
};

static sStructDesc WanderDesc = StructDescBuild(cAIWanderParams, kStructFlagNone, WanderPropertyFields);

////////////////////////////////////////

class cWanderOps : public cClassDataOps<cAIWanderParams>
{
public:
   cWanderOps() : cClassDataOps<cAIWanderParams>(kNoFlags) {};
};

typedef cListPropertyStore<cWanderOps> cWanderPropertyStore;

typedef cSpecificProperty<IAIWanderProperty, &IID_IAIWanderProperty, sAIWanderParams *, cWanderPropertyStore> cBaseWanderProp;

class cAIWanderProp : public cBaseWanderProp
{
public:
   cAIWanderProp(const sPropertyDesc* desc) : cBaseWanderProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAIWanderParams);
};

float ShockAIGetWanderDist(ObjID objID)
{
   sAIWanderParams *pParams = AIGetProperty(g_pAIWanderProperty, objID, (sAIWanderParams *)&g_AIDefaultWanderParams);
   return pParams->m_wanderDist;
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void ShockAIInitWanderProp(void)
{
   g_pAIWanderProperty = new cAIWanderProp(&g_AIWanderPropertyDesc);
   StructDescRegister(&WanderDesc);
}

///////////////////////////////////////

void ShockAITermWanderProp(void)
{
   SafeRelease(g_pAIWanderProperty);
}

