/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkloot.h,v 1.3 1998/10/27 20:26:22 dc Exp $
#pragma once  
#ifndef __DRKLOOT_H
#define __DRKLOOT_H

#include <objtype.h>

//
// "LOOT" SYSTEM
//

EXTERN void DarkLootInit(); 
EXTERN void DarkLootTerm(); 

// Figure out starting loot.
EXTERN void DarkLootPrepMission();

// helper function
// count all loot (not held by except)
EXTERN int count_all_loot(ObjID except);

#endif // __DRKLOOT_H
