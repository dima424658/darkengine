/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/wrcast.c,v 1.38 1998/02/19 15:22:26 MAT Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The raycaster returns TRUE if you can get from the start to the
   end, and FALSE otherwise.

   The comments on the earlier raycaster, which is still present as
   OldPortalRaycast, follow these.  The two routines have enough in
   common that I've skimped in places where the earlier description is
   still valid (esp.: epsilon issues, coplanar surfaces).

   We're working parametrically, with the ray going from 0 to 1.0.
   For each plane we might exit through, we compute an exit time T.
   The math is described the the earlier comments.

   We eliminate some planes through backface culling, without finding
   Ts.  The first step in finding a T is calculating the distance from
   the plane to the starting point of the ray, so the culling is free.

   Of those planes which face the right direction, our exit plane is
   the one with the smallest T.  If it has no portals we stop, and if
   it's all one portal we go through it.  For more complex cases we
   use the plane's normal to identify its larger axes and check which
   portal we've hit using a 2d convex-hull test.  There are six cases
   for this: three axes x two winding directions.  There's a separate
   goofy little tester function for each case.  If we haven't hit any
   of the portals, we've hit a solid polygon, and we stop.

   Optimizations...

   Naturally, once things are optimized a bit, all explanation of
   theory is turned into half-truths.  We do not, in fact, find our
   times using our start and end points, because we wouldn't be able
   to backface-cull in the first cell.  Instead we back up to T = -2
   and perform all calculations from that point (for large cells with
   short raycasts, this won't help; but doing it the right way, using
   the diameter of the bounding sphere of our starting cell, requires
   a divide, and turns out to be slower).

   If a plane has more than one portal to test, but no nonportals, we
   don't have to test the last portal.  We just enter it.

   (Not done:) We're deriving a lot of static data on-the-fly,
   including the major axes and directions of our plane normals and
   which polygons are in which planes.  We could add fields for these
   to the world rep, though if it were only used here the change
   wouldn't gain us much.

   (Not done:) In the exit time loop, we could accumulate distances to
   divide in an array and run them two at a time so they overlapped.
   This might be more of an ASM approach.  Unfortunately we don't have
   any integer operations to stick in between.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


// So here's the comments on the earlier version.  Still good reading!

// World-rep raycast

// This routine casts an infinitely thin ray from one location to
// another, and checks for intersecting terrain.  If the ray is
// coplanar with a surface, that surface doesn't affect it.  If a ray
// hits the edge of a surface, it is stopped.  (This prevents the ray
// from passing through the crack between polygons, but lets us get
// useful data comparing points on the surfaces of polygons.)

// The function returns true if you can get from one point to the
// other.  If it returns false, then you can call a query function to
// determine what cell, what plane, and what polygon the ray was
// stopped by.  (Determining what polygon is expensive; determining
// the others is free once the ray has been cast.)

// If the start point isn't in the database, no cell or plane or
// polygon will be available.

// If the function returns true, the cell cache for the endpoint will
// be set.  Thus, for example, if you test for moving a thing from x
// to y with a raycast, and it's valid, you should copy the entire
// location y into the object's storage, so that its cell cache is
// updated correctly

// There's an extra parameter to raycast.  This is the flag value.
// Any portal which has any bits in common with the flag value will
// block the raycast.  (I suppose people might also want to check for
// particular clutids to do things like "this raycast doesn't go
// through water", but there are a few flag bits free still!)

// IMPLEMENTATION

// The basic approach is to find the cell that the start location is
// in.  Then we explore through the series of cells the ray encounters
// in order.
//
// We begin by considering the ray as parameterized
//   start + t*(end-start)
//
// We will explore the range of t from 0..1.

// Suppose that at time r we enter a particular cell.  (We enter the
// first cell at some negative time t, but we don't actually need to
// know it).

// Then we can iterate over all the planes of the cell.  We compute
// the point-to-plane distance for both the start and end point.  If
// they're on opposite sides, we compute the time s at which they
// intersect.

// Then we know that we exit the cell through the plane which has the
// least s greater than r.

// Now we must deal with the fact that there may be multiple coplanar
// surfaces along the exit plane.  We iterate over all of those
// surfaces.  If none are portals, then we collided with a wall, and
// are done.  If one of them is a portal, then we need to check
// whether we enter that cell at the same time we exited this cell.
// If we did, then we enter that cell, and if not, we're done.

// If there's more than one coplanar portal, then we test whether the
// entry time into each cell is identical to the exit time for this
// cell.

// COMPLICATIONS

// First of all, we need to be able to have a "tolerance" for deciding
// if two "times" are equal.  We'd like this tolerance to be based on
// space, not time, so that two things are considered coplanar if
// they're within e.g.  0.001 units; otherwise we get different
// results for short raycasts and long raycasts.

// Therefore we will actually do the following:
//    delta = end - start
//    len = || delta ||
//    delta = delta / len
//
// Now t ranges from 0..len instead of 0..1

// Second, when we try to decide if we make it through a portal, we
// have to "look ahead" to the next cell.  Rather than looking ahead,
// doing the math, and then coming back, we want to do it all in one
// step.  So if there's only one portal, then we will "tentatively"
// advance into the next cell.  When we check for a time s s.t. r < s,
// we also check for an s s.t. s ~= r.  We can just do this in general
// all the time, except in the first cell; but if we have to have a
// boolean which is set in the first cell, we might as well do it all
// the time.

// In the case of multiple coplanar portals, this is more complicated;
// we need to tentatively advance, but be able to back up and try
// another.  Rather than write this potentially complicated code, we
// note that this case probably occurs infrequently enough that we can
// afford to test the cells redundantly.  Consider the costs:

// Suppose we have n coplanar portals, and an equal chance of going
// through any of the portals (ignore the possibility of hitting a
// wall, which requires exploring all portals).  There are n cases.
// For the ith case, we explore i portals before determining which one
// we go through.

// If we assume naive lookahead, then we will explore all of the cells
// up to the ith, and then we will immediately reexplore that cell.
// If we assume smart lookahead, we explore all of the cells up to the
// ith, and then we're done and already have the information to
// continue the next cell.  Thus, in every case, smart lookahead saves
// one cell exploration every time there are coplanar portals.

// However, if there are n coplanar portals, then on average we will
// explore (n+1)/2 of them with smart, and (n+3)/2 of them with naive.
// If there's only 1 coplanar portal, this is savings (1 vs. 2), and
// the code is easy to do.  With 3 coplanar portals, the savings is
// smaller (2 vs. 3), and it gets progressively smaller as n gets
// larger.

// Thirdly, we can optimize the last cell.  If we know what cell the
// endpoint is in, then the moment we enter that cell, we can stop, as
// long as we've truly entered it, not tentatively entered it.  I
// think the old portal raycaster had bugs in this area because I
// never worked the logic out formally.

// Fourthly, we optimize for the cast where both end points are in the
// same cell, but we don't know that the end point is in the cell when
// we start.  The way we do this that when we check the distance of
// points from each plane, we check the outside point first.  If it's
// inside the plane, then we skip testing the inside point.  If we're
// checking entry time, we can't skip it, but we never are in the
// first cell.

// Fifthly, if our raycast succeeds, we stuff the cell cache for the
// destination, in case it actually gets reused.

#include <math.h>    // fabs()
#include <stdlib.h>  // exit()

#include <lg.h>
#include <dbg.h>
#include <mprintf.h>
#include <r3d.h>
#include <matrix.h>

#include <_portal.h>

#include <port.h>
#include <wrtype.h>
#include <wrdb.h>
#include <wrfunc.h>


// These should really be in dbg.h.  If I need to use them more than
// once, I'll put them there.

#ifdef DBG_ON
# define DbgSetDbgSlot(b,s)    dbgBank[b].gate[DG_DBG]  |=  (s)
# define DbgClearDbgSlot(b,s)  dbgBank[b].gate[DG_DBG]  &= ~(s)
# define DbgSetMonoSlot(b,s)   dbgBank[b].gate[DG_MONO] |=  (s)
# define DbgClearMonoSlot(b,s) dbgBank[b].gate[DG_MONO] &= ~(s)
#else
# define DbgSetDbgSlot(b,s)
# define DbgClearDbgSlot(b,s)
# define DbgSetMonoSlot(b,s)
# define DbgClearMonoSlot(b,s)
#endif


#ifndef _WIN32
#define RAYCAST_ERROR
#endif


#define DBG_Raycast(x)   DBG(DSRC_PORTAL_Raycast, x)
#define SpewRaycast(x)   Spew(DSRC_PORTAL_Raycast, x)


bool debug_raycast;

#ifdef WARN_ON
# define Complain(x) Warning((x));
#else
# define Complain(x) mprintf(x);
#endif

#define RAYCAST_EPSILON 0.001
#define EXTENDED_TIME_RAYCAST_EPSILON (RAYCAST_EPSILON / 3.0)
#define HULL_TEST_EPSILON 0.00001


// OPTIMIZE: use the dot product cache
#define PlaneDist(vec,pln)   (mx_dot_vec(vec, &(pln)->norm->raw) + (pln)->plane_constant)

//////////////////////////////
//
// test if a given point lies along the surface of a cell
//
bool PortalPointOnCell(int cell, mxs_vector *pt)
{
   PortalCell *r = WR_CELL(cell);
   mxs_real dist;
   PortalPlane *p = r->plane_list;
   int n = r->num_planes, i;
   bool found_surface = FALSE;

   SpewRaycast (("PortalPointOnCell: "));

   for (i=0; i < n; ++i,++p) {
      dist = PlaneDist(pt, p);
      SpewRaycast (("!%g ", dist));

      // if the point is behind the plane, we're not in this cell
      if (dist < -RAYCAST_EPSILON * 2.0)
         return FALSE;

      if (fabs(dist) <= RAYCAST_EPSILON * 2.0) // OPTIMIZE: I don't think we need this fabs [DS]
         found_surface = TRUE;
   }

   // this means we're inside the cell, which is also FALSE
   return found_surface;
}

//////////////////////////////
//
// Raycast along a vector, instead of from start point to end point
//
void PortalRaycastVector (Location *start_loc, mxs_vector *vec, 
                          Location *hit_loc, int use_zero_epsilon)
{
   // The way we do this is to do a bunch of finite raycasts, and keep
   // projecting forward if we don't hit.

   Location loc1, loc2;         // start and end of this raycast
   mxs_vector next;             // used to construct loc2;

   loc1 = *start_loc;

   // We could make this faster with pointers switching back and forth
   // so that we're not copying Locations around.  I'm sure it's not
   // worth it; I'm just mentioning it here so Sean doesn't think I'm
   // an idiot. (DS)

   // I think we can afford one copy every 100 units, so long as
   // people don't decide a unit is a millimeter.  (SB)

   // Five bucks to the first person to make this code go into an
   // infinite loop
   for (;;)
   {
      mx_scale_add_vec (&next, &loc1.vec, vec, 100.0);
      MakeLocationFromVector (&loc2, &next);
      if (!PortalRaycast (&loc1, &loc2, hit_loc, use_zero_epsilon))
         return;
      loc1 = loc2;              // oh, the inefficiency
   }
}


// These globals are used for passing values into the six routines
// which follow, and are not guaranteed to have any particular value
// at any time outside the raycaster since they're used by other
// functions which borrow the raycaster's internals.  Say, do you
// think these names are long enough?
static PortalPolygonCore *_portal_raycast_cur_poly;
static mxs_vector *_portal_raycast_cur_vectors;
static uchar *_portal_raycast_point_indirection;
static mxs_vector _portal_raycast_perp_edge;
static mxs_vector _portal_raycast_contact;


// These are set by PortalRaycast().
int PortalRaycastPlane;                 // PLANE_INVALID if we hit nothing
int PortalRaycastCell;                  // end cell of cast if we didn't hit
mxs_vector PortalRaycastHit;            // undefined if we hit nothing

bool PortalRaycastResult = TRUE;


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The next six routines are used for convex hull tests on the larger
   axes of a polygon.  We have a total of six since the standard for
   right-sidedness depends on which direction the ray is coming from
   (that is, it's a winding thing).  TRUE means our ray passes through
   this polygon.

   We check whether a point is within a polygon using a dot product.
   But we're cheap.  We don't want to have to normalize the components.
   So we use a quick and dirty Manhattan distance.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
bool _PortalConvexHullXYPosZ(void)
{
   mxs_vector *end;
   uchar i;
   float size1, size2;
   uchar sides = _portal_raycast_cur_poly->num_vertices;
   mxs_vector *start = _portal_raycast_cur_vectors
                     + _portal_raycast_point_indirection[sides - 1];

   for (i = 0; i < sides; i++) {
      end = _portal_raycast_cur_vectors
          + _portal_raycast_point_indirection[i];
      _portal_raycast_perp_edge.x = end->y - start->y;
      _portal_raycast_perp_edge.y = start->x - end->x;

      if (_portal_raycast_perp_edge.x >= 0.0)
         size1 = _portal_raycast_perp_edge.x;
      else
         size1 = -_portal_raycast_perp_edge.x;

      if (_portal_raycast_perp_edge.y >= 0.0)
         size2 = _portal_raycast_perp_edge.y;
      else
         size2 = -_portal_raycast_perp_edge.y;

      // If we're to the left of any side, we're outside our polygon.
      if ((_portal_raycast_perp_edge.x
           * (_portal_raycast_contact.x - start->x)
           + _portal_raycast_perp_edge.y
           * (_portal_raycast_contact.y - start->y))
       <= (-HULL_TEST_EPSILON * (size1 + size2)))
         return FALSE;

      start = end;
   }

   return TRUE;
}


// This is just like _PortalConvexHullXYPosZ, above.
bool _PortalConvexHullXYNegZ(void)
{
   mxs_vector *end;
   uchar i;
   float size1, size2;
   uchar sides = _portal_raycast_cur_poly->num_vertices;
   mxs_vector *start = _portal_raycast_cur_vectors
                     + _portal_raycast_point_indirection[sides - 1];

   for (i = 0; i < sides; i++) {
      end = _portal_raycast_cur_vectors
          + _portal_raycast_point_indirection[i];
      _portal_raycast_perp_edge.x = end->y - start->y;
      _portal_raycast_perp_edge.y = start->x - end->x;

      if (_portal_raycast_perp_edge.x >= 0.0)
         size1 = _portal_raycast_perp_edge.x;
      else
         size1 = -_portal_raycast_perp_edge.x;

      if (_portal_raycast_perp_edge.y >= 0.0)
         size2 = _portal_raycast_perp_edge.y;
      else
         size2 = -_portal_raycast_perp_edge.y;

      // If we're to the left of any side, we're outside our polygon.
      if ((_portal_raycast_perp_edge.x
           * (_portal_raycast_contact.x - start->x)
           + _portal_raycast_perp_edge.y
           * (_portal_raycast_contact.y - start->y))
       >= (HULL_TEST_EPSILON * (size1 + size2)))
         return FALSE;

      start = end;
   }

   return TRUE;
}


// This is just like _PortalConvexHullXYPosZ, above.
bool _PortalConvexHullYZPosX(void)
{
   mxs_vector *end;
   uchar i;
   float size1, size2;
   uchar sides = _portal_raycast_cur_poly->num_vertices;
   mxs_vector *start = _portal_raycast_cur_vectors
                     + _portal_raycast_point_indirection[sides - 1];

   for (i = 0; i < sides; i++) {
      end = _portal_raycast_cur_vectors
          + _portal_raycast_point_indirection[i];
      _portal_raycast_perp_edge.y = end->z - start->z;
      _portal_raycast_perp_edge.z = start->y - end->y;

      if (_portal_raycast_perp_edge.y >= 0.0)
         size1 = _portal_raycast_perp_edge.y;
      else
         size1 = -_portal_raycast_perp_edge.y;

      if (_portal_raycast_perp_edge.z >= 0.0)
         size2 = _portal_raycast_perp_edge.z;
      else
         size2 = -_portal_raycast_perp_edge.z;

      if ((_portal_raycast_perp_edge.y
           * (_portal_raycast_contact.y - start->y)
           + _portal_raycast_perp_edge.z
           * (_portal_raycast_contact.z - start->z))
       <= (-HULL_TEST_EPSILON * (size1 + size2)))
         return FALSE;

      start = end;
   }

   return TRUE;
}


// This is just like _PortalConvexHullXYPosZ, above.
bool _PortalConvexHullYZNegX(void)
{
   mxs_vector *end;
   uchar i;
   float size1, size2;
   uchar sides = _portal_raycast_cur_poly->num_vertices;
   mxs_vector *start = _portal_raycast_cur_vectors
                     + _portal_raycast_point_indirection[sides - 1];

   for (i = 0; i < sides; i++) {
      end = _portal_raycast_cur_vectors
          + _portal_raycast_point_indirection[i];
      _portal_raycast_perp_edge.y = end->z - start->z;
      _portal_raycast_perp_edge.z = start->y - end->y;

      if (_portal_raycast_perp_edge.y >= 0.0)
         size1 = _portal_raycast_perp_edge.y;
      else
         size1 = -_portal_raycast_perp_edge.y;

      if (_portal_raycast_perp_edge.z >= 0.0)
         size2 = _portal_raycast_perp_edge.z;
      else
         size2 = -_portal_raycast_perp_edge.z;

      if ((_portal_raycast_perp_edge.y
           * (_portal_raycast_contact.y - start->y)
           + _portal_raycast_perp_edge.z
           * (_portal_raycast_contact.z - start->z))
       >= (HULL_TEST_EPSILON * (size1 + size2)))
         return FALSE;

      start = end;
   }

   return TRUE;
}


// This is just like _PortalConvexHullXYPosZ, above.
bool _PortalConvexHullZXPosY(void)
{
   mxs_vector *end;
   uchar i;
   float size1, size2;
   uchar sides = _portal_raycast_cur_poly->num_vertices;
   mxs_vector *start = _portal_raycast_cur_vectors
                     + _portal_raycast_point_indirection[sides - 1];

   for (i = 0; i < sides; i++) {
      end = _portal_raycast_cur_vectors
          + _portal_raycast_point_indirection[i];
      _portal_raycast_perp_edge.z = end->x - start->x;
      _portal_raycast_perp_edge.x = start->z - end->z;

      if (_portal_raycast_perp_edge.z >= 0.0)
         size1 = _portal_raycast_perp_edge.z;
      else
         size1 = -_portal_raycast_perp_edge.z;

      if (_portal_raycast_perp_edge.x >= 0.0)
         size2 = _portal_raycast_perp_edge.x;
      else
         size2 = -_portal_raycast_perp_edge.x;

      if ((_portal_raycast_perp_edge.z
           * (_portal_raycast_contact.z - start->z)
           + _portal_raycast_perp_edge.x
           * (_portal_raycast_contact.x - start->x))
       <= (-HULL_TEST_EPSILON * (size1 + size2)))
         return FALSE;

      start = end;
   }

   return TRUE;
}


// This is just like _PortalConvexHullXYPosZ, above.
bool _PortalConvexHullZXNegY(void)
{
   mxs_vector *end;
   uchar i;
   float size1, size2;
   uchar sides = _portal_raycast_cur_poly->num_vertices;
   mxs_vector *start = _portal_raycast_cur_vectors
                     + _portal_raycast_point_indirection[sides - 1];

   for (i = 0; i < sides; i++) {
      end = _portal_raycast_cur_vectors
          + _portal_raycast_point_indirection[i];
      _portal_raycast_perp_edge.z = end->x - start->x;
      _portal_raycast_perp_edge.x = start->z - end->z;

      if (_portal_raycast_perp_edge.z >= 0.0)
         size1 = _portal_raycast_perp_edge.z;
      else
         size1 = -_portal_raycast_perp_edge.z;

      if (_portal_raycast_perp_edge.x >= 0.0)
         size2 = _portal_raycast_perp_edge.x;
      else
         size2 = -_portal_raycast_perp_edge.x;

      if ((_portal_raycast_perp_edge.z
           * (_portal_raycast_contact.z - start->z)
           + _portal_raycast_perp_edge.x
           * (_portal_raycast_contact.x - start->x))
       >= (HULL_TEST_EPSILON * (size1 + size2)))
         return FALSE;

      start = end;
   }

   return TRUE;
}


bool OldPortalRaycast(Location *start_loc, Location *end_loc, 
                      Location *hit_loc, int portal_block_flags);
bool use_old_raycast = FALSE;
bool (*_portal_raycast_hull_test)() = _PortalConvexHullXYPosZ;


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We have six routines for convex hull tests.  Each projects the
   polygon onto a major axis plane.  So we choose our test based on
   the normal of the polygon's plane.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void _PortalRaycastSetHullTest(mxs_vector *norm)
{
   float x_size = fabs(norm->x);
   float y_size = fabs(norm->y);
   float z_size = fabs(norm->z);

   if (x_size > y_size)
      if (x_size > z_size)
         if (norm->x < 0)
            _portal_raycast_hull_test = _PortalConvexHullYZNegX;
         else
            _portal_raycast_hull_test = _PortalConvexHullYZPosX;
      else
         if (norm->z < 0)
            _portal_raycast_hull_test = _PortalConvexHullXYNegZ;
         else
            _portal_raycast_hull_test = _PortalConvexHullXYPosZ;
   else
      if (y_size > z_size)
         if (norm->y < 0)
            _portal_raycast_hull_test = _PortalConvexHullZXNegY;
         else
            _portal_raycast_hull_test = _PortalConvexHullZXPosY;
      else
         if (norm->z < 0)
            _portal_raycast_hull_test = _PortalConvexHullXYNegZ;
         else
            _portal_raycast_hull_test = _PortalConvexHullXYPosZ;
}


typedef struct {
   PortalPolygonCore *poly;
   uint vertex_offset;
} _PortalRaycastExit;


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Here's the latest raycaster.  It differs from the earlier one
   mainly in that it does not calculate entry times, and decides where
   to go next using convex hull tests.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
bool PortalRaycast(Location *start_loc, Location *end_loc, 
                   Location *hit_loc, int use_zero_epsilon)
{
   int cur_cell;
   int i;

   // ray and associated error terms
   mxs_vector *plane_norm, start_point, end_point;
   mxs_vector ray_vector, early_point;
   double time_epsilon, space_epsilon, ray_length, cur_time, end_time;

   // used in deciding what surface we've reached in a cell
   int num_nonportals_in_cell;
   PortalPolygonCore *cur_poly;
   PortalPlane *cur_plane;
   int plane_is_all_portals;
   int num_portals_in_plane;
   int num_portals_to_test;
   _PortalRaycastExit possible_exit[256];

#ifndef SHIP
   if (start_loc->cell == CELL_INVALID && start_loc->hint == CELL_INVALID)
      Warning(("raycast loc w/ no cell, no hint: (%g %g %g)\n",
               start_loc->vec.x, start_loc->vec.y, start_loc->vec.z));
#endif

   cur_cell = CellFromLoc(start_loc);

   // set our return value & contact info
   PortalRaycastResult = TRUE;
   PortalRaycastPlane = PLANE_INVALID;

#ifndef SHIP
if (use_old_raycast)
   return OldPortalRaycast(start_loc, end_loc, 
                           hit_loc, 0);

   {
      int start_cell = start_loc->cell;
      int start_hint = start_loc->hint;

      if (start_cell == CELL_INVALID)
         if (start_hint != cur_cell)
            mprintf("PortalRaycast: bad hint at (%g %g %g)\n",
                    start_loc->vec.x, start_loc->vec.y, start_loc->vec.z);
   }
#endif

   // check to make sure we're in the database
   if (cur_cell == CELL_INVALID) {

      // stuff our globals used for query functions and return FALSE
      PortalRaycastCell = CELL_INVALID;
      SpewRaycast (("Started outside.\n"));
      *hit_loc = *start_loc;
      PortalRaycastResult = FALSE;

      goto finished;
   }


   start_point = start_loc->vec;
   end_point = end_loc->vec;

   // We want an error term proportional to the length of the segment
   // we're casting along.  Along the way we find the length of our
   // ray.  We have to account for the fact that we are really casting
   // along a ray three times the length of the one passed in.
   mx_sub_vec(&ray_vector, &end_point, &start_point);
   ray_length = mx_mag_vec(&ray_vector);

   if (use_zero_epsilon) {
      time_epsilon = space_epsilon = 0.0;
      end_time = 1.0;
   } else {
      space_epsilon = EXTENDED_TIME_RAYCAST_EPSILON;
      time_epsilon = space_epsilon / ray_length;
      end_time = 1.0 - time_epsilon;
   }

   // Instead of finding our times from the starting point, we'll find
   // them from T = -2 to try and backface-cull some planes in our
   // first cell.
   mx_scale_add_vec(&early_point, &start_point, &ray_vector, -2.0);

   // So now our ray runs from early_point to end_point.
   mx_scaleeq_vec(&ray_vector, 3.0);

   // We advance through the cells until the next plane ahead is at
   // least as far along the ray as our endpoint.
   while (1) {
      double start_dist, end_dist, time;
      int plane_index, vertex_offset;
      int cur_plane_index;
      PortalCell *cell = wr_cell[cur_cell];

      // We want the plane which is in front of us, but closer to our
      // starting point than the other planes in front of us.  We can
      // backface cull the ones facing away from our ray.

      cur_time = 1000000.0;     // big enough for ya?
      for (plane_index = 0, cur_plane = cell->plane_list;
           plane_index < cell->num_planes;
           plane_index++, cur_plane++) {
         plane_norm = &(cur_plane->norm->raw);

         start_dist = plane_norm->x * early_point.x
                    + plane_norm->y * early_point.y
                    + plane_norm->z * early_point.z
                    + cur_plane->plane_constant;

         if (start_dist > space_epsilon) {
            end_dist = plane_norm->x * end_point.x
                     + plane_norm->y * end_point.y
                     + plane_norm->z * end_point.z
                     + cur_plane->plane_constant;

            if (end_dist < 0) {
               time = start_dist / (start_dist - end_dist);
               if (time < cur_time) {
                  cur_time = time;
                  cur_plane_index = plane_index;
               }
            }
         }
      }

      // If every plane we didn't backface cull is beyond our
      // endpoint, we must have reached the end of our ray without
      // colliding with anything.  We return TRUE.
      if (cur_time > end_time) {
         PortalRaycastCell = cur_cell;
         goto finished;
      }

      PortalRaycastPlane = cur_plane_index;

      // If we have nonportals in our plane, we need to test our ray
      // against all the portals; if we've hit none of them, we've hit
      // a nonportal.  So this is the bookkeeping to find out what
      // we've got in this plane.  Pity it's all deriving static info.
      cur_poly = cell->poly_list;
      num_nonportals_in_cell = cell->num_polys - cell->num_portal_polys;
      plane_is_all_portals = 1;
      vertex_offset = 0;
      for (i = 0; i < num_nonportals_in_cell; i++) {
         if (cur_poly->planeid == cur_plane_index)
            plane_is_all_portals = 0;
         vertex_offset += cur_poly->num_vertices;
         cur_poly++;
      }

      // We may have more than one portal in the plane we're exiting.
      // If so, we'll record which ones they are in an array.
      num_portals_in_plane = 0;
      for (; i < cell->num_polys; i++) {
         if (cur_poly->planeid == cur_plane_index) {
            possible_exit[num_portals_in_plane].poly = cur_poly;
            possible_exit[num_portals_in_plane].vertex_offset = vertex_offset;
            num_portals_in_plane++;
         }
         vertex_offset += cur_poly->num_vertices;
         cur_poly++;
      }


      // If there's no cells we can reach through this plane, here we
      // are, and we stop.
      if (num_portals_in_plane == 0)
         goto finished_with_collision;

      // If this plane is nothing but one big portal, we go through it.
      if (num_portals_in_plane == 1
       && plane_is_all_portals) {
         cur_cell = possible_exit[0].poly->destination;
      } else {
         // Here's the nasty case.  A surface with more than one
         // portal or a mixture of portals and nonportals.  We'll test
         // our intersection point against the portals, in 2d, using
         // the larger axes of the plane.
         _PortalRaycastSetHullTest(&(cell->plane_list + cur_plane_index)
                                   ->norm->raw);
         {
            // Here's the point we've hit on the plane.
            mx_scale_add_vec(&_portal_raycast_contact,
                             &early_point, &ray_vector, cur_time);

            // If one of these portals has our intersection point then
            // we head into its cell.  If not, we've hit a regular
            // poly and we're done.  We can skip the last portal if
            // there's no nonportals to hit.
            num_portals_to_test = num_portals_in_plane
                                - plane_is_all_portals;

            _portal_raycast_cur_vectors = cell->vpool;

            for (i = 0; i < num_portals_to_test; i++) {
               _portal_raycast_cur_poly = possible_exit[i].poly;
               _portal_raycast_point_indirection
                  = cell->vertex_list + possible_exit[i].vertex_offset;

               if ((*_portal_raycast_hull_test)())
                  break;
            }

            if (i == num_portals_to_test && !plane_is_all_portals)
               goto finished_with_collision;

            cur_cell = possible_exit[i].poly->destination;
         }
      }
   }

   // We set a few globals before we leave, for anyone who wants to
   // know more about what we hit and where.
finished_with_collision:
   PortalRaycastResult = FALSE;
   PortalRaycastCell = cur_cell;
   mx_scale_add_vec(&PortalRaycastHit, &early_point, &ray_vector,
                    cur_time);

   hit_loc->vec = PortalRaycastHit;

finished:
   return PortalRaycastResult;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   The raycaster only deals with planes and portals.  If we hit
   something, and we want to know which plane, we call this.  A return
   value of -1 means that either the most recent raycast had a clear path
   or it started outside the world rep.  Either way, there's no result we
   can sensibly return.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
int PortalRaycastFindPolygon(void)
{
   int i;
   PortalCell *cell;

   if (PortalRaycastResult || PortalRaycastCell == CELL_INVALID)
      return -1;

   cell = WR_CELL(PortalRaycastCell);

   _portal_raycast_point_indirection = cell->vertex_list;
   _portal_raycast_cur_vectors = cell->vpool;
   _PortalRaycastSetHullTest(&(cell->plane_list + PortalRaycastPlane)
                                   ->norm->raw);
   _portal_raycast_contact = PortalRaycastHit;

   // We don't test the last poly since we know we hit _something_.
   for (i = 0; i < cell->num_render_polys - 1; i++) {
      _portal_raycast_cur_poly = &cell->poly_list[i];
      if (_portal_raycast_cur_poly->planeid == PortalRaycastPlane
       && (*_portal_raycast_hull_test)())
            break;

      _portal_raycast_point_indirection
         += _portal_raycast_cur_poly->num_vertices;
   }
   return i;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This wrapper for testing a point against a polygon insulates the
   outside world from the gawdawfulness of it all.  It trashes some of
   the raycaster's internal globals.

   We allow the client to specify not setting the hull test because
   it's cheaper if we have many coplanar polygons.

   This does not check whether the point is actually in the plane of
   the polygon.  The test is basically 2D.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
bool PortalPointInPolygon(mxs_vector *point, PortalCell *cell,
                          PortalPolygonCore *polygon, 
                          int vertex_offset, bool set_hull_test)
{
   _portal_raycast_point_indirection = cell->vertex_list + vertex_offset;
   _portal_raycast_cur_vectors = cell->vpool;
   _portal_raycast_cur_poly = polygon;
   _portal_raycast_contact = *point;

   if (set_hull_test)
      _PortalRaycastSetHullTest(&(cell->plane_list + polygon->planeid)
                                ->norm->raw);

   return (*_portal_raycast_hull_test)();
}



/* ##################################################################

   Here's the original slow, temporary raycaster.  We'll retire it
   soon.

*/

void compute_raycast_times(int cell, mxs_vector *start, mxs_vector *end,
         double *enter, double *exit)
{
   PortalCell *r = WR_CELL(cell);
   double start_dist, end_dist;
   double enter_time, exit_time;
   int i, n;

   n = r->num_planes;

      // time is on scale of 0..1
   enter_time = -1;
   exit_time = 2;

   for (i=0; i < n; ++i) {
      double time;

#define PlaneDist(vec,pln)   (mx_dot_vec(vec, &(pln)->norm->raw) + (pln)->plane_constant)
#if 0
      start_dist = PlaneDist(start, &r->plane_list[i]);
      end_dist   = PlaneDist(end  , &r->plane_list[i]);
#else
      {
         mxs_vector *norm = &r->plane_list[i].norm->raw;
         start_dist = norm->x * start->x + norm->y * start->y
                    + norm->z * start->z + r->plane_list[i].plane_constant;
           end_dist = norm->x *   end->x + norm->y *   end->y
                    + norm->z *   end->z + r->plane_list[i].plane_constant;
       }    
#endif

      // now, there are icky cases if start_dist ~= 0 or if end_dist ~= 0.
      // we assume we fixup start_dist cases by displacing start location
      // away from cell boundary.

      if (start_dist < -RAYCAST_EPSILON) {
         // we're currently outside.  if end_dist > RAYCAST_EPSILON, then we end
         // inside.  otherwise, we never make it inside

         if (end_dist > 0) {
            time = - start_dist / ((double) end_dist - start_dist);
            if (time > enter_time)
               enter_time = time;
         } else {
            *enter = 3;
            *exit = 2;
            return;
         }
      } else if (start_dist > RAYCAST_EPSILON) {
         // we're currently inside.  end_dist < 0, we make it out 
         // otherwise, we don't get out, so we ignore it

         if (end_dist < 0) {
            time = - start_dist / ((double) end_dist - start_dist);
            if (time < exit_time) 
               exit_time = time;
         }
      }
   }

   *enter = enter_time;
   *exit = exit_time;
}


#define RAYCAST_SLOW_EPSILON 0.0001

// We mark those cells which we have determined do not intersect our
// ray, so we can avoid revisiting them.
PortalCell *_portal_outside_cell[2000];
int _portal_outside_cell_count = 0;


//////////////////////////////
//
// Do a raycast from start_loc to end_loc.
// Returns whether the raycast was clear (not blocked).
// If it returns FALSE, the point where it was blocked is stuffed into
// hit_loc.
//
// portal_block_flags is a bitfield of what kinds of portals block
// this raycast.  See wr.h.
//
bool OldPortalRaycast(Location *start_loc, Location *end_loc, 
                      Location *hit_loc, int portal_block_flags)
{
#ifndef SHIP
   int old_cell = start_loc->cell, old_hint = start_loc->hint;
#endif
   int cur_cell = CellFromLoc(start_loc);
   int adjacent_cell;
   double current_enter, current_exit, epsilon;
   int i;
   bool clear_path = FALSE;

#ifndef SHIP
   if (old_cell==CELL_INVALID)
      if (old_hint!=cur_cell)
         mprintf("Warning: PortalRaycast called with bad hint\n");
#endif

   // check to make sure we're in the database
   if (cur_cell == CELL_INVALID)
   {
      // stuff our globals used for query functions
      PortalRaycastCell = CELL_INVALID;
      SpewRaycast (("Started outside.\n"));
      *hit_loc = *start_loc;
      goto finished;
   }

   {
      mxs_vector delta;
      mx_sub_vec(&delta, &start_loc->vec, &end_loc->vec);
      epsilon = RAYCAST_SLOW_EPSILON / mx_mag_vec(&delta);
   }

   compute_raycast_times(cur_cell, &start_loc->vec, &end_loc->vec,
                         &current_enter, &current_exit);

   if (current_enter >= 1.0 - epsilon) {
      Warning(("PortalRaycast: Raycast starts outside cell or something.\n"));
      *hit_loc = *start_loc;
      goto finished;
   }

   _portal_outside_cell_count = 0;
   for (;;) {
      double this_enter, this_exit;
      double best_enter, best_exit;
      int best_cell = -1, n;
      PortalCell *r = WR_CELL(cur_cell);

      if (current_exit >= 1.0 - epsilon) {
         clear_path = TRUE;
         goto finished;
      }

      // iterate over all portals out of this cell

      best_enter = 2.0;

      n = r->num_portal_polys;
      for (i=0; i < n; ++i) {
         adjacent_cell = r->portal_poly_list[i].destination;

         // If we've already been to the chosen cell and decided it
         // doesn't touch our ray, we can decide it again much faster.
         if (wr_cell[adjacent_cell]->flags & CELL_TRAVERSED)
            continue;

         compute_raycast_times(adjacent_cell,
                               &start_loc->vec, &end_loc->vec, 
                               &this_enter, &this_exit);

         if (this_enter < this_exit) {  // non-empty intersection
            if (this_enter >= current_exit - epsilon // next in raycast chain?
             && this_exit > current_exit) {
               if (this_enter < best_enter) {  // first intersection?
                  best_enter = this_enter;
                  best_exit  = this_exit;
                  best_cell  = r->portal_poly_list[i].destination;
               }
            }
         } else {
            wr_cell[adjacent_cell]->flags |= CELL_TRAVERSED;
            _portal_outside_cell[_portal_outside_cell_count++]
            = wr_cell[adjacent_cell];
         }
      }

      if (best_enter > current_exit + epsilon) {
         if (hit_loc)
            mx_interpolate_vec(&hit_loc->vec, &start_loc->vec, 
                               &end_loc->vec, current_exit);
         goto finished;
      }

      if (best_enter < current_exit - epsilon) {
         if (best_exit < current_exit) {
#ifdef RAYCAST_ERROR
            Error(1, "PortalRaycast: raycast went backwards; failing.\n");
#else
            Warning(("PortalRaycast: raycast went backwards; failing.\n"));
            if (hit_loc)
               mx_interpolate_vec(&hit_loc->vec, &start_loc->vec, 
                                  &end_loc->vec, current_exit);
            goto finished;
#endif
         }
 
         Warning(("PortalRaycast: raycast went backwards.\n")); 
      }

      cur_cell = best_cell;
      current_enter = best_enter;
      current_exit  = best_exit;
   }

finished:
   // clear our visitation flags
   for (i = 0; i < _portal_outside_cell_count; i++)
      _portal_outside_cell[i]->flags &= ~CELL_TRAVERSED;
   return clear_path;
}


/* ##################################################################

   The old fast raycaster is still here for reference, but
   preprocessed out.  Apparently it still has a bug somewhere.
   For that matter, later changes have overtaken it.  */

//#define INCLUDE_OLD_FAST
#ifdef INCLUDE_OLD_FAST

bool PortalRaycastFast(Location *start_loc, Location *end_loc, Location *hit_loc, int portal_block_flags)
{
   int cur_cell = CellFromLoc(start_loc);
   int end_cell = IS_CELLFROMLOC_FAST(end_loc) ? CellFromLoc(end_loc) : -1;

#ifdef DBG_ON
   int prev_cell = -1;
#endif

   mxs_vector *start;           // beginning pos
   mxs_vector *end;             // ending pos
   mxs_vector delta;            // unit vector from start to end

   mxs_real max_time;           // number of delta's to get from start to end
   mxs_real time;               // time of collision

   bool check_entry_time;       // need to make sure this is the right cell?
   mxs_vector entry_point;      // where we enter the new cell

   // Set debug/spew flags by hand
   if (debug_raycast)
   {
      DbgSetDbgSlot  (DB_PORTAL, DS_PORTAL_Raycast);
      DbgSetMonoSlot (DB_PORTAL, DS_PORTAL_Raycast);
   }
// else
// {
//    DbgClearDbgSlot  (DB_PORTAL, DS_PORTAL_Raycast);
//    DbgClearMonoSlot (DB_PORTAL, DS_PORTAL_Raycast);
// }

   // If check_entry_time is true, then we will check every
   // plane to see if we entered on that plane.  If we did,
   // then we will clear check_entry_time to false.
   // This is really combining two different booleans (whether
   // we should be checking, and the results of that check),
   // but it allows us to short circuit the checking, and actually
   // makes the code a little shorter.
#define FP(x)  ((int) ((x) * 65536.0))

   SpewRaycast (("From %g %g %g (%d) to %g %g %g (%d).\n",
                 start_loc->vec.x, start_loc->vec.y, start_loc->vec.z, cur_cell,
                 end_loc->vec.x, end_loc->vec.y, end_loc->vec.z, end_cell));

   // check to make sure we're in the database
   if (cur_cell == CELL_INVALID) {
      // stuff our globals used for query functions
      PoeralRaycastCell = CELL_INVALID;
      SpewRaycast (("Started outside.\n"));
      *hit_loc = *start_loc;
      return FALSE;
   }

   // quick exit if we didn't leave this cell

   if (cur_cell == end_cell) {
      SpewRaycast (("Started together.\n"));
      return TRUE;
   }

   start = &start_loc->vec;
   end = &end_loc->vec;

   // compute length of line
   mx_sub_vec(&delta, end, start);
   max_time = mx_normeq_vec(&delta); // delta now a unit vector

   check_entry_time = FALSE;

end_cell = -1; // disable optimization

   for(;;) {
      PortalCell *r;            // current cell
      PortalPlane *p;           // plane we're checking
      mxs_real inside_dist;     // distance from start point to this plane
      mxs_real outside_dist;    // distance from end point to this plane,
                                // negative if point is "behind" plane (not in cell)

        // if the end point is on a polygon, allow it:
      mxs_real best_time = max_time - RAYCAST_EPSILON; // nearest collision
      int best_plane = -1;      // plane of nearest collision
      int i, n;

      DBG_Raycast ({
         if (check_entry_time)
            SpewRaycast (("Tentatively enter cell %d\n", cur_cell));
         else
            SpewRaycast (("Enter cell %d\n", cur_cell));
      })

      r = WR_CELL(cur_cell);
      p = r->plane_list;
      n = r->num_planes;

      // iterate through all the planes
      for (i=0; i < n; ++i,++p) {
         outside_dist = PlaneDist(end, p);

         DBG_Raycast ({
            inside_dist = -PlaneDist(start, p) / (outside_dist - PlaneDist(start,p));
            SpewRaycast (("%g:%g ", PlaneDist(start,p),
                                    inside_dist));
         })

         if (outside_dist < -RAYCAST_EPSILON) {     // < 0
            // the end point is behind this plane

            inside_dist = PlaneDist(start, p);

            if (inside_dist > -RAYCAST_EPSILON) {   // >= 0

               if (inside_dist < RAYCAST_EPSILON)
                  inside_dist = RAYCAST_EPSILON; // math depends on positive

               // the start point is in front of the plane
               //   compute  inside + (outside-inside)*time = 0
               time = inside_dist / (inside_dist - outside_dist);

               // rescale it into true distance

               // OPTIMIZE: Maybe we can do the rescale just once, after we've
               // found the smallest value of time? [DS]
               time *= max_time;

               // if (time > RAYCAST_EPSILON && time < best_time)
               if (time < best_time) {
                  best_time = time;
                  best_plane = i;
               }

               // we don't bother with check entry time,
               // since this plane faces the start point

            }
         } else {               // this plane faces the end point
            // BUG: To check entry time, we have to check that the
            // _MAXIMAL_ entry time is equal to our expected time...
            
            // the end point is inside the plane, so we can't
            // hit it on the way out... but we may need to check
            // the entry time
            
            if (check_entry_time) {
               time = PlaneDist(&entry_point, p);
               if (time < -RAYCAST_EPSILON)  // < 0
               {
                  // entry point is behind all planes in cell, so our
                  // assumption that we got into this cell was
                  // wrong... fortunately, we'll have already stuffed
                  // the collision globals before we got here, since
                  // we don't know the values any more

                  SpewRaycast (("Failed to pass through portal coplanar with wall.\n"));

                  collide_loc = entry_point;
                  MakeLocationFromVector (hit_loc, &collide_loc);
                  return FALSE;
               }
            }
         }
      }

      // If best_plane == -1, then we didn't find any exit times
      // that were sooner than max_time.  That means that all of
      // the outside_dists were in front of the plane (proof?),
      // and thus the end point was inside this cell.

      if (best_plane == -1) {
         // update the cell cache for the end point
         // this will speed up successive raycasts to the same point
         SpewRaycast (("We made it!\n"));
         end_loc->cell = cur_cell;  // HACK: reaching inside a data structure!
         return TRUE;
      }

      SpewRaycast (("(plane %d) ", best_plane));

      // Check that we're really exiting this cell at a cell boundary.
      // Call the debug version of ourself if we failed.
      {
         mxs_vector pt;
         mx_scale_add_vec(&pt, start, &delta, best_time);
         if (!PortalPointOnCell(cur_cell, &pt)) {
#ifdef RAYCAST_ERROR
            if (!debug_raycast) {
               debug_raycast = TRUE;
               PortalRaycast(start_loc, end_loc, hit_loc, portal_block_flags);
            }
            SpewRaycast(("\ntime was %g; max_time %g; unscaled %g\n", best_time, max_time, best_time/max_time));
            Error (1, "PortalRaycast: PortalPointOnCell failed.\n");
#else
            Complain ("PortalRaycast: PortalPointOnCell failed!\n");
            goto hit_wall;
#endif            
         }
      }

      // update our collision values in case we collide
      PortalRaycastCell = cur_cell;
      PortalRaycastPlane = best_plane;

      // Now that we know what plane we exit through, we have to look
      // at all of the polygons for this plane.

      // OPTIMIZE: add a next-with-same-plane link field to PortalPolygonCore,
      //    and a first-poly-in-this-plane field to PortalPlane (or a parallel array)

      // OPTIMIZE: special case if there's only one polygon which lies in this plane

      {
         PortalPolygonCore *p = r->poly_list;
           // put the plane # back into an unsigned char for fast compare
         unsigned char plane = best_plane;  // plane we hit
         int num_walls;                     // number of walls in this cell
         int new_cell;                      // cell we're moving into
         unsigned char seen_wall = FALSE;   // have we examined any walls?
         unsigned char seen_portal = FALSE; // have we encountered any portals?

         n = r->num_polys;
         num_walls = n - r->num_portal_polys;

         // now, we need a flag to indicate whether or not
         // we've seen a wall or not, and another to indicate
         // whether or not we've seen a valid poly.

         for (i=0; i < n; ++i,++p) {
            if (p->planeid == plane) { // on the plane we hit?
               if (i < num_walls)
                  seen_wall = TRUE;
               else if (p->flags & portal_block_flags)
                  seen_wall = TRUE;
               else {
#if 0 // not working yet
                  if (!seen_portal) {
                     seen_portal = TRUE;
                     new_cell = p->destination;
                  } else {
                     // we've seen more than one portal, so
                     // check the last one (new_cell) to see if it was THE one
  
                     mxs_vector pt;
                     mx_scale_add_vec(&pt, start, &delta, best_time);

                     if (PortalPointOnCell(new_cell, &pt)) {
                        SpewRaycast (("Went through parallel portal.\n"));
                        // ok, this was the one!
                        #ifdef DBG_ON
                        if (prev_cell == new_cell)
                           goto infinite_raycast;
                        prev_cell = cur_cell;
                        #endif
                        seen_wall = FALSE;  // we don't need to check entry time
                        seen_portal = TRUE; // a redundant assignment, but slightly clearer
                        break;
                     }
                     else
                        SpewRaycast(("Didn't go through parallel portal.\n"));
                     new_cell = p->destination;
                  }
#else
                  // it's a portal
                  mxs_vector pt; // OPTIMIZE: Haven't we computed this
                                 // point already?  Maybe [DS]
                  mx_scale_add_vec(&pt, start, &delta, best_time);
                  if (PortalPointOnCell(p->destination, &pt)) {
                     new_cell = p->destination;
                     seen_wall = FALSE;  // we don't need to check entry time
                     seen_portal = TRUE; // a redundant assignment, but slightly clearer
                     break;
                  }
                  else
                     SpewRaycast (("(not %d) ", p->destination));
#endif
               }
            }
         }

         if (!seen_portal) {
            if (!seen_wall) {
#ifdef RAYCAST_ERROR
               if (!debug_raycast) {
                  debug_raycast = TRUE;
                  PortalRaycast(start_loc, end_loc, hit_loc, portal_block_flags);
               }
               Error (1,"PortalRaycast: Didn't hit either a wall or a portal.\n");
#else
               Complain (("PortalRaycast: Didn't hit either a wall or a portal!\n"));
#endif
            }

            SpewRaycast (("Hit a wall.\n"));
            time = best_time;
            goto hit_wall;
         }

#ifdef DBG_ON
         if (prev_cell == new_cell)
            goto infinite_raycast;

         prev_cell = cur_cell;
#endif

         // update our current cell and go around again
         cur_cell = new_cell;

         // if there was a wall, we need to check our entry time
         check_entry_time = seen_wall;
         if (check_entry_time)
            mx_scale_add_vec(&entry_point, start, &delta, best_time);

         // (the following comment only applies to the #if 0 code)

         // note that if there are several coplanar portals and
         // no walls, we don't call PortalPointOnCell() for the
         // very last portal.  If we didn't enter any of the others
         // we assume we went through the last one.  But we don't
         // bother setting check_entry_time (which is only set if
         // we saw a wall).  So that means we never bother verifying
         // that we went through that portal.  But that's ok; we must
         // have gone through it; there is no other option, except
         // for EPSILON failures.  But EPSILON failures always favor
         // us going through portals, not missing them, so if we didn't
         // make it through any of the others, regardless of tolerance,
         // then we must have made it through this one.
      }

      // if we've truly reached the exit cell, we're done
      if (cur_cell == end_cell && !check_entry_time) {
         SpewRaycast (("Made it to exit cell %d.\n", end_cell));
         return TRUE;
      }
   }
   // above is an infinite loop so we don't fall off it

hit_wall:   // jump here with time==dist along the line we hit

   // collide_loc = start + time*delta
   mx_scale_add_vec(&collide_loc, start, &delta, time);
   MakeLocationFromVector (hit_loc, &collide_loc);
   return FALSE;

#ifdef DBG_ON
infinite_raycast:
   // There's either a bug in the code or in the database
   // (e.g. a concave cell, or an incorrect connectivity)
   Warning(("PortalRaycast: infinite loop: %d-%d\n", prev_cell, cur_cell));
   return FALSE;
#endif
}

#endif

/*
Local Variables:
typedefs:("Location" "PortalCell" "PortalPlane" "PortalPolygonCore" "mxs_real" "mxs_vector")
End:
*/
