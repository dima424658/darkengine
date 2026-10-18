/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editsave.h,v 1.6 2000/01/29 13:11:51 adurant Exp $
// editor and save load 
#pragma once

#ifndef __EDITSAVE_H
#define __EDITSAVE_H

#include <tagfile.h>
#include <objtype.h>

// save out the editor components of a Cow
EXTERN BOOL editor_SaveCow(ITagFile *file);
EXTERN BOOL editor_LoadCow(ITagFile *file);
EXTERN BOOL editor_PostLoadCow(void); 

// A retail .MIS normally has its source brush blocks stripped after
// portalization.  Keep that distinct from an intentionally empty editor map
// so views can offer an honest, display-only compiled-geometry fallback.
EXTERN BOOL editor_BrushDataPresent(void);
EXTERN void editor_ResetBrushDataState(void);
EXTERN ulong editor_BrushDataGeneration(void);
// Object brushes are serialized in BRLIST even though their transforms are
// rebuilt from the object database.  This membership test preserves the
// author's deliberate HasBrush exclusions when reconstructing that view.
EXTERN BOOL editor_ObjectHasSourceBrush(ObjID object);

// save out minibrush (ie. multibrush brush only) info
EXTERN BOOL editor_SaveMiniBrush(char *fname, int group);
EXTERN BOOL editor_LoadMiniBrush(char *fname, int new_group);

// dump stats to text buffer
EXTERN void _editsave_text_info(char *buf);

// say NO to textures
EXTERN bool _editor_loadNoTextures;

EXTERN void VbrushSaveLoadInit();

#endif  // __EDITSAVE_H
