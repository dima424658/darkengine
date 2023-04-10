/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkswmpr.cpp,v 1.2 2000/02/19 13:26:20 toml Exp $
//
// Swarm ability property
//

#include <shkswmpr.h>

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
   kPropertyInstantiate,
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

