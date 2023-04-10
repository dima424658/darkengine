/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaircp.cpp,v 1.1 1999/03/25 17:39:27 JON Exp $
//
// Ranged-combat-related properties and relations
//

#include <shkaircp.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <aiprops.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <prophash.h>
#include <propert_.h>
#include <dataops_.h>

#include <sdesc.h>
#include <sdesbase.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

IShockRangedProperty  *g_pShockRangedProp = NULL;

////////////////////////////////////////////////////////////////////////////////

sShockRanged g_defaultShockRanged = {10, 40, 0, FALSE};

cShockRanged::cShockRanged()
{
   *this = *((cShockRanged*)&g_defaultShockRanged);
};

////////////////////////////////////////////////////////////////////////////////

static char *range_values_6[] = 
{
   "Never",
   "Very Rarely",
   "Rarely",
   "Sometimes",
   "Often",
   "Very Often"
};
   
////////////////////////////////////////////////////////////////////////////////

static sFieldDesc ShockAIRangedFields[] = 
{
   { "Minimum Distance",  kFieldTypeInt,   FieldLocation(cShockRanged, m_minimumDist),  kFieldFlagNone },
   { "Maximum Distance",    kFieldTypeInt,   FieldLocation(cShockRanged, m_maximumDist),    kFieldFlagNone },
   { "Firing Delay",      kFieldTypeFloat, FieldLocation(cShockRanged, m_firingDelay),      kFieldFlagNone },
   { "Fire While Moving", kFieldTypeEnum,  FieldLocation(cShockRanged, m_fireWhileMoving), kFieldFlagNone, 0, 6, 6, range_values_6 },
};

static sStructDesc ShockAIRangedSDesc = StructDescBuild(cShockRanged, kStructFlagNone, ShockAIRangedFields);

////////////////////////////////////////

static sPropertyConstraint ShockAIRangedConstraints[] = 
{
   { kPropertyNullConstraint },
};

static sPropertyDesc ShockAIRangedPDesc =
{
   SHKAI_RANGED_PROP_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   ShockAIRangedConstraints,
   1, 
   0, 
   { AI_ABILITY_CAT, "Shock Ranged" },
   kPropertyChangeLocally,  // net_flags
};

class cShockAIRangedOps : public cClassDataOps<cShockRanged>
{
public:
   cShockAIRangedOps() : cClassDataOps<cShockRanged>(kNoFlags) {};
};

typedef cHashPropertyStore<cShockAIRangedOps> cShockAIRangedStore;
typedef cSpecificProperty<IShockRangedProperty, &IID_IShockRangedProperty, cShockRanged *, cShockAIRangedStore> cBaseShockAIRangedProperty;

class cShockAIRangedProperty : public cBaseShockAIRangedProperty
{
public:
   cShockAIRangedProperty(const sPropertyDesc *desc) : cBaseShockAIRangedProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(cShockRanged);
};

///////////////////////////////////////

void ShockAIInitRangedProp()
{
   Assert_(g_pShockRangedProp == NULL);
   g_pShockRangedProp = new cShockAIRangedProperty(&ShockAIRangedPDesc);
  
   StructDescRegister(&ShockAIRangedSDesc);
}

void ShockAITermRangedProp()
{
   SafeRelease(g_pShockRangedProp);
   g_pShockRangedProp = NULL;
}

sShockRanged* ShockAIGetRangedProp(ObjID objID)
{
   cShockRanged* pShockRanged;

   if (g_pShockRangedProp->Get(objID, &pShockRanged))
      return pShockRanged;
   return &g_defaultShockRanged;
}
