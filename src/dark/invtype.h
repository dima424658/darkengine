/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/invtype.h,v 1.3 1999/11/02 16:16:23 adurant Exp $
#pragma once  
#ifndef __INVTYPE_H
#define __INVTYPE_H

////////////////////////////////////////////////////////////
// PLAYER INVENTORY TYPES
//

typedef enum eWhichInvObj
{
   kCurrentWeapon,
   kCurrentItem,

   kNumItemSels
} eWhichInvObj;


typedef enum eCycleDirection
{
   kCycleForward  = +1,
   kCycleBackward = -1, 
} eCycleDirection;


typedef enum eInventoryType
{
   kInvTypeJunk = 0, // default
   kInvTypeItem,
   kInvTypeWeapon,
} eInventoryType; 

//
// Dark contain types
// That is, the list of possible sub-locations within a container
// 

typedef enum eDarkContainType
{
   // rendered types (negative numbers)
   kContainTypeAlt = -3,
   kContainTypeHand = -2, 
   kContainTypeBelt = -1,
   kContainTypeRendMin = kContainTypeAlt,
   kContainTypeRendMax = 0,

   // non-rendered
   kContainTypeNonRendMin = 0,
   kContainTypeGeneric = kContainTypeNonRendMin,  // generic contents
   kContainTypeInventory = kContainTypeGeneric,  // general inventory
   kContainTypeNonRendMax, 

   // total range
   kContainTypeMin = kContainTypeRendMin,
   kContainTypeMax = kContainTypeNonRendMax, 
} eDarkContainType; 

#endif // __INVTYPE_H
