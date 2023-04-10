/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprutil.h,v 1.8 2000/03/25 22:15:35 adurant Exp $
//
// @TBD (toml 06-09-98): these should be moved into their corresponding
// ability files?
//

#ifndef __AIPRUTL_H
#define __AIPRUTL_H

#include <aiprops.h>

#pragma once
#pragma pack(4)

struct sAISoundType;
F_DECLARE_INTERFACE(IAISoundTypeProperty);

///////////////////////////////////////////////////////////////////////////////

void AIInitUtilProps();
void AITermUtilProps();
void AIDBResetUtilProps();

///////////////////////////////////////////////////////////////////////////////
//
// UTILITY PROPERTIES
//

////////////////////////////////////////
//
// Sound types
//

#define PROP_AI_SNDTYPE "AI_SndType"

EXTERN IAISoundTypeProperty * g_pAISoundTypeProperty;

#define AIGetSoundType(obj) AIGetProperty(g_pAISoundTypeProperty, (obj), (sAISoundType *)NULL)

///////////////////////////////////////////////////////////////////////////////
//
// PROPERTY: IAISoundTypeProperty
//

#undef INTERFACE
#define INTERFACE IAISoundTypeProperty
DECLARE_PROPERTY_INTERFACE(IAISoundTypeProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAISoundType *);
};

////////////////////////////////////////
//
// Knockout stimuli
//

#define PROP_STIM_KNOCKOUT "StimKO"

EXTERN IBoolProperty * g_pStimKnockoutProperty;

#define AIGetStimKnockout(obj) AIGetProperty(g_pStimKnockoutProperty, (obj), FALSE)

///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////
//
// AI Shoots Through objects prop
//

#define PROP_AI_SHOOTS_THROUGH_NAME "AIFiresThrough"
        
EXTERN void SetAIShootsThrough(ObjID object, BOOL shootsthrough);
EXTERN BOOL GetAIShootsThrough(ObjID object);

#pragma pack()

#endif /* !__AIPRUTL_H */



