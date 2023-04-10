/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/bspsphr.c,v 1.2 1997/11/18 15:06:31 MAT Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   bspsphr.c

   In this module we intersect spheres with those leaves of the BSP
   which correspond to cells in the world rep.  It ignores, entirely,
   the question of whether the result is contiguous.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#include <wrdb.h>
#include <wrfunc.h>
#include <bspsphr.h>


// This limit really depends on the caller, but this should let us
// catch pathological failure.
#define OUTPUT_LIMIT 300

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
   wrBspNode *BSP;
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
#ifndef SHIP
      if (cell_id == CELL_INVALID) {
         Warning(("portal_cells_intersecting_sphere: bad loc.\n"));
         return 0;
      }
#endif // ~SHIP
   }

#ifdef DBG_ON
   if (!PortalTestInCell(loc->cell, loc)) {
      Warning(("portal_cells_intersecting_sphere: loc with fabulous hint.\n"));
      return 0;
   }
#endif // ~SHIP

   // It's a lot quicker if our sphere is entirely within our initial
   // cell.
   cell = WR_CELL(loc->cell);
   plane = cell->plane_list;
   end_plane = plane + cell->num_planes;

   while (plane != end_plane) {
      normal = &plane->norm->raw;
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
   stack[0] = wrBspTree;

   while (stack_pos != -1) {
      BSP = stack[stack_pos];
      --stack_pos;

      if (BSP->leaf == TRUE) {
         cell_id = BSP->cell_id;

         // Is this a real leaf, or an artifact of the solid medium in
         // the CSG BSP?
         if (cell_id != -1) {
            output_list[num_cells++] = cell_id;
#ifndef SHIP
            AssertMsg(num_cells < OUTPUT_LIMIT, 
                      "portal_cells_intersecting_sphere: "
                      "too many cells generated.\n");
#endif // ~SHIP
         }

      } else {

         // It's an internal node.  If our sphere straddles the plane
         // we end up visiting both sides.
         dist = center.x * BSP->plane.x
              + center.y * BSP->plane.y
              + center.z * BSP->plane.z
              + BSP->plane.d;

         if (dist > minus_radius)
            stack[++stack_pos] = BSP->inside;

         if (dist < radius)
            stack[++stack_pos] = BSP->outside;

#ifndef SHIP
         AssertMsg(stack_pos < MAX_STACK_SIZE, 
                   "portal_cells_intersecting_sphere: "
                   "out of stack space.\n");
#endif // ~SHIP
      }
   }

   return num_cells;
}
