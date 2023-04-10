/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editsave.h,v 1.6 2000/01/29 13:11:51 adurant Exp $
// editor and save load 
#pragma once

#ifndef __EDITSAVE_H
#define __EDITSAVE_H

// save out the editor components of a Cow
EXTERN BOOL editor_SaveCow(ITagFile *file);
EXTERN BOOL editor_LoadCow(ITagFile *file);
EXTERN BOOL editor_PostLoadCow(void); 

// save out minibrush (ie. multibrush brush only) info
EXTERN BOOL editor_SaveMiniBrush(char *fname, int group);
EXTERN BOOL editor_LoadMiniBrush(char *fname, int new_group);

// dump stats to text buffer
EXTERN void _editsave_text_info(char *buf);

// say NO to textures
EXTERN bool _editor_loadNoTextures;

EXTERN void VbrushSaveLoadInit();

#endif  // __EDITSAVE_H
