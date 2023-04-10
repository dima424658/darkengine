/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/wrbsp.h,v 1.6 2000/01/31 10:00:54 adurant Exp $
#pragma once

#ifndef __WRBSP_H__
#define __WRBSP_H__

#include <matrixs.h>
#include <bspdata.h>
#include <wr.h>

////////////////////////////////////////

#define WRBSP_INVALID  (0x00FFFFFF)
#define WRBSP_HEAD     (0)

////////////////////////////////////////

typedef enum
{
   kIsLeaf      = 0x01,
   kIsMarked    = 0x02,
   kIsReversed  = 0x04
} eWrBspNodeFlags;

////////////////////////////////////////

#define wrBspIsLeaf(x)     ((x)->flags & kIsLeaf)
#define wrBspIsMarked(x)   ((x)->flags & kIsMarked)
#define wrBspIsReversed(x) ((x)->flags & kIsReversed)

#define wrBspSetLeaf(x)  ((x)->flags |= kIsLeaf)
#define wrBspSetNode(x)  ((x)->flags &= ~kIsLeaf)

#define wrBspMark(x)   ((x)->flags |= kIsMarked)
#define wrBspUnmark(x) ((x)->flags &= ~kIsMarked)

#define wrBspSetReversed(x)  ((x)->flags |= kIsReversed)
#define wrBspSetNormal(x)    ((x)->flags &= ~kIsReversed)

#define wrSetParent(x,y)  ((x)->parent_index = ((x)->parent_index & 0xFF000000) | (y & 0x00FFFFFF))

#define wrParentIndex(x)  ((x)->parent_index & 0x00FFFFFF)
#define wrInsideIndex(x)  ((x)->inside_index)
#define wrOutsideIndex(x) ((x)->outside_index)

////////////////////////////////////////

EXTERN wrBspNode *g_wrBspTree;
EXTERN int g_wrBspTreeSize;

typedef void (*PortalReadWrite) (void *buf, size_t elsize, size_t nelem);

////////////////////////////////////////

EXTERN uint wrBspTreeCopy(BspNode *node, uint parent_id);
EXTERN void wrBspTreeDeallocate();
EXTERN void wrBspTreeRefCells(uint node_index);

EXTERN void wrBspTreeAddPostSplit(BspNode *n, int cell_id);
EXTERN void wrBspTreeApplyPostSplits();

EXTERN void wrBspTreeDump();

EXTERN void wrBspTreeWrite(PortalReadWrite write);
EXTERN void wrBspTreeRead(PortalReadWrite read);

////////////////////////////////////////

#endif





