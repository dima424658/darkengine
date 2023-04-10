/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Ranged-combat-related properties and relations
//

#include <dpcaircp.h>

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

IDPCRangedProperty  *g_pDPCRangedProp = NULL;

////////////////////////////////////////////////////////////////////////////////

sDPCRanged g_defaultDPCRanged = {10, 40, 0, FALSE};

cDPCRanged::cDPCRanged()
{
   *this = *((cDPCRanged*)&g_defaultDPCRanged);
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

static sFieldDesc DPCAIRangedFields[] = 
{
   { "Minimum Distance",  kFieldTypeInt,   FieldLocation(cDPCRanged, m_minimumDist),        kFieldFlagNone },
   { "Maximum Distance",  kFieldTypeInt,   FieldLocation(cDPCRanged, m_maximumDist),        kFieldFlagNone },
   { "Firing Delay",      kFieldTypeFloat, FieldLocation(cDPCRanged, m_firingDelay),        kFieldFlagNone },
   { "Fire While Moving", kFieldTypeEnum,  FieldLocation(cDPCRanged, m_fireWhileMoving),    kFieldFlagNone, 0, 6, 6, range_values_6 },
};

static sStructDesc DPCAIRangedSDesc = StructDescBuild(cDPCRanged, kStructFlagNone, DPCAIRangedFields);

////////////////////////////////////////

static sPropertyConstraint DPCAIRangedConstraints[] = 
{
   { kPropertyNullConstraint },
};

static sPropertyDesc DPCAIRangedPDesc =
{
   DPCAI_RANGED_PROP_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   DPCAIRangedConstraints,
   1, 
   0, 
   { AI_ABILITY_CAT, "DPC Ranged" },
   kPropertyChangeLocally,  // net_flags
};

class cDPCAIRangedOps : public cClassDataOps<cDPCRanged>
{
public:
   cDPCAIRangedOps() : cClassDataOps<cDPCRanged>(kNoFlags) {};
};

typedef cHashPropertyStore<cDPCAIRangedOps> cDPCAIRangedStore;
typedef cSpecificProperty<IDPCRangedProperty, &IID_IDPCRangedProperty, cDPCRanged *, cDPCAIRangedStore> cBaseDPCAIRangedProperty;

class cDPCAIRangedProperty : public cBaseDPCAIRangedProperty
{
public:
   cDPCAIRangedProperty(const sPropertyDesc *desc) : cBaseDPCAIRangedProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(cDPCRanged);
};

///////////////////////////////////////

void DPCAIInitRangedProp()
{
   Assert_(g_pDPCRangedProp == NULL);
   g_pDPCRangedProp = new cDPCAIRangedProperty(&DPCAIRangedPDesc);
  
   StructDescRegister(&DPCAIRangedSDesc);
}

void DPCAITermRangedProp()
{
   SafeRelease(g_pDPCRangedProp);
   g_pDPCRangedProp = NULL;
}

sDPCRanged* DPCAIGetRangedProp(ObjID objID)
{
   cDPCRanged* pDPCRanged;

   if (g_pDPCRangedProp->Get(objID, &pDPCRanged))
      return pDPCRanged;
   return &g_defaultDPCRanged;
}
