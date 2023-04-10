/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgmerge.c,v 1.9 2000/02/19 12:55:02 toml Exp $

#include <lg.h>
#include <r3d.h>
#include <wr.h>
#include <portal.h>
#include <csg.h>
#include <csgbrush.h>
#include <bspdata.h>
#include <csgcheck.h>
#include <bsppinfo.h>
#include <csgalloc.h>
#include <mprintf.h>
#include <csgutil.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//  Given two cells, create a new single cell out of both
//  of them.  This isn't necessarily geometrically valid,
//  and is actually used with special code which splits
//  without generating boundary portals, and then merges.

extern int stat_edge_merge, stat_poly_merge;

void PortalMergeCells(PortalPolyhedron *ph, PortalPolyhedron *ph2)
{
   // relink all of the polygons from ph2 onto ph
   PortalPolygon *p, *q, *first;

   p = first = ph2->poly;
   if (!p) return;

   do {
      if (p->ph[0] == ph2) {
         if (p->ph[1] == ph)
            Error(1, "PortalMergeCells: Found boundary polygon.\n");
         p->ph[0] = ph;
         p = p->ph_next[0];
      } else if (p->ph[1] == ph2) {
         if (p->ph[0] == ph)
            Error(1, "PortalMergeCells: Found boundary polygon.\n");
         p->ph[1] = ph;
         p = p->ph_next[1];
      } else
         Error(1, "PortalMergeCells: Poly didn't belong to cell.\n");
//mprintf("a");
   } while (p != first);

   // now link that list onto the first polyhedron
   // since they're circular, this is trivial, yet
   // odd looking

   // ok, we've got this:
   //
   //    ph
   //     |
   //    p1 -> q -> ...ph_polys... -> p1
   //
   //
   //    ph2
   //     |
   //    first -> p -> ..ph2_polys... -> first

   // we'll output:
   //
   //    ph
   //     |
   //    p1 -> p -> ph2_polys -> first -> q -> ph_polys -> p1
   //     ^^^^                      ^^^^^
   //    these are the two links to update
  
   if (!ph->poly) {
      // really easy
      ph->poly = ph2->poly;
      ph2->poly = NULL;
      return;
   }

   first = ph2->poly;

   if (first->ph[0] == ph)  // it was ph2, but we've relinked it!
      p = first->ph_next[0];
   else
      p = first->ph_next[1];

   if (ph->poly->ph[0] == ph) {
      q = ph->poly->ph_next[0];
      ph->poly->ph_next[0] = p;
   } else {
      q = ph->poly->ph_next[1];
      ph->poly->ph_next[1] = p;
   }

   if (first->ph[0] == ph)
      first->ph_next[0] = q;
   else
      first->ph_next[1] = q;

   ph2->poly = NULL;
   PortalPolyhedronFree(ph2);
}

// if a given edge is in dest, then delete it from dest
// and src; otherwise, change it to being in dest
static void PortalUpdateEdge(PortalPolygon *dest, PortalPolygon *src,
                      PortalPolyEdge *edge)
{
   PortalPolyEdge *e = edge, *p;

#if 1
   int m=0;
   e = edge;
   do {
      ++m;
      e = e->edge_next;
   } while (e != edge);
#endif

   p = edge;
   do {
//mprintf("!");
      p = p->edge_next;
   } while (p != edge);

   p = NULL;
   do {
      if (e->poly == dest)
         break;
      p = e;
      e = e->edge_next;
//mprintf("b");
   } while (e != edge);

   // e is now dest's copy of edge
   // p is its predecessor

   if (e->poly != dest) {
      // add this edge to dest. don't have to remove it from
      // src because src is going away

      e->poly_next = dest->edge->poly_next;
      dest->edge->poly_next = e;

      e->poly = dest;

      // edge_next is unchanged because edge still belongs
      // to identical number of polygons
   } else {
      // delete this edge from src and dest.
      // src is going away anyway
      if (!p)
         Error(1, "PortalUpdateEdge: p was NULL");

      // delete dest's copy of edge from shared edge list
      //    currently p->edge_next = e

      p->edge_next = e->edge_next;

      // delete dest's copy of edge from dest's edge list

      if (dest->edge == e) {
         // currently e is first edge in dest, which messes
         // up deletion

         dest->edge = dest->edge->poly_next;
         if (dest->edge == e)
            Error(1, "PortalUpdateEdge: tried to delete last edge in dest!\n");
      }

      p = dest->edge;
      while (p->poly_next != e) {
//mprintf("c");
         p = p->poly_next;
      }
      p->poly_next = e->poly_next;

      // delete src's copy of edge from shared edge list

      p = edge;
      while (p->edge_next != edge) {
//mprintf("d");
         p = p->edge_next;
      }
      p->edge_next = p->edge_next->edge_next;

#if 1
      {
         int n=0;
         e = p;
         do {
            ++n;
            e = e->edge_next;
         } while (e != p);

         // if e exists in db 0 times, the above
         // loop miscounts it as 1
         //zb: what the hell is that??  if (m == 2 & n == 1) n = m-2;
         if ((m == 2) && (n == 1)) n = m-2; //zb <-- that??

         if (n != m-2)
            Error(1, "PortalUpdateEdge: edge got short-changed\n");
         if (n == 1) {
            // this edge has become stranded.  this is ok
            // if the only polyhedron it belongs to is in solid
            // space, so it's not actually needed
            if (e->poly->ph[0] && e->poly->ph[0]->leaf->cell_id != 0)
               Error(1, "Not enough copies of edge after deletion!\n");
            if (e->poly->ph[1] && e->poly->ph[1]->leaf->cell_id != 0)
               Error(1, "Not enough copies of edge after deletion!\n");

//          Warning(("PortalUpdateEdge: stranded edge, maybe ok.\n"));
         }
      }
#endif
   }
}

static void PortalRemovePolygon(PortalPolyhedron *ph, PortalPolygon *poly)
{
   PortalPolygon *p, *next;

   if (!ph) return;

   next = GetPhNext(poly, ph);

   if (ph->poly == poly) {
      // we're going to delete the "head" so advance head one
      // to make it simpler

      ph->poly = next;
   }

   // iterate through the polys in this ph looking for poly

   p = ph->poly;
   while (GetPhNext(p, ph) != poly) {
//mprintf("e");
      p = GetPhNext(p, ph); 
   }

   // now delete this poly

   SetPhNext(p, ph, next);
}

// Given two polygons, with one or more shared edges,
// merge them into one.

void PortalMergePolys(PortalPolygon *dest, PortalPolygon *src)
{
   PortalPolyEdge *edge, *next, *first;

   edge = src->edge;

   first = edge;
   do {
      next = edge->poly_next;
      PortalUpdateEdge(dest, src, edge);
      edge = next;
//mprintf("f");
   } while (edge != first);

   // now delete src from its polyhedra

   PortalRemovePolygon(src->ph[0], src);
   PortalRemovePolygon(src->ph[1], src);
   PortalPolygonFree(src);
   ++stat_poly_merge;
}

static PortalEdge *MakeJoinedEdge(PortalEdge *e1, PortalEdge *e2)
{
   BspVertex *v[4];
   int i,j, a,b;
   PortalEdge *d;

   v[0] = &e1->start;
   v[1] = &e1->end;
   v[2] = &e2->start;
   v[3] = &e2->end;

   // find the two which match
   for (i=1; i < 4; ++i)
      for (j=0; j < i; ++j)
         if (!VERTEX_COMPARE(*v[i], *v[j]))
            goto match;

   Error(1, "MakeJoinedEdge: No shared vertex!\n");

  match:

   // now set a & b to the remaining numbers

   // we have i,j, and we know i > j

   if (j > 0)
      a = 0;
   else  // we know that j is 0, so i is from 1..3
      a = (i == 1) ? 2 : 1;

   b = 6 - i - j - a;    // 0 + 1 + 2 + 3 == 6

   d = PortalEdgeAlloc();

   d->start = *v[a];
   d->end = *v[b];

   return d;
}

// e must not be the first edge in the polygon
void remove_poly_edge(PortalPolygon *p, PortalPolyEdge *e)
{
   PortalPolyEdge *a;
   int k = 0;

   // first remove it from the polygon's edge list

   a = p->edge;
   if (a == e)
      a = p->edge = a->edge_next;

   if (a == e) {
      if (e->edge_next != e)
         Error(1, "remove_poly_edge: input constraint not met.\n");
      p->edge = NULL;
   } else {
      while (a->poly_next != e) {
         if (++k > 2048) Error(1, "remove_poly_edge: edge not in poly list.\n");
         a = a->poly_next;
      }
      // assert a->poly_next == e

      a->poly_next = e->poly_next;
   }

   a = p->edge;
   do {
      if (++k > 2048) Error(1, "remove_poly_edge: poly list screwed.\n");
      if (a == e) Error(1, "remove_poly_edge: somehow failed!\n");
      a = a->poly_next;
   } while (a != p->edge);

   // now remove it from the circular edge list

   a = e;
   while (a->edge_next != e) {
      if (++k > 2048) Error(1, "remove_poly_edge: edge not in edge list.\n");
      a = a->edge_next;
   }

   // assert a->edge_next == e

   if (a == e)
      // if it's the last element of the list, free the PortalEdge
      PortalEdgeFree(e->data);
   else
      a->edge_next = e->edge_next;

   // now, since only one edge_next and one poly_next point
   // to e, we should have gotten rid of all pointers to e.
   // somehow we haven't.  Argh.

   e->edge_next = NULL;
   e->poly_next = NULL;
#if 1
   PortalPolyEdgeFree(e);
#else
   mprintf("Clear edge %p\n", e);
#endif
   
}

static PortalPolygon *find_opaque_sharer(PortalPolygon *p, PortalPolyEdge *e,
                    PortalPolyEdge **f)
{
   PortalPolyEdge *a;
   PortalPolygon *q;
   a = e->edge_next;

   do {
      q = a->poly;

      // check if q is opaque
      if (q->misc && !q->misc->portal) {
         *f = a;
         return q;
      }

      a = a->edge_next;
   } while (a != e);

   return 0;
}

static bool polygon_contains_edge(PortalPolygon *p, PortalEdge *e, PortalPolyEdge **f)
{
   PortalPolyEdge *a;
   a = p->edge;
   do {
      if (a->data == e) {
         *f = a;
         return TRUE;
      }

      a = a->poly_next;
   } while (a != p->edge);
   return FALSE;
}

extern bool debug_merge;

bool try_edge_merge(PortalPolygon *p, PortalPolyhedron *ph,
    PortalPolyEdge *e1, PortalPolyEdge *e2)
{
   // ok, here's the deal so far.

   //  e1,e2 are colinear and adjacent, mergeable candidates
   //  both of them come from poly p

   // now, basically, we can merge them if all the polygons
   // which contain one or the other contain both.

   // in reality, we can do even better.  we really only care
   // about t-joints between two rendered polygons, not portals.

   // so, if the source polygon is opaque, we go find the
   // rendered polygon that shares the first edge.  If that
   // polygon also shares the second edge, then we merge them
   // together for both polygons--leaving them split elsewhere
   // (e.g. if a portal shares one edge).

   // if the source polygon is a portal (of either type), simply
   // collapse the edges for this polygon only

   // check if it's a portal
   if (p->misc->portal) {   // simplest test
      PortalEdge *j;
      PortalPolyEdge *k;

if (debug_merge) {
   mprintf("Edge merge\n");
   mprintf("%g,%g,%g--%g %g %g\n",
       e1->data->start.x, e1->data->start.y, e1->data->start.z,
       e1->data->end.x, e1->data->end.y, e1->data->end.z);
   mprintf("%g,%g,%g--%g %g %g\n",
       e2->data->start.x, e2->data->start.y, e2->data->start.z,
       e2->data->end.x, e2->data->end.y, e2->data->end.z);
}

#if 0
mprintf("Merge edges:\n");
dump_edge(e1);
dump_edge(e2);
mprintf("from poly:\n");
dump_portal_poly(p);
#endif

      j = MakeJoinedEdge(e1->data, e2->data);
      k = PortalPolyEdgeAlloc();
      k->data = j;

      k->poly = p;
      k->edge_next = k;
      k->poly_next = p->edge->poly_next;
      p->edge->poly_next = k;
      p->edge = k;

      remove_poly_edge(p, e1);
      remove_poly_edge(p, e2);

      return TRUE;
   } else {
      // find a polygon that shares this
      PortalPolyEdge *f1,*f2;
      PortalPolygon *q = find_opaque_sharer(p, e1, &f1);
      if (!q)
          Warning(("try_edge_merge: no opaque sharer found.\n"));
      if (q && polygon_contains_edge(q, e2->data, &f2)) {
         // both polygons contain both edges
         PortalEdge *j = MakeJoinedEdge(e1->data, e2->data);
         PortalPolyEdge *e,*f;

if (debug_merge) {
   mprintf("Edge merge\n");
   mprintf("%g,%g,%g--%g %g %g\n",
       e1->data->start.x, e1->data->start.y, e1->data->start.z,
       e1->data->end.x, e1->data->end.y, e1->data->end.z);
   mprintf("%g,%g,%g--%g %g %g\n",
       e2->data->start.x, e2->data->start.y, e2->data->start.z,
       e2->data->end.x, e2->data->end.y, e2->data->end.z);
}

//mprintf("edge_merge %p and %p\n");

         e = PortalPolyEdgeAlloc();
         f = PortalPolyEdgeAlloc();

         e->data = j;
         f->data = j;

         e->edge_next = f;
         f->edge_next = e;

         e->poly = p;
         f->poly = q;

         e->poly_next = p->edge->poly_next;
         p->edge->poly_next = e;
         p->edge = e;

         f->poly_next = q->edge->poly_next;
         q->edge->poly_next = f;
         q->edge = f;

         remove_poly_edge(p, e1);
         remove_poly_edge(p, e2);

         remove_poly_edge(q, f1);
         remove_poly_edge(q, f2);

         return TRUE;
      }
   }

   return FALSE;
}

// build a wound polygon from a portalpolygon

static int compute_winding(PortalPolygon *p, BspVertex *wind)
{
   int n=0,i;
   PortalPolyEdge *e;
   BspVertex *w;

   e = p->edge;
   do {
      ++n;
      e = e->poly_next;
   } while (e != p->edge);

   if (n > 128)
      Error(1, "compute_winding: Winding too large.\n");
   w = wind;

   // initialize first two points
   w[0] = p->edge->data->start;
   w[1] = p->edge->data->end;

   for (i=2; i < n; ++i) {
      e = p->edge;
      for(;;) {
         if (!VERTEX_COMPARE(e->data->start,w[i-1])) {
            if (VERTEX_COMPARE(e->data->end,w[i-2])) {
               w[i] = e->data->end;
               break;
            }
         } else if (!VERTEX_COMPARE(e->data->end,w[i-1])) {
            if (VERTEX_COMPARE(e->data->start,w[i-2])) {
               w[i] = e->data->start;
               break;
            }
         }

         e = e->poly_next;
         if (e == p->edge)
            Error(1, "compute_winding: unmatched vertex.\n");
//mprintf("g");
      }
   }

   return n;
}

// force polygon winding to match plane's normal
// (this doesn't necessarily match up with the polyhedron
// we're merging with, but hey, so what, just so long as
// all of the ones in a given plane match that plane)
static void maybe_reverse_winding(int n, BspVertex *w, PortalPolygon *p)
{
   mxs_vector norm;
   int h,i;
   norm.x = norm.y = norm.z = 0;
   h = n-1;
   for (i=0; i < n; ++i) {
      norm.x +=  (w[h].y - w[i].y)
                *(w[h].z + w[i].z);
      norm.y +=  (w[h].z - w[i].z)
                *(w[h].x + w[i].x);
      norm.z +=  (w[h].x - w[i].x)
                *(w[h].y + w[i].y);
      h = i;
   }

   // compare norm against plane in PortalPolygon

   if (norm.x * p->plane->a + norm.y * p->plane->b + norm.z * p->plane->c > 0)
      return;

   for (i=0; i < n/2; ++i) {
      BspVertex temp = w[i];
      w[i] = w[n-1-i];
      w[n-1-i] = temp;
   }
}

#define CONTINUOUS_EPSILON 0.001

   // now we duplicate a chunk of code out of the merge stuff from opt bsp
static bool try_merge(int n1, BspVertex *w1, int n2, BspVertex *w2, BspPlane *p)
{
   int i,j, e,f;
   BspVertex *p1, *p2;
   mxs_vector norm, edge, rel_norm;
   mxs_real res;

   // now go look for a shared edge

   for (i=0; i < n1; ++i) {
      j = (i+1)%n1;

      p1 = &w1[i];
      p2 = &w1[j];

      for (e=0; e < n2; ++e) {
         f = (e+1)%n2;
         if (!VERTEX_COMPARE(*p1, w2[f]) && !VERTEX_COMPARE(*p2, w2[e]))
            goto found_match;
      }
   }
   return 0;

  found_match:

   //++num_merged;

   // now see if we can extend the match (which means
   // we have some colinear points to delete)
   //   currently we've matched
   //        i == f
   //        j == e
   // So, we could have points before i, if i is 0.
   // And we could have points after j.

   if (i == 0) {
      while (!VERTEX_COMPARE(w1[(i+n1-1)%n1], w2[(f+1)%n2]))
         i = (i+n1-1)%n1, f = (f+1)%n2;
   }

   while (!VERTEX_COMPARE(w1[(j+1)%n1], w2[(e+n2-1)%n2]))
      j = (j+1)%n1, e = (e+n2-1)%n2;

   // ok, now we've extended it...

   // now we need to check if this is really valid.
   // basically,  I.e. whether the result of the
   // merge is actually convex

   // what we do is test the adjacent edges between
   // polys to see if their cross product is the same
   // as that for the poly itself

   // oddly, this doesn't seem to work

#ifdef SHOW_MERGE
   {
      int i;
      mprintf("Matching winding:\n");
      for (i=0; i < n1; ++i)
         mprintf("%lg,%lg,%lg; ", w1[i].x, w1[i].y, w1[i].z);
      mprintf("\n");
      for (i=0; i < n2; ++i)
         mprintf("%lg,%lg,%lg; ", w2[i].x, w2[i].y, w2[i].z);
      mprintf("\n");
   }
#endif

     // compute the polygon normal
     //  by construction it's the p passed in
   norm.x = p->a;
   norm.y = p->b;
   norm.z = p->c;

     // compute the outward normal to the back edge
     // can't use mx_sub_vec since BspVertex's are doubles
   edge.x = w1[i].x - w1[(i+n1-1)%n1].x;
   edge.y = w1[i].y - w1[(i+n1-1)%n1].y;
   edge.z = w1[i].z - w1[(i+n1-1)%n1].z;
   mx_cross_vec(&rel_norm, &norm, &edge);
   mx_normeq_vec(&rel_norm);
#ifdef SHOW_MERGE
mprintf("norm %lg,%lg,%lg\n", norm.x, norm.y, norm.z);
mprintf("edge %lg,%lg,%lg\n", edge.x, edge.y, edge.z);
mprintf("rel_norm %lg,%lg,%lg\n", rel_norm.x, rel_norm.y, rel_norm.z);
#endif

     // compute the extending edge on the other poly
   edge.x = w2[(f+1)%n2].x - w2[f].x;
   edge.y = w2[(f+1)%n2].y - w2[f].y;
   edge.z = w2[(f+1)%n2].z - w2[f].z;
#ifdef SHOW_MERGE
mprintf("edge2 %lg,%lg,%lg\n", edge.x, edge.y, edge.z);
#endif
   mx_normeq_vec(&edge);
   res = mx_dot_vec(&edge, &rel_norm);
#ifdef SHOW_MERGE
mprintf("dot: %lg\n", res);
#endif

   if (res < -CONTINUOUS_EPSILON)
      return FALSE;

     // compute the outward norml to the forward edge on other poly
   edge.x = w2[e].x - w2[(e+n2-1)%n2].x;
   edge.y = w2[e].y - w2[(e+n2-1)%n2].y;
   edge.z = w2[e].z - w2[(e+n2-1)%n2].z;
   mx_cross_vec(&rel_norm, &norm, &edge);
   mx_normeq_vec(&rel_norm);
#ifdef SHOW_MERGE
mprintf("norm %lg,%lg,%lg\n", norm.x, norm.y, norm.z);
mprintf("edge %lg,%lg,%lg\n", edge.x, edge.y, edge.z);
mprintf("rel_norm %lg,%lg,%lg\n", rel_norm.x, rel_norm.y, rel_norm.z);
#endif

     // compute the extending edge on this poly
   edge.x = w1[(j+1)%n1].x - w1[j].x;
   edge.y = w1[(j+1)%n1].y - w1[j].y;
   edge.z = w1[(j+1)%n1].z - w1[j].z;
#ifdef SHOW_MERGE
mprintf("edge2 %lg,%lg,%lg\n", edge.x, edge.y, edge.z);
#endif
   mx_normeq_vec(&edge);
   res = mx_dot_vec(&edge, &rel_norm);
#ifdef SHOW_MERGE
mprintf("dot: %lg\n", res);
#endif

   if (res < -CONTINUOUS_EPSILON)
      return FALSE;

   return TRUE;
}

static BspVertex windbuf1[128], windbuf2[128];

bool combination_is_convex(PortalPolygon *dest, PortalPolygon *src)
{
   // we don't have these computed with proper windings (we're
   // so stupid, how hard would it have been?)

   // so first we need to generate wound versions, then compare
   // them.  Or, we could merge them while winding them.  Yuck.

   int n1,n2;
   bool res;

   n1 = compute_winding(dest, windbuf1);
   n2 = compute_winding(src, windbuf2);

   // make sure both polys face the same way
   maybe_reverse_winding(n1, windbuf1, dest);
   maybe_reverse_winding(n2, windbuf2, dest);

   res = try_merge(n1, windbuf1, n2, windbuf2, dest->plane);

   return res;
}

static void merge_poly(PortalPolygon *a, PortalPolygon *b)
{
//   mprintf("merged polygons\n");
   Free(b->misc);
   b->misc = NULL;
   PortalMergePolys(a,b);
}

static int compare_poly_info(PortalPolygon *x, PortalPolygon *y)
{
   pinfo *a = (pinfo *) x->misc, *b = (pinfo *) y->misc;

   if (x->plane != y->plane)
      return FALSE;

   if (a->portal || b->portal) {
#if 1
      if (a->portal && b->portal)
         if ((a->portal == b->portal && a->portal_2 == b->portal_2) ||
             (a->portal == b->portal_2 && a->portal_2 == b->portal))
            return 1;
#endif
      return 0;
   } else if (a->brface == b->brface)
      return 1;

   {
     int br1,br2, f1,f2;
     br1 = a->brface >> 8;
     br2 = b->brface >> 8;

     f1 = a->brface & 255;
     f2 = b->brface & 255;

     if (CB_FACE_TEXTURE(br1,f1) != CB_FACE_TEXTURE(br2,f2)) return FALSE;
     if (CB_FACE_TEX_ROT(br1,f1) != CB_FACE_TEX_ROT(br2,f2)) return FALSE;
     if (CB_FACE_TEX_SCALE(br1,f1) != CB_FACE_TEX_SCALE(br2,f2)) return FALSE;
     if (CB_FACE_TEX_ALIGN_U(br1,f1) != CB_FACE_TEX_ALIGN_U(br2,f2)) return 0;
     if (CB_FACE_TEX_ALIGN_V(br1,f1) != CB_FACE_TEX_ALIGN_V(br2,f2)) return 0;

     return TRUE;
   }
}

static bool try_merge_poly(PortalPolyhedron *ph, PortalPolygon *poly)
{
   PortalPolyEdge *edge, *poly_edge;
   edge = poly->edge;

   // look for polygons which share an edge with this poly

   // iterate through this poly's edges
   do {
//mprintf("h");

      // iterate through other polys that share the edge
      poly_edge = edge->edge_next;
      while (poly_edge != edge) {
//mprintf("i");
         if ((poly_edge->poly->ph[0] == ph || poly_edge->poly->ph[1] == ph)
              && compare_poly_info(poly, poly_edge->poly))
            if (combination_is_convex(poly, poly_edge->poly)) {
               merge_poly(poly, poly_edge->poly);
#ifdef DBG_ON
               CheckPolyhedron(ph, "(post-merge)");
#endif
               return TRUE;
            }
         poly_edge = poly_edge->edge_next;
      }
      edge = edge->poly_next;
   } while (edge != poly->edge);

   return FALSE;
}

#define COLINEAR_EPSILON 0.0001
#define COLINEAR_MIN     0.001

static bool colinear(BspVertex *e1, BspVertex *e2, BspVertex *f1, BspVertex *f2)
{
   mxs_vector a,b;
   // check if f lies on a line with e
   // form two lines, normalize, dot product
   // and check the resultant length.

   // e1 matches f1, so build e1->e2, f2->f1
   a.x = e2->x - e1->x;
   a.y = e2->y - e1->y;
   a.z = e2->z - e1->z;

   b.x = f1->x - f2->x;
   b.y = f1->y - f2->y;
   b.z = f1->z - f2->z;

   // if vectors are too short to be safe, abort!

#if 0
   if (mx_normeq_vec(&a) < COLINEAR_MIN)
      return FALSE;
   if (mx_normeq_vec(&b) < COLINEAR_MIN)
      return FALSE;
   mx_cross_vec(&c, &a, &b);
#endif
   mx_normeq_vec(&a);
   mx_normeq_vec(&b);

   if (mx_dot_vec(&a, &b) >= 1-COLINEAR_EPSILON)
      return TRUE;
   return FALSE;
}

#define ST  data->start
#define EN  data->end
  
static bool edge_colinear(PortalPolyEdge *e1, PortalPolyEdge *e2)
{
   if (!VERTEX_COMPARE(e1->ST, e2->ST))
      return colinear(&e1->ST, &e1->EN, &e2->ST, &e2->EN);
   if (!VERTEX_COMPARE(e1->ST, e2->EN))
      return colinear(&e1->ST, &e1->EN, &e2->EN, &e2->ST);
   if (!VERTEX_COMPARE(e1->EN, e2->ST))
      return colinear(&e1->EN, &e1->ST, &e2->ST, &e2->EN);
   if (!VERTEX_COMPARE(e1->EN, e2->EN))
      return colinear(&e1->EN, &e1->ST, &e2->EN, &e2->ST);
   return FALSE;
}

static void try_merge_edges(PortalPolyhedron *ph, PortalPolygon *poly,
                                          bool retry)
{
   // look for colinear edges
   PortalPolyEdge *e1, *e2;
   bool matched;
   int n;

   do {
      matched = FALSE;
      n = 0;
      e1 = poly->edge;
      do {
         e2 = poly->edge;
         while (e2 != e1) {
            if (edge_colinear(e1, e2)) {
               // count number of edges in poly
               // if only 3 edges, don't merge
               if (poly->edge->poly_next->poly_next->poly_next != poly->edge) {
                  // we could record polygon in case something goes wrong
                  if (!debug_merge && try_edge_merge(poly, ph, e1, e2)) {
                     ++stat_edge_merge;
                     matched = TRUE;
                     break;
                  }
               }
            }
            e2 = e2->poly_next;
         }
         e1 = e1->poly_next;
         ++n;
      } while (!matched && e1 != poly->edge);

#ifdef DBG_ON
      if (matched) {
         CheckPolyhedron(ph, "(after edge merge)");
      }
#endif
   } while (matched);

   if (retry && n > 32) {
      double x=0,y=0,z=0;
      int n=0;
      e1 = poly->edge;
      do {
         x += e1->data->start.x + e1->data->end.x;
         y += e1->data->start.y + e1->data->end.y;
         z += e1->data->start.z + e1->data->end.z;

         e1 = e1->poly_next;
         n+=2;
      } while (e1 != poly->edge);
      x /= n; y /= n; z /= n;
      mprintf("More than 32 edges after edge-merge!\n");
      mprintf("On polygon at location: %lg, %lg, %lg\n", x, y, z);
      // do it again so we can watch in debugger
      try_merge_edges(ph,poly, FALSE);
   }
}

static bool core_merge(PortalPolyhedron *ph)
{
   PortalPolygon *poly;

   poly = ph->poly;
   do {
//mprintf("j");
      if (try_merge_poly(ph, poly))
         return TRUE;

      if (poly->ph[0] == ph)
         poly = poly->ph_next[0];
      else
         poly = poly->ph_next[1];
   } while (poly != ph->poly);
   return FALSE;
}

void emit_merge_cell_polys(PortalPolyhedron *ph)
{
   // repeat until we can't merge anymore

   while (core_merge(ph)) {
//mprintf("l");
   }

#ifdef DBG_ON
   CheckPolyhedron(ph, "(after poly merge)");
#endif
}

void emit_merge_cell_edges(PortalPolyhedron *ph)
{
   PortalPolygon *poly;

#ifdef DBG_ON
   CheckPolyhedron(ph, "(before any edge merge)");
#endif

   poly = ph->poly;
   do {
      try_merge_edges(ph, poly, TRUE);

      if (poly->ph[0] == ph)
         poly = poly->ph_next[0];
      else
         poly = poly->ph_next[1];
   } while (poly != ph->poly);
}
