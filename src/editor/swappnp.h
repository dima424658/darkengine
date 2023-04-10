/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/swappnp.h,v 1.10 2000/01/29 13:13:16 adurant Exp $
#pragma once

#ifndef __SWAPPNP_H
#define __SWAPPNP_H

#include <gadget.h>
#include <brushgfh.h>
#include <editbr.h>

//
// API for connecting to the "plug and play" region of the GFH
//

typedef enum PNPkind
{
   PNP_TEXTURE,
   PNP_GRID,
   PNP_ALIGN,
   PNP_LIGHT,
   PNP_AREA,
   PNP_OBJ,
   PNP_FLOW,
   PNP_ROOM,
   PNP_USER,
   PNP_NULL
} PNPkind;

// go figure what sort of PnP to use for this brush
int PnP_ForBrush(editBrush *brush);

// Create the PNP of the specified kind and make it the current PNP
EXTERN void GFHOpen_a_PNP(int which, PNPkind kind, LGadRoot* root, Rect* bounds, editBrush* brush);
#define GFHOpenPNP(kind,root,bounds,brush) GFHOpen_a_PNP(0,kind,root,bounds,brush)

//
//  Update the current pnp
//

EXTERN void GFHUpdate_a_PNP(int which, GFHUpdateOp op, editBrush* brush);
#define GFHUpdatePnP(op,brush) GFHUpdate_a_PNP(0,op,brush)

//
// Destroy the current pnp
//

EXTERN void GFHClose_a_PNP(int which);
#define GFHClosePnP() GFHClose_a_PNP(0)

/// check for am i one of the PnPs up
// returns -1 if not, else returns the PnP idx of the other one
int GFH_PNP_Control_2Up(int me);

////////////////////////////////////////////////////////////
// The swappable button region is much like the PNP, but its 
// gadgets tend to be simpler.
////////////////////////////////////////////////////////////

typedef enum GFHSwapKind
{
   GFH_SWAP_CREATE,
   GFH_SWAP_LAYOUT,
   GFH_SWAP_SCROLL,
   GFH_SWAP_FILTER,
   GFH_SWAP_TEST,
} GFHSwapKind;

// Create the swappable of the specified kind and make it the current swappable
EXTERN void GFHOpenSwappable(GFHSwapKind kind, LGadRoot* root, Rect* bounds, editBrush* brush);

//  Update the current swappable
EXTERN void GFHUpdateSwappable(GFHUpdateOp op, editBrush* brush);

// Destroy the current swappable
EXTERN void GFHCloseSwappable(void);

// Get current swappable.
EXTERN GFHSwapKind GFHCurrentSwappable(void);

#endif // __SWAPPNP_H
