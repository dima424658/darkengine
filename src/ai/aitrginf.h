/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aitrginf.h,v 1.5 2000/01/31 09:34:13 adurant Exp $
//
//
//
#pragma once

#ifndef __AITRGINF_H
#define __AITRGINF_H

///////////////////////////////////////////////////////////////////////////////
//
// AI Combat shared target information
//

///////////////////////////////////////

enum eAITargetFlags
{
   kAITF_CanRaycast   = 0x01,
   kAITF_Behind       = 0x02,
   kAITF_XXBehind       = 0x04,

};

///////////////////////////////////////

enum eAICombatRange
{
   kAICR_None,
   kAICR_Near,
   kAICR_Norm,
   kAICR_JustFar,
   kAICR_Far,
   kAICR_Huge,
   kAICR_HugeZ,
   kAICR_Unseen,

   kAICR_IntMax = 0xffffffff
};

///////////////////////////////////////

struct sAITargetInfo
{
   // Object
   ObjID                id;

   // Presumed location
   mxs_vector           loc;
   float                zLocNearFloor;
   tAIPathCellID        cell;

   // Raw distance, angle
   float                distSq;
   floatang             targetHeading;       // the heading of the target from us
   floatang             targetHeadingDelta;  // delta between our heading & above

   float                zdist; // z distance
   float                fStrikeRange; // for pathing near enough to strike.

   // Tactical range
   eAICombatRange       range;

   // Flags
   unsigned             flags;

   // Raw awareness
   const sAIAwareness * pAwareness;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AITRGINF_H */
