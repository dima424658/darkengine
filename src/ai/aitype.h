/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aitype.h,v 1.11 1998/11/18 16:21:57 MROWLEY Exp $
//

#ifndef __AITYPE_H
#define __AITYPE_H

#pragma once

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lg.h>
#include <comtools.h>
#include <matrixs.h>
#include <timings.h>

#include <fastflts.h>
#include <fltang.h>
#include <matrixc.h>
#include <objtype.h>
#include <wrtype.h>
#include <nettypes.h>

///////////////////////////////////////////////////////////////////////////////

// As the AI wants to do a lot of squared constants, this macro may be useful
// for code cleanliness
#define sq(n) ((n) * (n))

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

#define kAIEpsilon ((float)(0.000001))

///////////////////////////////////////////////////////////////////////////////
//
// Typedefs
//

// Vector alias
typedef cMxsVector cMxsVector;

// Save/load function
typedef int (*tAIReadWriteFunc)(void * pContext, void * pBuf, size_t elsize, size_t nelem);

///////////////////////////////////////////////////////////////////////////////
//
// CONSTANTS & ENUMERATIONS
//

EXTERN const cMxsVector kInvalidLoc;
#define kFloatMax FLT_MAX

///////////////////////////////////////
//
// High-level modes
//

enum eAIMode
{
   kAIM_Asleep,
   kAIM_SuperEfficient,
   kAIM_Efficient,
   kAIM_Normal,
   kAIM_Combat,
   kAIM_Dead,
   
   kAIM_Num,

   kAIM_TypeMax = 0xffffffff // force it to be an int
};

const char * AIGetModeName(eAIMode);

///////////////////////////////////////
//
// Prioritization
//

enum eAIPriority
{
   kAIP_None,

   kAIP_VeryLow,                                 // generally non combat
   kAIP_Low,                                     // generally non combat
   kAIP_Normal,
   kAIP_High,                                    // normal combat
   kAIP_VeryHigh,                                // immediately life threatening
   kAIP_Absolute,                                // designer override

   kAIP_Num,
   kAIP_IntMax=0xffffffff                        // force it use an int
};

const char * AIGetPriorityName(eAIPriority);

///////////////////////////////////////

EXTERN const char * g_ppszAIPriority[kAIP_Num];

#define AI_PRIORITY_FIELD(tag, struct, field) \
   { tag, kFieldTypeEnum, FieldLocation(struct, field), kFieldFlagNone, 0, FieldNames(g_ppszAIPriority) }

///////////////////////////////////////
//
// Rating
//

enum eAIRating
{
   kAIRT_Null,
   kAIRT_WellBelowAvg,
   kAIRT_BelowAvg,
   kAIRT_Avg,
   kAIRT_AboveAvg,
   kAIRT_WellAboveAvg,

   kAIRT_Num,
   kAIRT_IntMax=0xffffffff                        // force it use an int
};

const char * AIGetRatingName(eAIRating);

///////////////////////////////////////
//
// Speed
//

enum eAISpeed
{
   kAIS_Stopped,
   kAIS_VerySlow,
   kAIS_Slow,
   kAIS_Normal,
   kAIS_Fast,
   kAIS_VeryFast,

   kAIS_Num,
   kAIS_IntMax=0xffffffff                        // force it use an int
};

const char * AIGetSpeedName(eAISpeed);
eAISpeed AIGetSpeedFromName(const char * pszSpeed);

///////////////////

extern const char * g_ppszAISpeed[kAIS_Num];

#define AI_SPEED_FIELD(tag, struct, field) \
   { tag, kFieldTypeEnum, FieldLocation(struct, field), kFieldFlagNone, 0, FieldNames(g_ppszAISpeed) }

///////////////////////////////////////
//
// Request/goal/action status
//

enum eAIResult
{
   // The activity is in progrss
   kAIR_NoResult,

   // The activity is in progress, but this is a good time to interrupt
   kAIR_NoResultSwitch,

   // The activity is complete
   kAIR_Success,

   // The activity failed, generally
   kAIR_Fail,

   // The activity failed, due to timeout
   kAIR_Timeout,

   // The activity failed, due to interruption
   kAIR_Interrupted,

   // There's nothing to do
   kAIR_Nothing,

   // There is an error
   kAIR_Error,
   
   // Undefined value, used for state machines
   kAIR_Undefined,

   kAIR_IntMax = 0xffffffff
};

#define AIInProgress(result)   (result < kAIR_Success)

///////////////////////////////////////

enum eAIPropUpdateKind
{
   kAI_FromProps,
   kAI_ToProps,

   kAIPUK_IntMax = 0xffffffff
};

///////////////////////////////////////
//
// Awarenes/alertness constants
//

enum eAIAwareLevel
{
   kAIAL_Lowest,
   kAIAL_Low,
   kAIAL_Moderate,
   kAIAL_High,

   kAIAL_Num,

   kAIAL_TypeMax = 0xffffffff // force it to be an int
};

typedef eAIAwareLevel eAIAlertLevel;

EXTERN const char * g_ppszAIAwareAlertLevels[kAIAL_Num];

#define AIGetAwareLevelName(level) g_ppszAIAwareAlertLevels[(level)]
#define AIGetAlertLevelName(level) g_ppszAIAwareAlertLevels[(level)]

///////////////////////////////////////////////////////////////////////////////
//
// STRUCTURES
//

// The first fields of all network messages sent from witin AI.
typedef struct sAINetMsg_Header
{
   tNetMsgHandlerID handlerID;  // the AIManager handler ID.
   NetObjID aiObj;  // the ObjID of the ai.  In the ObjID space of the sender.
   ubyte stopping;  // one byte boolean, are we stopping an action (vs starting it).
   unsigned action; // The action being started/stopped 
} sAINetMsg_Header;

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AITYPE_H */
