/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/render/sphrcst.cpp,v 1.37 2000/02/19 12:35:38 toml Exp $
//

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   sphrcst.c

   The spherecaster returns 0 if you can get from the start to the
   end, and the number of contacts within epsilon of the first one if
   you can't.  The tricky bit is that each contact can be with a
   plane, an edge, or a vertex, and (naturally) these require
   different representations; so the output format is something of a
   tangle.

   For each polygon, edge, or vertex we hit which is within epsilon
   time of the first one, we generate an sSphrContact.  This includes
   the world coordinates, and an index into an array of
   sSphrContactData structures, which in turn give us the cell,
   polygon, and vertex indices.  See sphrcsts.h for the structures.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

//#define PROFILE_ON
//#define FULL_PROFILE

#include <math.h>
#include <string.h>

#include <lg.h>
#include <lgassert.h>
#include <dbg.h>
#include <config.h>
#include <mprintf.h>
#include <r3d.h>
#include <matrix.h>
#include <matrixd.h>
#include <timings.h>

#include <wr.h>
#include <wrbsp.h>
#include <portal.h>
#include <bspsphr.h>

#include <sphrcst.h>
#include <sphrcsts.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// weirdness
#define SPHR_EPSILON 0.001
#define PARALLEL_EPSILON 0.0001   // um, why not

#define FLOAT_WITHIN_EPSILON(f1, f2) \
   ((fabs((f1) - (f2)) <= SPHR_EPSILON)? TRUE : FALSE)

// convert mxs_vector to mxds_vector
#define TO_MXDS_VECTOR(dv, v) \
   { (dv)->x = (v)->x; (dv)->y = (v)->y; (dv)->z = (v)->z; }

#define TO_MXS_VECTOR(v, dv) \
   { (v)->x = (dv)->x; (v)->y = (dv)->y; (v)->z = (dv)->z; }


// These are set by SphrSpherecast.  Which fields are valid depends
// partly on whether we struck a vertex, an edge, a polygon, or
// nothing.  All contacts within epsilon are reported.
sSphrContact gaSphrContact[SPHR_MAX_CONTACT];
sSphrContactData gaSphrContactData[SPHR_MAX_CONTACT_DATA];

int gSphrContactCount;
int gSphrContactDataCount;


// internal globals and constants

// time and error terms--epsilon is relative to time rather than world
// space (for space we use SPHR_EPSILON)
static double epsilon;

// a time we can ignore
#define REALLY_JUST_AMAZINGLY_BIG 100000000.0

// the end of time
#define NO_CONTACT (1.0)
static mxs_real earliest_time;


// These are just the points passed in when the spherecaster is
// called.  So the starting point is not the earliest point in the
// cast.  It's the center of the sphere where it starts.
static mxs_vector start_point, end_point;
static mxds_vector mxds_start_point;

// Our cast has a bounding box.
static mxs_vector cast_min, cast_max;

// This is overkill, but we burn only a few bytes and we have a real
// mean attitude.
#define MAX_POINTS_PER_PLANE 128
#define MAX_PLANES_PER_CELL 64

// How does each vertex relate to the cast's plane and bounding box?
#define VERTEX_TESTED           (1 << 0)
#define VERTEX_PAST_MIN_X       (1 << 1)
#define VERTEX_PAST_MAX_X       (1 << 2)
#define VERTEX_PAST_MIN_Y       (1 << 3)
#define VERTEX_PAST_MAX_Y       (1 << 4)
#define VERTEX_PAST_MIN_Z       (1 << 5)
#define VERTEX_PAST_MAX_Z       (1 << 6)

#define VERTEX_BBOX \
   VERTEX_PAST_MIN_X + VERTEX_PAST_MAX_X \
 + VERTEX_PAST_MIN_Y + VERTEX_PAST_MAX_Y \
 + VERTEX_PAST_MIN_Z + VERTEX_PAST_MAX_Z

static uchar vertex_flags[MAX_POINTS_PER_PLANE];


// Along the way we may find that some of the contacts we've generated
// are redundant.  We'll weed 'em out here.
static bool contact_valid[SPHR_MAX_CONTACT];


// ray at the center of the cast
mxs_vector cast_vector;

// The base plane is perpendicular to the cast and includes the
// starting point.  So its normal is the same as the unit vector
// parallel to the cast.
static mxs_vector cast_unit_direction;
static mxs_real base_plane_constant;
static mxds_vector mxds_cast_unit_direction;


// These are not quite what the client passes in since they include an
// epsilon.
static mxs_real radius, radius_squared;


// These don't need no steeenking epsilon.
static mxs_real cast_length;


// This is a copy of the flags passed in to the spherecaster.  We can
// hit any portal with at least one flag bit in common with this.
static uchar portal_flags;


int gaSphrCellsReached[SPHR_MAX_CELLS_REACHED];
int gSphrCellCount;

static bool ParallelVectors(mxs_vector *edge_1, mxs_vector *edge_2);

DECLARE_TIMER(SPHR_Total, Average);
#ifdef FULL_PROFILE
DECLARE_TIMER(SPHR_InitialCells, Average);
DECLARE_TIMER(SPHR_Polygons, Average);
DECLARE_TIMER(SPHR_Edges, Average);
DECLARE_TIMER(SPHR_Vertices, Average);
#endif

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Does this polygon intersect this sphere?

   This isn't part of the spherecaster proper.

   There are four tests.

   The first is trivial rejection based on distance from the center to
   the plane.  This includes backface culling, since we assume that we
   will be testing all cells intersecting the sphere, so that only one
   out of each pair of portals is relevent.

   The second is acceptance when the projection of the center of the
   sphere onto the plane is within the polygon.

   The third is acceptance when the sphere encompasses at least one
   vertex of the polygon.

   Finally, we have to check whether any edge intersects the sphere,
   despite both its vertices being outside.  This is a computationally
   uncouth process requiring a square root for every edge.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool PolygonIntersectsSphere(PortalCell *cell,
                                    PortalPolygonCore *polygon,
                                    uint vertex_offset,
                                    mxs_vector *center,
                                    mxs_real radius)
{
   mxs_vector center_projection;
   mxs_real plane_distance;
   mxs_real radius_squared;
   mxs_vector edge, start_to_center, *start_vertex, *end_vertex;
   mxs_vector unit_edge_direction;
   mxs_real start_to_radius_intersection_dist;
   mxs_real approach_squared;
   PortalPlane *plane;
   int i;
   uchar *vertex_list = cell->vertex_list + vertex_offset;
   int num_vertices = polygon->num_vertices;

   // trivial rejects: backface and distance tests
   plane = cell->plane_list + polygon->planeid;
   plane_distance
      = mx_dot_vec(&plane->normal, center) + plane->plane_constant;

   if (plane_distance < 0.0 || plane_distance > radius)
      return FALSE;


   // sort-of-trivial accept: If the projection of center of sphere
   // onto plane of polygon is inside the poly (the work is done in
   // wrcast) then we're done.
   mx_scale_add_vec(&center_projection, center, 
                    &plane->normal, -plane_distance);

   if (PortalPointInPolygon(&center_projection, cell, polygon,
                            vertex_offset, TRUE))
       return TRUE;


   // accept if any vertex is within radius
   radius_squared = radius * radius;

   for (i = 0; i < num_vertices; ++i) {
      mxs_vector *vertex = cell->vpool + vertex_list[i];

      if (mx_dist2_vec(center, vertex) <= radius_squared)
         return TRUE;
   }


   // full-blown edge testing: By now, the only case left is that the
   // sphere touches the polygon on an edge, so if we fail this we're
   // not intersecting.
   start_vertex = cell->vpool + vertex_list[num_vertices - 1];

   for (i = 0; i < num_vertices; ++i) {
      end_vertex = cell->vpool + vertex_list[i];

      mx_sub_vec(&start_to_center, center, start_vertex);
      mx_sub_vec(&edge, end_vertex, start_vertex);
      mx_norm_vec(&unit_edge_direction, &edge);

      // This is the length of the projection of start_to_center onto
      // the line of our poly edge.
      start_to_radius_intersection_dist
         = mx_dot_vec(&unit_edge_direction, &start_to_center);

      // An angle > pi/2 or < -pi/2 means the edge is outside since
      // both endpoints are already known to be out.  This comment
      // is inexplicably vague.
      if (start_to_radius_intersection_dist < 0.0)
         goto next_edge;

      // If the point on the edge's line closest to the sphere center
      // is not within the edge, this edge doesn't count for much.
      if ((start_to_radius_intersection_dist 
         * start_to_radius_intersection_dist) > mx_mag2_vec(&edge))
         goto next_edge;

      // This is (the square of) the closest distance the line of the
      // edge comes to the center of our sphere.
      approach_squared
         = mx_dist2_vec(center, start_vertex)
         - (start_to_radius_intersection_dist
          * start_to_radius_intersection_dist);

      if (approach_squared < radius_squared)
         return TRUE;

   next_edge:
      start_vertex = end_vertex;
   }

   return FALSE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We only add a cell to a list of cells to visit if we haven't
   visited it yet and it's not much later than our earliest contact.

   What sophistication.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#ifdef DBG_ON
static BOOL warned_cell_reach_overflow;
#endif // DBG_ON

static void AddCellToList(int cell_index, float time,
                          int *cell_output, int *cell_output_count)
{
   PortalCell *cell;

   if (*cell_output_count == SPHR_MAX_CELLS_REACHED) {
#ifdef DBG_ON
      if (!warned_cell_reach_overflow)
      {
         Warning(("reached more than %d cells in spherecaster!\n",
                  *cell_output_count));
         warned_cell_reach_overflow = TRUE;
      }
#endif // DBG_ON
      return;
   }

   cell = WR_CELL(cell_index);

   if (cell->flags & CELL_TRAVERSED)
      return;

   if (time > (earliest_time + epsilon))
      return;

   cell->flags |= CELL_TRAVERSED;
   cell_output[*cell_output_count] = cell_index;
   *cell_output_count = *cell_output_count + 1;

#ifdef DBG_ON
   if (config_is_defined("SphereSpew"))
   {
      int i;

      mprintf("< ");
      for (i = 0; i < *cell_output_count; ++i)
         mprintf("%d ", cell_output[i]);
      mprintf(">");
   }
#endif // DBG_ON

   return;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We set the bounding box of the ray just once, then compare the
   vertices to it when we visit each cell.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void SetRayBoundingBox(void)
{
   if (start_point.x > end_point.x) {
      cast_max.x = start_point.x + radius;
      cast_min.x = end_point.x - radius;
   } else {
      cast_max.x = end_point.x + radius;
      cast_min.x = start_point.x - radius;
   }

   if (start_point.y > end_point.y) {
      cast_max.y = start_point.y + radius;
      cast_min.y = end_point.y - radius;
   } else {
      cast_max.y = end_point.y + radius;
      cast_min.y = start_point.y - radius;
   }

   if (start_point.z > end_point.z) {
      cast_max.z = start_point.z + radius;
      cast_min.z = end_point.z - radius;
   } else {
      cast_max.z = end_point.z + radius;
      cast_min.z = start_point.z - radius;
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We call this once per cell.  It sets the bounding box flags of each
   vertex in the cell, and on the side clears any other vertex flags.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void SetVertexBoundingBoxes(PortalCell *cell)
{
   int i;
   mxs_vector *vertex = cell->vpool;
   int num_vertices = cell->num_vertices;

   memset(&vertex_flags[0], 0, num_vertices);

   // record how the marked vertices relate to the bounding box of the cast
   for (i = 0; i < num_vertices; ++i) {

      if (vertex->x < cast_min.x)
         vertex_flags[i] |= VERTEX_PAST_MIN_X;
      else if (vertex->x > cast_max.x)
         vertex_flags[i] |= VERTEX_PAST_MAX_X;

      if (vertex->y < cast_min.y)
         vertex_flags[i] |= VERTEX_PAST_MIN_Y;
      else if (vertex->y > cast_max.y)
         vertex_flags[i] |= VERTEX_PAST_MAX_Y;

      if (vertex->z < cast_min.z)
         vertex_flags[i] |= VERTEX_PAST_MIN_Z;
      else if (vertex->z > cast_max.z)
         vertex_flags[i] |= VERTEX_PAST_MAX_Z;

      ++vertex;
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Each sSphrContact has a linked list of sSphrContactData structures.
   So this would be your basic linked-list insertion.

   In fact, it is.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void AddDataToContact(ushort contact_index,
                             ushort cell_index, ushort polygon_index,
                             ushort vertex_offset, uchar vertex_in_poly)
{
   if (gSphrContactDataCount == SPHR_MAX_CONTACT_DATA) {
      Warning(("ran out of contact data structures for spherecaster!"));
      return;
   }

   gaSphrContactData[gSphrContactDataCount].cell_index = cell_index;
   gaSphrContactData[gSphrContactDataCount].polygon_index = polygon_index;
   gaSphrContactData[gSphrContactDataCount].vertex_offset = vertex_offset;
   gaSphrContactData[gSphrContactDataCount].vertex_in_poly = vertex_in_poly;
   gaSphrContactData[gSphrContactDataCount].next
      = gaSphrContact[contact_index].first_contact_data;

   gaSphrContact[contact_index].first_contact_data = gSphrContactDataCount;

   gSphrContactDataCount++;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is called any time we hit something.  The culling and
   coordination to figure out which pairs of edges are really the same
   edge, etc. happen later.

   At first, even for edges and vertices, we only know about one
   sSphrContactData (the polygon, plane, and all that).  Those are
   stored in linked lists, which we can extend in AddDataToContact.

   Some so-called contacts are baloney because we know of an earlier
   contact.  If we actually register the contact we return TRUE.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool RegisterContact(sSphrContactKind kind, 
                            mxs_vector *point_in_world, 
                            mxs_vector *point_on_ray, 
                            int cell_index, ushort polygon_index,
                            ushort vertex_offset, uchar vertex_in_poly,
                            mxs_real time)
{
   if (gSphrContactCount == SPHR_MAX_CONTACT) {
      Warning(("ran out of contact structures for spherecaster!"));
      return FALSE;
   }

   if (time > (earliest_time + epsilon) || time < 0.0)
      return FALSE;

   if (time < earliest_time)
      earliest_time = time;

   gaSphrContact[gSphrContactCount].kind = kind;
   gaSphrContact[gSphrContactCount].first_contact_data = NO_NEXT_CONTACT_DATA;
   gaSphrContact[gSphrContactCount].point_in_world = *point_in_world;
   gaSphrContact[gSphrContactCount].point_on_ray = *point_on_ray;
   gaSphrContact[gSphrContactCount].time = time;

   AddDataToContact(gSphrContactCount,
                    cell_index, polygon_index,
                    vertex_offset, vertex_in_poly);

   gSphrContactCount++;

   return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This combines the data for two contacts, then destroys the second
   contact.  We use it when we discover that two hits on edges are
   really the same edge in different cells.  Note that this does not
   add any new data.  It just hooks in the data for contact_index_2
   and marks the old contact structure as invalid.

   We'll just tie the second list onto the end of the first.  We know
   that neither list is empty.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void CombineContacts(int contact_to_keep, int contact_to_fold_in)
{
   int data_index, next_data_index;

   data_index = gaSphrContact[contact_to_keep].first_contact_data;
   next_data_index = gaSphrContactData[data_index].next;

   while (next_data_index != NO_NEXT_CONTACT_DATA) {
      data_index = next_data_index;
      next_data_index = gaSphrContactData[data_index].next;
   }

   gaSphrContactData[data_index].next
      = gaSphrContact[contact_to_fold_in].first_contact_data;

   contact_valid[contact_to_fold_in] = FALSE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The spherecaster uses this to flag the cells in its initial sphere.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void MarkCells(int *cell_output, int cell_output_count)
{
   int i;

   for (i = 0; i < cell_output_count; ++i)
      WR_CELL(cell_output[i])->flags |= CELL_TRAVERSED;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The spherecaster flags cells it has visited.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void UnmarkCells(int *cell_output, int cell_output_count)
{
   int i;

   for (i = 0; i < cell_output_count; ++i)
      WR_CELL(cell_output[i])->flags &= ~CELL_TRAVERSED;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Our world is large and blocky, with great big flat walls and plenty
   of corners.  The polygon tests should account for most of our hits.
   If we hit this plane on a polygon at all it will be at a particular
   point, where the radius is perpendicular to the plane, so we find
   that point and test all the polygons in the plane against it.

   We treat portals with the given flags set as if they were solid.

   For each polygon of this cell, if it's in this plane then we check
   it against the point of contact.  Contact with a solid polygon or
   flagged portal means we're done with this plane.  Contact with a
   portal we're not set to collide with, however, is inconclusive,
   since we may still have hit the edge of an adjacent solid polygon.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool TestPolygonsInPlane(int cell_index, int current_plane_index,
                                int *cell_output, int *cell_count)
{
   #ifdef FULL_PROFILE
   AUTO_TIMER(SPHR_Polygons);
   #endif

   mxs_vector contact_point, contact_point_on_ray;
   mxs_real contact_time;
   mxs_real norm_dot_cast_vector;
   int polygon_index;
   PortalCell *cell = WR_CELL(cell_index);
   PortalPlane *current_plane = cell->plane_list + current_plane_index;
   mxs_vector *current_plane_normal = &current_plane->normal;
   bool set_plane_function = TRUE;
   PortalPolygonCore *polygon = cell->poly_list;
   int num_polys = cell->num_polys;
   int num_solid_polys = num_polys - cell->num_portal_polys;
   uint vertex_offset = 0;

#ifdef DBG_ON
   if (config_is_defined("SphereSpew"))
      mprintf("%dp", cell_index);
#endif // DBG_ON

   norm_dot_cast_vector
      = mx_dot_vec(&cast_unit_direction, current_plane_normal);

   // If the cast is parallel to the target plane (that is,
   // perpendicular to the target plane's normal) then we will have to
   // rely on our edge and vertex tests to catch any intersections.
   if (norm_dot_cast_vector < PARALLEL_EPSILON
    && norm_dot_cast_vector > -PARALLEL_EPSILON)
      return FALSE;

   norm_dot_cast_vector *= cast_length;

   contact_time = (mx_dot_vec(current_plane_normal, &start_point)
                 + current_plane->plane_constant - radius)
                / -norm_dot_cast_vector;

   // Here's the point on the ray at the time of contact...
   mx_scale_add_vec(&contact_point_on_ray, &start_point, 
                    &cast_vector, contact_time);

   // ...and here's our contact point on the plane.
   mx_scale_add_vec(&contact_point, &contact_point_on_ray, 
                    current_plane_normal, -radius);

   // We iterate over all polygons, and test against those in this
   // plane.  We set the point-in-poly function only for the first
   // test.
   for (polygon_index = 0; polygon_index < num_polys; ++polygon_index) {
      if (polygon->planeid == current_plane_index) {
         if (PortalPointInPolygon(&contact_point, cell, polygon,
                                  vertex_offset, set_plane_function)) {

            // If we hit a portal then we add its destination to
            // the list of cells to visit--unless the portals'
            // flags field has bits in common with the flags we're
            // colliding with, in which case we register a hit,
            // same as with a regular polygon.
            if (polygon_index < num_solid_polys
             || ((polygon->flags & portal_flags) != 0)) {
               if (RegisterContact(kSphrPolygon, 
                                   &contact_point, &contact_point_on_ray,
                                   cell_index, polygon_index,
                                   vertex_offset, 0, contact_time))
                  return TRUE;
            } else
               AddCellToList(polygon->destination, contact_time,
                             cell_output, cell_count);
         }
         set_plane_function = FALSE;
      }
      vertex_offset += polygon->num_vertices;
      ++polygon;
   }

   return FALSE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The closest approach is the next cut for each edge after the
   bounding box test.  Note that the value returned is signed.

   We export our last two arguments, though edge_base_to_ray_base
   will not be valid if the cast and edge are parallel since we
   short-circuit.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static double ClosestApproach(mxds_vector *v1, mxds_vector *v2,
                              mxds_vector *edge_unit_direction, 
                              mxds_vector *approach_unit_direction,
                              mxds_vector *edge_base_to_ray_base)
{
   // The approach is perpendicular to both the cast and the edge, and
   // is in the direction from the edge to the cast.
   double approach_direction_length;
   mxds_vector approach_direction;

   mxd_cross_vec(&approach_direction, 
                 &mxds_cast_unit_direction, edge_unit_direction);
   approach_direction_length
      = mxd_norm_vec(approach_unit_direction, &approach_direction);

   // If the cast is parallel to the edge then least it's not an
   // intersection, since we'll check for vertices within the cigar
   // separately, and we assume that the initial sphere of our cast
   // does not intersect the world.  We cheat and lie, returning a
   // value exptected to be too large.  Yecch.
   if (approach_direction_length < SPHR_EPSILON)
      return REALLY_JUST_AMAZINGLY_BIG;

   mxd_sub_vec(edge_base_to_ray_base, &mxds_start_point, v1);

   return mxd_dot_vec(edge_base_to_ray_base, approach_unit_direction);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We iterate over all polygons and test every edge, filtering each
   edge though a bounding box test, a closest approache test, and
   finally finding its time of intersection with our cast.  Each edge
   is shared by two polys, of course, but in some cases the other poly
   will be in another cell.

   We regret to inform you that we even have to test against the edges
   of portals, because some of these will not bound any solid polygons
   and may be the only places where we'll know we hit those portals.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool TestEdgesInPlane(int cell_index, int current_plane_index,
                             int *cell_output, int *cell_count)
{
   #ifdef FULL_PROFILE
   AUTO_TIMER(SPHR_Edges);
   #endif

   int polygon_index;
   int num_sides;
   PortalCell *cell = WR_CELL(cell_index);
   mxs_vector *vpool = cell->vpool;
   uchar *vertex_list = cell->vertex_list;
   PortalPolygonCore *polygon = cell->poly_list;
   int num_polys = cell->num_polys;
   int num_portals = cell->num_portal_polys;
   int num_solid_polys = num_polys - num_portals;
   uint vertex_offset = 0;

#ifdef DBG_ON
   if (config_is_defined("SphereSpew"))
      mprintf("e");
#endif // DBG_ON

   for (polygon_index = 0; polygon_index < num_polys; ++polygon_index) {
      num_sides = polygon->num_vertices;

      if (polygon->planeid == current_plane_index) {
         mxds_vector start_vertex, end_vertex;
         mxds_vector edge_direction, edge_unit_direction;
         mxds_vector approach_unit_direction;
         mxds_vector edge_base_to_ray_base;
         double edge_length, approach_length;
         mxds_vector proj_radius;
         double proj_radius_length;
         mxds_vector point_in_world, point_on_ray;
         double proj_dist_ray_to_contact;
         double time_along_edge;
         double contact_time;
         int end, start;

         start = num_sides - 1;

         for (end = 0; end < num_sides; ++end) {
            uchar start_offset = vertex_list[vertex_offset + start];
            uchar end_offset = vertex_list[vertex_offset + end];

            // Can we eliminate this edge through a bounding box test?
            if ((vertex_flags[start_offset]
               & vertex_flags[end_offset]
               & VERTEX_BBOX) != 0)
               goto next_edge;

            TO_MXDS_VECTOR(&start_vertex, vpool + start_offset);
            TO_MXDS_VECTOR(&end_vertex, vpool + end_offset);

            mxd_sub_vec(&edge_direction, &end_vertex, &start_vertex);
            edge_length = mxd_norm_vec(&edge_unit_direction, &edge_direction);

            // The last two arguments here are set by ClosestApproach.
            approach_length = ClosestApproach(&start_vertex, &end_vertex,
                                              &edge_unit_direction,
                                              &approach_unit_direction,
                                              &edge_base_to_ray_base);

            if (fabs(approach_length) <= radius) {
               // Anything starting with proj_ here is in the plane
               // which contains the edge and is perpendicular to the
               // cast.
               double proj_dist_ray_along_edge;
               mxds_vector proj_unit_perp_to_cast;
               mxds_vector proj_unit_perp_to_edge;

               // The point at which the projection of the cast
               // intersects our edge isn't either of our contact
               // points, but hey, we're getting there.
               mxd_cross_vec(&proj_unit_perp_to_cast, 
                             &approach_unit_direction,
                             &mxds_cast_unit_direction);

               mxd_cross_vec(&proj_unit_perp_to_edge, 
                             &approach_unit_direction, &edge_unit_direction);

               // This is the length of the component of
               // edge_base_to_ray_base which is perpendicular to the cast.
               proj_dist_ray_along_edge
                  = mxd_dot_vec(&edge_base_to_ray_base, 
                                &proj_unit_perp_to_cast);

               proj_radius_length
                  = sqrt(radius_squared - (approach_length * approach_length));

               mxd_scale_vec(&proj_radius, 
                             &proj_unit_perp_to_edge, proj_radius_length);

               proj_dist_ray_to_contact
                  = mxd_dot_vec(&proj_radius, &proj_unit_perp_to_cast);

               time_along_edge
                  = (proj_dist_ray_along_edge
                   - proj_dist_ray_to_contact)
                  / mxd_dot_vec(&edge_direction, &proj_unit_perp_to_cast);

               // If the point where we'd touch this edge is not
               // actually on the edge, we can save ourselves some
               // time...
               if ((time_along_edge < 0.0) || time_along_edge > 1.0)
                  goto next_edge;

               mxd_scale_add_vec(&point_in_world, &start_vertex, 
                                 &edge_direction, time_along_edge);

               // All right, then, where's the point on the ray of the
               // cast (that is to say, where the heck's our sphere)?
               // As it happens, if we measure from the point on the
               // edge, we've got both components of it: the closest
               // approach, and the projection of the radius onto our
               // plane.
               mxd_scale_add_vec(&point_on_ray, &point_in_world, 
                                 &approach_unit_direction, approach_length);

               mxd_addeq_vec(&point_on_ray, &proj_radius);

               contact_time = (mxd_dot_vec(&point_on_ray,
                                           &mxds_cast_unit_direction)
                             + base_plane_constant) / cast_length;

#if 0
               if (contact_time >= 0 && contact_time <= 1.0) {
                  mxds_vector start_to_point_on_ray;

                  mx_sub_vec(&start_to_point_on_ray, 
                             &point_on_ray, &start_point);

                  if (!ParallelVectors(&start_to_point_on_ray, 
                                       &cast_vector))
                     mprintf("Spherecaster: point_on_ray not"
                             " colinear in edge case.\n");
               }
#endif // 0

               if (polygon_index < num_solid_polys
                || ((polygon->flags & portal_flags) != 0)) {
                  mxs_vector mxs_in_world, mxs_on_ray;

                  TO_MXS_VECTOR(&mxs_in_world, &point_in_world);
                  TO_MXS_VECTOR(&mxs_on_ray, &point_on_ray);

                  if (RegisterContact(kSphrEdge,
                                      &mxs_in_world, &mxs_on_ray,
                                      cell_index, polygon_index,
                                      vertex_offset, start,
                                      contact_time))
                     return TRUE;
               } else
                  AddCellToList(polygon->destination, contact_time,
                                cell_output, cell_count);
            }
         next_edge:
            start = end;
         }
      }

      vertex_offset += num_sides;
      ++polygon;
   }

   return FALSE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This helper for TestVerticesInPlane checks whether a point
   is within the sphere's radius of the cast.

   If we return TRUE we also export a bit of our math though the
   second and third arguments.  Otherwise we leave those alone.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool VertexCloseToRay(mxs_vector *point, 
                             mxs_real *base_dist, mxs_real *ray_dist_squared)
{
   mxs_real dist_to_base_plane;
   mxs_real dist_to_ray_squared;
   mxs_real dist_to_start_point_squared;

   dist_to_base_plane
      = mx_dot_vec(point, &cast_unit_direction) + base_plane_constant;

   // Are we before the beginning or after the end?  This check counts
   // the hemisphere at the end of the cast as if it were cylindrical.
   if (dist_to_base_plane < -epsilon
    || dist_to_base_plane > (cast_length + radius))
      return FALSE;

   // The rejections using the cylinder take a little more work.
   dist_to_start_point_squared = mx_dist2_vec(point, &start_point);

   dist_to_ray_squared = dist_to_start_point_squared
                       - (dist_to_base_plane * dist_to_base_plane);

   // Are we more than one sphere radius from the infinitely long
   // cylinder?
   if (dist_to_ray_squared > radius_squared)
      return FALSE;

   // If we're past the end plane then we're only in if we're inside
   // the ending cap of the cigar.
   if (dist_to_base_plane > cast_length)
      if (mx_dist2_vec(point, &end_point) > radius_squared)
         return FALSE;

   // Apparently we were in the cylindrical part, not the caps, and
   // close enough.
   *base_dist = dist_to_base_plane;
   *ray_dist_squared = dist_to_ray_squared;
   return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   When we check our cast against vertices, we have to iterate in
   terms of the polygons even though there's a separate vertex list
   because the physics will want to know what poly we hit.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void TestVerticesInPlane(int cell_index, int current_plane_index,
                                int *cell_output, int *cell_count)
{
   #ifdef FULL_PROFILE
   AUTO_TIMER(SPHR_Vertices);
   #endif

   int polygon_index, vertex_index;
   int num_sides;
   mxs_vector contact_on_ray;
   PortalCell *cell = WR_CELL(cell_index);
   mxs_vector *vpool = cell->vpool;
   uchar *vertex_list = cell->vertex_list;
   uint palette_index;
   int num_polys = cell->num_polys;
   int num_portals = cell->num_portal_polys;
   int num_solid_polys = num_polys - num_portals;
   PortalPolygonCore *polygon = cell->poly_list;
   uint vertex_offset = 0;

#ifdef DBG_ON
   if (config_is_defined("SphereSpew"))
      mprintf("v");
#endif // DBG_ON

   for (polygon_index = 0; polygon_index < num_polys; ++polygon_index) {
      num_sides = polygon->num_vertices;

      if (polygon->planeid == current_plane_index) {

         for (vertex_index = 0; vertex_index < num_sides; ++vertex_index) {
            palette_index = vertex_list[vertex_offset + vertex_index];

            // If it's outside the cast, or we've already tested it,
            // blow it off.
            if ((vertex_flags[palette_index]
              & (VERTEX_BBOX | VERTEX_TESTED)) == 0) {
               mxs_vector *vertex = vpool + palette_index;
               mxs_real base_dist, ray_dist_squared;

               vertex_flags[palette_index] |= VERTEX_TESTED;

               if (VertexCloseToRay(vertex, &base_dist, &ray_dist_squared)) {
                  // The distance from the point to the sphere is the
                  // distance to the base plane minus the distance
                  // from the base plane to the sphere (at the radius
                  // of the point).  Eh?
                  mxs_real sphere_overlap
                     = sqrt(radius_squared - ray_dist_squared);
                  mxs_real contact_distance = base_dist - sphere_overlap;
                  mxs_real contact_time = contact_distance / cast_length;
                  mx_scale_add_vec(&contact_on_ray, 
                                   &start_point,
                                   &cast_vector,
                                   contact_time);

                  if (polygon_index < num_solid_polys
                   || ((polygon->flags & portal_flags) != 0)) {
                     if (RegisterContact(kSphrVertex, 
                                         vertex,
                                         &contact_on_ray,
                                         cell_index, polygon_index, 
                                         vertex_offset, vertex_index,
                                         contact_time))
                        return;
                  } else
                     AddCellToList(polygon->destination, contact_time,
                                   cell_output, cell_count);
               }
            }
         }
      }
      vertex_offset += num_sides;
      ++polygon;
   }                    
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Here we recover a polygon edge from a contact structure.

   The only trick in extracting an edge--that is, two adjacent
   vertices--from a polygon is that you might wrap around the vertex
   list.

   That, and the usual unusual indirection of the world rep.

   contact_data_link is the entry of the linked list of
   sSphrContactData associated with this contact.  We expect it to be
   0 in practice, meaning the first link.

   Our output is the setting of the start and end arguments.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void GetEdgeVerticesFromContact(int contact_index,
                                       int contact_data_link,
                                       mxs_vector **start, mxs_vector **end)
{
   sSphrContact *contact;
   sSphrContactData *contact_data;
   PortalCell *cell;
   PortalPolygonCore *polygon;
   uint vertex_offset;
   int vertex_in_poly;
   uchar *vertex_list;
   int i;

   contact = &gaSphrContact[contact_index];   
   contact_data = &gaSphrContactData[contact->first_contact_data];

   for (i = 0; i < contact_data_link; ++i)
      contact_data = &gaSphrContactData[contact_data->next];

   cell = WR_CELL(contact_data->cell_index);
   polygon = cell->poly_list + contact_data->polygon_index;
   vertex_offset = contact_data->vertex_offset;
   vertex_in_poly = contact_data->vertex_in_poly;
   vertex_list = cell->vertex_list + vertex_offset;

   *start = cell->vpool + vertex_list[vertex_in_poly];
   if ((vertex_in_poly + 1) == polygon->num_vertices)
      *end = cell->vpool + vertex_list[0];
   else
      *end = cell->vpool + vertex_list[vertex_in_poly + 1];
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We call the lines which contain edges parallel if their cross
   product is close to the zero vector.  By the time we get to this
   routine, we've turned each edge into a vector by subtracting one
   end from the other.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool ParallelVectors(mxs_vector *edge_1, mxs_vector *edge_2)
{
   mxs_vector cross;
   mxs_vector norm_1, norm_2;

   mx_norm_vec(&norm_1, edge_1);
   mx_norm_vec(&norm_2, edge_2);
   mx_cross_vec(&cross, &norm_1, &norm_2);
   if (!FLOAT_WITHIN_EPSILON(0.0, cross.x)
    || !FLOAT_WITHIN_EPSILON(0.0, cross.y)
    || !FLOAT_WITHIN_EPSILON(0.0, cross.z))
      return FALSE;

   return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We call the lines which contain two edges colinear if they are more
   or less parallel and one contains a point from the other.

   Note that this it doesn't concern itself with whether the edges
   overlap, only whether they are somewhere on the same line.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool ColinearEdges(mxs_vector *start_1, mxs_vector *end_1,
                          mxs_vector *start_2, mxs_vector *end_2)
{
   mxs_vector edge_1, edge_2, start_to_start;

   mx_sub_vec(&edge_1, end_1, start_1);
   mx_sub_vec(&edge_2, end_2, start_2);

   if (!ParallelVectors(&edge_1, &edge_2))
      return FALSE;

   mx_sub_vec(&start_to_start, start_2, start_1);
   return ParallelVectors(&start_to_start, &edge_2);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Each edge exists in two planes, and physics will want both.

   Here we identify an edge which we've found both instances of in the
   process of casting our sphere.  We'll delete one reference and give
   its plane and polygon data to the other.

   The return value indicates whether we found a match.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool FindMatchingEdgeContact(int contact_index)
{
   int i;
   mxs_vector *start, *end;
   mxs_vector *compare_start, *compare_end;
   sSphrContact *contact = &gaSphrContact[contact_index];
   mxs_vector *contact_point = &contact->point_in_world;
   sSphrContactData *contact_data
      = &gaSphrContactData[contact->first_contact_data];
   int cell_index = contact_data->cell_index;

   GetEdgeVerticesFromContact(contact_index, 0, &start, &end);

   for (i = contact_index + 1; i < gSphrContactCount; ++i) {
      sSphrContact *compare_contact;
      mxs_vector *compare_contact_point;

      if (!contact_valid[i])
         continue;

      compare_contact = &gaSphrContact[i];
      if (compare_contact->kind != kSphrEdge)
         continue;

      // We know the matching edge won't be in the same cell.
      if (gaSphrContactData[compare_contact->first_contact_data].cell_index
       == cell_index)
         continue;

      compare_contact_point = &compare_contact->point_in_world;

      // If two edge contacts have roughly the same contact point, and
      // their edges are colinear, they must really be the same edge
      // (in some mind-bending, cosmic sense).
      if (FLOAT_WITHIN_EPSILON(contact_point->x, compare_contact_point->x)
       && FLOAT_WITHIN_EPSILON(contact_point->y, compare_contact_point->y)
       && FLOAT_WITHIN_EPSILON(contact_point->z, compare_contact_point->z)) {

         GetEdgeVerticesFromContact(i, 0, &compare_start, &compare_end);

         if (ColinearEdges(start, end, compare_start, compare_end)) {
            CombineContacts(contact_index, i);
            return TRUE;
          }
      }
   }

   return FALSE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   It's curious that we use the same epsilon for unrelated operations.
   But we're too lazy to work out how they interact and all that.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool PointNearPlane(mxs_vector *point, mxs_vector *norm,
                           mxs_real constant)
{
   mxs_real dist = fabs(mx_dot_vec(point, norm) + constant);

   if (dist < -SPHR_EPSILON || dist > SPHR_EPSILON)
      return FALSE;
   else
      return TRUE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This is used for getting our complete cell lists for both edge and
   vertex contacts.  It finds all the cells which contain a given
   point, with the assumption that the point is one we can collide
   with--that is, it's in a splitting plane.  So the point should be
   in at least two cells.

   For each cell we find, we add an sSphrContactData to the linked list
   for the given sSphrContact.

   There are two weird things about the sSphrContactData entries we're
   adding.  First, their only valid fields are cell_index and next,
   the linked list index.  And while each of them contains our point,
   some contain it only in a portal; we'll filter those out later and
   discard them.

   A cell already on the list is not added.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void SearchBSPForCellsWithPoint(mxs_vector *point, uint node_index, 
                                       int contact_index)
{
   mxs_real dist;

   wrBspNode *pCurNode = &g_wrBspTree[node_index];

   if (wrBspIsLeaf(pCurNode)) {
      int cell_id = pCurNode->cell_id;

      if (cell_id != -1) {
         int contact_data_index;
         sSphrContact *contact = &gaSphrContact[contact_index];

         for (contact_data_index = contact->first_contact_data;
              contact_data_index != NO_NEXT_CONTACT_DATA;
              contact_data_index = gaSphrContactData[contact_data_index].next)
            if (gaSphrContactData[contact_data_index].cell_index == cell_id)
               return;

         AddDataToContact(contact_index, cell_id, 0, 0, 0);
#ifdef DBG_ON
         if (config_is_defined("SphereSpew"))
            mprintf("&%d", cell_id);
#endif // DBG_ON
      }

      return;
   }

   if (wrBspIsReversed(pCurNode))
      dist = -mx_dot_vec(point, &pCurNode->plane->normal) - pCurNode->plane->plane_constant;
   else
      dist = mx_dot_vec(point, &pCurNode->plane->normal) + pCurNode->plane->plane_constant;


   // If we're roughly coplanar we end up visiting both sides.
   if ((dist > -SPHR_EPSILON) && (wrInsideIndex(pCurNode) != WRBSP_INVALID))
      SearchBSPForCellsWithPoint(point, wrInsideIndex(pCurNode), contact_index);

   if ((dist < SPHR_EPSILON) && (wrOutsideIndex(pCurNode) != WRBSP_INVALID))
      SearchBSPForCellsWithPoint(point, wrOutsideIndex(pCurNode), contact_index);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   So we've got a contact someplace, and a cell which it's in.  So we
   look for the contact in the cell by checking it against every
   polygon.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static int FindPolygonWithEdgeContact(sSphrContactData *contact_data,
                                       mxs_vector *point)
{
   int plane_index, polygon_index;
   BOOL plane_near_point[MAX_PLANES_PER_CELL];
   int cell_index = contact_data->cell_index;
   PortalCell *cell = WR_CELL(cell_index);
   PortalPlane *plane = cell->plane_list;
   int num_planes = cell->num_planes;
   PortalPolygonCore *polygon = cell->poly_list;
   int num_solid_polys = cell->num_polys - cell->num_portal_polys;
   uint vertex_offset = 0;

#ifdef DBG_ON
   if (config_is_defined("SphereSpew"))
      mprintf("@");
#endif // DBG_ON

   memset(&plane_near_point[0], 0, num_planes);

   // First, we need to find the plane this point is closest to.
   for (plane_index = 0; plane_index < num_planes; ++plane_index) {
      if (PointNearPlane (point, &plane->normal, plane->plane_constant))
         plane_near_point[plane_index] = TRUE;

      ++plane;
   }

   for (polygon_index = 0; polygon_index < num_solid_polys; ++polygon_index) {
      if (plane_near_point[polygon->planeid]
       && PortalPointInPolygon(point, cell, polygon, vertex_offset, TRUE))
         return polygon_index;

      vertex_offset += polygon->num_vertices;
      ++polygon;
   }

   return -1;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   An edge we can hit is in just two cells.  But we need both of them
   so physics can take the average of their normals; and we need cells
   which have solid polygons which contain our edge so physics can
   check their texture ids to determine physical properties.

   By the time we've gotten to this routine, we know we have only one
   of the cells we need.  So we go traipsing through the BSP tree in
   search of its match.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void FindMatchingEdge(int contact_index)
{
   int contact_data_index, polygon_index;
   sSphrContactData *contact_data;

   sSphrContact *contact = &gaSphrContact[contact_index];
   mxs_vector *contact_point = &contact->point_in_world;
   int original_contact_data_index = contact->first_contact_data;

   // find all remaining cells containing our contact point
   SearchBSPForCellsWithPoint(contact_point, WRBSP_HEAD, contact_index);

   // We've got any number of cells now, including our original; the
   // one we want; and miscellaneous rubbish.  Let's find our puppy.
   for (contact_data_index = contact->first_contact_data;
        contact_data_index != original_contact_data_index;
        contact_data_index = contact_data->next) {

      contact_data = &gaSphrContactData[contact_data_index];
      polygon_index = FindPolygonWithEdgeContact(contact_data, contact_point);

      if (polygon_index != -1) {
         // This is the one we want!  We'll make it the head of the
         // linked list for this contact data, discard the rest of the
         // sSphrContactData links, and head for the hillocks.
         contact->first_contact_data = contact_data_index;
         contact_data->polygon_index = polygon_index;
         contact_data->next = original_contact_data_index;
         return;
      }
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Here, we find all contacts which are really with the same point,
   just in different cells.  Corresponding vertex contacts have the
   advantage of being exactly the same, rather than just within
   epsilon.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static bool FindMatchingVertexContacts(int contact_index)
{
   int i;
   sSphrContact *contact = &gaSphrContact[contact_index];
   mxs_vector *contact_point = &contact->point_in_world;
   sSphrContactData *contact_data
      = &gaSphrContactData[contact->first_contact_data];
   int cell_index = contact_data->cell_index;

   for (i = 0; i < gSphrContactCount; ++i) {
      sSphrContact *compare_contact;
      mxs_vector *compare_contact_point;

      if (i == contact_index)
         continue;

      if (!contact_valid[i])
         continue;

      compare_contact = &gaSphrContact[i];
      if (compare_contact->kind != kSphrVertex)
         continue;

      // We know the matching vertices won't be in the same cell.
      if (gaSphrContactData[compare_contact->first_contact_data].cell_index
       == cell_index)
         continue;

      compare_contact_point = &compare_contact->point_in_world;

      if (contact_point->x == compare_contact_point->x
       && contact_point->y == compare_contact_point->y
       && contact_point->z == compare_contact_point->z)
         CombineContacts(contact_index, i);
   }

   return FALSE;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   So we've got a contact someplace, and a cell which it's in.  It's a
   vertex contact, so any polygon which has it, has it exactly.

   We return the polygon's index, if applicable, and -1 if not.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static int FindPolygonWithVertexContact(sSphrContactData *contact_data,
                                        mxs_vector *point)
{
   uint polygon_index;
   uint vertex_index;
   uint polygon_vertex_index;
   uint num_vertices_in_polygon;
   int cell_index = contact_data->cell_index;
   PortalCell *cell = WR_CELL(cell_index);
   PortalPolygonCore *polygon = cell->poly_list;
   int num_solid_polys = cell->num_polys - cell->num_portal_polys;
   int num_vertices = cell->num_vertices;
   mxs_vector *vertex = cell->vpool;
   uchar *vertex_list = cell->vertex_list;

#ifdef DBG_ON
   if (config_is_defined("SphereSpew"))
      mprintf("@");
#endif // DBG_ON

   // locate our point in the cell's vertex pool
   for (vertex_index = 0; vertex_index < num_vertices; ++vertex_index) {
      if (vertex->x == point->x
       && vertex->y == point->y
       && vertex->z == point->z)
         break;
      ++vertex;
   }

   // find a solid poly which contains our point
   for (polygon_index = 0; polygon_index < num_solid_polys; ++polygon_index) {
      num_vertices_in_polygon = polygon->num_vertices;

      for (polygon_vertex_index = 0; 
           polygon_vertex_index < num_vertices;
           ++polygon_vertex_index)
         if (vertex_list[polygon_vertex_index] == vertex_index)
            return polygon_index;

      vertex_list += num_vertices_in_polygon;
      ++polygon;
   }

   return -1;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   So what we gots to do, see, is trace down through the BSP tree and
   find all the cells which contain the given vertex.  Then for each
   of these, locate the first solid polygon which contains it
   (sometimes there won't be one).

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void FindMatchingVertices(int contact_index)
{
   int contact_data_index, next_index, polygon_index;
   sSphrContactData *contact_data;

   sSphrContact *contact = &gaSphrContact[contact_index];
   mxs_vector *contact_point = &contact->point_in_world;
   int original_contact_data_index = contact->first_contact_data;

   // find all remaining cells containing our contact point
   SearchBSPForCellsWithPoint(contact_point, WRBSP_HEAD, contact_index);

   // Were any new cells found?  If not, this is a lot simpler...
   if (contact->first_contact_data == original_contact_data_index)
      return;

   // Our new contact data has been added to the start of the list.
   // We want the old list at the head so we can append things to it.
   // We set our iterator to the first of the new contact data so we
   // won't lose track of the new ones altogether.
   contact_data_index = contact->first_contact_data;
   contact->first_contact_data = original_contact_data_index;

   // We've got any number of cells now, including our original; the
   // ones we want; and miscellaneous rubbish.  Let's find our litter.
   while (contact_data_index != original_contact_data_index) {

      contact_data = &gaSphrContactData[contact_data_index];
      next_index = contact_data->next;
      polygon_index = FindPolygonWithVertexContact(contact_data,
                                                   contact_point);

      if (polygon_index != -1) {

         // This one looks good!  We link it back in at the head of
         // the list.  But we still have to test the others.
         contact_data->polygon_index = polygon_index;
         contact_data->next = contact->first_contact_data;
         contact->first_contact_data = contact_data_index;
      }

      contact_data_index = next_index;
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Here's where we clean up our output to yield the contact points we
   want without all those others.

   All contacts later than the earliest by more than epsilon are
   punted on the spot.

   Every edge we can actually hit is in two cells, and every vertex we
   can hit is in at least three.  We need to return all the plane and
   polygon information for these, which means some hideous exhaustive
   searching through our output set.  Fun, fun, fun.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
static void FinalizeContacts(void)
{
   int i;

   // Mi-issed!  Mi-issed!  Nyanny-nyah-de-boo-boo!
   if ((earliest_time == (NO_CONTACT - epsilon)) || (gSphrContactCount == 0))
      return;

   // Let's waste any contact which is later than our earliest by more
   // than epsilon--we do this now, instead of in the next loop, so
   // that we won't decide that one edge or vertex matches another,
   // then later on nuke the second one because its time is too great.
   for (i = 0; i < gSphrContactCount; ++i)
      if ((gaSphrContact[i].time - epsilon) <= earliest_time)
         contact_valid[i] = TRUE;
      else
         contact_valid[i] = FALSE;


   // In a single, sweeping gesture, we find all the cells for edges
   // and vertex contacts, including combining redundant entries.
   for (i = 0; i < gSphrContactCount; ++i) {
      if (!contact_valid[i])
         continue;

      switch(gaSphrContact[i].kind) {
         case kSphrPolygon:
         {
            break;
         }
         case kSphrEdge:
         {
            if (!FindMatchingEdgeContact(i))    // match already found?
               FindMatchingEdge(i);             // if not, search world rep
            break;
         }
         case kSphrVertex:
         {
            // We don't know offhand whether we've got all the cells
            // with solid polys containing a vertex.  We assume not.
            FindMatchingVertexContacts(i);
            FindMatchingVertices(i);
            break;
         }
      }
   }

   // Now let's pack the valid contacts into the start of the output
   // array so the client won't have to deal.  Note that
   // gaSphrContactData may still have unused entries.
   {
      int num_contacts_valid = 0;

      for (i = 0; i < gSphrContactCount; ++i)
         if (contact_valid[i])
            gaSphrContact[num_contacts_valid++] = gaSphrContact[i];

      gSphrContactCount = num_contacts_valid;
   }
}

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The main spherecaster.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int SphrSpherecast(Location *start_loc, Location *end_loc, 
                   mxs_real sphere_radius, uchar flags,
                   int *cell_output, int *cell_output_count)
{
   AUTO_TIMER(SPHR_Total);
 
   PortalCell *cell;
   int cell_index;
   int c;
   bool player_bounding_box_set = FALSE;
   bool post_early_out_init = FALSE;

   static bool plane_has_solid_polys[MAX_PLANES_PER_CELL];
   static BOOL solid_polys_initialized = FALSE;

   if (!solid_polys_initialized)
   {
      memset(plane_has_solid_polys, 0, sizeof(bool) * MAX_PLANES_PER_CELL);
      solid_polys_initialized = TRUE;
   }

   cell_index = CellFromLoc(start_loc);

   #ifdef DBG_ON
   int start_cell_index = start_loc->cell;
   int start_hint_index = start_loc->hint;

   if (start_cell_index == CELL_INVALID)
      if (start_hint_index != cell_index)
         Warning(("SphrSpherecast called with bad hint: %d.\n",
                  start_hint_index));
                     
   warned_cell_reach_overflow = FALSE;
   #endif

   if (cell_index == CELL_INVALID)
      return 0;

   cell = WR_CELL(cell_index);

   // Some initialization
   gSphrContactCount = 0;
   start_point = start_loc->vec;
   end_point = end_loc->vec;
   radius = sphere_radius + SPHR_EPSILON;

   #ifdef DBG_ON
   if (config_is_defined("SphereSpew"))
      mprintf(" ");
   #endif // DBG_ON

   // Find the bounding box that encompasses our whole spherecast
   #ifdef FULL_PROFILE
   TIMER_Start(SPHR_InitialCells);
   #endif

   // Build a sphere that contains both our start and end positions
   Location bsphere_loc;
   mxs_real bsphere_radius;

   bsphere_loc.vec = start_loc->vec;
   bsphere_loc.cell = CELL_INVALID;
   bsphere_loc.hint = (start_loc->cell == CELL_INVALID) ? start_loc->hint : start_loc->cell;;

   bsphere_radius = mx_dist_vec(&start_loc->vec, &end_loc->vec) + radius;

   *cell_output_count = portal_cells_intersecting_sphere(&bsphere_loc, bsphere_radius, cell_output);
   MarkCells(cell_output, *cell_output_count);

   #ifdef FULL_PROFILE
   TIMER_MarkStop(SPHR_InitialCells);
   #endif

   #ifdef DBG_ON
   if (config_is_defined("SphereSpew"))
      mprintf("+");  
   #endif // DBG_ON

   // Now we process each candidate cell in turn, quite possibly
   // adding some along the way.
   int current_plane_index, num_planes;
   PortalPlane *current_plane;
   mxs_vector *current_plane_normal;
   mxs_real start_distance;
   bool vertex_flags_set;

   PortalPolygonCore *polygon;
   PortalPolygonCore *polygon_end;

   for (c = 0; c < *cell_output_count; ++c) 
   {
      vertex_flags_set = FALSE;

      cell_index = cell_output[c];
      cell = WR_CELL(cell_index);
      num_planes = cell->num_planes;

      // Build a list of which planes actually have solid polygons in them,
      // because if they don't, we don't care about them at all.
      polygon = cell->poly_list;
      polygon_end = cell->poly_list + cell->num_polys - cell->num_portal_polys;
      while (polygon < polygon_end)
      {
         plane_has_solid_polys[polygon->planeid] = TRUE;
         polygon++;
      }

      // For each plane with solid polygons, we'll apply four levels
      // of testing: a trivial reject against the entire plane, based
      // on the distance from it to the endpoints; tests against the
      // polygons; against the edges; and finally against the
      // vertices.
      current_plane = cell->plane_list;
      for (current_plane_index = 0;
           current_plane_index < num_planes;
           ++current_plane_index) {
         
         if (!plane_has_solid_polys[current_plane_index])
            continue;

         current_plane_normal = &current_plane->normal;

         // We'll often trivial reject against the entire plane if the
         // object is a projectile in mid-air.  Fun!  (Of course, we
         // almost never will for spheres close to things like
         // stairways, having surfaces at different angles close
         // together.  But it's fun anyway.)

         start_distance = mx_dot_vec(&start_point, current_plane_normal)
                        + current_plane->plane_constant;

         if (start_distance < 0)
            goto next_plane;

         if (start_distance > radius)
         {
            if ((mx_dot_vec(&end_point, current_plane_normal) + current_plane->plane_constant) > radius)
               goto next_plane;
         }

         if (!post_early_out_init)
         {
            // We want an error term proportional to the length of the segment
            // we're casting along.  All these terms refer to the segment in
            // the center of the cast, independent of the rest of the cigar.
            mx_sub_vec(&cast_vector, &end_point, &start_point);
            cast_length = mx_norm_vec(&cast_unit_direction, &cast_vector);
            TO_MXDS_VECTOR(&mxds_cast_unit_direction,
                           &cast_unit_direction);
            TO_MXDS_VECTOR(&mxds_start_point, &start_point);

            epsilon = SPHR_EPSILON / cast_length;
            earliest_time = NO_CONTACT - epsilon;

            radius_squared = radius * radius;
            gSphrContactDataCount = 0;
            portal_flags = flags;

            base_plane_constant = -mx_dot_vec(&cast_unit_direction, &start_point);

            post_early_out_init = TRUE;
         }

         if (TestPolygonsInPlane(cell_index, current_plane_index,
                                 cell_output, cell_output_count))
            goto next_plane;

         // The edge test is the first to use our bounding boxes.
         if (!vertex_flags_set) {
            if (!player_bounding_box_set) {
               SetRayBoundingBox();
               player_bounding_box_set = TRUE;
            }
            SetVertexBoundingBoxes(cell);
            vertex_flags_set = TRUE;
         }

         if (TestEdgesInPlane(cell_index, current_plane_index,
                              cell_output, cell_output_count))
            goto next_plane;

         TestVerticesInPlane(cell_index, current_plane_index,
                             cell_output, cell_output_count);

next_plane:
         ++current_plane;
      }

      memset(plane_has_solid_polys, 0, sizeof(bool) * num_planes);
   }

   UnmarkCells(cell_output, *cell_output_count);
   FinalizeContacts();

#ifdef DBG_ON
   if (config_is_defined("SphereSpew"))
      if (gSphrContactCount)
         SphrDumpCastResult();
#endif // DBG_ON

   return gSphrContactCount;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This wrapper for PortalSpherecast drops the list of cells
   intersecting the cast, and the number of cells, into variables
   local to this module rather than expecting them to be passed in.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int SphrSpherecastStatic(Location *start_loc, Location *end_loc, 
                         mxs_real sphere_radius, uchar flags)
{
   return SphrSpherecast(start_loc, end_loc, sphere_radius, flags,
                         &gaSphrCellsReached[0], &gSphrCellCount);
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This returns FALSE if the sphere intersects even one polygon, other
   than clear portals (ones which don't match our flags).

   The flags parameter is much like that of the spherecaster: it tells
   us which portals we can collide with.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
bool SphrSphereInWorld(Location *center_loc, mxs_real radius, uchar flags)
{
   int i, j;
   PortalCell *cell;
   int cell_index = CellFromLoc(center_loc);

#ifdef DBG_ON
   {
      int center_cell_index = center_loc->cell;
      int center_hint_index = center_loc->hint;

      if (center_cell_index == CELL_INVALID)
         if (center_hint_index != cell_index)
            Warning(("SphrSphereInWorld called with bad hint:%d.\n",
                     center_hint_index));
   }
#endif // DBG_ON

   if (cell_index == CELL_INVALID)
      return FALSE;

   cell = WR_CELL(cell_index);
   //   radius += SPHR_EPSILON;

   gSphrCellCount = portal_cells_intersecting_sphere(center_loc, radius,
                                                     gaSphrCellsReached);

   for (i = 0; i < gSphrCellCount; ++i) {
      PortalCell *cell = WR_CELL(gaSphrCellsReached[i]);
      PortalPolygonCore *polygon = cell->poly_list;
      int num_polys = cell->num_polys;
      int num_solid_polys = num_polys - cell->num_portal_polys;
      uint vertex_offset = 0;

      for (j = 0; j < num_polys; ++j) {
         if ((j < num_solid_polys)
          || ( (polygon->flags & flags) != 0)) //zb is that right??
            if (PolygonIntersectsSphere(cell, polygon, vertex_offset,
                                        &center_loc->vec, radius + SPHR_EPSILON))
               return FALSE;

         vertex_offset += polygon->num_vertices;
         ++polygon;
      }
   }
   return TRUE;
}


#ifdef DBG_ON
/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   There's nothing like a good mono dump in the morning.  Try it!

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void SphrDumpCastResult(void)
{
   int i;
   int contact_data_index;
   sSphrContactData *contact_data;
   sSphrContact *contact = &gaSphrContact[0];

   mprintf("Spherecast result: %d contact%c\n", 
           gSphrContactCount, (gSphrContactCount == 1)? ' ' : 's');

   for (i = 0; i < gSphrContactCount; ++i) {
      switch(contact->kind) {
         case kSphrPolygon:
            mprintf("  polygon");
            break;
         case kSphrEdge:
            mprintf("  edge");
            break;
         case kSphrVertex:
            mprintf("  vertex");
            break;
      }

      mprintf(": (%g %g %g) at time %g\n",
              contact->point_on_ray.x, contact->point_on_ray.y, 
              contact->point_on_ray.z, contact->time);

      for (contact_data_index = contact->first_contact_data;
           contact_data_index != NO_NEXT_CONTACT_DATA;
           contact_data_index
              = gaSphrContactData[contact_data_index].next) {

         contact_data = &gaSphrContactData[contact_data_index];
         mprintf("    cell: %d  polygon: %d  vertex of polygon: %d\n",
                 contact_data->cell_index, 
                 contact_data->polygon_index,
                 contact_data->vertex_in_poly);
      }

      ++contact;
   }
}

#endif // DBG_ON
