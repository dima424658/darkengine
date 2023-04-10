/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// AI properties
//

#include <dpcaipr.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <proplist.h>
#include <propert_.h>
#include <dataops_.h>

#include <sdesc.h>
#include <sdesbase.h>

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

void DPCAIInitMeleeTargetProp(void)
{
   g_pAINotMeleeTargetProperty = CreateBoolProperty(&g_AINotMeleeTargetPropertyDesc, kPropertyImplVerySparse);
}

void DPCAITermMeleeTargetProp(void)
{
   SafeRelease(g_pAINotMeleeTargetProperty);
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Dog_Combat", Complex
//

IAIDogCombatProperty * g_pAIDogCombatProperty;

////////////////////////////////////////

sAIDogCombatParams g_AIDefaultDogCombatParams =
{
   6, 
   0,
   "WeaponStim",
   1,
   5,
   25,
   2, 
   4,
};

///////////////////////////////////////////////////////////////////////////////

static sFieldDesc DogCombatPropertyFields[] = 
{
   { "Leap Distance", kFieldTypeFloat,  FieldLocation(sAIDogCombatParams, m_leapDist),    kFieldFlagNone     },
   { "Bite Distance", kFieldTypeFloat,  FieldLocation(sAIDogCombatParams, m_biteDist),    kFieldFlagNone     },
   { "Stimulus",      kFieldTypeString, FieldLocation(sAIDogCombatParams, m_stimulus),    kFieldFlagNone     },
   { "Intensity",     kFieldTypeFloat,  FieldLocation(sAIDogCombatParams, m_intensity),   kFieldFlagNone     },
   { "Leap Speed X",  kFieldTypeFloat,  FieldLocation(sAIDogCombatParams, m_leapSpeedX),  kFieldFlagNone     },
   { "Leap Speed Z",  kFieldTypeFloat,  FieldLocation(sAIDogCombatParams, m_leapSpeedZ),  kFieldFlagNone     },
   { "Min Leap Time", kFieldTypeInt,    FieldLocation(sAIDogCombatParams, m_minLeapTime), kFieldFlagUnsigned },
   { "Max Leap Time", kFieldTypeInt,    FieldLocation(sAIDogCombatParams, m_maxLeapTime), kFieldFlagUnsigned },
};                                                                                                          

////////////////////////////////////////

static sPropertyDesc g_AIDogCombatPropertyDesc = 
{
   PROP_AI_DOG_COMBAT,
   0,
   NULL, 0, 0,
   { AI_ABILITY_CAT, "DogCombat: parameters" },
};

////////////////////////////////////////

class cAIDogCombatParams
{
 public:
   cAIDogCombatParams()
   {
      m_leapDist = g_AIDefaultDogCombatParams.m_leapDist;    
      m_biteDist = g_AIDefaultDogCombatParams.m_biteDist;    
      strcpy(m_stimulus, g_AIDefaultDogCombatParams.m_stimulus);
      m_intensity   = g_AIDefaultDogCombatParams.m_intensity;
      m_leapSpeedX  = g_AIDefaultDogCombatParams.m_leapSpeedX;
      m_leapSpeedZ  = g_AIDefaultDogCombatParams.m_leapSpeedZ;
      m_minLeapTime = g_AIDefaultDogCombatParams.m_minLeapTime;
      m_maxLeapTime = g_AIDefaultDogCombatParams.m_maxLeapTime; 
   }

   float    m_leapDist;     // distance at which to bite
   float    m_biteDist;     // distance at which to bite
   char     m_stimulus[32]; // stimulus delivered
   float    m_intensity;    // stim intensity
   float    m_leapSpeedX;    
   float    m_leapSpeedZ;   
   unsigned m_minLeapTime;  // min time between leaps
   unsigned m_maxLeapTime;  // max time between leaps (if able to)
};

static sStructDesc DogCombatDesc = StructDescBuild(cAIDogCombatParams, kStructFlagNone, DogCombatPropertyFields);

////////////////////////////////////////

class cDogCombatOps : public cClassDataOps<cAIDogCombatParams>
{
public:
   cDogCombatOps() : cClassDataOps<cAIDogCombatParams>(kNoFlags) {};
};

typedef cListPropertyStore<cDogCombatOps> cDogCombatPropertyStore;

typedef cSpecificProperty<IAIDogCombatProperty, &IID_IAIDogCombatProperty, sAIDogCombatParams *, cDogCombatPropertyStore> cBaseDogCombatProp;

class cAIDogCombatProp : public cBaseDogCombatProp
{
public:
   cAIDogCombatProp(const sPropertyDesc* desc) : cBaseDogCombatProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAIDogCombatParams);
};

float DPCAIGetBiteDist(ObjID objID)
{
   sAIDogCombatParams *pParams = AIGetProperty(g_pAIDogCombatProperty, objID, (sAIDogCombatParams *)&g_AIDefaultDogCombatParams);
   return pParams->m_biteDist;
}

const char* DPCAIGetStimulus(ObjID objID)
{
   sAIDogCombatParams *pParams = AIGetProperty(g_pAIDogCombatProperty, objID, (sAIDogCombatParams *)&g_AIDefaultDogCombatParams);
   return pParams->m_stimulus;
}

float DPCAIGetIntensity(ObjID objID)
{
   sAIDogCombatParams *pParams = AIGetProperty(g_pAIDogCombatProperty, objID, (sAIDogCombatParams *)&g_AIDefaultDogCombatParams);
   return pParams->m_intensity;
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void DPCAIInitDogCombatProp(void)
{
   StructDescRegister(&DogCombatDesc);
   g_pAIDogCombatProperty = new cAIDogCombatProp(&g_AIDogCombatPropertyDesc);
}

///////////////////////////////////////

void DPCAITermDogCombatProp(void)
{
   SafeRelease(g_pAIDogCombatProperty);
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

float DPCAIGetWanderDist(ObjID objID)
{
   sAIWanderParams *pParams = AIGetProperty(g_pAIWanderProperty, objID, (sAIWanderParams *)&g_AIDefaultWanderParams);
   return pParams->m_wanderDist;
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void DPCAIInitWanderProp(void)
{
   g_pAIWanderProperty = new cAIWanderProp(&g_AIWanderPropertyDesc);
   StructDescRegister(&WanderDesc);
}

///////////////////////////////////////

void DPCAITermWanderProp(void)
{
   SafeRelease(g_pAIWanderProperty);
}

