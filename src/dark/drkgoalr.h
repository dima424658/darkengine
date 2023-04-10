/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkgoalr.h,v 1.2 1998/09/18 19:50:49 mahk Exp $
#pragma once  
#ifndef __DRKGOALR_H
#define __DRKGOALR_H
#include <str.h>
#include <comtools.h>

//////////////////////////////////////////////////////////////
//
// GOAL RESOURCE NAMES 
//
//////////////////////////////////////////////////////////////

//------------------------------------------------------------
// DESCRIPTIVE GOAL STRINGS 
//

//
// String descriptions of goals are stored in a string resoure table.  
// We swap out a table for each mission
// 

// Resname of goal string table.  We might just want to use the generic "mission" one.
extern cStr GoalStringTable(); 

// Short goal description string 
extern cStr GoalDescription(int i);
 
// Long fictional goal description 
extern cStr GoalFiction(int i); 

// Goal status icon 
F_DECLARE_INTERFACE(IDataSource); 
extern IDataSource* GoalStatus(int state); 




#endif // __DRKGOALR_H

