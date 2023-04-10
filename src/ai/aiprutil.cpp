/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprutil.cpp,v 1.11 2000/03/25 22:15:36 adurant Exp $
//
//
//

#include <lg.h>

#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <propfac_.h> //cGenericBoolProperty 
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiprops.h>
#include <aiprrngd.h>
#include <aiprutil.h>
#include <aisndtyp.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

IAISoundTypeProperty * g_pAISoundTypeProperty;
IBoolProperty *        g_pStimKnockoutProperty;
static IBoolProperty *g_pAIShootsThroughProp = NULL;

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAISoundTypeProperty
//

static sPropertyDesc g_AISoundTypePropertyDesc =
{
   PROP_AI_SNDTYPE,
   0,
   NULL, 2, 1,  // constraints, version
   { AI_UTILITY_CAT, "Sound value" },
   kPropertyChangeLocally,  // net_flags
};


///////////////////////////////////////

static const char * _g_ppszAISoundTypes[] = 
{
   "None",
   "Inform",
   "Minor anomaly",
   "Major anomaly",
   "Non-combat high",
   "Combat high", 
};

///////////////////////////////////////

static sFieldDesc _g_AISoundTypeFieldDesc[] = 
{
   { "Type", kFieldTypeEnum,  FieldLocation(sAISoundType, type), kFieldFlagNone, 0, FieldNames(_g_ppszAISoundTypes) },
   { "Signal", kFieldTypeString, FieldLocation(sAISoundType, szSignal)                                              },
}; 

static sStructDesc _g_AISoundTypeStructDesc = StructDescBuild(sAISoundType, kStructFlagNone, _g_AISoundTypeFieldDesc);

///////////////////////////////////////

typedef cSpecificProperty<IAISoundTypeProperty, &IID_IAISoundTypeProperty, sAISoundType *, cHashPropertyStore< cClassDataOps<sAISoundType> > > cAISoundTypePropertyBase;

class cAISoundTypeProperty : public cAISoundTypePropertyBase
{
public:

   cAISoundTypeProperty()
      : cAISoundTypePropertyBase(&g_AISoundTypePropertyDesc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sAISoundType);
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: StimKnockoutProperty
//

static sPropertyDesc g_AIStimKnockoutPropertyDesc = 
{
   PROP_STIM_KNOCKOUT,
   0,
   NULL, 0, 0,
   { AI_UTILITY_CAT, "Is Knockout" },
   kPropertyChangeLocally,  // net_flags
};

////////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: AIShootsThroughProperty
// Property for whether Ranged AI's ignore object when trying
// to decide if they have a clear line of fire.

// to speed up the AI physcast 
static cDynArray<ObjID> m_AIShootsThroughObjects;

// for DynArray ops
static int ShootCompare(const ObjID *pLeft, const ObjID *pRight);
static int ShootSearch(const void *pKey, const ObjID *pRight);


static sPropertyDesc AIShootsThroughDesc = 
{
   PROP_AI_SHOOTS_THROUGH_NAME,
   0,
   NULL,
   0,
   0,
   { AI_UTILITY_CAT, "AI Fires Through"},
};


typedef cGenericBoolProperty cAIShootsThroughPropBase;

class cAIShootsThroughProp : public cAIShootsThroughPropBase
{
public:
   cAIShootsThroughProp (const sPropertyDesc* desc)
      : cAIShootsThroughPropBase(desc,kPropertyImplSparseHash)
   {
      SetRebuildConcretes(TRUE);
   }
   
   virtual void RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue, ObjID donor);
};

///////////////////////////////////////

void cAIShootsThroughProp::RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue val, ObjID donor)
{
   BOOL shootsthrough=(BOOL)val.intval; //default value     
   
   int objindex = m_AIShootsThroughObjects.BSearch((void *)obj,ShootSearch);

   BOOL fHad = (objindex != BAD_INDEX);

   if ((fIsRelevant) && (shootsthrough) && (!fHad))
   {
      m_AIShootsThroughObjects.Append(obj);
      m_AIShootsThroughObjects.Sort(ShootCompare);
   }
   else if (fHad && (!fIsRelevant || !shootsthrough))
   {
      m_AIShootsThroughObjects.DeleteItem(objindex);
      //doesn't now, did before, delete it.  Still sorted.
   }
   else if (fHad && !ObjExists(obj))
   {
      m_AIShootsThroughObjects.DeleteItem(objindex);
      //nonexistent, delete it.  Still sorted.
   }      

   cAIShootsThroughPropBase::RebuildConcrete(obj, fIsRelevant, val, donor);
};


void InitAIShootsThroughProp()
{
   m_AIShootsThroughObjects.SetSize(0);
   g_pAIShootsThroughProp = new cAIShootsThroughProp(&AIShootsThroughDesc);
}

void TermAIShootsThroughProp()
{
}

static void AIDBResetShootsThroughProp()
{
   m_AIShootsThroughObjects.SetSize(0);
}

////////////////////////////////////////
// For concrete search

static int ShootCompare(const ObjID *pLeft, const ObjID *pRight)
{
   if (*pLeft < *pRight)
      return -1;
   else if (*pLeft > *pRight)
      return 1;
   else
      return 0;
}

static int ShootSearch(const void *pKey, const ObjID *pRight)
{
   if (((ObjID)(pKey)) < *pRight)
      return -1;
   else if (((ObjID)(pKey)) > *pRight)
      return 1;
   else
      return 0;
}

//just lookup the index
static BOOL ObjectCanBeFiredThrough(ObjID object)
{
   int nIndex = m_AIShootsThroughObjects.BSearch((void *)object,ShootSearch);
   return (nIndex != BAD_INDEX);
}

void SetAIShootsThrough(ObjID object, BOOL shootsthrough)
{
   g_pAIShootsThroughProp->Set(object, shootsthrough);
}

BOOL GetAIShootsThrough(ObjID object)
{
   return ObjectCanBeFiredThrough(object);
}

///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void AIInitUtilProps()
{
   StructDescRegister(&_g_AISoundTypeStructDesc);

   g_pAISoundTypeProperty = new cAISoundTypeProperty;

   g_pStimKnockoutProperty = CreateBoolProperty(&g_AIStimKnockoutPropertyDesc, kPropertyImplVerySparse);

   InitAIShootsThroughProp();
   InitVantagePointProperty();
   InitCoverPointProperty();
   InitAngleLimitProperty();

   InitAIPtDibsRelation();
}

///////////////////////////////////////

void AITermUtilProps()
{
   SafeRelease(g_pAISoundTypeProperty);

   SafeRelease(g_pStimKnockoutProperty);

   TermAIShootsThroughProp();
   TermVantagePointProperty();
   TermCoverPointProperty();
   TermAngleLimitProperty();

   TermAIPtDibsRelation();
}

///////////////////////////////////////

void AIDBResetUtilProps()
{
   AIDBResetShootsThroughProp();
}

///////////////////////////////////////////////////////////////////////////////




