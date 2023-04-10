/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrtyp.h,v 1.4 1999/04/16 17:03:28 JON Exp $
//

#ifndef __AICBRTYP_H
#define __AICBRTYP_H

#include <objtype.h>

#pragma once
#pragma pack(4)

////////////////////////////////////////
// Events
//

typedef int eAINewRangedCombatEvent;

enum eAINewRangedCombatEvent_
{
   kAIRC_EventWounded,
};

typedef struct sAIRangedCombatEvent
{
   eAINewRangedCombatEvent m_type;
   void* m_pData;
} sAIRangedCombatEvent;

////////////////////////////////////////
// Ranges
//

typedef int eAIRangedCombatRange;
// Note: must keep these in this order, certain modes rely on it
enum eAIRangedCombatRange_
{
   kAIRC_RangeVeryShort = 0,
   kAIRC_RangeShort     = 1,
   kAIRC_RangeIdeal     = 2,
   kAIRC_RangeLong      = 3,
   kAIRC_RangeVeryLong  = 4,
   kAIRC_NumRanges,
};

///////////////////////////////////////

typedef int eAIRangedCombatProjTestFlags;

enum eAIRangedCombatProjTestFlags_
{
   kAIRC_ProjClear =               0x0000,   // LOF is not blocked
   kAIRC_ProjPartial =             0x0001,   // partial LOF 
   kAIRC_ProjBlocked =             0x0002,   // LOF is blocked
};

///////////////////////////////////////

class cAINewRangedSubcombat;

///////////////////////////////////////

// passed to installed callbacks as data
struct sAIRangedCombatPhyscastData
{
   ObjID sourceID;   // ignore both of these
   ObjID targetID;   
   eAIRangedCombatProjTestFlags flags; // callback sets these
};

////////////////////////////////////////


#pragma pack()

#endif /* !__AICBRTYP_H */


