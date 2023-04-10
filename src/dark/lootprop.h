/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/lootprop.h,v 1.2 1998/08/25 18:53:18 mahk Exp $
#pragma once  
#ifndef __LOOTPROP_H
#define __LOOTPROP_H
#include <property.h>
#include <propdef.h>

////////////////////////////////////////////////////////////
// LOOT PROPERTY TYPE/INTERFACE
//

enum eLootTypes 
{
   kLootGold,
   kLootGems,
   kLootArt, 
   kLootSpecial, // Not a count, actually a bitmask 

   kNumLoot
} ; 

struct sLootProp
{
   int count[kNumLoot]; 
}; 

//
// PROPERTY INTERFACE
//

#undef INTERFACE
#define INTERFACE ILootProperty
DECLARE_PROPERTY_INTERFACE(ILootProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sLootProp*);
}; 

#undef INTERFACE 

EXTERN ILootProperty* InitLootProp(void); 

#endif // __LOOTPROP_H


