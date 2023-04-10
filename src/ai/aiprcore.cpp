/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprcore.cpp,v 1.42 2000/03/06 17:19:20 toml Exp $
//
// @TBD (toml 06-19-98):  the whole "derive an init class and init from global" property (e.g.,efficiency) could be a single template
//
//

#include <lg.h>
#include <str.h>

#include <playrobj.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aialert.h>
#include <aiaware.h>
#include <aiapi.h>
#include <aiapibhv.h>
#include <aiapiiai.h>
#include <aiprconv.h>
#include <aiprops.h>
#include <aiprcore.h>
#include <aiprdev.h>
#include <aiteams.h>
#include <aivision.h>

#include <isdescty.h>
#include <isdescst.h>
#include <isdesced.h>
#include <simpwrap.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

static sPropertyDesc _g_AITrackMediumPropertyDesc =
{
  PROP_AI_TRACK_MEDIUM,
  0,
  NULL, 0, 0,
  { AI_UTILITY_CAT, "Track Medium" },
  kPropertyChangeLocally,
};

static sPropertyDesc _g_AICanPathWaterProperty =
{
  PROP_AI_USEWATER,
  0,
  NULL, 0, 0,
  { AI_UTILITY_CAT, "Path Water" },
  kPropertyChangeLocally,
};


static sPropertyDesc _g_AINeedsBigDoors =
{
  PROP_AI_AIISLARGE,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "AI Needs Big Doors" },
  kPropertyChangeLocally,
};


static sPropertyDesc _g_AIIgnoresCamerasProperty =
{
  PROP_IGNORES_CAMERAS,
  0,
  NULL, 0, 0,
  { AI_UTILITY_CAT, "Ignores Cameras" },
  kPropertyChangeLocally,
};


static sPropertyDesc _g_AINoGrowOBBPropertyDesc =
{
  "AI_NGOBB",
  0,
  NULL, 0, 0,
  { AI_UTILITY_CAT, "Path Exact OBB" },
  kPropertyChangeLocally,
};


IBoolProperty *         g_pAITrackMediumProperty = NULL;

IAIProperty *           g_pAIProperty           = NULL;
IAIEfficiencyProperty * g_pAIEfficiencyProperty = NULL;

IBoolProperty *         g_pAICanPathWaterProperty = NULL;

IBoolProperty *         g_pAINeedsLargeDoors = NULL;

IBoolProperty * g_pAIIgnoresCamerasProperty = NULL;


IFloatProperty *        g_pAIMoveZOffsetProperty;
IFloatProperty *        g_pAIMoveSpeedProperty;
IFloatProperty *        g_pAITurnRateProperty;

IIntProperty *          g_pAITeamProperty;

IIntProperty *          g_pAIVisionJointProperty;

IStringProperty *       g_pAIStandTagsProperty;
IStringProperty *       g_pAISoundTagsProperty;
IStringProperty *       g_pAIMotionTagsProperty;

IBoolProperty *         g_pAINoGhostProperty;
IBoolProperty *         g_pAINoHandoffProperty;
IBoolProperty *         g_pAIIsProxyProperty;

IBoolProperty *         g_pAIForceImmediateInform;
IBoolProperty *         g_pAIInformSeenFrom;
IBoolProperty *         g_pAIIsSmallCreatureProp;

IAIAlertnessProperty *  g_pAIAlertnessProperty;
IAIAlertCapProperty *   g_pAIAlertCapProperty;
IAIAwareDelayProperty * g_pAIAwareDelayProperty;

IAIModeProperty *       g_pAIModeProperty;
IAIVisibilityProperty * g_pAIVisibilityProperty;

IAIVisionDescProperty * g_pAIVisionDescProperty;
IAIVisibilityModProperty * g_pAIVisibilityModProperty;
IAIAlertSenseMultsProperty *    g_pAIAlertSenseMultsProperty;
IIntProperty *          g_pAIVisionTypeProperty;
IAIVisCtrlProperty *    g_pAIVisCtrlProperty;
IAIAwareCapProperty *   g_pAIAwareCapProperty;

IAIFrustratedProperty *         g_pAIFrustratedProperty;

IBoolProperty *         g_pAINoGrowOBBProperty = NULL; // A T2 hack property for Bank to get shipped (toml 03-06-00)

BOOL AIPathExactOBB(ObjID object) 
{ 
   BOOL result = FALSE;
   g_pAINoGrowOBBProperty->Get(object, &result);
   return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIProperty, "AI", sAIProp, cAIProperty
//
// Contains functionality for keeping set of C++ runtime objects consistent
// with concrete objects in the hierarchy.
//

static sPropertyDesc g_AIPropertyDesc =
{
   PROP_AI,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "AI" },
};

///////////////////////////////////////

typedef cSpecificProperty<IAIProperty, &IID_IAIProperty, sAIProp *, cHashPropertyStore< cClassDataOps<sAIProp> > > cAIPropertyBase;

class cAIProperty : public cAIPropertyBase
{
public:

   cAIProperty()
      : cAIPropertyBase(&g_AIPropertyDesc)
   {
      SetRebuildConcretes(TRUE);
   };

   STANDARD_DESCRIBE_TYPE(sAIProp);

private:

   virtual void RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue, ObjID donor);
   virtual void OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue val);

};

///////////////////////////////////////

void cAIProperty::RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue val, ObjID donor)
{
   AutoAppIPtr(AIManager);

   sAIProp *        pAIProp = (sAIProp *) val.ptrval;
   IInternalAI *    pAI;
   IAIBehaviorSet * pBehaviorSet;
   BOOL             fWasAI;

   pAI = (IInternalAI *)pAIManager->GetAI(obj);

   if (pAI)
   {
      fWasAI = TRUE;
      pBehaviorSet = pAI->AccessBehaviorSet();
      SafeRelease(pAI);
   }
   else
      fWasAI = FALSE;

   if (fIsRelevant)
   {
      IProperty * pPuppetProperty = GetPropertyNamed("Puppet");

      if (!fWasAI)
      {
         if (!pPuppetProperty || !pPuppetProperty->IsRelevant(obj))
            pAIManager->CreateAI(obj, pAIProp->szBehaviorSet);
      }
      else if (stricmp(pBehaviorSet->GetName(), pAIProp->szBehaviorSet) != 0)
      {
         pAIManager->DestroyAI(obj);
         if (!pPuppetProperty || !pPuppetProperty->IsRelevant(obj))
            pAIManager->CreateAI(obj, pAIProp->szBehaviorSet);
      }

      SafeRelease(pPuppetProperty);
   }
   else if (fWasAI)
   {
      pAIManager->DestroyAI(obj);
   }
   cAIPropertyBase::RebuildConcrete(obj,fIsRelevant,val,donor);
}

///////////////////////////////////////

void cAIProperty::OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue val)
{
   cAIPropertyBase::OnListenMsg(type, obj, val);

   // Fix-up the behavior set: trim out extra space, look for an empty field and default
   sAIProp * pAIProp = (sAIProp *) val.ptrval;

   if ((type & (kListenPropModify | kListenPropSet)) && pAIProp)
   {
      cStr behaviorSetStr(pAIProp->szBehaviorSet);
      behaviorSetStr.Trim();

      strncpy(pAIProp->szBehaviorSet,
              (behaviorSetStr.GetLength()) ? behaviorSetStr.BufIn() : g_pszAIDefBehaviorSet,
              sizeof(pAIProp->szBehaviorSet) - 1);
   }
}

///////////////////////////////////////
//
// Structure descriptor
//

static sFieldDesc g_AIPropFieldDesc [] =
{
   { "Behavior set",  kFieldTypeString, FieldLocation(sAIProp, szBehaviorSet) },
};

static sStructDesc _g_AIPropStructDesc = StructDescBuild(sAIProp, kStructFlagNone, g_AIPropFieldDesc);

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIEfficiency, "AI_Efficiency", sAIEfficiency
//

sAIEfficiency g_AIDefaultEfficiency =
{
   TRUE,

   // Super Efficiency limits
   sq(150.0), // distEnterSuperEffSq
   45.0,  // zEnterSuperEff

   sq(120.0), // distExitSuperEffSq
   30.0,  // zExitSuperEff

   // Efficiency limits
   sq(120.0), // distEnterEffSq
   30.0,  // zEnterEff

   sq(90.0),  // distExitEffSq
   20.0,  // zExitEff
};

///////////////////////////////////////

class cAIEfficiency : public sAIEfficiency
{
public:
   cAIEfficiency()
   {
      *((sAIEfficiency *)this) = g_AIDefaultEfficiency;
   }
};

///////////////////////////////////////

static sPropertyDesc g_AIEfficiencyPropertyDesc =
{
   PROP_AI_EFFICIENCY,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Efficiency settings" },
   kPropertyChangeLocally,  // netflags
};

///////////////////////////////////////

typedef cSpecificProperty<IAIEfficiencyProperty, &IID_IAIEfficiencyProperty, sAIEfficiency *, cHashPropertyStore< cNoZeroDataOps<cAIEfficiency> > > cAIEfficiencyPropertyBase;

class cAIEfficiencyProperty : public cAIEfficiencyPropertyBase
{
public:
   cAIEfficiencyProperty()
      : cAIEfficiencyPropertyBase(&g_AIEfficiencyPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIEfficiency);
};

///////////////////////////////////////
//
// Structure descriptor
//

static sFieldDesc g_AIEfficiencyFieldDesc [] =
{
   { "Enabled",  kFieldTypeBool, FieldLocation(sAIEfficiency, fEnabled) },

   // Super Efficiency limits
   { "Super effic. Enter dist sq.", kFieldTypeFloat, FieldLocation(sAIEfficiency, distEnterSuperEffSq)   },
   { "Super effic. Enter Z",        kFieldTypeFloat, FieldLocation(sAIEfficiency, zEnterSuperEff)        },

   { "Super effic. Exit dist sq.",  kFieldTypeFloat, FieldLocation(sAIEfficiency, distExitSuperEffSq)    },
   { "Super effic. Exit Z",         kFieldTypeFloat, FieldLocation(sAIEfficiency, zExitSuperEff)         },

   // Efficiency limits
   { "Effic. Enter dist sq.",       kFieldTypeFloat, FieldLocation(sAIEfficiency, distEnterEffSq)        },
   { "effic. Enter Z",              kFieldTypeFloat, FieldLocation(sAIEfficiency, zEnterEff)             },

   { "Effic. Exit dist sq.",        kFieldTypeFloat, FieldLocation(sAIEfficiency, distExitEffSq)         },
   { "Effic. Exit Z",               kFieldTypeFloat, FieldLocation(sAIEfficiency, zExitEff)              },
};

static sStructDesc _g_AIEfficiencyStructDesc = StructDescBuild(sAIEfficiency, kStructFlagNone, g_AIEfficiencyFieldDesc);



///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_MoveZOffset", float
//

static sPropertyDesc g_AIMoveZOffsetPropertyDesc =
{
  PROP_AI_MOVE_ZOFFSET,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Movement: z offset" },
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_MoveSpeed", float
//

static sPropertyDesc g_AIMoveSpeedPropertyDesc =
{
  PROP_AI_MOVE_SPEED,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Movement: max speed" },
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_TurnRate", float
//

static sPropertyDesc g_AITurnRatePropertyDesc =
{
  PROP_AI_TURN_RATE,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Movement: turn rate" },
};

///////////////////////////////////////
//
// PROPERTY: "AI_Team", int
//

#define PROP_AI_TEAM "AI_Team"

static sPropertyDesc g_AITeamPropertyDesc =
{
  PROP_AI_TEAM,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Team" },
   kPropertyChangeLocally,  // netflags
};

static sPropertyTypeDesc g_AITeamPropertyTypeDesc =
{
   "eAITeam",
   sizeof(int),
};

///////////////////////////////////////

static sFieldDesc _g_AITeamFieldDesc[] =
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, kAIT_NumTeams, kAIT_NumTeams, g_ppszAITeams },
};

static sStructDesc _g_AITeamStructDesc =
{
   "eAITeam",
   sizeof(int),
   kStructFlagNone,
   sizeof(_g_AITeamFieldDesc)/sizeof(_g_AITeamFieldDesc[0]),
   _g_AITeamFieldDesc,
};

///////////////////////////////////////

int _AIGetTeam(ObjID obj)
{
   int result = kAIT_NumTeams;
   g_pAITeamProperty->Get(obj, &result);
   if (result == kAIT_NumTeams)
   {
      if (IsAPlayer(obj))
         return kAIT_Good;

      if (ObjIsAI(obj))
         return kAIT_Bad1;

      return kAIT_Neutral;
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// Whether the AI should play the stand motion
//

EXTERN const char * g_pszAIDefStandTags = "Stand 0";

static sPropertyDesc _g_AIStandTagsPropertyDesc =
{
  "AI_StandTags",
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Standing motion tags" },
};

///////////////////////////////////////////////////////////////////////////////
//
// Sound/motion tags
//

#define PROP_AI_SND_TAGS "AI_SndTags"

static sPropertyDesc _g_AISoundTagsPropertyDesc =
{
  PROP_AI_SND_TAGS,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Sound tags" },
};

#define PROP_AI_MOT_TAGS "AI_MotTags"

static sPropertyDesc _g_AIMotionTagsPropertyDesc =
{
  PROP_AI_MOT_TAGS,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Motion tags" },
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_NoGhost", BOOL
//

static sPropertyDesc _g_AINoGhostPropertyDesc =
{
  PROP_AI_NO_GHOST,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "No Multiplayer Ghost" },
  kPropertyChangeLocally,
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_NoHandoff", BOOL
//

static sPropertyDesc _g_AINoHandoffPropertyDesc =
{
  PROP_AI_NO_HANDOFF,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "No Multiplayer Handoff" },
  kPropertyChangeLocally,
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_IsProxy", BOOL
//

static sPropertyDesc _g_AIIsProxyPropertyDesc =
{
  PROP_AI_IS_PROXY,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Handed-off proxy" },
  kPropertyChangeLocally,
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: "AI_IsFrustrated", BOOL
//

static sPropertyDesc _g_AIFrustratedPropertyDesc =
{
  PROP_AI_IS_FRUSTRATED,
  0,
  NULL, 0, 0,
  { AI_UTILITY_CAT, "Is Frustrated" },
  kPropertyChangeLocally,
};

///////////////////////////////////////

typedef cSpecificProperty<IAIFrustratedProperty, &IID_IAIFrustratedProperty, sAIFrustrated *, cHashPropertyStore< cNoZeroDataOps<sAIFrustrated> > > cAIFrustratedPropertyBase;

class cAIFrustratedProperty : public cAIFrustratedPropertyBase
{
public:
   cAIFrustratedProperty()
      : cAIFrustratedPropertyBase(&_g_AIFrustratedPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIFrustrated);
};

///////////////////////////////////////
//
// Structure descriptor
//

static sFieldDesc g_AIFrustratedFieldDesc [] =
{
   { "Source",      kFieldTypeInt,  FieldLocation(sAIFrustrated, nSourceId)},
   { "Dest",        kFieldTypeInt,  FieldLocation(sAIFrustrated, nDestId)},
};

static sStructDesc _g_AIFrustratedStructDesc = StructDescBuild(sAIFrustrated, kStructFlagNone, g_AIFrustratedFieldDesc);


///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIAlertnessProperty, "AI_Alertness", sAIAlertness
//

const char * g_ppszAIAwareAlertLevels[kAIAL_Num] =
{
   "(0) None",
   "(1) Low",
   "(2) Moderate",
   "(3) High"
};

///////////////////////////////////////

static sPropertyDesc g_AIAlertnessPropertyDesc =
{
   PROP_AI_ALERTNESS,
   kPropertyInstantiate,
   NULL, 2, 1,  // constraints, version
   { AI_STATE_CAT, "Current alertness" },
   kPropertyChangeLocally,  // netflags
};

///////////////////////////////////////

typedef cSpecificProperty<IAIAlertnessProperty, &IID_IAIAlertnessProperty, sAIAlertness *, cHashPropertyStore< cNoZeroDataOps<sAIAlertness> > > cAIAlertnessPropertyBase;

class cAIAlertnessProperty : public cAIAlertnessPropertyBase
{
public:
   cAIAlertnessProperty()
      : cAIAlertnessPropertyBase(&g_AIAlertnessPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIAlertness);
};

///////////////////////////////////////
//
// Structure descriptor
//

static sFieldDesc g_AIAlertnessFieldDesc [] =
{
   { "Level",       kFieldTypeEnum,  FieldLocation(sAIAlertness, level),     kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "Peak",        kFieldTypeEnum,  FieldLocation(sAIAlertness, peak),      kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
};

static sStructDesc _g_AIAlertnessStructDesc = StructDescBuild(sAIAlertness, kStructFlagNone, g_AIAlertnessFieldDesc);

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIAlertCapProperty, "AI_AlertCap", sAIAlertCap
//

static sAIAlertCap g_AIDefAlertCap;
sAIAlertCap * g_pAIDefAlertCap = &g_AIDefAlertCap;

///////////////////////////////////////

static sPropertyDesc g_AIAlertCapPropertyDesc =
{
   "AI_AlertCap",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Alertness cap" },
};

///////////////////////////////////////

typedef cSpecificProperty<IAIAlertCapProperty, &IID_IAIAlertCapProperty, sAIAlertCap *, cHashPropertyStore< cNoZeroDataOps<sAIAlertCap> > > cAIAlertCapPropertyBase;

class cAIAlertCapProperty : public cAIAlertCapPropertyBase
{
public:
   cAIAlertCapProperty()
      : cAIAlertCapPropertyBase(&g_AIAlertCapPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIAlertCap);
};

///////////////////////////////////////
//
// Structure descriptor
//

static sFieldDesc g_AIAlertCapFieldDesc [] =
{
   { "Max level",             kFieldTypeEnum,  FieldLocation(sAIAlertCap, maxLevel),  kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "Min level",             kFieldTypeEnum,  FieldLocation(sAIAlertCap, minLevel),  kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
   { "Min relax after peak",  kFieldTypeEnum,  FieldLocation(sAIAlertCap, minRelax),  kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },
};

static sStructDesc _g_AIAlertCapStructDesc = StructDescBuild(sAIAlertCap, kStructFlagNone, g_AIAlertCapFieldDesc);

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIAwareDelayProperty, "AI_AlertCap", sAIAwareDelay
//

static sAIAwareDelay g_AIDefAwareDelay =
{
   750,
   500,

   12000,
   22000,

   9
};

sAIAwareDelay * g_pAIDefAwareDelay = &g_AIDefAwareDelay;

///////////////////////////////////////

static sPropertyDesc g_AIAwareDelayPropertyDesc =
{
   "AI_AwrDel2",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Awareness delay (react time)" },
};

///////////////////////////////////////

typedef cSpecificProperty<IAIAwareDelayProperty, &IID_IAIAwareDelayProperty, sAIAwareDelay *, cHashPropertyStore< cClassDataOps<sAIAwareDelay> > > cAIAwareDelayPropertyBase;

class cAIAwareDelayProperty : public cAIAwareDelayPropertyBase
{
public:
   cAIAwareDelayProperty()
      : cAIAwareDelayPropertyBase(&g_AIAwareDelayPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIAwareDelay);
};

///////////////////////////////////////
//
// Structure descriptor
//

static sFieldDesc g_AIAwareDelayFieldDesc [] =
{
   { "Reaction time to two",    kFieldTypeInt, FieldLocation(sAIAwareDelay, toTwo) },
   { "Reaction time to three",  kFieldTypeInt, FieldLocation(sAIAwareDelay, toThree) },

   { "Retrigger two delay",     kFieldTypeInt, FieldLocation(sAIAwareDelay, twoReuse) },
   { "Retrigger three delay",   kFieldTypeInt, FieldLocation(sAIAwareDelay, threeReuse) },

   { "Ignore delay range",      kFieldTypeInt, FieldLocation(sAIAwareDelay, ignoreRange) },

};

static sStructDesc _g_AIAwareDelayStructDesc = StructDescBuild(sAIAwareDelay, kStructFlagNone, g_AIAwareDelayFieldDesc);

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIVisibilityProperty, "AI_Visibility", sAIVisibility
//

static sPropertyDesc g_AIVisibilityPropertyDesc =
{
   PROP_AI_VISIBILITY,
   kPropertyInstantiate | kPropertyTransient,
   NULL, 0, 0,  // constraints, version
   { AI_STATE_CAT, "Current visibility" },
   kPropertyChangeLocally,  // netflags
};

///////////////////////////////////////

typedef cSpecificProperty<IAIVisibilityProperty, &IID_IAIVisibilityProperty, sAIVisibility *, cHashPropertyStore< cClassDataOps<sAIVisibility> > > cAIVisibilityPropertyBase;

class cAIVisibilityProperty : public cAIVisibilityPropertyBase
{
public:
   cAIVisibilityProperty()
      : cAIVisibilityPropertyBase(&g_AIVisibilityPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIVisibility);
};

///////////////////////////////////////
//
// Structure descriptor
//

static sFieldDesc g_AIVisibilityFieldDesc [] =
{
   { "Level",             kFieldTypeInt, FieldLocation(sAIVisibility, level) },
   { "Light rating",      kFieldTypeInt, FieldLocation(sAIVisibility, lighting) },
   { "Movement rating",   kFieldTypeInt, FieldLocation(sAIVisibility, movement) },
   { "Exposure rating",   kFieldTypeInt, FieldLocation(sAIVisibility, exposure) },
   { "Last update time",  kFieldTypeInt, FieldLocation(sAIVisibility, updateTime) },
};

static sStructDesc _g_AIVisibilityStructDesc = StructDescBuild(sAIVisibility, kStructFlagNone, g_AIVisibilityFieldDesc);

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIModeProperty, "AI_Mode", sAIMode
//

static sPropertyDesc g_AIModePropertyDesc =
{
   PROP_AI_MODE,
   kPropertyInstantiate,
   NULL, 0, 0,  // constraints, version
   { AI_STATE_CAT, "Current mode" },
   kPropertyChangeLocally,  // netflags
};


///////////////////////////////////////

static const char * _g_ppszAIModes[kAIM_Num] =
{
   "Asleep",
   "Super efficient",
   "Efficient",
   "Normal",
   "Combat",
   "Dead"
};

const char * AIGetModeName(eAIMode mode)
{
   return _g_ppszAIModes[mode];
}

///////////////////////////////////////

static sFieldDesc _g_AIModeFieldDesc[] =
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, kAIM_Num, kAIM_Num, _g_ppszAIModes },
};

static sStructDesc _g_AIModeStructDesc =
{
   "eAIMode",
   sizeof(int),
   kStructFlagNone,
   sizeof(_g_AIModeFieldDesc)/sizeof(_g_AIModeFieldDesc[0]),
   _g_AIModeFieldDesc,
};

///////////////////////////////////////

class cAIModeDataOps : public cSimpleDataOps
{
public:
   STDMETHOD_(sDatum,New)()
   {
      return (void *)(kAIM_Normal);
   }
};

///////////////////////////////////////

typedef cSpecificProperty<IAIModeProperty, &IID_IAIModeProperty, eAIMode, cHashPropertyStore< cAIModeDataOps > > cAIModePropertyBase;

class cAIModeProperty : public cAIModePropertyBase
{
public:

   cAIModeProperty()
      : cAIModePropertyBase(&g_AIModePropertyDesc)
   {
   }

   STANDARD_DESCRIBE_TYPE(eAIMode);
};

///////////////////////////////////////
//
// PROPERTY: "Vision Type", int
//

static sPropertyDesc g_AIVisionTypePropertyDesc =
{
  PROP_AI_VISTYPE,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Vision Type" },
   kPropertyChangeLocally,  // netflags
};

static sPropertyTypeDesc g_AIVisionTypePropertyTypeDesc = {PROP_AI_VISTYPE, sizeof(int)};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIVisionDescProperty
//

static sPropertyDesc g_AIVisionDescPropertyDesc =
{
   PROP_AI_VISDESC,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Vision description" },
   kPropertyChangeLocally,  // netflags
};

///////////////////////////////////////

static const char * g_pszAIVisConeFlags[] =
{
   "Active",                                     // kAIVC_Active
   "No alert 0",
   "No alert 1",
   "No alert 2",
   "No alert 3",
   "Peripheral",
   "Omni",
   "Night vision",
   "Behind",
};

///////////////////////////////////////////////////////////////////////////////

#define VIS_CONE_FIELDS(i) \
   { "Cone " #i ": Flags",               kFieldTypeBits,   FieldLocation(sAIVisionDesc, cones[i-1].flags), kFieldFlagUnsigned, 0, FieldNames(g_pszAIVisConeFlags)}, \
   { "Cone " #i ": Angle",               kFieldTypeInt,    FieldLocation(sAIVisionDesc, cones[i-1].angle)                                                        }, \
   { "Cone " #i ": Z angle",             kFieldTypeInt,    FieldLocation(sAIVisionDesc, cones[i-1].zAngle)                                                       }, \
   { "Cone " #i ": Range",               kFieldTypeInt,    FieldLocation(sAIVisionDesc, cones[i-1].range)                                                        }, \
   { "Cone " #i ": Acuity",              kFieldTypeInt,    FieldLocation(sAIVisionDesc, cones[i-1].acuity)                                                       }

static sFieldDesc _g_AIVisionDescFieldDesc[] =
{
   {"Z Offset", kFieldTypeFloat, FieldLocation(sAIVisionDesc, zOffset),},
   VIS_CONE_FIELDS(1),
   VIS_CONE_FIELDS(2),
   VIS_CONE_FIELDS(3),
   VIS_CONE_FIELDS(4),
   VIS_CONE_FIELDS(5),
   VIS_CONE_FIELDS(6),
   VIS_CONE_FIELDS(7),
   VIS_CONE_FIELDS(8),
   VIS_CONE_FIELDS(9),
   VIS_CONE_FIELDS(10),
};

static sStructDesc _g_AIVisionDescStructDesc = StructDescBuild(sAIVisionDesc, kStructFlagNone, _g_AIVisionDescFieldDesc);

///////////////////////////////////////

class cAIVisionDesc : public sAIVisionDesc
{
public:
   cAIVisionDesc()
   {
      *((sAIVisionDesc *)this) = g_AIDefVisionDesc;
   }
};

///////////////////////////////////////

typedef cSpecificProperty<IAIVisionDescProperty, &IID_IAIVisionDescProperty, sAIVisionDesc *, cHashPropertyStore< cNoZeroDataOps<cAIVisionDesc> > > cAIVisionDescPropertyBase;

class cAIVisionDescProperty : public cAIVisionDescPropertyBase
{
public:
   cAIVisionDescProperty()
      : cAIVisionDescPropertyBase(&g_AIVisionDescPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIVisionDesc);
};

///////////////////////////////////////////////////////////////////////////////
//
// Alertness sense multipliers
//

static sPropertyDesc g_AIAlertSenseMultsPropertyDesc =
{
   "AI_AlSnMul",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Alertness sense multipliers" },
};

///////////////////////////////////////

#define ALERT_SENSE_MULT_FIELDS(i) \
   { "Alert " #i ": XY angle mult",   kFieldTypeFloat,  FieldLocation(sAIAlertSenseMults, mults[i].XYMult)   }, \
   { "Alert " #i ": Z angle mult",     kFieldTypeFloat,  FieldLocation(sAIAlertSenseMults, mults[i].ZMult)   }, \
   { "Alert " #i ": Range mult",       kFieldTypeFloat,  FieldLocation(sAIAlertSenseMults, mults[i].RMult)  }, \
   { "Alert " #i ": Knowledge mult",   kFieldTypeFloat,  FieldLocation(sAIAlertSenseMults, mults[i].KnowledgeMult)  }

static sFieldDesc _g_AIAlertSenseMultsFieldDesc[] =
{
   ALERT_SENSE_MULT_FIELDS(0),
   ALERT_SENSE_MULT_FIELDS(1),
   ALERT_SENSE_MULT_FIELDS(2),
   ALERT_SENSE_MULT_FIELDS(3),
   {"Combat knowledge mult", kFieldTypeFloat, FieldLocation(sAIAlertSenseMults, CombatKnowledgeMult),},
};

static sStructDesc _g_AIAlertSenseMultsStructDesc = StructDescBuild(sAIAlertSenseMults, kStructFlagNone, _g_AIAlertSenseMultsFieldDesc);

///////////////////////////////////////

class cAIAlertSenseMults : public sAIAlertSenseMults
{
public:
   cAIAlertSenseMults()
   {
      *((sAIAlertSenseMults *)this) = g_AIDefAlertSenseMults;
   }
};

///////////////////////////////////////

typedef cSpecificProperty<IAIAlertSenseMultsProperty, &IID_IAIAlertSenseMultsProperty, sAIAlertSenseMults *, cHashPropertyStore< cNoZeroDataOps<cAIAlertSenseMults> > > cAIAlertSenseMultsPropertyBase;

class cAIAlertSenseMultsProperty : public cAIAlertSenseMultsPropertyBase
{
public:
   cAIAlertSenseMultsProperty()
      : cAIAlertSenseMultsPropertyBase(&g_AIAlertSenseMultsPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIAlertSenseMults);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIVisibilityModProperty
//

static sPropertyDesc g_AIVisibilityModPropertyDesc =
{
   PROP_AI_VISMOD,
   0,
   NULL, 1, 1,  // constraints, version
   { AI_CORE_CAT, "Visibility Modifier" },
   // kPropertyChangeLocally,  // netflags
};

///////////////////////////////////////////////////////////////////////////////

static sFieldDesc _g_AIVisibilityModFieldDesc[] =
{
   { "Vis Type 0 Mod", kFieldTypeFloat, FieldLocation(sAIVisibilityMods,m_mods[0]) },
   { "Vis Type 1 Mod", kFieldTypeFloat, FieldLocation(sAIVisibilityMods,m_mods[1]) },
   { "Vis Type 2 Mod", kFieldTypeFloat, FieldLocation(sAIVisibilityMods,m_mods[2]) },
   { "Vis Type 3 Mod", kFieldTypeFloat, FieldLocation(sAIVisibilityMods,m_mods[3]) },
   { "Vis Type 4 Mod", kFieldTypeFloat, FieldLocation(sAIVisibilityMods,m_mods[4]) },
   { "Vis Type 5 Mod", kFieldTypeFloat, FieldLocation(sAIVisibilityMods,m_mods[5]) },
};

static sStructDesc _g_AIVisibilityModStructDesc = StructDescBuild(sAIVisibilityMods, kStructFlagNone, _g_AIVisibilityModFieldDesc);

///////////////////////////////////////

class cAIVisibilityMods : public sAIVisibilityMods
{
public:
   cAIVisibilityMods()
   {
      *((sAIVisibilityMods *)this) = g_AIDefVisibilityMods;
   }
};

///////////////////////////////////////

typedef cSpecificProperty<IAIVisibilityModProperty, &IID_IAIVisibilityModProperty, sAIVisibilityMods *, cHashPropertyStore< cNoZeroDataOps<cAIVisibilityMods> > > cAIVisibilityModPropertyBase;

class cAIVisibilityModProperty : public cAIVisibilityModPropertyBase
{
public:
   cAIVisibilityModProperty()
      : cAIVisibilityModPropertyBase(&g_AIVisibilityModPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIVisibilityMods);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIVisCtrlProperty
//
// @TBD (toml 06-19-98): move this to aiprutil

static sPropertyDesc g_AIVisCtrlPropertyDesc =
{
   PROP_AI_VISCTRL,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_UTILITY_CAT, "Visibility control" },
   kPropertyChangeLocally,  // netflags
};

///////////////////////////////////////

static sFieldDesc _g_AIVisCtrlFieldDesc[] =
{
   { "Low light",       kFieldTypeInt,    FieldLocation(sAIVisibilityControl, lowVisibility)                                                       },
   { "Mid light",       kFieldTypeInt,    FieldLocation(sAIVisibilityControl, midVisibility)                                                       },
   { "High light",      kFieldTypeInt,    FieldLocation(sAIVisibilityControl, highVisibility)                                                      },

   { "Slow",            kFieldTypeFloat,  FieldLocation(sAIVisibilityControl, lowSpeed)                                                      },
   { "Fast",            kFieldTypeFloat,  FieldLocation(sAIVisibilityControl, highSpeed)                                                      },

   { "Slow mod",        kFieldTypeInt,    FieldLocation(sAIVisibilityControl, lowSpeedMod)                                                       },
   { "Norm mod",        kFieldTypeInt,    FieldLocation(sAIVisibilityControl, normSpeedMod)                                                       },
   { "Fast mod",        kFieldTypeInt,    FieldLocation(sAIVisibilityControl, highSpeedMod)                                                      },

   { "Wall dist",       kFieldTypeFloat,  FieldLocation(sAIVisibilityControl, wallDist)                                                      },

   { "Crouch mod",      kFieldTypeInt,    FieldLocation(sAIVisibilityControl, crouchMod)                                                       },
   { "Wall mod",        kFieldTypeInt,    FieldLocation(sAIVisibilityControl, wallMod)                                                      },

   { "Corner lean mod", kFieldTypeInt,    FieldLocation(sAIVisibilityControl, cornerLeanMod)                                                      },

};

static sStructDesc _g_AIVisCtrlStructDesc = StructDescBuild(sAIVisibilityControl, kStructFlagNone, _g_AIVisCtrlFieldDesc);

///////////////////////////////////////

class cAIVisibilityControl : public sAIVisibilityControl
{
public:
   cAIVisibilityControl()
   {
      *((sAIVisibilityControl *)this) = g_AIDefVisCtrl;
   }
};

///////////////////////////////////////

typedef cSpecificProperty<IAIVisCtrlProperty, &IID_IAIVisCtrlProperty, sAIVisibilityControl *, cHashPropertyStore< cNoZeroDataOps<cAIVisibilityControl> > > cAIVisCtrlPropertyBase;

class cAIVisCtrlProperty : public cAIVisCtrlPropertyBase
{
public:
   cAIVisCtrlProperty()
      : cAIVisCtrlPropertyBase(&g_AIVisCtrlPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIVisibilityControl);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAwareCapProperty
//

static sPropertyDesc g_AIAwareCapPropertyDesc =
{
   PROP_AI_AWARECAP,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Awareness capacitor" },
   kPropertyChangeLocally,  // netflags
};

///////////////////////////////////////

static sFieldDesc _g_AIAwareCapFieldDesc[] =
{
   { "Discharge time 1", kFieldTypeInt,    FieldLocation(sAIAwareCapacitor, low)                                                       },
   { "Discharge time 2", kFieldTypeInt,    FieldLocation(sAIAwareCapacitor, medium)                                                       },
   { "Discharge time 3", kFieldTypeInt,    FieldLocation(sAIAwareCapacitor, high)                                                      },
};

static sStructDesc _g_AIAwareCapStructDesc = StructDescBuild(sAIAwareCapacitor, kStructFlagNone, _g_AIAwareCapFieldDesc);

///////////////////////////////////////

class cAIAwareCapacitor : public sAIAwareCapacitor
{
public:
   cAIAwareCapacitor()
   {
      *((sAIAwareCapacitor *)this) = g_AIDefAwareCap;
   }
};

///////////////////////////////////////

typedef cSpecificProperty<IAIAwareCapProperty, &IID_IAIAwareCapProperty, sAIAwareCapacitor *, cHashPropertyStore< cNoZeroDataOps<cAIAwareCapacitor> > > cAIAwareCapPropertyBase;

class cAIAwareCapProperty : public cAIAwareCapPropertyBase
{
public:
   cAIAwareCapProperty()
      : cAIAwareCapPropertyBase(&g_AIAwareCapPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIAwareCapacitor);
};


///////////////////////////////////////
//
// PROPERTY: "AI_VisionJoint", int
//

static sPropertyDesc g_AIVisionJointPropertyDesc =
{
  PROP_AI_VISION_JOINT,
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Vision Joint" },
   kPropertyChangeLocally,  // netflags
};

static sPropertyTypeDesc g_AIVisionJointPropertyTypeDesc =
{
   "VisionJoint",
   sizeof(int),
};

///////////////////////////////////////

static sFieldDesc _g_AIVisionJointFieldDesc[] =
{
   {"joint",    kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 22, 22, g_pJointNames},
};

static sStructDesc _g_AIVisionJointStructDesc =
{
   "VisionJoint",
   sizeof(int),
   kStructFlagNone,
   sizeof(_g_AIVisionJointFieldDesc)/sizeof(_g_AIVisionJointFieldDesc[0]),
   _g_AIVisionJointFieldDesc,
};



static sPropertyDesc _g_AIForceImmediateInform =
{
  PROP_AI_FORCE_IMMEDIATE,
  0,
  NULL, 0, 0,
  { AI_UTILITY_CAT, "Immediate inform" },
  kPropertyChangeLocally,
};


static sPropertyDesc _g_AIInformSeenFrom =
{
  PROP_AI_INFORM_SEEN_FROM,
  0,
  NULL, 0, 0,
  { AI_UTILITY_CAT, "inform from last seen" },
  kPropertyChangeLocally,
};


static sPropertyDesc _g_AIIsSmallCreatureProp =
{
  PROP_AI_SMALL_CREATURE,
  0,
  NULL, 0, 0,
  { AI_UTILITY_CAT, "small creature" },
  kPropertyChangeLocally,
};



///////////////////////////////////////////////////////////////////////////////
//
// Initialization and cleanup
//

void AIInitCoreProps()
{
   StructDescRegister(&_g_AIPropStructDesc);
   StructDescRegister(&_g_AIEfficiencyStructDesc);
   StructDescRegister(&_g_AIAlertnessStructDesc);
   StructDescRegister(&_g_AIFrustratedStructDesc);
   StructDescRegister(&_g_AIAlertCapStructDesc);
   StructDescRegister(&_g_AIVisibilityStructDesc);
   StructDescRegister(&_g_AIModeStructDesc);
   StructDescRegister(&_g_AITeamStructDesc);
   StructDescRegister(&_g_AIVisionJointStructDesc);
   StructDescRegister(&_g_AIVisionDescStructDesc);
   StructDescRegister(&_g_AIVisibilityModStructDesc);
   StructDescRegister(&_g_AIVisCtrlStructDesc);
   StructDescRegister(&_g_AIAwareCapStructDesc);
   StructDescRegister(&_g_AIAlertSenseMultsStructDesc);
   StructDescRegister(&_g_AIAwareDelayStructDesc);

   g_pAINoGrowOBBProperty = CreateBoolProperty(&_g_AINoGrowOBBPropertyDesc, kPropertyImplSparse);
   
   g_pAITrackMediumProperty = CreateBoolProperty(&_g_AITrackMediumPropertyDesc, kPropertyImplSparse);

   g_pAICanPathWaterProperty = CreateBoolProperty(&_g_AICanPathWaterProperty, kPropertyImplSparse);

   g_pAINeedsLargeDoors = CreateBoolProperty(&_g_AINeedsBigDoors, kPropertyImplSparse);

   g_pAIIgnoresCamerasProperty = CreateBoolProperty(&_g_AIIgnoresCamerasProperty, kPropertyImplSparse);


   g_pAIProperty           = new cAIProperty;
   g_pAIEfficiencyProperty = new cAIEfficiencyProperty;

   g_pAIMoveZOffsetProperty = CreateFloatProperty(&g_AIMoveZOffsetPropertyDesc, kPropertyImplDense);
   g_pAIMoveSpeedProperty = CreateFloatProperty(&g_AIMoveSpeedPropertyDesc, kPropertyImplDense);
   g_pAITurnRateProperty = CreateFloatProperty(&g_AITurnRatePropertyDesc, kPropertyImplDense);

   g_pAITeamProperty = CreateIntegralProperty(&g_AITeamPropertyDesc,&g_AITeamPropertyTypeDesc,kPropertyImplDense);

   g_pAIVisionJointProperty = CreateIntegralProperty(&g_AIVisionJointPropertyDesc,&g_AIVisionJointPropertyTypeDesc,kPropertyImplDense);

   g_pAIStandTagsProperty  = CreateStringProperty(&_g_AIStandTagsPropertyDesc, kPropertyImplDense);
   g_pAISoundTagsProperty  = CreateStringProperty(&_g_AISoundTagsPropertyDesc, kPropertyImplDense);
   g_pAIMotionTagsProperty = CreateStringProperty(&_g_AIMotionTagsPropertyDesc, kPropertyImplDense);

   g_pAINoGhostProperty = CreateBoolProperty(&_g_AINoGhostPropertyDesc, kPropertyImplSparse);
   g_pAINoHandoffProperty = CreateBoolProperty(&_g_AINoHandoffPropertyDesc, kPropertyImplSparse);
   g_pAIIsProxyProperty = CreateBoolProperty(&_g_AIIsProxyPropertyDesc, kPropertyImplSparse);

   g_pAIForceImmediateInform = CreateBoolProperty(&_g_AIForceImmediateInform, kPropertyImplSparse);

   g_pAIInformSeenFrom = CreateBoolProperty(&_g_AIInformSeenFrom, kPropertyImplSparse);

   g_pAIIsSmallCreatureProp = CreateBoolProperty(&_g_AIIsSmallCreatureProp, kPropertyImplSparse);

   g_pAIAlertnessProperty = new cAIAlertnessProperty;
   g_pAIAlertCapProperty = new cAIAlertCapProperty;
   g_pAIAwareDelayProperty =  new cAIAwareDelayProperty;

   g_pAIVisibilityProperty = new cAIVisibilityProperty;
   g_pAIModeProperty = new cAIModeProperty;

   g_pAIFrustratedProperty = new cAIFrustratedProperty;

   g_pAIVisionDescProperty = new cAIVisionDescProperty;
   g_pAIVisibilityModProperty = new cAIVisibilityModProperty;
   g_pAIVisionTypeProperty = CreateIntegralProperty(&g_AIVisionTypePropertyDesc, &g_AIVisionTypePropertyTypeDesc, kPropertyImplDense);
   g_pAIVisCtrlProperty = new cAIVisCtrlProperty;
   g_pAIAwareCapProperty = new cAIAwareCapProperty;

   g_pAIAlertSenseMultsProperty = new cAIAlertSenseMultsProperty;

   AIInitDeviceProp();
   AIInitTurretProp();
   AIInitCameraProp();
   AIInitConversationProps();
}

///////////////////////////////////////

void AITermCoreProps()
{
   SafeRelease(g_pAINoGrowOBBProperty);
   
   SafeRelease(g_pAITrackMediumProperty);
   SafeRelease(g_pAICanPathWaterProperty);
   SafeRelease(g_pAINeedsLargeDoors);

   SafeRelease(g_pAIIgnoresCamerasProperty);
   SafeRelease(g_pAIProperty);
   SafeRelease(g_pAIEfficiencyProperty);

   SafeRelease(g_pAIMoveZOffsetProperty);
   SafeRelease(g_pAIMoveSpeedProperty);
   SafeRelease(g_pAITurnRateProperty);

   SafeRelease(g_pAITeamProperty);
   SafeRelease(g_pAIVisionJointProperty);

   SafeRelease(g_pAIStandTagsProperty);
   SafeRelease(g_pAISoundTagsProperty);
   SafeRelease(g_pAIMotionTagsProperty);

   SafeRelease(g_pAIAlertnessProperty);
   SafeRelease(g_pAIFrustratedProperty);
   SafeRelease(g_pAIAlertCapProperty);
   SafeRelease(g_pAIAwareDelayProperty);

   SafeRelease(g_pAIForceImmediateInform);
   SafeRelease(g_pAIInformSeenFrom);
   SafeRelease(g_pAIIsSmallCreatureProp);

   SafeRelease(g_pAIVisibilityProperty);
   SafeRelease(g_pAIModeProperty);

   SafeRelease(g_pAIVisionDescProperty);
   SafeRelease(g_pAIVisibilityModProperty);
   SafeRelease(g_pAIVisCtrlProperty);
   SafeRelease(g_pAIAwareCapProperty);

   SafeRelease(g_pAIAlertSenseMultsProperty);

   AITermDeviceProp();
   AITermTurretProp();
   AITermCameraProp();
   AITermConversationProps();
}


///////////////////////////////////////////////////////////////////////////////


#ifdef EDITOR

//  The new Vision Description dialog, with a list and sub-dialogs rather than a single huge one
//

static sFieldDesc _g_AIVisionDescHeaderFieldDesc[] =
{
   {"Z Offset", kFieldTypeFloat, FieldLocation(sAIVisionDesc, zOffset),},
};

static sFieldDesc _g_AIVisionDescConeFieldDesc[] =
{
   { "Cone: Flags",   kFieldTypeBits, FieldLocation(sAIVisionCone, flags), kFieldFlagUnsigned, 0, FieldNames(g_pszAIVisConeFlags)},
   { "      Angle",   kFieldTypeInt,  FieldLocation(sAIVisionCone, angle)                                                        },
   { "      Z angle", kFieldTypeInt,  FieldLocation(sAIVisionCone, zAngle)                                                       },
   { "      Range",   kFieldTypeInt,  FieldLocation(sAIVisionCone, range)                                                        },
   { "      Acuity",  kFieldTypeInt,  FieldLocation(sAIVisionCone, acuity)                                                       }
};

static sStructDesc _g_AIVisionDescHeaderStructDesc = StructDescBuild(sAIVisionDesc, kStructFlagNone, _g_AIVisionDescHeaderFieldDesc);
static sStructDesc _g_AIVisionDescConeStructDesc = StructDescBuild(sAIVisionCone, kStructFlagNone, _g_AIVisionDescConeFieldDesc);

IStructEditor* NewVisionDescDialog (void* data)
{
    int i;
    int numMenuSlots = kAIMaxVisionCones + 2;
    char **responseMenu = (char **) malloc(sizeof(char*) * numMenuSlots);
    sStructEditorDesc headerDesc = { "Header", kStructEditAllButtons };
    sStructEditorDesc stepDesc = { "Cone", kStructEditAllButtons };
    BOOL result = FALSE;

    //  Add the "done" choice at the beginning
    //
    responseMenu[0] = (char*) malloc(sizeof(char) * 16);
    sprintf(responseMenu[0], "-- DONE --");

    //  Put the header in as an option
    //
    responseMenu[1] = (char*) malloc(sizeof(char) * 16);
    sprintf(responseMenu[1], "- Header");

    //  Build the list of response steps
    //
    AutoAppIPtr_(StructDescTools,pSdescTools);
    char buf[54];
    for (i = 2; i < numMenuSlots; i++)
    {
        responseMenu[i] = (char*) malloc(sizeof(char) * 64);
        pSdescTools->UnparseFull(&_g_AIVisionDescConeStructDesc,&((sAIVisionDesc*) data)->cones[i - 2],buf,sizeof(buf));
        if (i-1 < 10)
            sprintf(responseMenu[i], "Cone 0%d: %.54s", i-1, buf);
        else
            sprintf(responseMenu[i], "Cone %d: %.54s", i-1, buf);
    }

    //  Display the list, and let the user choose
    //
    int choice = PickFromStringList("Vision Cones", (const char* const*)responseMenu, numMenuSlots);
    for (i = 0; i < numMenuSlots; i++)
       free (responseMenu[i]);
    free (responseMenu);

    //  Done
    //
    if (choice == 0)
    {
        return NULL;
    }

    //  Bring up the header dialog
    //
    else if (choice == 1)
    {
        return CreateStructEditor(&headerDesc, &_g_AIVisionDescHeaderStructDesc, data);
    }

    //  Bring up the specific dialog for cone chosen
    //
    else if (choice >= 2 && choice < numMenuSlots)
    {
        return CreateStructEditor(&stepDesc, &_g_AIVisionDescConeStructDesc, &((sAIVisionDesc*) data)->cones[choice - 2]);
    }

    return NULL;
}

#endif
