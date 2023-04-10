/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/wrfunc.h,v 1.9 1998/02/11 11:38:53 MAT Exp $

// World Representation Functions

#ifndef __WRFUNC_H
#define __WRFUNC_H

#include <lg.h>        // bool
#include <wrtype.h>
#include <wrdb.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // the way cell references work is like this:
  //   the first time you reference a cell in a given
  //   frame, use WR_CELL(n) to get a pointer to it
  //   after that, you can use wr_cell[n] to refer to
  //   it, until the end of the frame.
  //
  //   Every time you call ResetWorldRep(), you can no
  //   longer count on data which you have dangled off
  //   of the world rep, and you need to call WR_CELL
  //   again.
  //
  //   In reality, for this round, we're not going to
  //   swap the db to disk.

extern PortalCell *wr_cell[];
extern int wr_num_cells;
extern wrBspNode *wrBspTree;
void ResetWorldRep(void);

#ifndef DYNAMIC_CELLS
  #define WR_CELL(n)  (wr_cell[n])
#else
  #define WR_CELL(n)  (wr_cell[n] ? wr_cell[n] : LoadWorldRepCell(n))
  PortalCell *LoadWorldRepCell(int n);
#endif

  // A macro which if true means that we definitely have a valid
  // cell, and we can quickly compute it
#define IS_CELLFROMLOC_FAST(loc)   ((loc)->cell != CELL_INVALID)

#define CellFromLoc(loc) (IS_CELLFROMLOC_FAST(loc) ? (loc)->cell : ComputeCellForLocation(loc))
#define CellFromPos(p)   CellFromLoc(&((p)->loc))

// Macros for the worldrep bsp tree
#define wrIS_LEAF(b)   (b->leaf)
#define wrIS_NODE(b)   (!b->leaf)
#define wrIS_ROOT(b)   (b->parent == NULL)
#define wrIS_MARKED(b) (b->mark)
#define wrMARK(b)      (b->mark = TRUE)
#define wrUNMARK(b)    (b->mark = FALSE)

int ComputeCellForLocation(Location *loc);
int PortalComputeCellFromPoint(mxs_vector *seed_pos);
bool PortalTestInCell(int r, Location *loc);

void PortalComputeBoundingSphere(PortalCell *cell);

// for opening and closing doors
int PortalBlockVision(PortalCell *starting_cell);
int PortalBlockVisionFromLocation(Location *seed_loc);
int PortalUnblockVision(PortalCell *starting_cell);
int PortalUnblockVisionFromLocation(Location *seed_loc);

#ifdef __cplusplus
};
#endif

#endif

