/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrmpr.h,v 1.3 2000/01/29 12:45:07 adurant Exp $
//
// Ranged-combat-mode related properties
//
#pragma once

#ifndef __AICBRMPR_H
#define __AICBRMPR_H

#include <propface.h>
#include <aicbrfle.h>
#include <aicbrmds.h>
#include <aicbrsht.h>

////////////////////////////////////////////////////////////////////////////////
// 
// Ranged mode applicabilities
//

#define PROP_AI_RANGED_APPS "AI_RngApps"

#undef  INTERFACE
#define INTERFACE IAIRangedApplicabilitiesProperty
DECLARE_PROPERTY_INTERFACE(IAIRangedApplicabilitiesProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIRangedApplicabilities *);
};

EXTERN IAIRangedApplicabilitiesProperty* g_pAIRangedApplicabilitiesProperty;
EXTERN sAIRangedApplicabilities g_defaultAIRangedApplicabilities;

#define AIGetRangedApplicabilities(obj) AIGetProperty(g_pAIRangedApplicabilitiesProperty, (obj), &g_defaultAIRangedApplicabilities)

////////////////////////////////////////////////////////////////////////////////
// 
// Ranged flee params
//

#define PROP_AI_RANGED_FLEE "AI_RngFlee"

#undef  INTERFACE
#define INTERFACE IAIRangedFleeProperty
DECLARE_PROPERTY_INTERFACE(IAIRangedFleeProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIRangedFleeParams *);
};

EXTERN IAIRangedFleeProperty* g_pAIRangedFleeProperty;
EXTERN sAIRangedFleeParams g_defaultAIRangedFleeParams;

#define AIGetRangedFlee(obj) AIGetProperty(g_pAIRangedFleeProperty, (obj), &g_defaultAIRangedFleeParams)

////////////////////////////////////////////////////////////////////////////////
// 
// Ranged shoot params
//

#define PROP_AI_RANGED_SHOOT "AI_RngShoot"

#undef  INTERFACE
#define INTERFACE IAIRangedShootProperty
DECLARE_PROPERTY_INTERFACE(IAIRangedShootProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIRangedShootParams *);
};

EXTERN IAIRangedShootProperty* g_pAIRangedShootProperty;
EXTERN sAIRangedShootParams g_defaultAIRangedShootParams;

#define AIGetRangedShoot(obj) AIGetProperty(g_pAIRangedShootProperty, (obj), &g_defaultAIRangedShootParams)

// Get launch offset, taking into account AI location & heading (not pitch)
BOOL AIGetLaunchOffset(ObjID AIID, Location* pLocation);

////////////////////////////////////////////////////////////////////////////////

EXTERN void InitRangedModeProperties(void);
EXTERN void TermRangedModeProperties(void);

////////////////////////////////////////////////////////////////////////////////

#endif   // __AICBRMPR_H






