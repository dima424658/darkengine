/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprcore.h,v 1.22 2000/02/28 19:35:15 bfarquha Exp $
//
//

#ifndef __AIPRAI_H
#define __AIPRAI_H

#include <aiprops.h>

#include <crjoint.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAIProperty);
F_DECLARE_INTERFACE(IAIEfficiencyProperty);
F_DECLARE_INTERFACE(IAIAwareCapProperty);
F_DECLARE_INTERFACE(IAIAlertnessProperty);
F_DECLARE_INTERFACE(IAIAlertCapProperty);
F_DECLARE_INTERFACE(IAIAwareDelayProperty);
F_DECLARE_INTERFACE(IAIVisibilityProperty);
F_DECLARE_INTERFACE(IAIVisionDescProperty);
F_DECLARE_INTERFACE(IAIVisibilityModProperty);
F_DECLARE_INTERFACE(IAIVisionTypeProperty);
F_DECLARE_INTERFACE(IAIVisCtrlProperty);
F_DECLARE_INTERFACE(IAIModeProperty);
F_DECLARE_INTERFACE(IAIAlertSenseMultsProperty);
F_DECLARE_INTERFACE(IAIFrustratedProperty);


typedef struct sAIAlertness      sAIAlertness;
typedef struct sAIAlertCap       sAIAlertCap;
typedef struct sAIAwareDelay     sAIAwareDelay;
typedef struct sAIAwareCapacitor sAIAwareCapacitor;
typedef struct sAIVisibility     sAIVisibility;
typedef struct sAIVisionDesc     sAIVisionDesc;
typedef struct sAIVisibilityControl sAIVisibilityControl;
typedef struct sAIVisibilityMods sAIVisibilityMods;
typedef struct sAIAlertSenseMults sAIAlertSenseMults;
typedef struct sAIFrustrated     sAIFrustrated;

///////////////////////////////////////////////////////////////////////////////

void AIInitCoreProps();
void AITermCoreProps();

///////////////////////////////////////////////////////////////////////////////
//
// CONFIGURATION PROPERTIES
//

#define PROP_AI_TRACK_MEDIUM "AI_TrackM"
EXTERN IBoolProperty * g_pAITrackMediumProperty;


///////////////////////////////////////
//
// Core AI property
//

#define PROP_AI "AI"

EXTERN IAIProperty * g_pAIProperty;

///////////////////////////////////////
//
// Efficiency
//

#define PROP_AI_EFFICIENCY "AI_Efficiency"

EXTERN IAIEfficiencyProperty * g_pAIEfficiencyProperty;

#define AIGetEfficiency(obj) AIGetProperty(g_pAIEfficiencyProperty, (obj), &g_AIDefaultEfficiency)


///////////////////////////////////////
//
// Can path across water?
//

#define PROP_AI_USEWATER "AI_UseWater"

EXTERN IBoolProperty * g_pAICanPathWaterProperty;


/////////////////////////////////////////////////////////////////////
//
// Is this AI considered "large", and requires a door to be large
//

#define PROP_AI_AIISLARGE "AI_IsBig"
EXTERN IBoolProperty *g_pAINeedsLargeDoors;
#define AINeedsLargeDoors(obj, bVal) (g_pAINeedsLargeDoors->Get(obj, &bVal) && bVal)


///////////////////////////////////////
//
// Ignores cameras
//

#define PROP_IGNORES_CAMERAS "AI_IgCam"

EXTERN IBoolProperty * g_pAIIgnoresCamerasProperty;


///////////////////////////////////////
//
// Vision description
//

#define PROP_AI_VISDESC "AI_VisDesc"

EXTERN IAIVisionDescProperty * g_pAIVisionDescProperty;

#define AIGetVisionDesc(obj) AIGetProperty(g_pAIVisionDescProperty, (obj), &g_AIDefVisionDesc)

///////////////////////////////////////
//
// Alertness sense multipliers
//

EXTERN IAIAlertSenseMultsProperty * g_pAIAlertSenseMultsProperty;

#define AIAlertSenseMults(obj) AIGetProperty(g_pAIAlertSenseMultsProperty, (obj), &g_AIDefAlertSenseMults)

///////////////////////////////////////
//
// Vision type
//

#define PROP_AI_VISTYPE "AI_VisType"

EXTERN IIntProperty * g_pAIVisionTypeProperty;

///////////////////////////////////////
//
// Visibility modifier
//

#define PROP_AI_VISMOD "AI_VisModifier"

EXTERN IAIVisibilityModProperty * g_pAIVisibilityModProperty;

#define AIGetVisibilityMod(obj) AIGetProperty(g_pAIVisibilityModProperty, (obj), &g_AIDefVisibilityMods)

///////////////////////////////////////
//
// Visibility control
//

#define PROP_AI_VISCTRL "AI_VisCtrl"

EXTERN IAIVisCtrlProperty * g_pAIVisCtrlProperty;

#define AIGetVisCtrl(obj) AIGetProperty(g_pAIVisCtrlProperty, (obj), &g_AIDefVisCtrl)

///////////////////////////////////////
//
// Awareness
//

#define PROP_AI_AWARECAP "AI_AwareCap"

EXTERN IAIAwareCapProperty * g_pAIAwareCapProperty;

#define AIGetAwareCap(obj) AIGetProperty(g_pAIAwareCapProperty, (obj), &g_AIDefAwareCap)


///////////////////////////////////////
//
// Movement: z offset
//

#define PROP_AI_MOVE_ZOFFSET "AI_MoveZOffset"

EXTERN IFloatProperty *g_pAIMoveZOffsetProperty;

#define AIGetGroundOffset(obj) AIGetProperty(g_pAIMoveZOffsetProperty,(obj), (float)0.0)


///////////////////////////////////////
//
// Movement: speed
//

#define PROP_AI_MOVE_SPEED "AI_MoveSpeed"

EXTERN IFloatProperty *g_pAIMoveSpeedProperty;

#define AIGetMaxSpeed(obj) AIGetProperty(g_pAIMoveSpeedProperty,(obj), (float)1.4)

///////////////////////////////////////
//
// Movement: turn rate
//

#define PROP_AI_TURN_RATE "AI_TurnRate"

EXTERN IFloatProperty *g_pAITurnRateProperty;

#define AIGetTurnRate(obj) (DEGREES(AIGetProperty(g_pAITurnRateProperty,(obj), (float)380.0)) / 1000.0)

///////////////////////////////////////
//
// Team
//

#define PROP_AI_TEAM "AI_Team"

EXTERN IIntProperty * g_pAITeamProperty;

EXTERN int _AIGetTeam(ObjID obj);
#define AIGetTeam(obj)  ((eAITeam)_AIGetTeam((obj)))

///////////////////////////////////////
//
// Vision Joint
//

#define PROP_AI_VISION_JOINT "AI_VisJoint"

EXTERN IIntProperty * g_pAIVisionJointProperty;

#define AIGetVisionJoint(obj) AIGetProperty(g_pAIVisionJointProperty,(obj), int(kCJ_Invalid))

///////////////////////////////////////
//
// Whether the AI should play the stand motion
//

EXTERN IStringProperty * g_pAIStandTagsProperty;
EXTERN const char * g_pszAIDefStandTags;
#define AIStandTags(obj) AIGetProperty(g_pAIStandTagsProperty, (obj), g_pszAIDefStandTags)

///////////////////////////////////////
//
// Motion and sound tags
//

EXTERN IStringProperty * g_pAISoundTagsProperty;
EXTERN IStringProperty * g_pAIMotionTagsProperty;

#define AIGetSoundTags(obj)  AIGetProperty(g_pAISoundTagsProperty,(obj), ((const char *) NULL))
#define AIGetMotionTags(obj) AIGetProperty(g_pAIMotionTagsProperty,(obj), ((const char *) NULL))

///////////////////////////////////////
//
// Suppress ghosts
//
// This is appropriate for AIs that we *know* will never move, so we
// shouldn't waste time networking their location in multiplayer.
//

#define PROP_AI_NO_GHOST "AI_NoGhost"

EXTERN IBoolProperty *g_pAINoGhostProperty;

#define AIShouldNotHaveGhost(obj) AIGetProperty(g_pAINoGhostProperty, (obj), (BOOL)(FALSE))

///////////////////////////////////////
//
// Suppress handoff
//
// This should be set on any AIs that we don't want to hand from player to
// player in multiplayer games. Appropriate uses include AIs that don't
// need especially precise aim, and those that are heavily scripted.
//

#define PROP_AI_NO_HANDOFF "AI_NoHandoff"

EXTERN IBoolProperty *g_pAINoHandoffProperty;

#define AIShouldNotHandoff(obj) AIGetProperty(g_pAINoHandoffProperty, (obj), (BOOL)(FALSE))

///////////////////////////////////////
//
// Is a handed-off proxy
//
// This property is basically a hack to work around the fact that, as
// currently constructed, networking's model of object ownership doesn't
// survive level transitions well. If we transition away from a level,
// and then back to it, we figure out the ownership of handed-off objects
// *after* postload. That's really too late for AI, though, which needs
// to know *during* postload.
//
// So this property gets set on any AI that gets handed off between two
// players. If it is set, it indicates whether or not the AI is a proxy.
// That way, we can establish the AI properly during postload. When
// objnet gets its well-deserved rewrite to a more robust ownership
// representation, this property can go away.
//
// Note that, if this property is not set, that just means that the AI
// has not been handed off, and we should use default algorithms to
// determine who owns it.
//

#define PROP_AI_IS_PROXY "AI_IsProxy"

EXTERN IBoolProperty *g_pAIIsProxyProperty;

///////////////////////////////////////////////////////////////////////////////
//
// STATE PROPERTIES (intantiated)
//


#define PROP_AI_FORCE_IMMEDIATE "AI_InfNow"

EXTERN IBoolProperty *g_pAIForceImmediateInform;


#define PROP_AI_INFORM_SEEN_FROM "AI_InfFrm"

EXTERN IBoolProperty *g_pAIInformSeenFrom;


#define PROP_AI_SMALL_CREATURE "AI_IsSmall"

EXTERN IBoolProperty *g_pAIIsSmallCreatureProp;


///////////////////////////////////////
//
// Mode
//

#define PROP_AI_MODE  "AI_Mode"

EXTERN IAIModeProperty * g_pAIModeProperty;

#define AIGetMode(obj) AIGetProperty(g_pAIModeProperty, (obj), kAIM_Normal)


#define PROP_AI_IS_FRUSTRATED "AI_Frustrated"
EXTERN IAIFrustratedProperty *g_pAIFrustratedProperty;


///////////////////////////////////////
//
// Alertness/awareness
//

#define PROP_AI_ALERTNESS  "AI_Alertness"

EXTERN IAIAlertnessProperty * g_pAIAlertnessProperty;
EXTERN IAIAlertCapProperty *  g_pAIAlertCapProperty;
EXTERN IAIAwareDelayProperty * g_pAIAwareDelayProperty;

#define AIGetAlertness(obj)  AIGetProperty(g_pAIAlertnessProperty, (obj), ((sAIAlertness *)NULL))
#define AIGetAlertCap(obj)   AIGetProperty(g_pAIAlertCapProperty, (obj), g_pAIDefAlertCap)
#define AIGetAwareDelay(obj) AIGetProperty(g_pAIAwareDelayProperty, (obj), g_pAIDefAwareDelay)

EXTERN sAIAlertCap *   g_pAIDefAlertCap;
EXTERN sAIAwareDelay * g_pAIDefAwareDelay;

///////////////////////////////////////
//
// Visibility
//

#define PROP_AI_VISIBILITY "AI_Visibility"

EXTERN IAIVisibilityProperty * g_pAIVisibilityProperty;

#define AIGetVisibility(obj) AIGetProperty(g_pAIVisibilityProperty, (obj), ((sAIVisibility *)NULL))

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIProperty, "AI", sAIProp
//

struct sAIProp
{
   //
   // The behavior set to use when creating an AI with the property
   //
   char szBehaviorSet[32];
};

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAIProperty
DECLARE_PROPERTY_INTERFACE(IAIProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIProp *);
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIEfficiencyProperty, "AI_Efficiency", sAIEfficiency
//

///////////////////////////////////////
//
// STRUCT: sAIEfficiency
//
// Describes the threshholds for entering and exiting efficiency modes.
//

struct sAIEfficiency
{
   BOOL  fEnabled;

   // Super Efficiency limits
   float distEnterSuperEffSq,
         zEnterSuperEff;

   float distExitSuperEffSq,
         zExitSuperEff;

   // Efficiency limits
   float distEnterEffSq,
         zEnterEff;

   float distExitEffSq,
         zExitEff;
};

EXTERN sAIEfficiency g_AIDefaultEfficiency;

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAIEfficiencyProperty
DECLARE_PROPERTY_INTERFACE(IAIEfficiencyProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIEfficiency *);
};


///////////////////////////////////////////////////////////////////////////////
//
// Frustrated
//

#undef INTERFACE
#define INTERFACE IAIFrustratedProperty
DECLARE_PROPERTY_INTERFACE(IAIFrustratedProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIFrustrated *);
};


///////////////////////////////////////////////////////////////////////////////
//
// Awareness
//

#undef INTERFACE
#define INTERFACE IAIAwareCapProperty
DECLARE_PROPERTY_INTERFACE(IAIAwareCapProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIAwareCapacitor *);
};

///////////////////////////////////////////////////////////////////////////////
//
// Alertness
//

#undef INTERFACE
#define INTERFACE IAIAlertnessProperty
DECLARE_PROPERTY_INTERFACE(IAIAlertnessProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIAlertness *);
};

///////////////////////////////////////////////////////////////////////////////
//
// Alertness cap
//

#undef INTERFACE
#define INTERFACE IAIAlertCapProperty
DECLARE_PROPERTY_INTERFACE(IAIAlertCapProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIAlertCap *);
};

///////////////////////////////////////////////////////////////////////////////
//
// Alertness reaction delay
//

#undef INTERFACE
#define INTERFACE IAIAwareDelayProperty
DECLARE_PROPERTY_INTERFACE(IAIAwareDelayProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIAwareDelay *);
};

///////////////////////////////////////////////////////////////////////////////
//
// Vision description
//

#undef INTERFACE
#define INTERFACE IAIVisionDescProperty
DECLARE_PROPERTY_INTERFACE(IAIVisionDescProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIVisionDesc *);
};

///////////////////////////////////////////////////////////////////////////////
//
// Alert sense multiplier
//

#undef INTERFACE
#define INTERFACE IAIAlertSenseMultsProperty
DECLARE_PROPERTY_INTERFACE(IAIAlertSenseMultsProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIAlertSenseMults *);
};

///////////////////////////////////////////////////////////////////////////////
//
// Visibility Modifier
//

#undef INTERFACE
#define INTERFACE IAIVisibilityModProperty
DECLARE_PROPERTY_INTERFACE(IAIVisibilityModProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIVisibilityMods *);
};

///////////////////////////////////////////////////////////////////////////////
//
// Visibility control property
//

#undef INTERFACE
#define INTERFACE IAIVisCtrlProperty
DECLARE_PROPERTY_INTERFACE(IAIVisCtrlProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIVisibilityControl *);
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIVisibilityProperty, "AI_Visibility", sAIVisibility
//

#undef INTERFACE
#define INTERFACE IAIVisibilityProperty
DECLARE_PROPERTY_INTERFACE(IAIVisibilityProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIVisibility *);
};

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIModeProperty
//

#undef INTERFACE
#define INTERFACE IAIModeProperty
DECLARE_PROPERTY_INTERFACE(IAIModeProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(eAIMode);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPRAI_H */
