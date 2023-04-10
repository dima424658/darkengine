/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brushgfh.h,v 1.9 2000/01/29 13:11:20 adurant Exp $
#pragma once

#ifndef __BRUSHGFH_H
#define __BRUSHGFH_H
#include <lg.h>
#include <rect.h>

// create the Brush Gadget From Hell.  bounds specifies
// the bounding rectangle for the gadget
EXTERN void CreateBrushGFH(Rect* bounds);
EXTERN void DestroyBrushGFH(void);

// forces a redraw and such
EXTERN void GFHRedrawPnPnSuch(void);

// tell the GFH to edit a different brush
EXTERN void GFHSetCurrentBrush(struct _editBrush*);

// tell the GFH to update
typedef enum 
{ 
   GFH_FORCE,   // force full update
   GFH_MINIMAL, // only update changes
   GFH_FRAME,   // this really means minimal, and safe to do region stuff
} GFHUpdateOp;

EXTERN void GFHUpdate(GFHUpdateOp op);

// hack for dealing with dual PnP stuff
EXTERN BOOL brushGFH_AlignNTextureUp(void);

// Coordinate mask

#define GFH_NO_X     0x0001
#define GFH_NO_Y     0x0002
#define GFH_NO_Z     0x0004

#define GFH_NO_D     0x0008
#define GFH_NO_W     0x0010
#define GFH_NO_H     0x0020

#define GFH_NO_BANK  0x0040
#define GFH_NO_PITCH 0x0080
#define GFH_NO_HEAD  0x0100

#define GFH_ALL_COORDS 0x0000

EXTERN void GFHSetCoordMask(ulong mask);

#endif // __BRUSHGFH_H
