/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidfndpt.h,v 1.4 1998/07/28 21:18:38 TOML Exp $
//
//
//

#ifndef __AIDFNDPT_H
#define __AIDFNDPT_H

#include <aitype.h>
#include <aidist.h>

#pragma pack(4)
#pragma once

struct sAIDefendPoint;
typedef long LinkID;
F_DECLARE_INTERFACE(IInternalAI);

///////////////////////////////////////////////////////////////////////////////
//
// Get the defend point of the AI. Do not store the pointer.
//

const sAIDefendPoint * AIGetDefendPoint(ObjID objId);


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIDefendPoint
//

enum eAIDefendFlags
{
   // Does the point apply to combat as well?
   kAIDF_RestrictsCombat = 0x01, 

   kAIDF_All = 0xffffffff,
};

///////////////////////////////////////

#define kAIMaxDfndRanges 3
#define kAINoDefend kAIMaxDfndRanges

struct sAIDefendPoint
{
   BOOL InRange(const mxs_vector & testPos) const
   {
      if (object && iActive != kAINoDefend)
         return AIInsideCylinder(testPos, location, sq(ranges[iActive].radius), ranges[iActive].height / 2);
      return TRUE;
   }

   //
   // Refreshed fields (not shown in sdesc)
   //
   
   // The defend point object
   ObjID         object;
   
   // The location of the defend point object
   mxs_vector    location;
   
   // The index of the active defend point range, or kAINoDefend if none
   int           iActive;
   
   // The approximate distance squared from the AI to the defend point
   float         distSq;

   // The associated link id
   LinkID        linkID;

   // Whether AI is inside the active range
   BOOL          fInRange;
   
   DWORD         reserved[3];
   
   //
   // Designer fields (some not shown right now (toml 07-24-98))
   //
   eAIPriority   priority;
   unsigned      flags;
// @TBD (toml 07-26-98): 
   unsigned      reservedForReturnType;
   eAISpeed      returnSpeed;
   char          returnTags[64];
   DWORD         reserved2[4];
   
   // Nested range struct
   struct sRange
   {
      int           radius;
      int           height;
      
      eAIAlertLevel minAlert;
      eAIAlertLevel maxAlert;
      
      DWORD         reserved[4];
   };
   
   sRange ranges[kAIMaxDfndRanges];
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIDFNDPT_H */
