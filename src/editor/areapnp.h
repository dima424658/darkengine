/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/areapnp.h,v 1.3 2000/01/29 13:11:09 adurant Exp $
// areapnp named file block support
#pragma once

#ifndef __AREAPNP_H
#define __AREAPNP_H

#include <tagfile.h>
#include <brlist.h>

// reset the names to nothing
EXTERN void AreaPnP_Names_Reset(void);

// load a new set from the tagfile
EXTERN BOOL AreaPnP_Names_Load(ITagFile *file);

// for now, a horror to fixup old broken area pnp things
EXTERN void AreaPnP_Names_Fixup(void);

// delete this hotregions name tags
EXTERN void AreaPnP_Delete_Brush_Tags(editBrush *us);

// go save them off
EXTERN BOOL AreaPnP_Names_Save(ITagFile *file);

// really just needs to retag the list
EXTERN void AreaPnP_Names_Remap(int *mapping_array);

// for app init
EXTERN void AreaPnP_Names_Init(void);

// and app close
EXTERN void AreaPnP_Names_Term(void);


#endif  // __AREAPNP_H
