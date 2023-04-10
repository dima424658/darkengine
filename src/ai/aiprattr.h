/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprattr.h,v 1.4 1998/10/30 10:58:19 dc Exp $
//
//
//

#ifndef __AIPRATTR_H
#define __AIPRATTR_H

#include <aiprops.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAIRatingProperty);

///////////////////////////////////////////////////////////////////////////////

void AIInitAttrProps();
void AITermAttrProps();

///////////////////////////////////////////////////////////////////////////////
//
// AI Stats
//

#define PROP_AI_STAT_VISION      "AI_Vision"
#define PROP_AI_STAT_HEARING     "AI_Hearing"

// combat uses these
#define PROP_AI_STAT_AGGRESSIVE  "AI_Aggression"
#define PROP_AI_STAT_DODGINESS   "AI_Dodginess"
#define PROP_AI_STAT_SLOTH       "AI_Sloth"
#define PROP_AI_STAT_VERBOSITY   "AI_Verbosity"
#define PROP_AI_STAT_DEFENSIVE   "AI_Defensive"
#define PROP_AI_STAT_APTITUDE    "AI_Aptitude"

EXTERN IAIRatingProperty * g_pAIVisionStatProp;
EXTERN IAIRatingProperty * g_pAIHearingStatProp;

// combat uses these
EXTERN IAIRatingProperty * g_pAIAggressionStatProp;
EXTERN IAIRatingProperty * g_pAIDodginessStatProp;
EXTERN IAIRatingProperty * g_pAISlothStatProp;
EXTERN IAIRatingProperty * g_pAIVerbosityStatProp;
EXTERN IAIRatingProperty * g_pAIDefensiveStatProp;
EXTERN IAIRatingProperty * g_pAIAptitudeStatProp;

#define AIGetVision(obj)      AIGetRating(g_pAIVisionStatProp, obj)
#define AIGetHearing(obj)     AIGetRating(g_pAIHearingStatProp, obj)

// combat uses these 
#define AIGetAggression(obj)  AIGetRating(g_pAIAggressionStatProp, obj)
#define AIGetDodginess(obj)   AIGetRating(g_pAIDodginessStatProp, obj)
#define AIGetSloth(obj)       AIGetRating(g_pAISlothStatProp, obj)
#define AIGetVerbosity(obj)   AIGetRating(g_pAIVerbosityStatProp, obj)
#define AIGetDefensive(obj)   AIGetRating(g_pAIDefensiveStatProp, obj)
#define AIGetAptitude(obj)    AIGetRating(g_pAIAptitudeStatProp, obj)

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAIRatingProperty
//

#define AIGetRating(pProp, obj) \
   AIGetProperty(pProp, (obj), ((eAIRating)(kAIRT_Avg)))

#undef INTERFACE
#define INTERFACE IAIRatingProperty
DECLARE_PROPERTY_INTERFACE(IAIRatingProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(eAIRating);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPRATTR_H */
