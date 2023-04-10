/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editbr.h,v 1.17 2000/01/29 13:11:31 adurant Exp $
#pragma once

//
// Brush Manager (editbr.h)
//    this system manages the creation, parameterization, and modification
// of a single brush structure.  this includes any edits to the media, 
// surface textures/parameters, sizing and stretching of the brush
// itself.  this does not include brush lists or construction.  
//    - struct editBrush
//    * editBrush *brushInstantiate(int brush_id)
//        instantiates a copy of brush_id into the edit buffer
//    * bool brushReload(editBrush *curBrush)
//        reloads the brush with parameters in editBrush
//    * bool brushDraw(editBrush *curBrush)
//        assumes we are in an edit3d context, i guess.  hmmmmm...
//

#ifndef __EDITBR_H
#define __EDITBR_H

//////////////////////////  - split this file
// basic brush system functions/support

#include <matrixs.h>
#include <editbrs.h>

// misc constants for use in picks - distance to virtual infinity
#define DISTANCE_INF     (1.0e20)

// sets the current heap for the brush system to use
EXTERN void brushSysInit(void);

// sets the current heap for the brush system to use
EXTERN void brushSysFree(void);

// zeroes all the data in an editBrush to that primal_brush
EXTERN void brushZero(editBrush *curBrush, int primal_brush);

// instantiates a copy of brush_id into the edit buffer
#define DEF_PRIMAL (-1)
EXTERN editBrush *brushInstantiate(int brush_id);
EXTERN int editbr_default_primal;  // if you need to set this, but really, for gedit

// makes a copy of the srcBrush into edit buffer/cur brush
EXTERN editBrush *brushCopy(editBrush *srcBrush);

// delete a brush - better get it out of list first
EXTERN bool brushDelete(editBrush *curBrush);

// goes through and clears and frees all brushes in memory
EXTERN void brushClearAll(void);

// file access stuff, assumes fp is pointing at data to do, FALSE if fails
EXTERN bool brushWritetoFile(editBrush *brdata, int out_hnd);

// warning, this returns the same address every time
// returns NULL if nothing else in file
// the idea is that you will copy the data out into your own real space
EXTERN editBrush *brushReadfromFile(int in_hnd);

#endif  // __EDITBR_H
