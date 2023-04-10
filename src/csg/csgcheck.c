/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgcheck.c,v 1.4 2000/02/19 12:26:43 toml Exp $

#include <lg.h>
#include <mprintf.h>
#include <bspdata.h>
#include <csgcheck.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern void cross_prod(BspVertex *cp, BspVertex *p1, BspVertex *p2, BspVertex *p3);

void dump_edge(PortalPolyEdge *edge)
{
   PortalEdge *a = edge->data;
   mprintf("(%g,%g,%g)-(%g,%g,%g)\n", a->start.x, a->start.y, a->start.z, a->end.x, a->end.y, a->end.z);
}

void dump_portal_poly(PortalPolygon *poly)
{
   PortalPolyEdge *edge, *first;

   edge = first = poly->edge;

   do {
      dump_edge(edge);
      edge = edge->poly_next;
   } while (edge != first);
}

void dump_portal_polyhedron(PortalPolyhedron *ph)
{
   PortalPolygon *first, *poly;

   first = poly = ph->poly;

   do {
      mprintf("  poly\n");
      dump_portal_poly(poly);
      if (poly->ph[0] == ph)
         poly = poly->ph_next[0];
      else
         poly = poly->ph_next[1];
   } while (poly != first);
}


//  data structure invariants:
//
//      PortalPolyEdge *edge;
//      PortalPolygon *poly;
//      PortalPolyhedron *ph;
//
//  Let the symbol   ->EDGE_NEXT  stand for
//  zero or more  ->edge_next   concatenated,
//  and the symbol   ->POLY_NEXT  stands for
//  zero or more  ->poly_next   concatenated
//
//  Polyhedron constraints:
//    ph->poly->ph[0] == ph  ||  ph->poly->ph[1] == ph
//
//  Polygon constraints:
//    poly->edge->POLY_NEXT->poly == poly
//    poly->edge->POLY_NEXT->start lies in the plane poly->plane
//       (as does ...->end)
//
//  Edge constraints:
//    edge->EDGE_NEXT = edge

bool post_edge_merge;

void CheckEdge(PortalPolyEdge *edge)
{
   PortalPolyEdge *x = edge;
   int n=0;

   do {
      if (x == NULL)
         Error(1, "CheckEdge: Found null in edge_next list\n");
      if (x->data != edge->data)
         Error(1, "CheckEdge: Data field not correctly linked\n");

      x = x->edge_next;
      ++n;
   } while (x != edge);

   if (!post_edge_merge && n < 2)
      Error(1, "CheckEdge: Edge was only shared once.\n");
}

// make sure every vertex exists exactly twice
void CheckVertices(PortalPolygon *poly, PortalPolyEdge *first,
                              BspVertex *z, char *str)
{
   int n=0;
   PortalPolyEdge *x = first;

   do {
      n += ! VERTEX_COMPARE(x->data->start, *z);
      n += ! VERTEX_COMPARE(x->data->end,   *z);
      x = x->poly_next;
   } while (x != first);

   if (n != 2) {
      dump_portal_poly(poly);
      Error(1, "CheckVertices%s: There were %d of one vertex, not 2.\n", str, n);
   }
}

static void bad_vertex(BspVertex *a, BspPlane *p, char *str)
{
   double dist = a->x*p->a + a->y*p->b + a->z*p->c + p->d;
   Error(1, "CheckPolygonPlane%s: Vertex not on plane (missed by %g).\n",
             str, dist);
}

extern int VertexCode(BspVertex *v, BspPlane *p);
void CheckPolygonPlane(PortalPolygon *poly, char *str)
{
   PortalPolyEdge *first, *x;

   if (poly->plane == NULL)
      return;

   x = first = poly->edge;

   do {
      if (VertexCode(&x->data->start, poly->plane) != COPLANAR)
         bad_vertex(&x->data->start, poly->plane, str);
      if (VertexCode(&x->data->end  , poly->plane) != COPLANAR)
         bad_vertex(&x->data->start, poly->plane, str);
      x = x->poly_next;
   } while (x != first);
}

void CheckPoly(PortalPolygon *poly, char *str)
{
   PortalPolyEdge *x, *first;
   int n=0;

   if (poly == NULL)
      Error(1, "CheckPoly: NULL polygon\n");

   x = first = poly->edge;

   if (x == NULL)
      return;

   do {
      CheckEdge(x);
      if (x->poly != poly)
         Error(1, "CheckPoly%s: Polygon edge not back-linked\n", str);
      x = x->poly_next;
      ++n;
   } while (x != first);

   if (n < 3) {
#ifdef DUMP
      mprintf("Poly:\n");
      dump_portal_poly(poly);
#endif
      Error(1, "CheckPoly%s: Polygon only contained %d edges.\n", str, n);
   }

   x = first;
   do {
      CheckVertices(poly, first, &x->data->start, str);
      CheckVertices(poly, first, &x->data->end, str);
      x = x->poly_next;
   } while (x != first);

   CheckPolygonPlane(poly, str);

      // check for all colinear
   x = first->poly_next;
   while (x != first) {
      BspVertex temp;
      cross_prod(&temp, &first->data->start, &first->data->end, &x->data->start);
      if (temp.x*temp.x || temp.y*temp.y || temp.z*temp.z) return;
      cross_prod(&temp, &first->data->start, &first->data->end, &x->data->end);
      if (temp.x*temp.x || temp.y*temp.y || temp.z*temp.z) return;
      x = x->poly_next;
   }
   Error(1, "CheckPoly%s: Polygon entirely colinear.\n", str);
}

void CheckPoly2(PortalPolygon *poly, char *str)
{
   PortalPolyEdge *x, *first;
   int n=0;

   if (poly == NULL)
      Error(1, "CheckPoly: NULL polygon\n");

   x = first = poly->edge;

   if (x == NULL)
      Error(1, "CheckPoly%s: Polygon is empty\n", str);

   do {
      if (x->poly != poly)
         Error(1, "CheckPoly%s: Polygon edge not back-linked\n", str);
      x = x->poly_next;
      ++n;
   } while (x != first);

   if (n < 3)
      Error(1, "CheckPoly%s: Polygon only contained %d edges.\n", str, n);

   x = first;
   do {
      CheckVertices(poly, first, &x->data->start, str);
      CheckVertices(poly, first, &x->data->end, str);
      x = x->poly_next;
   } while (x != first);

   CheckPolygonPlane(poly, str);

      // check for all colinear
   x = first->poly_next;
   while (x != first) {
      BspVertex temp;
      cross_prod(&temp, &first->data->start, &first->data->end, &x->data->start);
      if (temp.x*temp.x || temp.y*temp.y || temp.z*temp.z) return;
      cross_prod(&temp, &first->data->start, &first->data->end, &x->data->end);
      if (temp.x*temp.x || temp.y*temp.y || temp.z*temp.z) return;
      x = x->poly_next;
   }
   Error(1, "CheckPoly2%s: Polygon entirely colinear.\n", str);
}

// check that a given polygon is actually somewhere in the polyhedron
void CheckPolyInPolyhedron(PortalPolygon *poly, PortalPolyhedron *ph, char *str)
{
   PortalPolygon *first, *x;

   first = x = ph->poly;
   do {
      if (x == poly) return;
      if (x->ph[0] == ph) x = x->ph_next[0];
      else if (x->ph[1] == ph) x = x->ph_next[1];
      else Error(1, "CheckPolyInPolyhedron%s: Polygon list not correctly linked\n", str);
   } while(x != first);
   Error(1, "CheckPolyInPolyhedron%s: Polygon linked to polyhedron but not in poly list\n", str);
}

// check for every edge in a polygon that it belongs to the given polyhedron twice
void CheckPolyEdgeCount(PortalPolygon *poly, PortalPolyhedron *ph, char *str)
{
   PortalPolyEdge *x, *y, *first;
   int n;

   x = first = poly->edge;
   do {

      // run through all shared poly edges, count # that belong
      // to poly belonging to ph

      y = x;
      n = 0;

      do {
         if (y->poly->ph[0] == ph || y->poly->ph[1] == ph) {
            CheckPolyInPolyhedron(y->poly, ph, str);
            ++n;
         }
         y = y->edge_next;
      } while (y != x);

      if (!post_edge_merge && n != 2)
         Error(1, "CheckPolyEdgeCount%s: Edge belonged to polyhedron %d times, not 2.\n", str, n);

      x = x->poly_next;

   } while (x != first);
}

void CheckPolyhedron(PortalPolyhedron *ph, char *str)
{
   int n;
   PortalPolygon *x, *first;
   x = first = ph->poly;

   if (x == NULL)
      Error(1, "CheckPolyhedron%s: Polyhedron is empty\n", str);

   n = 0;
   do {
      CheckPoly(x, str);
      if (x->ph[0] == ph)
         x = x->ph_next[0];
      else if (x->ph[1] == ph)
         x = x->ph_next[1];
      else
         Error(1, "CheckPolyhedron%s: Polygon wasn't back-linked to this polyhedron\n", str);
      ++n;
   } while (x != first);
   if (n < 4)
      Error(1, "CheckPolyhedron%s: Polyhedron has %d polygons, needs at least 4\n", str, n);

   x = first;
   do {
      CheckPolyEdgeCount(x, ph, str);
      if (x->ph[0] == ph)
         x = x->ph_next[0];
      else
         x = x->ph_next[1];
   } while (x != first);
}

void CheckPolyhedronQuick(PortalPolyhedron *ph, char *str)
{
   PortalPolygon *x, *first;

   x = first = ph->poly;

   if (x == NULL)
      return;

   do {
      CheckPoly(x, str);
      if (x->ph[0] == ph)
         x = x->ph_next[0];
      else if (x->ph[1] == ph)
         x = x->ph_next[1];
      else
         Error(1, "CheckPolyhedronQuick%s: Polygon wasn't back-linked to this polyhedron\n", str);
   } while (x != first);
}

