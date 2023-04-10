/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aialert.h,v 1.10 1999/11/29 12:27:14 BFarquha Exp $
//
// Alertness/awareness structures & enums
//

#ifndef __AIALERT_H
#define __AIALERT_H

#include <aitype.h>

#pragma once
#pragma pack(4)



///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIFrustrated
// yeah, yeah. It's as good a place as any to put this.


struct IAIPath;

struct sAIFrustrated
{
   sAIFrustrated()
    : nSourceId(0),
      nDestId(0)
   {
   }

   int nSourceId; // Source of our frustration
   int nDestId; // If non-0, where we'd like to go when frustrated.
   IAIPath *pPath; // option path to object defined.
};


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIAlertness
//

struct sAIAlertness
{
   sAIAlertness()
    : level(kAIAL_Lowest),
      peak(kAIAL_Lowest)
   {
   }

   // Current alertness
   eAIAlertLevel level;

   // Current alertness
   eAIAlertLevel peak;
};

///////////////////////////////////////////////////////////////////////////////

struct sAIAlertCap
{
   sAIAlertCap()
    : maxLevel(kAIAL_High),
      minLevel(kAIAL_Lowest),
      minRelax(kAIAL_Low)
   {
   }

   // Maximum alerness cap
   eAIAlertLevel maxLevel;

   // Minimum alerness cap
   eAIAlertLevel minLevel;

   // Minimum alerness cap
   eAIAlertLevel minRelax;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIALERT_H */
