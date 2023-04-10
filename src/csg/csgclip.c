/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgclip.c,v 1.9 2000/02/19 12:54:56 toml Exp $

#include <lg.h>
#include <math.h>
#include <stdlib.h>
#include <matrix.h>
#include <mprintf.h>

#include <csg.h>
#include <bspdata.h>
#include <csgalloc.h>
#include <csgcheck.h>
#include <csgutil.h>
#include <csgbbox.h>
#include <wrbsp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

int debug_count, show_debug=0;
#define SHOW()   (debug_count == show_debug)

extern void write_brush_start(void);
extern void write_polyhedron_brushes(PortalPolyhedron *ph);
extern void write_plane_brush(BspPlane *clip);
extern void write_brush_end(void);

////////
//
//  Portalize the database
//
//
//  For portalizing, we want to use a "winged edge" kind of data
//  structure.  The basic idea here is to share edges and polygons
//  so when we split one, we split the other, avoiding t-joints.
//  Now, one oddball thing is that an edge can be shared by more
//  than two polygons.  Only two will ever be rendered as opaque
//  polygons and need t-joint processing (it's unclear whether portals
//  need it--seems the answer is no if we only take bounding boxes
//  of them), but we'd rather not distinguish solid and open space
//  during splitting.  So we have to allow an edge to be shared by
//  more than two polygons.  This is rather problematic.  So it looks
//  like things work like this:
//     Each polygon is a list of edges.  Shared edges are stored
//     multiple times, in a linked list.
//
//   Polyhedron:
//       ptr to one polygon
//
//   Polygon:
//       ptr to polyhedron A
//       ptr to next polygon for polyhedron A
//       ptr to polyhedron B
//       ptr to next polygon for polyhedron B
//       ptr to a polyedge
//       ptr to BspPlane that represents this surface
//
//   Polyedge:
//       ptr to polygon
//       ptr to next edge for that polygon
//       ptr to next polyedge for this edge
//       ptr to raw edge data (shared by all like polyedges)
//
//
//  Clipping boundary conditions
//
//    Clipping is pretty hard to get right due to boundary
//  conditions.  The problem is dealing with points, edges,
//  and polygons which lie in the plane that we're using to
//  clip with.
//
//    Fortunately, because we're clipping convex polyhedra
//  with planes, and thus clipping convex polygons with planes,
//  we happen to be able to avoid most of the nasty clipping
//  cases, because boundary conditions possible at one level
//  are no longer an issue.
//
//    For example, suppose we want to clip a polygon by a plane.
//  If an edge is embedded in the plane, a traditional clip
//  requires us to assign that edge to only one side of the plane
//  (the side with the rest of the polygon).  In an edge-based
//  format with no explicit ordering, this requires checking
//  other edges to see which side to put it on.  If the entire
//  polygon is embedded in the plane, the obvious clipper outputs
//  empty polygons on each side--but when clipping a polyhedron
//  we want the polygon to come out on the same side of the clip
//  plane as the rest of the polyhedron.
//
//    We can define the low-level clippers to not handle these
//  cases, and test for them explicitly up above; but this code
//  might make things more complex in the wrong place.
//
//    Fortunately, it turns out that we already check for those
//  cases in relatively natural ways as we traverse, and so we
//  can just not handle these cases.  This means the code below
//  never seems to check for these cases, and this comment is
//  the explanation why.
//
//    When we traverse the tree, we build up a polyhedron representing
//  the particular space a node contains.  We will clip that space
//  by the next subplane.
//
//    First off, we will never clip a polyhedron by a plane which
//  is coplanar with a surface of the polyhedron--no such BSP node
//  will ever get built, because that coplanarity will be detected
//  during build time.  Therefore there will never be a polygon
//  embedded in the clipping plane.  We can easily check for this
//  in the existing code, which point classifies every polygon, and
//  vertify that this never happens.
//
//    The second case, of an edge embedded in a clipping plane, will
//  also never happen.  If an edge is in a clipping plane, then the
//  polygon is "entirely" on one side or the other of the plane.
//  Since we clip polygons in the context of partitioning them to one
//  side or the other of a plane, and only clip if they cross, we
//  only have to handle the crossing case; we just make sure that if
//  any edge is embedded in the plane, we partition the polygon to
//  the correct side without clipping.  That's ok, because that just
//  means correctly handling the cases where some points code onto
//  the plane.


extern int VertexCode(BspVertex *v, BspPlane *p);

Real DistPointToPlane(BspVertex *v, BspPlane *p)
{
   return v->x*p->a + v->y*p->b + v->z*p->c + p->d;
}

int FineVertexCode(BspVertex *v, BspPlane *p)
{
   if (DistPointToPlane(v,p) < 0)
      return BEHIND;
   return IN_FRONT;
}

#define COLINEAR_EPSILON 0.0001
#define COLINEAR_MIN     0.001

static BOOL between;
static bool colinear(BspVertex *e1, BspVertex *e2, BspVertex *f2)
{
   double res;
   mxs_vector a,b;
   // check if f lies on a line with e
   // form two lines, normalize, dot product
   // and check the resultant length.

   // e1 matches f1, so build e1->e2, f2->f1
   a.x = e2->x - e1->x;
   a.y = e2->y - e1->y;
   a.z = e2->z - e1->z;

   b.x = e1->x - f2->x;
   b.y = e1->y - f2->y;
   b.z = e1->z - f2->z;

   mx_normeq_vec(&a);
   mx_normeq_vec(&b);

   res = mx_dot_vec(&a, &b);
   between = (res < 0);
   if (fabs(res) >= 1-COLINEAR_EPSILON)
      return TRUE;
   return FALSE;
}

BOOL colinear_test(BspVertex **list, BspVertex *k, int *num)
{
   // put first one in list
   if (*num == 0) {
      list[0] = k;
      *num = 1;
      return TRUE;
   }

   // check if matches first one
   if (!VERTEX_COMPARE(list[0], *k))
      return TRUE;

   if (*num == 1) {
      list[1] = k;
      *num = 2;
      return TRUE;
   }

   // check if matches second one
   if (!VERTEX_COMPARE(list[1], *k))
      return TRUE;
   
   // check if colinear with both of them
   if (colinear(k, list[0], list[1])) {
#if 0
      if (between)
         ... we could try to expand to use further apart ones ...
#endif
      return TRUE;
   }
   return FALSE;
}

// This messy routine decides if an apparently-coplanar polygon
// should be assigned to the inside or the outside, or split
int FinePolygonCode(PortalPolygon *p, BspPlane *clip)
{
   int counts[3];
   PortalPolyEdge *x;
   counts[BEHIND] = counts[IN_FRONT] = 0;

   Warning(("SplitPortalPolyhedronByPlane: Classifying coplanar polygon\n"));

   x = p->edge;
   do {
      ++counts[FineVertexCode(&x->data->start, clip)];
      ++counts[FineVertexCode(&x->data->end, clip)];
      x = x->poly_next;
   } while (x != p->edge);

   Warning(("%d in front, %d behind.\n",
      counts[IN_FRONT]>>1, counts[BEHIND]>>1));

   if (counts[IN_FRONT] == 0)
      return BEHIND;

   // also, if only two points are BEHIND, then if we split
   // it would be degenerate, so punt it

   if (counts[BEHIND] <= 4)   // two vertices, each seen twice
      return IN_FRONT;

   // now, the two polygons will be: all of the BEHIND vertices,
   // and all of the IN_FRONT vertices plus the two dominant BEHIND vertices
   // so check if all of the BEHIND vertices are colinear

   Warning(("SplitPortalPolyhedronByPlane: Checking for colineararity.\n"));
#if 1
   if (SHOW()) {
      mprintf("We think the coplanar polygon needs splitting.\n");
   }
   return CROSS;
#else
  {
   int num;
   BspVertex *list[2], *k;
   num = 0;

   x = p->edge;
   do {
      k = &x->data->start;
      if (FineVertexCode(k, clip) == BEHIND)
         if (!colinear_test(list, k, &num))
            return CROSS;
      k = &x->data->end;
      if (FineVertexCode(k, clip) == BEHIND)
         if (!colinear_test(list, k, &num))
            return CROSS;
      x = x->poly_next;
   } while (x != p->edge);

   return IN_FRONT;
  }
#endif
}

//  Take an edge.  Split it into two new edges.
//  Leave one where it is.  Attach the other to a new polygon.
BspVertex *clipped_point;
static PortalPolyEdge *ClipEdgeByPlane(PortalPolyEdge *edge, BspPlane *clip)
{
   PortalEdge *new_data = PortalEdgeAlloc();
   PortalPolyEdge *new_edge, *x, *y;
   Real d0,d1, interp;
   BspVertex *v0,*v1;

   v0 = &edge->data->start;
   v1 = &edge->data->end;

   d0 = DistPointToPlane(v0, clip);
   d1 = DistPointToPlane(v1, clip);

     // compute clipping point
     //    d0 + interp * (d1-d0) = 0
     //    interp * (d1-d0) = -d0

     // compute intersection point of line with clip plane

     // (bug) instantiate this code twice so for a given
     // clip plane we clip identically regardless of point order
     // this generates exactly equal vertices, not approximately equal

   if (d0 < 0) {

      interp = -d0 / (d1-d0);

      new_data->start.x = v0->x + interp * (v1->x - v0->x);
      new_data->start.y = v0->y + interp * (v1->y - v0->y);
      new_data->start.z = v0->z + interp * (v1->z - v0->z);
      clipped_point = &new_data->start;

   } else {

      interp = -d1 / (d0-d1);

      new_data->start.x = v1->x + interp * (v0->x - v1->x);
      new_data->start.y = v1->y + interp * (v0->y - v1->y);
      new_data->start.z = v1->z + interp * (v0->z - v1->z);
    }

   clipped_point = &new_data->start;

     // fixup edges so existing vertices are inside, new_data is outside

   if (d0 > 0) {  // start is inside
      new_data->end = *v1;
      *v1 = new_data->start;
   } else {       // end is inside
      new_data->end = *v0;
      *v0 = new_data->start;
   }

     // now edge->data is inside, and new_data is outside
     // This is good, because edge is still hooked up to
     // polygon "source", and we want to attach new_data
     // to polygon "outside"

     // iterate over all of the existing PolyEdge's which share
     // the old edge, and allocate new PolyEdges to match up to them

   x = edge;
   new_edge = 0;

   do {
      y = PortalPolyEdgeAlloc();

      if (new_edge) {
         // insert after first element of list
         y->edge_next = new_edge->edge_next;
         new_edge->edge_next = y;
      } else {
         // create one element circularly linked list
         y->edge_next = y;
         new_edge = y;
      }
      y->data = new_data;

        //  all x's belong to the inside, all y's belong to the
        //  outside; however, we're only clipping one of the polygons
        //  the edge belongs to.  So all of the x's and y's should get
        //  linked together via poly_next field, except for the first
        //  ones which are the ones belonging to the original polygon.
        //  just do them all and undo the first one later

      y->poly = x->poly;
      y->poly_next = x->poly_next;
      x->poly_next = y;

      x = x->edge_next;
   } while (x != edge);

     // now unlink the first new_edge from the poly edge list, because
     // it no longer belongs to the original polygon

      // none of this is necessary because currently the caller always relinks
     // the entire edge list, but we'll do it for cleanliness

   edge->poly_next = new_edge->poly_next;
   new_edge->poly_next = 0;

   return new_edge;
}

////////
//
//  clip a polyhedron by a plane
//

PortalPolygon *border_poly;

// Add an edge to the new polygon embedded in the clip plane
static void AddBorderPolyEdge(PortalPolyEdge *edge)
{
   PortalPolyEdge *e = PortalPolyEdgeAlloc();

   e->poly = border_poly;
   e->data = edge->data;
   if (border_poly->edge) {
      e->poly_next = border_poly->edge->poly_next;
      border_poly->edge->poly_next = e;
   } else {
      e->poly_next = e;
      border_poly->edge = e;
   }

   e->edge_next = edge->edge_next;
   edge->edge_next = e;
}

extern void remove_poly_edge(PortalPolygon *, PortalPolyEdge *);
static bool BorderEdgeExists(PortalPolyEdge *edge)
{
   PortalPolyEdge *x, *first;

   x = first = border_poly->edge;
   if (!first) return FALSE;

   do {
      if (x->data == edge->data) {
         remove_poly_edge(border_poly, edge);
         return TRUE;
      }
      x = x->poly_next;
   } while (x != first);
   return FALSE;
}

// build points for adding a new edge
int point_count = 0;

#define MAX_POINTIES  16
static BspVertex points[MAX_POINTIES];
static int point_counts[MAX_POINTIES];

BspVertex in_point, out_point;

static void AddCoplanarPoint(BspVertex *v)
{
   int i;
   for (i=0; i < point_count; ++i) {
      if (!VERTEX_COMPARE(*v, points[i])) {
         ++point_counts[i];
         return;
      }
   }

   if (point_count == MAX_POINTIES)
      Error(1, "Too many boundary points trying to cope with "
               "nearly coplanar edges.\n");

   points[point_count] = *v;
   point_counts[point_count] = 1;
   ++point_count;
}

//  Take all of the edges in 'source' and clip them against the clipping plane.
//  All of the edges which are "inside" the plane go into "in", all of the edges
//  which are "outside" the plane go into "out", none of them are coplanar,
//  and those which cross are split.

PortalPolygon *PortalClipPolygonByPlane(PortalPolygon *source, BspPlane *clip, PortalPolyhedron *ph, bool merge)
{
   PortalPolyEdge *edge, *next_edge, *first_edge, *in_edge, *out_edge;
   PortalPolygon *outer;

if (SHOW()) {
  mprintf("PortalClipPolygon\n");
  dump_portal_poly(source);
}

   // number of points we've seen that were coplanar
   point_count = 0;

     // allocate the second polygon so we can attach edges to it correctly

   outer = PortalPolygonAlloc();
   outer->misc = 0;

     // every point in the source polygon meets one of the following constraints:
     //   (1) it is "in front of" the plane
     //   (2) it is "behind" the plane
     //   (3) it is on the plane

     // This clipper is edge-based, not vertex based.  So we'll stupidly
     // recompute sidedness twice per vertex, but in return we get edge sharing
     // amongst all polygons.

     // Again, we need to pay ultra special careful attention to how we
     // handle boundary cases.  If an edge has already been split in the
     // right place, we need to deal.  If we already have an edge lying
     // in the clip plane, we need to deal.

     // (bug) Of course, we also have to generate one more edge, which lies in
     // the clipping plane.  This means we have to find all of the vertices which
     // lie in the clipping plane; there may be some which already do, or there's
     // some generated as part of the clip.

   first_edge = source->edge;

     // initialize both other polygons to be empty

   in_edge = out_edge = 0;

     // iterate over all edges

   edge = first_edge;
   do {
      int code_1, code_2, code;
      code_1 = VertexCode(&edge->data->start, clip);
      code_2 = VertexCode(&edge->data->end  , clip);

      // save the next pointer so we can update it safely
      next_edge = edge->poly_next;

if (SHOW()) {
   mprintf("Processing edge coded %d %d\n", code_1, code_2);
   dump_edge(edge);
}
      code = code_1 | code_2;

      if (code == CROSS) {
         PortalPolyEdge *outer_edge;

         outer_edge = ClipEdgeByPlane(edge, clip);
         AddCoplanarPoint(clipped_point);
if (SHOW()) {
   mprintf("Adding intersection point %g %g %g\n", clipped_point->x,
    clipped_point->y, clipped_point->z);
}

         in_edge = AddEdgeToList(in_edge, edge);
         out_edge = AddEdgeToList(out_edge, outer_edge);
         outer_edge->poly = outer;
   
      } else { // Don't need to clip

         if (code == COPLANAR) {
            // if we have an embedded apparently coplanar edge, we 
            // must be very careful.  We have to assign it to one side
            // or another, but be globally consistent; and we must pick
            // one vertex to be the one used to make the split edge

            code_1 = FineVertexCode(&edge->data->start, clip);
            code_2 = FineVertexCode(&edge->data->end  , clip);

if (SHOW()) {
   mprintf("Recoded finely to %d %d\n", code_1, code_2);
}
            code = code_1 | code_2;

            if (code == CROSS) {
               // whichever point is in behind, use that
               if (code_1 == BEHIND) AddCoplanarPoint(&edge->data->start);
               else if (code_2 == BEHIND) AddCoplanarPoint(&edge->data->end);
               else Error(1, "PortalClipPolygonByPlane: WTF???\n");

if (SHOW()) {
   if (code_1 == BEHIND) mprintf("Adding boundary point %g %g %g from coplanar edge\n", edge->data->start.x, edge->data->start.y, edge->data->start.z);
   if (code_2 == BEHIND) mprintf("Adding boundary point %g %g %g from coplanar edge\n", edge->data->end.x, edge->data->end.y, edge->data->end.z);
}

               // if the edge crosses, always associate it with the front
               // (because we're using the behind point as the split line)
               code = IN_FRONT;
            }

         } else {  // code == IN_FRONT || code == BEHIND
            // if one end is coplanar, then check if the point
            // is actually on the opposite side of the plane; if
            // it is, then use it for the split edge

            if (code == IN_FRONT && code_1 == COPLANAR) {
               if (FineVertexCode(&edge->data->start, clip) == BEHIND) {
                  AddCoplanarPoint(&edge->data->start);
if (SHOW()) {
   mprintf("Added boundary point from coplanar crossing to non-coplanar\n");
   mprintf("%g %g %g\n", edge->data->start.x, edge->data->start.y, edge->data->start.z);
}
               }
            } else if (code == IN_FRONT && code_2 == COPLANAR) {
               if (FineVertexCode(&edge->data->end, clip) == BEHIND) {
                   AddCoplanarPoint(&edge->data->end);
if (SHOW()) {
   mprintf("Added boundary point from coplanar crossing to non-coplanar\n");
   mprintf("%g %g %g\n", edge->data->start.x, edge->data->start.y, edge->data->start.z);
}
               }
            } else if (code == BEHIND && code_1 == COPLANAR) {
               if (FineVertexCode(&edge->data->start,clip) == IN_FRONT) {
                  AddCoplanarPoint(&edge->data->start);
if (SHOW()) {
   mprintf("Added boundary point from coplanar crossing to non-coplanar\n");
   mprintf("%g %g %g\n", edge->data->end.x, edge->data->end.y, edge->data->end.z);
}
               }
            } else if (code == BEHIND && code_2 == COPLANAR) {
               if (FineVertexCode(&edge->data->end,clip) == IN_FRONT) {
                  AddCoplanarPoint(&edge->data->end);
if (SHOW()) {
   mprintf("Added boundary point from coplanar crossing to non-coplanar\n");
   mprintf("%g %g %g\n", edge->data->end.x, edge->data->end.y, edge->data->end.z);
}
               }
            }
         }

         if (code == IN_FRONT) {
            in_edge = AddEdgeToList(in_edge, edge);
         } else if (code == BEHIND) {
            out_edge = AddEdgeToList(out_edge, edge);
            edge->poly = outer;
         } else
            Error(1, "PortalClipPolygonByPlane: edge was bad\n");
      }

      edge = next_edge;
   } while (edge != first_edge);

   if (!out_edge || !in_edge)
      Error(1, "PortalClipPolygonByPlane: Tried to clip polygon that didn't straddle the plane.\n");

     // now build the new edge and add it into both of the polygons

     // (worry) what if this edge already exists somewhere?  Can it happen?
     // it can happen now that we split faces for polygon subdivision
     // so we'll need a separate t-joint pass
   {
      PortalEdge *new_data;
      PortalPolyEdge *x, *y;

#if 1
   if (point_count != 2)
      Error(1, "PortalClipPolygonByPlane: Didn't find exactly two spanning points");
#endif

      in_point = points[0];
      out_point = points[1];

      new_data = PortalEdgeAlloc();
      x = PortalPolyEdgeAlloc();
      y = PortalPolyEdgeAlloc();

      new_data->start = in_point;
      new_data->end   = out_point;

      x->data = new_data;
      y->data = new_data;

      x->edge_next = y;
      y->edge_next = x;

      x->poly = source;
      y->poly = outer;

if (SHOW()) {
   mprintf("Adding polygon split edge to boundary:\n");
   dump_edge(x);
}

       in_edge = AddEdgeToList(in_edge, x);
      out_edge = AddEdgeToList(out_edge, y);

      if (ph && !merge)
         AddBorderPolyEdge(x);
   }

     // link up the outer polygon by copying everything then patching

   *outer = *source;
   outer->edge = out_edge;
   source->edge = in_edge;

   if (ph) {
      int n;

        // now find the ph of the other side of this polygon
      if (source->ph[0] == ph)
         n = 1;
      else if (source->ph[1] == ph)
         n = 0;
      else
         Error(1, "PortalClipPolygonByPlane: Polygon didn't belong to polyhedron\n");

        // other == NULL means we're at the edge of the universe
      if (source->ph[n]) {
           // link up the new poly
         outer->ph_next[n] = source->ph_next[n];
         source->ph_next[n] = outer;
      }
#ifdef DBG_ON
      CheckPoly(source, " (inner clipped poly)");
      CheckPoly(outer, " (outer clipped poly)");
#endif
   } else {
#ifdef DBG_ON
      CheckPoly2(source, " (inner clipped poly)");
      CheckPoly2(outer, " (outer clipped poly)");
#endif
   }
if (SHOW()) {
   mprintf("Inner polygon:\n"); dump_portal_poly(source);
   mprintf("Outer polygon:\n"); dump_portal_poly(outer);
}

     // it's the job of the caller to link them into place correctly

   return outer;
}

static int PortalPolygonPlaneCompareUpdateBorder(
           PortalPolygon *p, BspPlane *plane, int side)
{
   int code = 0, this_code, coplanar_edges=0;
   PortalPolyEdge *edge = p->edge;

   do {
      this_code  = VertexCode(&edge->data->start, plane);
      this_code |= VertexCode(&edge->data->end, plane);
      if (this_code == COPLANAR) {
         int code_s = FineVertexCode(&edge->data->start, plane);
         int code_e = FineVertexCode(&edge->data->end  , plane);
         int fcode = code_s | code_e;
            // if edge crosses the plane
            // the point that dominates is the one IN_FRONT
// the point that dominates is the one _BEHIND_
            // if the dominator is on the same side as this polygon
            // or if edge is on opposite side
         if ((fcode == CROSS && side == BEHIND) || fcode == CROSS-side) {
if (SHOW()) {
   mprintf("Adding coplanar boundary edge: fcode=%d, side=%d\n", fcode, side);
   dump_edge(edge);
}
            if (!BorderEdgeExists(edge)) {
               AddBorderPolyEdge(edge);
               ++coplanar_edges;
            }
#if 1
            else {
if (SHOW()) {
   mprintf("Deleting because it was duplicated.\n");
}
               Error(1, "Output border edge more than once.\n");
            }
#endif
         }
      }
      code |= this_code;
      edge  = edge->poly_next;
   } while (edge != p->edge);

   if (code == CROSS && coplanar_edges)
      Error(1, "Polygon crosses plane but has coplanar edge.\n");
   return code;
}

int PortalPolygonPlaneCompare(PortalPolygon *p, BspPlane *plane)
{
   int code = 0;
   PortalPolyEdge *edge = p->edge;

   do {
#if 0
      this_code  = VertexCode(&edge->data->start, plane);
      this_code |= VertexCode(&edge->data->end, plane);
      code |= this_code;
#else 
      Real d;
      BspVertex *v = &edge->data->start;
      d = v->x*plane->a + v->y*plane->b + v->z*plane->c + plane->d;
      if (d > REAL_EPSILON) code |= IN_FRONT;
      else if (-d > REAL_EPSILON) code |= BEHIND;
      v = &edge->data->end;
      d = v->x*plane->a + v->y*plane->b + v->z*plane->c + plane->d;
      if (d > REAL_EPSILON) code |= IN_FRONT;
      else if (-d > REAL_EPSILON) code |= BEHIND;
#endif
      edge  = edge->poly_next;
   } while (edge != p->edge);

   return code;
}

int PortalPolyhedronPlaneCompare(PortalPolyhedron *ph, BspPlane *plane)
{
   int code = 0;
   PortalPolygon *p = ph->poly;

   do {
      code |= PortalPolygonPlaneCompare(p, plane);
      p = GetPhNext(p, ph);
   } while (p != ph->poly);
   return code;
}

//  To split a polyhedron by a plane which we know it crosses, we must do
//  the following things:
//     split any polygon which crosses the plane
//     assign other polygons to one side or the other
//     build the boundary polygon
//
//  The boundary polygon consists of every edge which was embedded in the
//  plane, plus every edge we create clipping polygons against the plane.
//  So the routines that clip add the edge, and the "detect which side we're
//  on" processes embedded edges.
//
//  (bug) We will encounter every embedded edge _twice_, because every existing
//  edge belongs to a given polyhedron twice.  So we just make it stupid and
//  brute force check whether we already have an edge before adding it (made
//  easier by shared edges)

PortalPolyhedron *SplitPortalPolyhedronByPlane(PortalPolyhedron *s,
                  BspPlane *clip, bool merge, bool set_plane)
{
   PortalPolyhedron *out = PortalPolyhedronAlloc();
   PortalPolygon *in_poly, *out_poly, *poly, *next, *first_poly, *outer;

   ++debug_count;
   if (show_debug == -1)
      mprintf("%d\n", debug_count);
   else if (SHOW()) {
      mprintf("Splitting polyhedron by plane ");
      dump_plane(clip);
   }

   if (!merge) {
      border_poly = PortalPolygonAlloc();
      border_poly->edge = 0;
      border_poly->misc = 0;
      border_poly->plane = set_plane ? clip : 0;
   } else
      border_poly = 0;

   in_poly = out_poly = 0;

   first_poly = s->poly;
   out->leaf = 0;

   poly = first_poly;
   do {
      int code;
      next = GetPhNext(poly, s);

if (SHOW()) {
   mprintf("Processing polygon:\n");
   dump_portal_poly(poly);
}
      code = PortalPolygonPlaneCompare(poly, clip);
      if (code == COPLANAR)
         code = FinePolygonCode(poly, clip);

      switch (code) {
         case IN_FRONT:
if (SHOW()) mprintf("In front\n");
            if (!merge) PortalPolygonPlaneCompareUpdateBorder(poly, clip, IN_FRONT);
            in_poly = AddPolyToList(in_poly, poly, s);
            break;
         case BEHIND:
if (SHOW()) mprintf("Behind\n");
            if (!merge) PortalPolygonPlaneCompareUpdateBorder(poly, clip, BEHIND);
            ChangePh(poly, s, out);
            out_poly = AddPolyToList(out_poly, poly, out);
            break;
         case CROSS:
            outer = PortalClipPolygonByPlane(poly, clip, s, merge);
            ChangePh(outer, s, out);
            in_poly = AddPolyToList(in_poly, poly, s);
            out_poly = AddPolyToList(out_poly, outer, out);
            break;
         default:
            Error(1, "SplitPolyhedronByPlane: Invalid case %d.\n", code);
      }
      poly = next;
   } while (poly != first_poly);

   // update these so we can dump them...

   if (!merge) {
#if 1
      if (in_poly == NULL || out_poly == NULL) {
         s->poly = in_poly;
         out->poly = out_poly;
         if (in_poly) dump_portal_polyhedron(s);
         if (out_poly) dump_portal_polyhedron(out);
         write_brush_start();
         if (in_poly) write_polyhedron_brushes(s);
         if (out_poly) write_polyhedron_brushes(out);
         dump_plane(clip);
         write_plane_brush(clip);
         write_brush_end();
         Error(1, "SplitPortalPolyhedronByPlane: polyhedron didn't cross plane\n");
      }
#endif

      if (in_poly != NULL && out_poly != NULL) {
         border_poly->ph[0] = s;
         border_poly->ph[1] = out;

        #ifdef DBG_ON
if (SHOW()) {
   printf("boundary poly:\n");
   dump_portal_poly(border_poly);
}
           CheckPoly(border_poly, " (boundary polygon)");
        #endif

         in_poly = AddPolyToList(in_poly, border_poly, s);
         out_poly = AddPolyToList(out_poly, border_poly, out);
      }
   }

   s->poly = in_poly;
   out->poly = out_poly;
   return out;
}

extern BspNode *BspAllocateDummyNode(void);

// This permanently updates the portal database,
// so if we make multiple passes, this will have
// still been split
void split_polyhedron(PortalPolyhedron *ph)
{
   // first compute the bounds
   BspVertex minv, maxv, diff;
   int axis;
   double val;
   BspPlane split;

   compute_ph_bbox(ph, &minv, &maxv);

   // decide which axis is longest, and split along that

   diff.x = maxv.x - minv.x;
   diff.y = maxv.y - minv.y;
   diff.z = maxv.z - minv.z;

   if (diff.x > diff.y)
      axis = 0, val = diff.x;
   else
      axis = 1, val = diff.y;
   if (diff.z > val)
      axis = 2;

   split.a = split.b = split.c = 0;
   split.d = 0;

   switch (axis) {
      case 0: split.a = 1; split.d = -(minv.x + maxv.x) / 2; break;
      case 1: split.b = 1; split.d = -(minv.y + maxv.y) / 2; break;
      case 2: split.c = 1; split.d = -(minv.z + maxv.z) / 2; break;
   }

   {
      // we need a dummy bsp node to dangle our medium/cell_id info on,
      // and to keep the split plane around permanently
      BspNode *n = BspAllocateDummyNode();
      PortalPolyhedron *out;

      *n = *ph->leaf;
      n->split_plane = split;

      out = SplitPortalPolyhedronByPlane(ph, &n->split_plane, FALSE, TRUE);

#ifdef DBG_ON
      CheckPolyhedron(ph, " (inner split polyhedron)");
      CheckPolyhedron(out, " (outer split polyhedron)");
#endif

      out->leaf = n;
      n->ph = (void *) out;

      // Update the worldrep BSP tree with this new plane
      wrBspTreeAddPostSplit(n, ph->leaf->cell_id - 1);
   }
}

