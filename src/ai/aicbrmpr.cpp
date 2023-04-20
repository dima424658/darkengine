/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrmpr.cpp,v 1.3 1999/04/22 17:53:15 JON Exp $
//
//

#include <aicbrmpr.h>

#include <sdesbase.h>
#include <sdesc.h>

#include <propbase.h>
#include <propcomp.h>
#include <propface.h>
#include <prophash.h>
#include <property.h>

#include <aiprops.h>

#include <objpos.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////
//
// Ranged Applicabilities
//

sAIRangedApplicabilities g_defaultAIRangedApplicabilities = {0, 4, 6, 6, 4, 0, 5, 5, 0,};

class cAIRangedApplicabilities: public sAIRangedApplicabilities
{
public:
   cAIRangedApplicabilities() {*this = *(cAIRangedApplicabilities*)&g_defaultAIRangedApplicabilities;}
};

static sPropertyDesc g_AIRangedApplicabilitiesPDesc =
{
   PROP_AI_RANGED_APPS,
   NULL,
   NULL,
   1, 
   0, 
   { AI_RANGED_CAT, "Ranged Combat Applicabilities" },
   kPropertyChangeLocally,  // net_flags
};

class cAIRangedApplicabilitiesOps : public cClassDataOps<cAIRangedApplicabilities>
{
public:
   cAIRangedApplicabilitiesOps() : cClassDataOps<cAIRangedApplicabilities>(kNoFlags) {};
};

typedef cHashPropertyStore<cAIRangedApplicabilitiesOps> cAIRangedApplicabilitiesStore;
typedef cSpecificProperty<IAIRangedApplicabilitiesProperty, &IID_IAIRangedApplicabilitiesProperty, sAIRangedApplicabilities *, cAIRangedApplicabilitiesStore> cBaseAIRangedApplicabilitiesProperty;

class cAIRangedApplicabilitiesProperty : public cBaseAIRangedApplicabilitiesProperty
{
public:
   cAIRangedApplicabilitiesProperty(const sPropertyDesc *desc) : cBaseAIRangedApplicabilitiesProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(sAIRangedApplicabilities);
};

IAIRangedApplicabilitiesProperty* g_pAIRangedApplicabilitiesProperty = NULL;

////////////////////////////////////////////////////////////////////////////////

static const char *g_AIRangedModeNames[kAIRC_NumApplicabilities] =
{ 
   "None", 
   "Minimum", 
   "VeryLow", 
   "Low", 
   "Normal", 
   "High", 
   "VeryHigh", 
};
 
static sFieldDesc g_AIRangedApplicabilitiesFields[] = 
{
   { "Idle", kFieldTypeEnum, FieldLocation(sAIRangedApplicabilities, m_modeApplicability[kAIRC_IdleMode]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Close", kFieldTypeEnum, FieldLocation(sAIRangedApplicabilities, m_modeApplicability[kAIRC_CloseMode]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Backup", kFieldTypeEnum, FieldLocation(sAIRangedApplicabilities, m_modeApplicability[kAIRC_BackupMode]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Wound", kFieldTypeEnum, FieldLocation(sAIRangedApplicabilities, m_modeApplicability[kAIRC_WoundMode]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Vantage", kFieldTypeEnum, FieldLocation(sAIRangedApplicabilities, m_modeApplicability[kAIRC_VantageMode]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Left", kFieldTypeEnum, FieldLocation(sAIRangedApplicabilities, m_modeApplicability[kAIRC_LeftMode]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Right", kFieldTypeEnum, FieldLocation(sAIRangedApplicabilities, m_modeApplicability[kAIRC_RightMode]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
};

static sStructDesc g_AIRangedApplicabilitiesSDesc = StructDescBuild(sAIRangedApplicabilities, kStructFlagNone, g_AIRangedApplicabilitiesFields);

////////////////////////////////////////////////////////////////////////////////
//
// Ranged Flee
//

sAIRangedFleeParams g_defaultAIRangedFleeParams = {4, 0, 1.57, 5, 30., 6.,};

class cAIRangedFleeParams: public sAIRangedFleeParams
{
public:
   cAIRangedFleeParams() {*this = *(cAIRangedFleeParams*)&g_defaultAIRangedFleeParams;}
};

static sPropertyDesc g_AIRangedFleePDesc =
{
   PROP_AI_RANGED_FLEE,
   NULL,
   NULL,
   1, 
   0, 
   { AI_RANGED_CAT, "Ranged Combat Flee" },
   kPropertyChangeLocally,  // net_flags
};

class cAIRangedFleeOps : public cClassDataOps<cAIRangedFleeParams>
{
public:
   cAIRangedFleeOps() : cClassDataOps<cAIRangedFleeParams>(kNoFlags) {};
};

typedef cHashPropertyStore<cAIRangedFleeOps> cAIRangedFleeStore;
typedef cSpecificProperty<IAIRangedFleeProperty, &IID_IAIRangedFleeProperty, sAIRangedFleeParams *, cAIRangedFleeStore> cBaseAIRangedFleeProperty;

class cAIRangedFleeProperty : public cBaseAIRangedFleeProperty
{
public:
   cAIRangedFleeProperty(const sPropertyDesc *desc) : cBaseAIRangedFleeProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(sAIRangedFleeParams);
};

IAIRangedFleeProperty* g_pAIRangedFleeProperty = NULL;

////////////////////////////////////////////////////////////////////////////////

static sFieldDesc g_AIRangedFleeFields[] = 
{
   { "Very Short", kFieldTypeEnum, FieldLocation(sAIRangedFleeParams, m_rangeApplicability[kAIRC_RangeVeryShort]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Short", kFieldTypeEnum, FieldLocation(sAIRangedFleeParams, m_rangeApplicability[kAIRC_RangeShort]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Angle Range", kFieldTypeFloat, FieldLocation(sAIRangedFleeParams, m_angleRange), kFieldFlagNone },
   { "Number Points", kFieldTypeInt, FieldLocation(sAIRangedFleeParams, m_numPoints), kFieldFlagNone },
   { "Distance", kFieldTypeFloat, FieldLocation(sAIRangedFleeParams, m_distance), kFieldFlagNone },
   { "Clearance", kFieldTypeFloat, FieldLocation(sAIRangedFleeParams, m_clearance), kFieldFlagNone },
};

static sStructDesc g_AIRangedFleeSDesc = StructDescBuild(sAIRangedFleeParams, kStructFlagNone, g_AIRangedFleeFields);

////////////////////////////////////////////////////////////////////////////////
//
// Ranged Shoot
//

sAIRangedShootParams g_defaultAIRangedShootParams = {1, 3, 6, 3, 0, 0, 0, 0xf000, {0, 0, 0}};

class cAIRangedShootParams: public sAIRangedShootParams
{
public:
   cAIRangedShootParams() {*this = *(cAIRangedShootParams*)&g_defaultAIRangedShootParams;}
};

static sPropertyDesc g_AIRangedShootPDesc =
{
   PROP_AI_RANGED_SHOOT,
   NULL,
   NULL,
   1, 
   0, 
   { AI_RANGED_CAT, "Ranged Combat Shoot" },
   kPropertyChangeLocally,  // net_flags
};

class cAIRangedShootOps : public cClassDataOps<cAIRangedShootParams>
{
public:
   cAIRangedShootOps() : cClassDataOps<cAIRangedShootParams>(kNoFlags) {};
};

typedef cHashPropertyStore<cAIRangedShootOps> cAIRangedShootStore;
typedef cSpecificProperty<IAIRangedShootProperty, &IID_IAIRangedShootProperty, sAIRangedShootParams *, cAIRangedShootStore> cBaseAIRangedShootProperty;

class cAIRangedShootProperty : public cBaseAIRangedShootProperty
{
public:
   cAIRangedShootProperty(const sPropertyDesc *desc) : cBaseAIRangedShootProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(sAIRangedShootParams);
};

IAIRangedShootProperty* g_pAIRangedShootProperty = NULL;

////////////////////////////////////////////////////////////////////////////////

static sFieldDesc g_AIRangedShootFields[] = 
{
   { "Very Short", kFieldTypeEnum, FieldLocation(sAIRangedShootParams, m_rangeApplicability[kAIRC_RangeVeryShort]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Short", kFieldTypeEnum, FieldLocation(sAIRangedShootParams, m_rangeApplicability[kAIRC_RangeShort]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Ideal", kFieldTypeEnum, FieldLocation(sAIRangedShootParams, m_rangeApplicability[kAIRC_RangeIdeal]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Long", kFieldTypeEnum, FieldLocation(sAIRangedShootParams, m_rangeApplicability[kAIRC_RangeLong]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Very Long", kFieldTypeEnum, FieldLocation(sAIRangedShootParams, m_rangeApplicability[kAIRC_RangeVeryLong]), kFieldFlagNone, 0, kAIRC_NumApplicabilities, kAIRC_NumApplicabilities, g_AIRangedModeNames},
   { "Confirm Range", kFieldTypeBool, FieldLocation(sAIRangedShootParams, m_confirmRange), kFieldFlagNone},
   { "Confirm LOF", kFieldTypeBool, FieldLocation(sAIRangedShootParams, m_confirmLOF), kFieldFlagNone},
   { "Rotation Speed", kFieldTypeInt, FieldLocation(sAIRangedShootParams, m_rotationSpeed), kFieldFlagHex},
   { "Launch Offset", kFieldTypeVector, FieldLocation(sAIRangedShootParams, m_launchOffset), kFieldFlagNone},
};

static sStructDesc g_AIRangedShootSDesc = StructDescBuild(sAIRangedShootParams, kStructFlagNone, g_AIRangedShootFields);

/////////////////////////////////////////////////////////////////////////////////

BOOL AIGetLaunchOffset(ObjID AIID, Location* pLocation)
{
   sAIRangedShootParams* pParams;

   if (g_pAIRangedShootProperty->Get(AIID, &pParams))
   {
      mxs_vector offsetVec;
      mxs_vector launchVec;
      Position* pSourcePos = ObjPosGet(AIID);

      mx_rot_z_vec(&offsetVec, &pParams->m_launchOffset, pSourcePos->fac.tz);
      mx_add_vec(&launchVec, &pSourcePos->loc.vec, &offsetVec);
      MakeHintedLocationFromVector(pLocation, &launchVec, &pSourcePos->loc);
      return TRUE;
   }
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////

void InitRangedModeProperties(void)
{
   // applicabilities
   g_pAIRangedApplicabilitiesProperty = new cAIRangedApplicabilitiesProperty(&g_AIRangedApplicabilitiesPDesc);
   StructDescRegister(&g_AIRangedApplicabilitiesSDesc);
   // flee
   g_pAIRangedFleeProperty = new cAIRangedFleeProperty(&g_AIRangedFleePDesc);
   StructDescRegister(&g_AIRangedFleeSDesc);
   // Shoot
   g_pAIRangedShootProperty = new cAIRangedShootProperty(&g_AIRangedShootPDesc);
   StructDescRegister(&g_AIRangedShootSDesc);
}

void TermRangedModeProperties(void)
{
   SafeRelease(g_pAIRangedApplicabilitiesProperty);
   g_pAIRangedApplicabilitiesProperty = NULL;
   SafeRelease(g_pAIRangedFleeProperty);
   g_pAIRangedFleeProperty = NULL;
   SafeRelease(g_pAIRangedShootProperty);
   g_pAIRangedShootProperty = NULL;
}

