/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objremap.h,v 1.3 2000/01/29 13:24:01 adurant Exp $
#pragma once

#ifndef __OBJREMAP_H
#define __OBJREMAP_H

#include <comtools.h>
#include <objtype.h>

//------------------------------------------------------------
// OBJ ID REMAPPING API
//
// This system is used in re-connecting the map/mission to the game system 
// on load, to tolerate versioning of the game system behind the map/mission's 
// back.   
//
// Basically, we save a table of all the symnames of all the abstract objects. 
// At load time, we re-map the mission's abstract ObjIDs in a symname-preserving 
// way.  
//

F_DECLARE_INTERFACE(ITagFile);

//
// Saving and Loading the mapping table.
//

EXTERN void ClearObjMappingTable(void);
EXTERN void SaveObjMappingTable(ITagFile* file);
EXTERN void LoadObjMappingTable(ITagFile* file);

//
// Add a mapping to the table
//

EXTERN void AddObjMappingToTable(ObjID targ, ObjID src); 

//
// Remap on load based on the current mapping table.
//

EXTERN ObjID ObjRemapOnLoad(ObjID obj);

#endif // __OBJREMAP_H







