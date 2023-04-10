/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/wrfunc.c,v 1.26 2000/02/19 13:18:56 toml Exp $

// World Representation Functions

#include <wrtype.h>
#include <wrdb.h>
#include <wrfunc.h>
#include <wrbsp.h>
#include <matrix.h>
#include <math.h>
#include <mprintf.h>
#include <cfgdbg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


static int FindLeaf(Location *loc)
{
   mxs_real distance;
   mxs_vector *point = &loc->vec;

   wrBspNode *pCurNode = g_wrBspTree;

   while (!wrBspIsLeaf(pCurNode)) 
   {
      if (wrBspIsReversed(pCurNode))
         distance = -mx_dot_vec(point, &pCurNode->plane->normal) - pCurNode->plane->plane_constant;
      else
         distance = mx_dot_vec(point, &pCurNode->plane->normal) + pCurNode->plane->plane_constant;

      if (distance < 0.0)
      {
         if (wrOutsideIndex(pCurNode) == WRBSP_INVALID)
            return CELL_INVALID;

         pCurNode = &g_wrBspTree[wrOutsideIndex(pCurNode)];
      }
      else
      {
         if (wrInsideIndex(pCurNode) == WRBSP_INVALID)
            return CELL_INVALID;

         pCurNode = &g_wrBspTree[wrInsideIndex(pCurNode)];
      }
   }
   return pCurNode->cell_id;
}

  // Cache the current cell
int ComputeCellForLocation(Location *loc)
{
   int cell_id;

   if (!wr_num_cells) {
#ifndef SHIP
      ConfigSpew("LocSpew",
                 ("You can't find a Location when there's no world rep!\n"));
#endif // ~SHIP
      return (loc->cell = CELL_INVALID);
   }

   if (loc->hint != CELL_INVALID) {

      // test if we're still in the hinted cell

      // make sure this is DB valid in case we're in editor and just
      // rebuilt the level

      if (loc->hint < wr_num_cells && loc->hint >= 0) {
         if (PortalTestInCell(loc->hint, loc))
            return (loc->cell = loc->hint);   // put in cache and return
      }
   }

   cell_id = FindLeaf(loc);
   loc->hint = cell_id;

   if (cell_id != CELL_INVALID && PortalTestInCell(cell_id, loc))
      return (loc->cell = cell_id);
   else
      return (loc->cell = CELL_INVALID);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is a wrapper for ComputeCellForLocation(), above, for when we
   want to pass in a vector instead of a location.  It returns an
   index into wr_cell[].

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int PortalComputeCellFromPoint(mxs_vector *seed_pos)
{
   Location loc;

   loc.vec.x = seed_pos->x;
   loc.vec.y = seed_pos->y;
   loc.vec.z = seed_pos->z;
   loc.cell = CELL_INVALID;
   loc.hint = CELL_INVALID;

   // Is there a cell at our coordinates?
   return ComputeCellForLocation(&loc);
}


double PortalDistPointFromPlane(PortalPlane *p, Location *loc)
{
   return mx_dot_vec(&p->normal, &loc->vec) + p->plane_constant;
}

bool PortalTestLocationInsidePlane(PortalPlane *p, Location *loc)
{
   // please, compiler, inline all of this
   return PortalDistPointFromPlane(p,loc) >= 0;
}

bool PortalTestInCell(int r, Location *loc)
{
   PortalPlane *p = WR_CELL(r)->plane_list;
   int i, n = WR_CELL(r)->num_planes;

   // check if we're on the positive side of every side

   for (i=0; i < n; ++i, ++p) {
      if (!PortalTestLocationInsidePlane(p, loc))
         return FALSE;
   }
   return TRUE;
}

#define TOO_CLOSE    0.05
#define ADJUST_DIST  0.06

void PortalMovePointInsideCell(Location *loc)
{
   mxs_vector *vec = &loc->vec;
   int cell = CellFromLoc(loc);
   PortalPlane *p;
   int i,n, iter;
   bool changed;

   if(cell==CELL_INVALID)
      return;

   n = WR_CELL(cell)->num_planes;
   // check if we're very close to any planes
   iter = 0;
   do {
      p = WR_CELL(cell)->plane_list;
      changed = FALSE;
      for (i=0; i < n; ++i,++p) {
         double dist = PortalDistPointFromPlane(p, loc);
         if (dist < TOO_CLOSE) {
            mx_scale_addeq_vec(vec, &p->normal, ADJUST_DIST-dist);
            changed = TRUE;
         }
      }
      ++iter;
   } while (changed && iter < n);
}

// This is a utility function for constructing
// the portal representation, so I could have put
// it in the editor code instead, but I figured it's
// pretty generic...

// This is from a graphcis gem
void PortalComputeBoundingSphere(PortalCell *cell)
{
   int n = cell->num_vertices, i;
   Vertex *vpool = cell->vpool, *this;
   Vertex xmin,ymin,zmin,xmax,ymax,zmax;
   Vertex center;
   mxs_real radius, rad2, xlen, ylen, zlen, dist, dist_2;

   // find the points at maximal locations along the axes

   xmin = ymin = zmin = xmax = ymax = zmax = vpool[0];
   for (i=1, this=vpool+1; i < n; ++i, ++this) {
      if (this->x < xmin.x) xmin = *this;
      else if (this->x > xmax.x) xmax = *this;
      if (this->y < ymin.y) ymin = *this;
      else if (this->y > ymax.y) ymax = *this;
      if (this->z < zmin.z) zmin = *this;
      else if (this->z > zmax.z) zmax = *this;
   }

   // determine which pair (xmin,xmax), (ymin,ymax), (zmin,zmax)
   // has the maximal sphere

   xlen = mx_dist2_vec(&xmin, &xmax);
   ylen = mx_dist2_vec(&ymin, &ymax);
   zlen = mx_dist2_vec(&zmin, &zmax);

   if (ylen > xlen && ylen > zlen) {
      xmin = ymin;
      xmax = ymax;
   } else if (zlen > xlen) {
      xmin = zmin;
      xmax = zmax;
   }

   // compute initial sphere

   mx_interpolate_vec(&center, &xmin, &xmax, 0.5);
   radius = mx_dist_vec(&center, &xmin);

   // refine the sphere to contain all points

   rad2 = radius * radius;
   for (i=0, this = vpool; i < n; ++i,++this) {
      dist_2 = mx_dist2_vec(&center, this);
      if (dist_2 > rad2) {
         dist = sqrt(dist_2);

         // our new circle will have a diameter of radius + dist
         radius = (dist + radius)/2;
         rad2 = radius*radius;

         // now we need to interpolate from the old center point
         // towards the new end point

         // the distance from the old center to the new point is dist
         // we want to move along the line until the new center is
         // equadistant from the new point and the old back location

         // That means: dist from new center to new point is new_radius
         // Total distance is dist.  So we want to move by
         // (dist-new_radius)/dist

         mx_interpolate_vec(&center, &center, this, (dist-radius)/dist);
      }
   }

   /*
   for (i=0; i < n; ++i)
      if (mx_dist2_vec(&center, &vpool[i]) > rad2 + 0.01)
         Error(1, "Failed to create bounding sphere.\n");
   */

   cell->sphere_center = center;
   cell->sphere_radius = radius;
}

/*
 * portal_crosses_bbox:
 *   We want to know if a particular portal is partly within a bbox.
 *   We use an inexact conservative solution.  This approach
 *   is exact for comparing a line segment against a 2d bbox,
 *   but not in 3d.
 *
 *   A description in 2d: we "point code" the line segment vs.
 *   the 2d bbox, and check for trivial reject.  Then we check
 *   the points of the bbox against the infinite line of the
 *   line segment; if they cross, the line segment is partly
 *   within.
 */
bool portal_crosses_bbox(PortalCell *r, PortalPolygonCore *p,
           mxs_vector *mn, mxs_vector *mx, int vl)
{
   // first check the bbox against the plane
   PortalPlane *plane = &r->plane_list[p->planeid];
   mxs_vector rel_min, rel_max;
   mxs_real min_dist, max_dist;
   int code, i;
   uchar *vlist;
   Vertex *vpool;

   // compute the min and max dot products of the bbox
   // against the normal, using the graphics gem hack

   if (plane->normal.x < 0)
      rel_min.x = mx->x, rel_max.x = mn->x;
   else
      rel_min.x = mn->x, rel_max.x = mx->x;

   if (plane->normal.y < 0)
      rel_min.y = mx->y, rel_max.y = mn->y;
   else
      rel_min.y = mn->y, rel_max.y = mx->y;

   if (plane->normal.z < 0)
      rel_min.z = mx->z, rel_max.z = mn->z;
   else
      rel_min.z = mn->z, rel_max.z = mx->z;

   min_dist = mx_dot_vec(&rel_min, &plane->normal) + plane->plane_constant;
   max_dist = mx_dot_vec(&rel_max, &plane->normal) + plane->plane_constant;

   if (min_dist > 0 || max_dist < 0)
      return FALSE;

   vlist = r->vertex_list + vl;
   vpool = r->vpool;

   // now point code all of the points against the planes
   code = (1 << 6) - 1;

   for (i=0; i < p->num_vertices; ++i) {
      Vertex *v = &vpool[vlist[i]];
      if (v->x > mn->x) code &= ~1;
      if (v->x < mx->x) code &= ~2;
      if (v->y > mn->y) code &= ~4;
      if (v->y < mx->y) code &= ~8;
      if (v->z > mn->z) code &= ~16;
      if (v->z < mx->z) code &= ~32;
   }
   if (code) // trivial reject on some side
      return FALSE;
   return TRUE;
}

/*
 * PortalCellsInArea:
 *
 *   Given a world-aligned bbox and a "seed" location, this
 *   returns all of the cells which are reachable from the
 *   seed without going outside the bbox.
 *
 *   E.g. the bbox represents an object, and the seed is the
 *   center of the object.  If the bbox crosses both ends of
 *   a U without going through the middle, it will only return
 *   the end containing the cell.
 */

#define CELL_VISITED(x)       ((WR_CELL(x)->flags) & CELL_TRAVERSED)
#define SET_CELL_VISITED(x)   ((WR_CELL(x)->flags) |= CELL_TRAVERSED)
#define CLEAR_CELL_VISITED(x) ((WR_CELL(x)->flags) &= ~CELL_TRAVERSED)

// global variable used for object splitting hack
uchar portal_area_flags;

int PortalCellsInArea(int *cell_list, int list_max,
   mxs_vector *mn, mxs_vector *mx, Location *seed)
{
   int count, i, j, vl;
   int cell = CellFromLoc(seed);

   count = 1;
   cell_list[0] = cell;

   // is our seed point in the world?
   if (cell == CELL_INVALID)
      return 0;

   // is it an empty bounding box?
   if (mn->x == mx->x && mn->y == mx->y && mn->z == mx->z)
      return 1;

   SET_CELL_VISITED(cell);

   // breadth first search using the passed in list for storage
   portal_area_flags = 0;

   for (i=0; i < count; ++i) { // note count increases as we go
      PortalCell *p = WR_CELL(cell_list[i]);
      vl = p->portal_vertex_list;
      for (j=0; j < p->num_portal_polys; ++j) {
         cell = p->portal_poly_list[j].destination;
         if (!CELL_VISITED(cell)) {
            if (portal_crosses_bbox(p, &p->portal_poly_list[j], mn, mx, vl)) {
               portal_area_flags |= p->portal_poly_list[j].flags;
               cell_list[count++] = cell;
               SET_CELL_VISITED(cell);
               // have we run out of room to store cells?
               if (count == list_max)
                  goto done;
            }
         } else if (p->portal_poly_list[j].flags)
            if  (portal_crosses_bbox(p, &p->portal_poly_list[j], mn, mx, vl))
               portal_area_flags |= p->portal_poly_list[j].flags;
         vl += p->portal_poly_list[j].num_vertices;
      }
   }

 done:
   for (i=0; i < count; ++i)
      CLEAR_CELL_VISITED(cell_list[i]);
   return count;
}


uchar _portal_target_cell_flags;
uchar _portal_flags_set;
uchar _portal_flags_mask;
uchar _portal_flags_visit_test;

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Here's the recursive core for cell marking.  We mark cells with
   given flag bits set, starting from a given cell and exploring
   through the portals.

   Like all of these blocking thingbobs, it returns the number of
   cells marked.

   If we want to use this for bigger stuff than doors, we should
   switch to explicit recursion.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static int _PortalFlagGroupOfCells(PortalCell *cell)
{
   PortalPolygonCore *portal;
   PortalCell *adjacent_cell;
   int i;
   uchar adjacent_flags;
   int cells_marked = 1;            // start by counting this cell

   cell->flags = (cell->flags & _portal_flags_mask) | _portal_flags_set;

   portal = cell->portal_poly_list;

   // We recurse on any bordering cells we haven't been to, and should.
   // No chit.
   for (i = cell->num_portal_polys - 1; i >= 0; i--) {
      adjacent_cell = wr_cell[portal->destination];
      adjacent_flags = adjacent_cell->flags;

      // Two requirements for recursing: it's got our target flags
      // set, and the other flags are not yet as we want them.
      if (((adjacent_flags & _portal_target_cell_flags)
        == _portal_target_cell_flags)
       && ((adjacent_flags & _portal_flags_visit_test)
         != _portal_flags_set))
         cells_marked += _PortalFlagGroupOfCells(adjacent_cell);

      portal++;
   }

   return cells_marked;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is an internal helper function for the door blocking routines,
   which follow.

   flags_set is those flags we want to turn on for all qualified
   cells.

   flags_clear is not a bit mask.  It's the flags we want turned
   off--we find the mask ourselves.  If this seems backwards, you've
   twiddled one bit too many.  Get out and play in the snow!

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static int _PortalFlagCells(PortalCell *cell,
                            uchar target_cell_flags,
                            uchar flags_set,
                            uchar flags_clear)
{
   // If our cell don't cut it, we leave in a huff.
   if ((cell->flags & _portal_target_cell_flags)
    != _portal_target_cell_flags)
      return 0;

   _portal_target_cell_flags = target_cell_flags;
   _portal_flags_set = flags_set;
   _portal_flags_mask = 255 ^ flags_clear;
   _portal_flags_visit_test = flags_clear | flags_set;

   return _PortalFlagGroupOfCells(cell);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We close a door by setting all of its cells to block vision.
   If return value is zero, our starting cell was not flagged as
   a door.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int PortalBlockVision(PortalCell *starting_cell)
{
   return _PortalFlagCells(starting_cell, 
                           CELL_CAN_BLOCK_VISION,
                           CELL_BLOCKS_VISION, 0);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is like PortalSetBlocking(), above, but accepts a point.
   It's quite a bit slower since we find our starting cell through an
   exhaustive search.  We probably shouldn't use this in a game.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int PortalBlockVisionFromLocation(Location *seed_loc)
{
   int cell_index = CellFromLoc(seed_loc);

   if (cell_index == CELL_INVALID) {
      ConfigSpew("BlockSpew",
                 ("Door blocking out of world rep: (%g %g %G)\n",
                  seed_loc->vec.x, seed_loc->vec.y, seed_loc->vec.z));
      return 0;
   } else
      return PortalBlockVision(wr_cell[cell_index]);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We open a door by setting its cells to not block vision.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int PortalUnblockVision(PortalCell *starting_cell)
{
   return _PortalFlagCells(starting_cell,
                           CELL_CAN_BLOCK_VISION,
                           0, CELL_BLOCKS_VISION);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   As before, but using a location.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int PortalUnblockVisionFromLocation(Location *seed_loc)
{
   int cell_index = CellFromLoc(seed_loc);

   if (cell_index == CELL_INVALID) {
      ConfigSpew("BlockSpew", 
                 ("Door unblocking out of world rep: (%g %g %G)\n",
                  seed_loc->vec.x, seed_loc->vec.y, seed_loc->vec.z));
      return 0;
   } else
      return PortalUnblockVision(wr_cell[cell_index]);
}


#if 0
/*
 * find_split_portals
 *
 *   This code is just like the above, but it
 *   records all the planes it sees in the portals
 *   which cause splits
 */

#define MAX_SPLIT_CELLS 256
#define MAX_SPLIT_PLANES 16
static int cell_list[MAX_SPLIT_CELLS];
static mxs_plane split_planes[16];

#define EPSILON_VECDOT          0.99
#define EPSILON_PLANE_CONSTANT  0.01

static add_plane(int *num_planes, PortalPlane *pp)
{
   mxs_plane p;
   int i;

   p.x = pp->normal.x;
   p.y = pp->normal.y;
   p.z = pp->normal.z;
   p.d = pp->plane_constant;

   for (i=0; i < *num_planes; ++i) {
      if (mx_dot_vec(&p, &split_planes[i]) > EPSILON_VECDOT &&
          fabs(p.d - split_planes[i].d) < EPSILON_PLANE_CONSTANT)
        return;
   }
   split_planes[(*num_planes)++] = p;
}

int find_split_portals_in_area(mxs_plane **p, int max_planes,
   mxs_vector *mn, mxs_vector *mx, Location *seed)
{
   int count, i, j, vl, num_planes=0;
   int cell = CellFromLoc(seed);

   count = 1;
   cell_list[0] = cell;
   SET_CELL_VISITED(cell);

   // breadth first search to find all the portals we cross

   for (i=0; i < count; ++i) { // note count increases as we go
      PortalCell *p = WR_CELL(cell_list[i]);
      vl = p->portal_vertex_list;
      for (j=0; j < p->num_portal_polys; ++j) {
         cell = p->portal_poly_list[j].destination;
         if (!CELL_VISITED(cell)) {
            if (portal_crosses_bbox(p, &p->portal_poly_list[j], mn, mx, vl)) {
               cell_list[count++] = cell;
               SET_CELL_VISITED(cell);
               // have we run out of room to store cells?
               if (count == MAX_SPLIT_CELLS)
                  goto done;
               if (portal_poly_list[j].flags & PORTAL_SPLITS_OBJECT)
                  if (num_planes < max_planes)
                     add_plane(&num_planes,
                          &p->plane_list[p->portal_poly_list[j].planeid]);
            }
         } else if (portal_poly_list[j].flags & PORTAL_SPLITS_OBJECT)
            if (portal_crosses_bbox(p, &portal_poly_list[j], mn, mx, vl))
               if (num_planes < max_planes)
                  add_plane(&num_planes,
                          &p->plane_list[p->portal_poly_list[j].planeid]);
         vl += p->portal_poly_list[j].num_vertices;
      }
   }

 done:
   for (i=0; i < count; ++i)
      CLEAR_CELL_VISITED(cell_list[i]);
   
   if (num_planes) {
      *p = Malloc(sizeof(mxs_plane) * num_planes);
      memcpy(*p, split_planes, sizeof(mxs_plane) * num_planes);
   }
   return num_planes;
}

int find_split_portals(mxs_plane **sp, ObjID o, int max_planes)
{

}
#endif
