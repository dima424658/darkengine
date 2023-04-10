/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaitpr.cpp,v 1.2 2000/02/19 12:36:32 toml Exp $
//
// Turret ability properties
//

#include <shkaitpr.h>

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

///////////////////////////////////////////////////////////////////////////////

IAITurretProperty *     g_pAITurretProperty;

////////////////////////////////////////

const sAITurretParams g_AIDefaultTurretParams =
{
   0,    // raise/lower joint num
   0,    // inactive posn
   2,    // active posn
   0.1,  // raise/lower speed
   1,    // rotational joint
   0.1,  // facing epsilon
   0.5,  // firing epsilon
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_Turret", Complex
//

static sFieldDesc TurretPropertyFields[] = 
{
   { "Raise/Lower Joint", kFieldTypeInt, FieldLocation(sAITurretParams, m_jointRaise),  kFieldFlagNone },
   { "Inactive Posn", kFieldTypeFloat, FieldLocation(sAITurretParams, m_inactivePos),  kFieldFlagNone },
   { "Active Posn", kFieldTypeFloat, FieldLocation(sAITurretParams, m_activePos),  kFieldFlagNone },
   { "Raise/Lower Speed", kFieldTypeFloat, FieldLocation(sAITurretParams, m_raiseSpeed),  kFieldFlagNone },
   { "Rotational Joint", kFieldTypeInt, FieldLocation(sAITurretParams, m_jointRotate),  kFieldFlagNone },
   { "Facing Epsilon", kFieldTypeFloat, FieldLocation(sAITurretParams, m_facingEpsilon),  kFieldFlagNone },
   { "Firing Epsilon", kFieldTypeFloat, FieldLocation(sAITurretParams, m_fireEpsilon),  kFieldFlagNone },
};

////////////////////////////////////////

static sPropertyDesc g_AITurretPropertyDesc = 
{
   PROP_AI_TURRET,
   kPropertyInstantiate,
   NULL, 0, 0,
   { AI_ABILITY_CAT, "Turret: parameters" },
};

////////////////////////////////////////

class cAITurretParams : public sAITurretParams
{
public:
   cAITurretParams() { *this = *(cAITurretParams *)&g_AIDefaultTurretParams; }
};

static sStructDesc TurretDesc = StructDescBuild(cAITurretParams, kStructFlagNone, TurretPropertyFields);

////////////////////////////////////////

class cTurretOps : public cClassDataOps<cAITurretParams>
{
public:
   cTurretOps() : cClassDataOps<cAITurretParams>(kNoFlags) {};
};

typedef cListPropertyStore<cTurretOps> cTurretPropertyStore;

typedef cSpecificProperty<IAITurretProperty, &IID_IAITurretProperty, sAITurretParams *, cTurretPropertyStore> cBaseTurretProp;

class cAITurretProp : public cBaseTurretProp
{
public:
   cAITurretProp(const sPropertyDesc* desc) : cBaseTurretProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAITurretParams);
};

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void AIInitTurretProp(void)
{
   g_pAITurretProperty = new cAITurretProp(&g_AITurretPropertyDesc);
   StructDescRegister(&TurretDesc);
}

///////////////////////////////////////

void AITermTurretProp(void)
{
   SafeRelease(g_pAITurretProperty);
}

///////////////////////////////////////////////////////////////////////////////



