/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiwtctyp.h,v 1.5 1999/10/29 19:20:16 henrys Exp $
//
// Types used to control watching behaviors
//

#ifndef __AIWTCTYP_H
#define __AIWTCTYP_H

#include <aipsdscr.h>

struct sAIWatchPoint;

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// Watch links
//

///////////////////////////////////////
//
// Link options
//

enum eAIWatchKind
{
   kAIWK_Player,
   kAIWK_Enter,
  
   kAIWK_IntMax = 0xffffffff
};

enum eAIWatchKill
{
   kAIWK_Never,
   kAIWK_OnTrigger,
   kAIWK_OnComplete,
  
   kAIWK2_IntMax = 0xffffffff
};

enum eAIWatchLineReqs
{
   kAIWLR_None,
   kAIWLR_LOS,
   kAIWLR_Raycast,

   kAIWLR_IntMax = 0xffffffff
};

///////////////////////////////////////
//
// The actual link
//

#define kAIMaxWatchActs 16
#define kAIWatchVersion 1

#define kAIStepsComplete -1

struct sAIWatchPoint
{
   unsigned          version;
   
   // Refreshed fields
   ObjID             object;
   mxs_vector        center;
   LinkID            linkID;
   
   ObjID             triggerObj;
   float             distSq;
   
   unsigned          reuseTime;
   unsigned          resetTime;
   int               nextStep;
   
   BOOL              fCompleted;
   
   DWORD             reserved1[3];
   
   // Designer fields
   eAIWatchKind      kind;
   eAIAwareLevel     awarenessRequired;
   eAIWatchLineReqs  lineRequired;
   eAIPriority       priority;
   eAIWatchKill      kill;
   BOOL              fKillOthers;

   BOOL              fAlwaysComplete;
   unsigned          reuseDelay;
   unsigned          resetDelay;
   
   eAIAwareLevel     minAlert;
   eAIAwareLevel     maxAlert;
  
   DWORD             reserved2;
   
   int               radius;
   int               height;
   sAIPsdScrAct      acts[kAIMaxWatchActs];
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIWTCTYP_H */
