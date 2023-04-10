/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgutil.c,v 1.5 2000/02/19 12:55:03 toml Exp $

#include <lg.h>

#include <csg.h>
#include <bspdata.h>
#include <csgutil.h>
#include <csgalloc.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

void ChangePh(PortalPolygon *poly, PortalPolyhedron *ph, PortalPolyhedron *dest)
{
   if (poly->ph[0] == ph)
      poly->ph[0] = dest;
   else if (poly->ph[1] == ph)
      poly->ph[1] = dest;
   else
      Error(1, "ChangePh: Tried to change invalid polyhedron.\n");
}

PortalPolygon *GetPhNext(PortalPolygon *poly, PortalPolyhedron *ph)
{
   if (poly->ph[0] == ph)
      return poly->ph_next[0];
   if (poly->ph[1] == ph)
      return poly->ph_next[1];
   Error(1, "GetPhNext: Tried to get next field with wrong polyhedron.\n");
   return 0;
}

void SetPhNext(PortalPolygon *poly, PortalPolyhedron *ph, PortalPolygon *next)
{
   if (poly->ph[0] == ph)
      poly->ph_next[0] = next;
   else if (poly->ph[1] == ph)
      poly->ph_next[1] = next;
   else
      Error(1, "SetPhNext: Tried to set next field with wrong polyhedron.\n");
}

PortalPolygon *AddPolyToList(PortalPolygon *list, PortalPolygon *poly, PortalPolyhedron *ph)
{
   if (list) {
      SetPhNext(poly, ph, GetPhNext(list, ph));
      SetPhNext(list, ph, poly);
   } else {
      SetPhNext(poly, ph, poly);
      list = poly;
   }
   return list;
}

PortalPolyEdge *AddEdgeToList(PortalPolyEdge *list, PortalPolyEdge *edge)
{
   if (list) {
      edge->poly_next = list->poly_next;    // put edge after first element of list
      list->poly_next = edge;
   } else {
      edge->poly_next = edge;    // initialize circular linkage
      list = edge;
   }
   return list;
}

void *PortalMakeEdge(BspVertex *a, BspVertex *b)
{
   PortalEdge *e;
   e = PortalEdgeAlloc();
   e->start = *a;
   e->end = *b;
   return e;
}

void *PortalMakePolyhedron(void)
{
   PortalPolyhedron *ph;
   ph = PortalPolyhedronAlloc();
   ph->poly = 0;
   ph->leaf = 0;
   return ph;
}

void *PortalMakePolygon(void *ph_)
{
   PortalPolygon *p;
   PortalPolyhedron *ph = ph_;

   p = PortalPolygonAlloc();
   p->ph[1] = 0;
   p->ph_next[1] = 0;
   p->misc = 0;

   p->ph[0] = ph;

   if (ph->poly) {
      // we have to insert it _last_ to maintain the polygon/plane ordering!
      PortalPolygon *q;
      q = ph->poly;
      while (q->ph_next[0] != ph->poly)
         q = q->ph_next[0];

      p->ph_next[0] = ph->poly;
      q->ph_next[0] = p;
   } else
      ph->poly = AddPolyToList(ph->poly, p, ph);

   p->edge = 0;

   return p;
}

void *PortalAddPolygonEdge(void *poly_, void *edge_, void *pe2_)
{
   PortalPolygon *poly = poly_;
   PortalEdge *edge = edge_;
   PortalPolyEdge *pe2 = pe2_;
   PortalPolyEdge *pe;

   pe = PortalPolyEdgeAlloc();
   pe->data = edge;
   pe->poly = poly;

   // insert into the circular list for this poly
   poly->edge = AddEdgeToList(poly->edge, pe);

   // insert ourselves into the circular list for this edge
   if (pe2) {
      pe->edge_next = pe2->edge_next;
      pe2->edge_next = pe;
   } else
      pe->edge_next = pe;

   return pe;
}

PortalPolygon *PortalPolyCopy(PortalPolygon *source)
{
   PortalPolygon *out;
   PortalPolyEdge *edge, *list = NULL, *pe;
   PortalEdge *e;

   out = PortalPolygonAlloc();
   out->ph[0] = out->ph[1] = 0;
   out->ph_next[0] = out->ph_next[1] = 0;
   out->misc = 0;
   out->plane = source->plane;
   out->brface = source->brface;

   edge = source->edge;

   do {
      pe = PortalPolyEdgeAlloc();
      e = PortalEdgeAlloc();
      *e = *edge->data;
      pe->data = e;
      pe->poly_next = 0;
      pe->edge_next = pe;
      pe->poly = out;
      
      list = AddEdgeToList(list, pe);
      edge = edge->poly_next;
   } while (edge != source->edge);

   out->edge = list;
   return out;
}

