/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brrend.h,v 1.11 2000/01/29 13:11:18 adurant Exp $
// brush rendering controls
#pragma once

#ifndef __BRREND_H
#define __BRREND_H

#include <matrixs.h>
#include <editbrs.h>

// flags, defines, misc protos for per frame setup sort of stuff

// for color coding
#define brFlag_COLOR_GLOB     0     // use the global mask
#define brFlag_COLOR_NONE     1
#define brFlag_COLOR_DIST     2
#define brFlag_COLOR_TIME     3
#define brFlag_COLOR_MASK     3

#define brFlag_HotOnly        4
#define brFlag_IsoView        8

#define brFlag_FILTERSHIFT   (4)
#define brFlag_FILTERMASK    (0xFF<<brFlag_FILTERSHIFT)
#define brFlag_FILTER_TERR   (0x10<<brType_TERRAIN)
#define brFlag_FILTER_LIGHT  (0x10<<brType_LIGHT)
#define brFlag_FILTER_AREA   (0x10<<brType_HOTREGION)
#define brFlag_FILTER_OBJ    (0x10<<brType_OBJECT)
#define brFlag_FILTER_FLOW   (0x10<<brType_FLOW)
#define brFlag_FILTER_ROOM   (0x10<<brType_ROOM)

#define brFlag_GetFilterType(t) (1<<((t)+brFlag_FILTERSHIFT))

#define brFlag_NO_SELECTION  (0x1000)

#define brFlag_EDITBR_MASK   (0xffff)
// bottom 16 bits are brFlag Control

// set global draw context, color and filters
EXTERN void brushDrawContext(int filter, int color_mode);

// setup to draw, uses global context
// this is here so if we decide to make it per view, we easily can
EXTERN BOOL brushSetupDraw(int flags, int camera);

// for COMMAND system's convienience - directly modifiable
EXTERN int editbr_color_mode, editbr_filter;
EXTERN bool renderHotsOnly;
EXTERN bool renderHilightOnly;
EXTERN int editbr_filter_time_lo, editbr_filter_time_hi;
EXTERN float editbr_filter_size;

// filter utilities
BOOL brrendTypeFilterActive(void);
BOOL brrendCheckTypeFilter(editBrush *curBrush);

// fancy drawing/other wackiness

// assumes we are in an edit3d context, i guess.  hmmmmm...
EXTERN bool brushDraw(editBrush *curBrush);

// line primitives
EXTERN BOOL brushLineDraw(mxs_vector *p1, mxs_vector *p2, int color);
EXTERN float brushLineCheck(mxs_vector *p1, mxs_vector *p2, int x, int y);

// clickCheck returns a best distance for this figure
EXTERN float brushClickCheck(editBrush *curBrush, int x, int y);
// select fancy goes and sets the internal edge or point correctly
EXTERN BOOL brushSelectFancy(editBrush *curBrush, int x, int y);

// get boundary vectors for extent of brush
EXTERN BOOL brushCheckExtents(editBrush *curBrush, mxs_vector *bounds);

// Pick colors for editor
EXTERN void brushPickColors(void);   // in editbr

// if you want to get world coordinates for a given brush
EXTERN mxs_vector *brushBuildSolo(editBrush *us);

// call cback on all brushes drawn in last frame (ie. which pass the current
//   filter for the world)
typedef void (*editbr_callback)(editBrush *me);
EXTERN void brushRunOnActive(editbr_callback cback);

// conversion process for portalization
EXTERN void brushConvertStart(void);
EXTERN void brushConvertEnd(void);
EXTERN void *brushConvertPortal(editBrush *br);
// test (within convert) for being "in scene"
EXTERN BOOL brushConvertTest(editBrush *br);

// check if brush is filtered
EXTERN bool applyFilters(editBrush *curBrush);

#endif  // __BRREND_H
