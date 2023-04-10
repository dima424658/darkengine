/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/hilight.h,v 1.6 2000/01/29 13:12:20 adurant Exp $
// brush highlighting system
#pragma once

#ifndef __HILIGHT_H
#define __HILIGHT_H

#include <editbrs.h>

// actual hilight data
EXTERN uchar hilight_array[];
EXTERN uchar hilight_active;

// does current brush contain active highlight
#define isActiveHighlight(id) ((hilight_array[id]&hilight_active)!=0)

// clear all bits from bitfield in all hilight entries
EXTERN void hilightClearBits(int bitf);

// register commands
EXTERN void hilightCommandRegister(void);

// the horror, for use in brush queries
EXTERN BOOL highlight_check(editBrush *br);
   
// start/stop hilighting.  For commands that set hilights
EXTERN void hilight_begin(void);
EXTERN void hilight_end(ulong flags); 

enum hilight_end_flags
{
   kHilightCancel = 0,
   kHilightDone  = 1 << 0,
};

// ways to add a hilight from external systems (only should be used inside
// a begin/end pair)
EXTERN BOOL hilightAddByObjId(int obj_id);
EXTERN BOOL hilightAddByBrushId(int brush_id);
EXTERN BOOL hilightRemoveByObjId(int obj_id);
EXTERN BOOL hilightRemoveByBrushId(int brush_id);

//////////////////
// for "reserved" bits
#define PORTAL_HILIGHT_BIT (1<<7)

#endif  // __HILIGHT_H
