/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprrngd.cpp,v 1.16 1999/12/10 16:42:28 BODISAFA Exp $
//
// Ranged-combat-related properties and relations
//

#include <float.h>

#include <lg.h>

#include <comtools.h>
#include <appagg.h>

#include <linkbase.h>
#include <relation.h>
#include <linkstor.h>
#include <linkdraw.h>
#include <lnkquery.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <prophash.h>
#include <proplist.h>
#include <propert_.h>
#include <dataops_.h>
#include <bintrait.h>

#include <iobjsys.h>
#include <objdef.h>
#include <contain.h>
#include <crjoint.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <aiprops.h>
#include <aiprrngd.h>

#include <physapi.h>
#include <rendprop.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

IRelation             *g_pAIPtDibsRelation = NULL;
IRelation             *g_pAIProjectileRelation = NULL;

IVantagePtProperty    *g_pVantagePtProp = NULL;
ICoverPtProperty      *g_pCoverPtProp = NULL;
IAngleLimitProperty   *g_pAngleLimitProp = NULL;
IRangedCombatProperty *g_pRangedCombatProp = NULL;
IAIRangedRangesProperty *g_pRangedRangesProp = NULL;
IAIRangedWoundProperty *g_pRangedWoundProp = NULL;
IAIRangedWoundSoundProperty *g_pRangedWoundSoundProp = NULL;

////////////////////////////////////////////////////////////////////////////////

static char *constraint_types[] = 
{
   "None",
   "# others",
   "# misses (not implemented)"
};

// Make sure this stays in sync with eAITargetingMethod!
static char *targeting_methods[] = 
{
   "Straight-Line",
   "Arcing",
   "Reflecting",
   "Overhead"
};

static char *range_values_5[] = 
{
   "Very Low",
   "Low",
   "Moderate",
   "High",
   "Very High"
};

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

static sRelationDesc ai_point_dibs_desc = 
{
   AI_PT_DIBS_NAME,
   kRelationNetworkLocalOnly,
};

static sRelationDataDesc ai_point_dibs_data = 
{
   "None",
   0
};

void InitAIPtDibsRelation()
{
   Assert_(g_pAIPtDibsRelation == NULL);
   g_pAIPtDibsRelation = CreateStandardRelation(&ai_point_dibs_desc, &ai_point_dibs_data, kQCaseSetAll);
}

void TermAIPtDibsRelation()
{
   Assert_(g_pAIPtDibsRelation != NULL);
   SafeRelease(g_pAIPtDibsRelation);
   g_pAIPtDibsRelation = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static sRelationDesc ai_projectile_desc = 
{
   AI_PROJECTILE_REL_NAME,
   kRelationNetworkLocalOnly,
};

static sRelationDataDesc ai_projectile_data_desc = LINK_DATA_DESC_FLAGS(sAIProjectileRel, kRelationDataAutoCreate);

static sFieldDesc ai_projectile_data_fields[] =
{
   { "Constraint Type",  kFieldTypeEnum,  FieldLocation(sAIProjectileRel, constraint_type),  kFieldFlagNone, 0, 3, 3, constraint_types },
   { "Constraint Data",  kFieldTypeInt,   FieldLocation(sAIProjectileRel, data),             kFieldFlagNone },
   { "Targeting Method", kFieldTypeEnum,  FieldLocation(sAIProjectileRel, targeting_method), kFieldFlagNone, 0, 4, 4, targeting_methods },
   { "Selection Desire", kFieldTypeEnum,  FieldLocation(sAIProjectileRel, selection_desire), kFieldFlagNone, 0, 5, 5, range_values_5 },
   { "Firing Delay",     kFieldTypeFloat, FieldLocation(sAIProjectileRel, firing_delay),     kFieldFlagNone },
   { "Ammo",             kFieldTypeInt,   FieldLocation(sAIProjectileRel, ammo),             kFieldFlagNone },
   { "Burst Count",      kFieldTypeInt,   FieldLocation(sAIProjectileRel, burst_count),      kFieldFlagNone },
   { "Accuracy",         kFieldTypeEnum,  FieldLocation(sAIProjectileRel, accuracy),         kFieldFlagNone, 0, 5, 5, range_values_5 },
   { "Leads Target",     kFieldTypeBool,  FieldLocation(sAIProjectileRel, leads_target),     kFieldFlagNone },
   { "Launch Joint",     kFieldTypeEnum,  FieldLocation(sAIProjectileRel, launch_joint),     kFieldFlagNone, 0, 22, 22, g_pJointNames },
   //   { "Launch vhot",      kFieldTypeInt,   FieldLocation(sAIProjectileRel, launch_vhot),     kFieldFlagNone },
};

static sStructDesc ai_projectile_sdesc = StructDescBuild(sAIProjectileRel, kStructFlagNone, ai_projectile_data_fields);

void LGAPI AIProjectileRelationListener(sRelationListenMsg *msg, RelationListenerData data);

void InitAIProjectileRelation()
{
   Assert_(g_pAIProjectileRelation == NULL);
   g_pAIProjectileRelation = CreateStandardRelation(&ai_projectile_desc, &ai_projectile_data_desc, kQCaseSetAll);

   g_pAIProjectileRelation->Listen(kListenLinkModify | kListenLinkDeath, AIProjectileRelationListener, NULL);

   StructDescRegister(&ai_projectile_sdesc);
}

void TermAIProjectileRelation()
{
   Assert_(g_pAIProjectileRelation != NULL);
   SafeRelease(g_pAIProjectileRelation);
   g_pAIProjectileRelation = NULL;
}

BOOL AIProjRelListenerLock = FALSE;

void LGAPI AIProjectileRelationListener(sRelationListenMsg *msg, RelationListenerData data)
{
   
   if (AIProjRelListenerLock)
      return;

   if (msg->type & kListenLinkDeath)
   {
      // Delete all ammo when link snaps
      AutoAppIPtr_(ObjectSystem, pObjSys);

      if (OBJ_IS_CONCRETE(msg->link.dest) && pObjSys->Exists(msg->link.dest))
         pObjSys->Destroy(msg->link.dest);
   }

   if (msg->type & kListenLinkModify)
   {
      if (OBJ_IS_CONCRETE(msg->link.source))
      {
         sAIProjectileRel *pProjData = (sAIProjectileRel *)g_pAIProjectileRelation->GetData(msg->id);

         // Update the stack count
         if (pProjData->ammo > 0)
         {
            AutoAppIPtr_(ContainSys, pContainSys);
            IIntProperty *pStackCountProp = pContainSys->StackCountProp();
            int stack_count;
      
            pStackCountProp->Get(msg->link.dest, &stack_count);
            stack_count = pProjData->ammo;
            pStackCountProp->Set(msg->link.dest, stack_count);

            SafeRelease(pStackCountProp);
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

static sFieldDesc vantage_pt_data_fields[] =
{
   { "Value",       kFieldTypeInt,   FieldLocation(sVantagePtProp, value),       kFieldFlagNone },
   { "Decay Speed", kFieldTypeFloat, FieldLocation(sVantagePtProp, decay_speed), kFieldFlagNone },
};

static sStructDesc vantage_pt_sdesc = StructDescBuild(sVantagePtProp, kStructFlagNone, vantage_pt_data_fields);

////////////////////////////////////////

static sPropertyConstraint vantage_pt_constraints[] = 
{
   { kPropertyNullConstraint },
};

static sPropertyDesc vantage_pt_desc =
{
   AI_VANTAGE_PT_PROP_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   vantage_pt_constraints,
   1, 
   0, 
   { AI_UTILITY_CAT, "Marker: Vantage Point" },
   kPropertyChangeLocally,  // net_flags
};

class cVantagePtOps : public cClassDataOps<sVantagePtProp>
{
public:
   cVantagePtOps() : cClassDataOps<sVantagePtProp>(kNoFlags) {};
};

typedef cListPropertyStore<cVantagePtOps> cVantagePtStore;
typedef cSpecificProperty<IVantagePtProperty, &IID_IVantagePtProperty, sVantagePtProp *, cVantagePtStore> cBaseVantagePtProperty;

class cVantagePtProperty : public cBaseVantagePtProperty
{
public:
   cVantagePtProperty(const sPropertyDesc *desc) : cBaseVantagePtProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(sVantagePtProp);
};

////////////////////////////////////////

void InitVantagePointProperty()
{
   Assert_(g_pVantagePtProp == NULL);
   g_pVantagePtProp = new cVantagePtProperty(&vantage_pt_desc);

   StructDescRegister(&vantage_pt_sdesc);
}

void TermVantagePointProperty()
{
   Assert_(g_pVantagePtProp);
   SafeRelease(g_pVantagePtProp);
   g_pVantagePtProp = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static sFieldDesc cover_pt_data_fields[] =
{
   { "Value",       kFieldTypeInt,   FieldLocation(sCoverPtProp, value),       kFieldFlagNone },
   { "Decay Speed", kFieldTypeFloat, FieldLocation(sCoverPtProp, decay_speed), kFieldFlagNone },
   { "Can Duck",    kFieldTypeBool,  FieldLocation(sCoverPtProp, can_duck),    kFieldFlagNone },
};

static sStructDesc cover_pt_sdesc = StructDescBuild(sCoverPtProp, kStructFlagNone, cover_pt_data_fields);

////////////////////////////////////////

static sPropertyConstraint cover_pt_constraints[] = 
{
   { kPropertyNullConstraint },
};

static sPropertyDesc cover_pt_desc =
{
   AI_COVER_PT_PROP_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   cover_pt_constraints,
   1, 
   0, 
   { AI_UTILITY_CAT, "Marker: Cover Point" },
   kPropertyChangeLocally,  // net_flags
};

class cCoverPtOps : public cClassDataOps<sCoverPtProp>
{
public:
   cCoverPtOps() : cClassDataOps<sCoverPtProp>(kNoFlags) {};
};

typedef cListPropertyStore<cCoverPtOps> cCoverPtStore;
typedef cSpecificProperty<ICoverPtProperty, &IID_ICoverPtProperty, sCoverPtProp *, cCoverPtStore> cBaseCoverPtProperty;

class cCoverPtProperty : public cBaseCoverPtProperty
{
public:
   cCoverPtProperty(const sPropertyDesc *desc) : cBaseCoverPtProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(sCoverPtProp);
};

///////////////////////////////////////

void InitCoverPointProperty()
{
   Assert_(g_pCoverPtProp == NULL);
   g_pCoverPtProp = new cCoverPtProperty(&cover_pt_desc);

   StructDescRegister(&cover_pt_sdesc);
}

void TermCoverPointProperty()
{
   Assert_(g_pCoverPtProp);
   SafeRelease(g_pCoverPtProp);
   g_pCoverPtProp = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static sFieldDesc angle_limit_fields[] =
{
   { "Start",  kFieldTypeFloat,  FieldLocation(sAngleLimitProp, left_limit),  kFieldFlagNone },
   { "End",    kFieldTypeFloat,  FieldLocation(sAngleLimitProp, right_limit), kFieldFlagNone },
};

static sStructDesc angle_limit_sdesc = StructDescBuild(sAngleLimitProp, kStructFlagNone, angle_limit_fields);

////////////////////////////////////////

static sPropertyConstraint angle_limit_constraints[] = 
{
   { kPropertyNullConstraint },
};

static sPropertyDesc angle_limit_desc =
{
   ANGLE_LIMIT_PROP_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   angle_limit_constraints,
   1, 
   0, 
   { AI_UTILITY_CAT, "Angle Limits" },
   kPropertyChangeLocally,  // net_flags
};

class cAngleLimitOps : public cClassDataOps<sAngleLimitProp>
{
public:
   cAngleLimitOps() : cClassDataOps<sAngleLimitProp>(kNoFlags) {};
};

typedef cListPropertyStore<cAngleLimitOps> cAngleLimitStore;
typedef cSpecificProperty<IAngleLimitProperty, &IID_IAngleLimitProperty, sAngleLimitProp *, cAngleLimitStore> cBaseAngleLimitProperty;

class cAngleLimitProperty : public cBaseAngleLimitProperty
{
public:
   cAngleLimitProperty(const sPropertyDesc *desc) : cBaseAngleLimitProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(sAngleLimitProp);
};

///////////////////////////////////////

void InitAngleLimitProperty()
{
   Assert_(g_pAngleLimitProp == NULL);
   g_pAngleLimitProp = new cAngleLimitProperty(&angle_limit_desc);

   StructDescRegister(&angle_limit_sdesc);
}

void TermAngleLimitProperty()
{
   Assert_(g_pAngleLimitProp);
   SafeRelease(g_pAngleLimitProp);
   g_pAngleLimitProp = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static sFieldDesc ai_range_combat_fields[] = 
{
   { "Minimum Distance",  kFieldTypeInt,   FieldLocation(sAIRangedCombatProp, minimum_distance),  kFieldFlagNone },
   { "Ideal Distance",    kFieldTypeInt,   FieldLocation(sAIRangedCombatProp, ideal_distance),    kFieldFlagNone },
   { "Firing Delay",      kFieldTypeFloat, FieldLocation(sAIRangedCombatProp, firing_delay),      kFieldFlagNone },
   { "Cover Desire",      kFieldTypeEnum,  FieldLocation(sAIRangedCombatProp, cover_desire),      kFieldFlagNone, 0, 5, 5, range_values_5 },
   { "Decay Speed",       kFieldTypeFloat, FieldLocation(sAIRangedCombatProp, decay_speed),       kFieldFlagNone },
   { "Fire While Moving", kFieldTypeEnum,  FieldLocation(sAIRangedCombatProp, fire_while_moving), kFieldFlagNone, 0, 6, 6, range_values_6 },
   { "Contain Projectile", kFieldTypeBool,  FieldLocation(sAIRangedCombatProp, contain_projectile), kFieldFlagNone},
};

static sStructDesc ai_ranged_combat_sdesc = StructDescBuild(sAIRangedCombatProp, kStructFlagNone, ai_range_combat_fields);

////////////////////////////////////////

sAIRangedCombatProp g_defaultRangedCombatProp = {10, 40, 0., 2, 0.8, FALSE, FALSE,};

class cAIRangedCombatProp: public sAIRangedCombatProp
{
public:
   cAIRangedCombatProp() {*this = *(cAIRangedCombatProp*)&g_defaultRangedCombatProp;}
};

static sPropertyConstraint ai_ranged_combat_constraints[] = 
{
   { kPropertyNullConstraint },
};

static sPropertyDesc ai_ranged_combat_desc =
{
   AI_RANGED_COMBAT_PROP_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   ai_ranged_combat_constraints,
   1, 
   0, 
   { AI_RANGED_CAT, "Ranged Combat" },
   kPropertyChangeLocally,  // net_flags
};

class cAIRangedCombatOps : public cClassDataOps<cAIRangedCombatProp>
{
public:
   cAIRangedCombatOps() : cClassDataOps<cAIRangedCombatProp>(kNoFlags) {};
};

typedef cListPropertyStore<cAIRangedCombatOps> cAIRangedCombatStore;
typedef cSpecificProperty<IRangedCombatProperty, &IID_IRangedCombatProperty, sAIRangedCombatProp *, cAIRangedCombatStore> cBaseAIRangedCombatProperty;

class cAIRangedCombatProperty : public cBaseAIRangedCombatProperty
{
public:
   cAIRangedCombatProperty(const sPropertyDesc *desc) : cBaseAIRangedCombatProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(sAIRangedCombatProp);
};

///////////////////////////////////////

void LGAPI RangedCombatListener(sPropertyListenMsg *msg, PropListenerData data);

void InitRangedCombatProperty()
{
   Assert_(g_pRangedCombatProp == NULL);
   g_pRangedCombatProp = new cAIRangedCombatProperty(&ai_ranged_combat_desc);
  
   g_pRangedCombatProp->Listen(kListenPropSet | kListenPropUnset, RangedCombatListener, NULL);

   StructDescRegister(&ai_ranged_combat_sdesc);
}

void TermRangedCombatProperty()
{
   Assert_(g_pRangedCombatProp);
   SafeRelease(g_pRangedCombatProp);
   g_pRangedCombatProp = NULL;
}

void LGAPI RangedCombatListener(sPropertyListenMsg *msg, PropListenerData data)
{
   if ((msg->type & kListenPropUnset) && OBJ_IS_CONCRETE(msg->obj))
   {
      // Remove all ai projectile links

      ILinkQuery *query = g_pAIProjectileRelation->Query(msg->obj, LINKOBJ_WILDCARD);
      for (; !query->Done(); query->Next())
      {
         g_pAIProjectileRelation->Remove(query->ID());
      }
      SafeRelease(query);
   }

   if ((msg->type & kListenPropSet) && OBJ_IS_CONCRETE(msg->obj))
   {
      AutoAppIPtr(ContainSys);
      AutoAppIPtr_(ObjectSystem, pObjSys);
      IIntProperty *pStackCountProp;
   
      ILinkQuery *query;
      sLink link;
   
      query = g_pAIProjectileRelation->Query(msg->obj, LINKOBJ_WILDCARD);
   
      if (!query->Done())
      {
         SafeRelease(query);
         return;
      }
      SafeRelease(query);
   
      // Find all the links from the obj to its projectiles
      query = QueryInheritedLinksSingleUncached(g_pAIProjectileRelation, msg->obj, LINKOBJ_WILDCARD);
   
      for (; !query->Done(); query->Next())
      {
         ObjID new_obj;
         query->Link(&link);
      
         sAIProjectileRel *pProjRel = (sAIProjectileRel *)query->Data();
      
         // Create a concrete of this projectile
         new_obj = pObjSys->Create(link.dest, kObjectConcrete);
      
         // Add contain links
         if (((sAIRangedCombatProp*)msg->value.ptrval)->contain_projectile)
         {
            pContainSys->Add(msg->obj, new_obj, 0, CTF_COMBINE);
            if (pProjRel->ammo > 0)
            {
               pStackCountProp = pContainSys->StackCountProp();
               pStackCountProp->Set(new_obj, pProjRel->ammo);
               SafeRelease(pStackCountProp);
            }
         }
         else
         {
            ObjSetHasRefs(new_obj, FALSE);
            PhysDeregisterModel(new_obj);
         }
      
         // Instantiate projectile links
         g_pAIProjectileRelation->AddFull(msg->obj, new_obj, (void *)pProjRel); 
      }
      SafeRelease(query);
   }
}

////////////////////////////////////////////////////////////////////////////////

static sFieldDesc g_AIRangedRangesFields[] = 
{
   { "Minimum Distance",  kFieldTypeFloat,   FieldLocation(sAIRangedRanges, m_ranges[0]), kFieldFlagNone },
   { "Short Distance",    kFieldTypeFloat,   FieldLocation(sAIRangedRanges, m_ranges[1]), kFieldFlagNone },
   { "Long Distance",    kFieldTypeFloat,   FieldLocation(sAIRangedRanges, m_ranges[2]), kFieldFlagNone },
   { "Maximum Distance",  kFieldTypeFloat,   FieldLocation(sAIRangedRanges, m_ranges[3]), kFieldFlagNone },
};

static sStructDesc g_AIRangedRangesSDesc = StructDescBuild(sAIRangedRanges, kStructFlagNone, g_AIRangedRangesFields);

////////////////////////////////////////

sAIRangedRanges g_defaultRangedRanges = {0, 10, 30, FLT_MAX,};

class cAIRangedRanges: public sAIRangedRanges
{
public:
   cAIRangedRanges() {*this = *(cAIRangedRanges*)&g_defaultRangedRanges;}
};

static sPropertyDesc g_AIRangedRangesPDesc =
{
   AI_RANGED_RANGES,
   NULL,
   NULL,
   1, 
   0, 
   { AI_RANGED_CAT, "Ranged Combat Ranges" },
   kPropertyChangeLocally,  // net_flags
};

class cAIRangedRangesOps : public cClassDataOps<cAIRangedRanges>
{
public:
   cAIRangedRangesOps() : cClassDataOps<cAIRangedRanges>(kNoFlags) {};
};

typedef cHashPropertyStore<cAIRangedRangesOps> cAIRangedRangesStore;
typedef cSpecificProperty<IAIRangedRangesProperty, &IID_IAIRangedRangesProperty, sAIRangedRanges *, cAIRangedRangesStore> cBaseAIRangedRangesProperty;

class cAIRangedRangesProperty : public cBaseAIRangedRangesProperty
{
public:
   cAIRangedRangesProperty(const sPropertyDesc *desc) : cBaseAIRangedRangesProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(sAIRangedRanges);
};

void InitRangedRangesProperty(void)
{
   g_pRangedRangesProp = new cAIRangedRangesProperty(&g_AIRangedRangesPDesc);
   StructDescRegister(&g_AIRangedRangesSDesc);
}

void TermRangedRangesProperty(void)
{
   SafeRelease(g_pRangedRangesProp);
   g_pRangedRangesProp = NULL;
}

////////////////////////////////////////
//
// PROPERTY: IAIRangedWoundProperty, "AIRCWound"
//

static sFieldDesc g_AIRangedWoundFDesc[] = 
{
   { "Wound Threshold", kFieldTypeInt, FieldLocation(sAIRangedWound, m_woundThreshold),  kFieldFlagNone },
   { "Severe Threshold", kFieldTypeInt, FieldLocation(sAIRangedWound, m_severeThreshold),  kFieldFlagNone },
   { "Response Chance (%)", kFieldTypeInt, FieldLocation(sAIRangedWound, m_responseChance),  kFieldFlagNone },
};

////////////////////////////////////////

sAIRangedWound g_defaultRangedWound = {1, 10, 100};

class cAIRangedWound : public sAIRangedWound
{
public:
   cAIRangedWound() { *this = *(cAIRangedWound *)&g_defaultRangedWound; }
};

static sStructDesc g_AIRangedWoundSDesc = StructDescBuild(cAIRangedWound, kStructFlagNone, g_AIRangedWoundFDesc);

////////////////////////////////////////

class cAIRangedWoundOps : public cClassDataOps<cAIRangedWound>
{
public:
   cAIRangedWoundOps() : cClassDataOps<cAIRangedWound>(kNoFlags) {};
};

typedef cHashPropertyStore<cAIRangedWoundOps> cAIRangedWoundStore;

typedef cSpecificProperty<IAIRangedWoundProperty, &IID_IAIRangedWoundProperty, sAIRangedWound *, cAIRangedWoundStore> cBaseAIRangedWoundProp;

class cAIRangedWoundProp : public cBaseAIRangedWoundProp
{
public:
   cAIRangedWoundProp(const sPropertyDesc* desc) : cBaseAIRangedWoundProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAIRangedWound);
};

static sPropertyDesc g_AIRangedWoundPDesc = 
{
   AI_RANGED_WOUND,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_RANGED_CAT, "Ranged Combat Wound Motion" },
   kPropertyChangeLocally,  // net_flags
};

////////////////////////////////////////
//
// PROPERTY: IAIRangedWoundSoundProperty, "AIRCWndSnd"
//

typedef cSpecificProperty<IAIRangedWoundSoundProperty, &IID_IAIRangedWoundSoundProperty, sAIRangedWound *, cAIRangedWoundStore> cBaseAIRangedWoundSoundProp;

class cAIRangedWoundSoundProp : public cBaseAIRangedWoundSoundProp
{
public:
   cAIRangedWoundSoundProp(const sPropertyDesc* desc) : cBaseAIRangedWoundSoundProp(desc) {};

   STANDARD_DESCRIBE_TYPE(cAIRangedWound);
};

static sPropertyDesc g_AIRangedWoundSoundPDesc = 
{
   AI_RANGED_WOUND_SOUND,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_RANGED_CAT, "Ranged Combat Wound Sound" },
   kPropertyChangeLocally,  // net_flags
};

/////////////////////////////////

void InitRangedWoundProperty(void)
{
   g_pRangedWoundProp = new cAIRangedWoundProp(&g_AIRangedWoundPDesc);
   StructDescRegister(&g_AIRangedWoundSDesc);
   g_pRangedWoundSoundProp = new cAIRangedWoundSoundProp(&g_AIRangedWoundSoundPDesc);
}

void TermRangedWoundProperty(void)
{
   SafeRelease(g_pRangedWoundProp);
   g_pRangedWoundProp = NULL;
   SafeRelease(g_pRangedWoundSoundProp);
   g_pRangedWoundSoundProp = NULL;
}







