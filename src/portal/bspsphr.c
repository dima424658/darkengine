/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/bspsphr.c,v 1.8 2000/02/19 12:32:40 toml Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   bspsphr.c

   In this module we intersect spheres with those leaves of the BSP
   which correspond to cells in the world rep.  It ignores, entirely,
   the question of whether the result is contiguous.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <mprintf.h>
#include <wrdb.h>
#include <wrfunc.h>
#include <wrbsp.h>
#include <bspsphr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// If we hit this limit, that's one biiiig level.  Or horribly
// unbalanced.
#define MAX_STACK_SIZE 200


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   When we find the set of cells intersecting a given sphere we don't
   check whether the sphere also intersects terrain.  So since the set
   of cells returned is complete, it may not be one contiguous space.

   We return the number of cells.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int portal_cells_intersecting_sphere(Location *loc, float radius, 
                                     int *output_list)
{
   wrBspNode *stack[MAX_STACK_SIZE];
   wrBspNode *pCurNode;
   PortalCell *cell;
   PortalPlane *plane, *end_plane;
   mxs_vector *normal;
   int stack_pos;
   int cell_id;
   mxs_real dist;
   float minus_radius = -radius;
   mxs_vector center = loc->vec;
   int num_cells = 0;

   // We only find our initial cell if we're told the location is
   // marked as invalid.  Otherwise, outside of a ship build anyhow,
   // we spew if the hint is bad.
   if (loc->cell == CELL_INVALID) {
      cell_id = CellFromLoc(loc);

      if (cell_id == CELL_INVALID) {
         Warning(("portal_cells_intersecting_sphere: bad loc.\n"));
         return 0;
      }
   }

#ifdef DBG_ON
   if (!PortalTestInCell(loc->cell, loc)) {
      Warning(("portal_cells_intersecting_sphere: loc with fabulous hint.\n"));
      return 0;
   }
#endif // DBG_ON

   // It's a lot quicker if our sphere is entirely within our initial
   // cell.
   cell = WR_CELL(loc->cell);
   plane = cell->plane_list;
   end_plane = plane + cell->num_planes;

   while (plane != end_plane) {
      normal = &plane->normal;
      dist = center.x * normal->x
           + center.y * normal->y
           + center.z * normal->z
           + plane->plane_constant;

      if (dist < radius)
         goto more_than_one_cell;

      plane++;
   }

   *output_list = loc->cell;
   return 1;

more_than_one_cell:

   stack_pos = 0;
   stack[0] = g_wrBspTree;

   while (stack_pos != -1) {
      pCurNode = stack[stack_pos];
      --stack_pos;

      if (wrBspIsLeaf(pCurNode)) {
         cell_id = pCurNode->cell_id;

         // Is this a real leaf, or an artifact of the solid medium in
         // the CSG BSP?
         if (cell_id != -1) {

            if (num_cells == (BSPSPHR_OUTPUT_LIMIT - 1)) {
#ifndef SHIP
               mprintf("portal_cells_intersecting_sphere: "
                       "too many cells @ (%g %g %g)\n",
                       loc->vec.x, loc->vec.y, loc->vec.z);
#endif
               return num_cells;
            }

            output_list[num_cells++] = cell_id;
         }

      } else {

         // It's an internal node.  If our sphere straddles the plane
         // we end up visiting both sides.
         if (wrBspIsReversed(pCurNode))
            dist = -mx_dot_vec(&center, &pCurNode->plane->normal) - pCurNode->plane->plane_constant;
         else
            dist = mx_dot_vec(&center, &pCurNode->plane->normal) + pCurNode->plane->plane_constant;

         if (dist > minus_radius)
         {      
            if (wrInsideIndex(pCurNode) != WRBSP_INVALID)
               stack[++stack_pos] = &g_wrBspTree[wrInsideIndex(pCurNode)];
         }

         if (dist < radius)
         {
            if (wrOutsideIndex(pCurNode) != WRBSP_INVALID)
               stack[++stack_pos] = &g_wrBspTree[wrOutsideIndex(pCurNode)];
         }

#ifndef SHIP
         AssertMsg(stack_pos < MAX_STACK_SIZE, 
                   "portal_cells_intersecting_sphere: "
                   "out of stack space.\n");
#endif // ~SHIP
      }
   }

   return num_cells;
}

