/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgbuild.c,v 1.8 2000/02/19 12:26:41 toml Exp $
//
// build the BSP tree by inserting brushes and filtering them down the tree

#include <math.h>

#include <lg.h>
#include <csg.h>
#include <bspdata.h>
#include <csgcheck.h>
#include <media.h>
#include <csgutil.h>
#include <csgbrush.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern PortalPolyhedron *SplitPortalPolyhedronByPlane(PortalPolyhedron *s, BspPlane *clip, bool merge, bool set_plane);

extern void add_area(BspNode *b);
extern void *tree, *active;

extern void find_matched_normal(BspPlane *p);
extern void find_matched_plane (BspPlane *p);

extern void  preorder_update_active_brushes(BspNode *b);
extern void postorder_update_active_brushes(BspNode *b);

#define preorder      preorder_update_active_brushes
#define postorder    postorder_update_active_brushes

extern BOOL eq_planes(BspPlane *p, BspPlane *q);

static void SplitNodeByPlane(BspNode *b, BspPlane *p)
{
   if (b->parent) {
#if 0
      if (eq_planes(&b->parent->split_plane, p))
         Error(1, "SplitNodeByPlane: Built invalid BSP tree\n");
#endif
   }

   BspChangeLeafToNode(b, p);

   b->inside = BspAllocateLeaf();
   b->outside = BspAllocateLeaf();
   b->inside->medium = b->medium;
   b->outside->medium = b->medium;
   b->inside->parent = b;
   b->outside->parent = b;
   b->medium = NO_MEDIUM;
}

static int isPortalFullyClipped(PortalPolyhedron *z)
{
   PortalPolygon *p = z->poly;

   do {
      if (p->plane)
         return FALSE;
      p = GetPhNext(p, z);
   } while (p != z->poly);

   return TRUE;
}

static int PortalPolygonPlaneCompareClear(PortalPolygon *p, BspPlane *plane)
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
   // if we're coplanar, 0 out plane to indicate that that
   // plane was coplanar during the traversal
   if (code == COPLANAR)
      p->plane = 0;
   return code;
}

static int PortalPolyhedronPlaneCompare(PortalPolyhedron *z, BspPlane *plane)
{
   int code;
   PortalPolygon *p = z->poly;
   code = 0;
   do {
      code |= PortalPolygonPlaneCompareClear(p, plane);
      p = GetPhNext(p, z);
   } while (p != z->poly);
   return code;
}

void InsertPortalPolyhedronInNode(BspNode *b, PortalPolyhedron *z)
{
   // test which side of the plane this node is on
   int side;

   preorder(b);

   // check if the polyhedron is fully clipped
   if (isPortalFullyClipped(z)) {
      add_area(b);
      postorder(b);
      return;
   }

   // rebuild this polyhedron with its own copies of the polygons
   //z = PortalPolyhedronCopy(z);

   if (!IS_LEAF(b)) {
      side = PortalPolyhedronPlaneCompare(z, &b->split_plane);
      switch (side) {
         case IN_FRONT: InsertPortalPolyhedronInNode(b->inside, z); break;
         case BEHIND  : InsertPortalPolyhedronInNode(b->outside, z); break;
         case COPLANAR: printf("Warning: planar portal polyhedron found.\n"); break;
         default: { // crosses the plane
            PortalPolyhedron *s2;
            s2 = SplitPortalPolyhedronByPlane(z, &b->split_plane, FALSE, FALSE);
            InsertPortalPolyhedronInNode(b->inside, z);
            InsertPortalPolyhedronInNode(b->outside, s2);
            break;
         }
      }
   } else {
      // leaf node
      //   insert the plane for each of the faces that isn't clipped
      PortalPolygon *p = z->poly;
      BspNode *t = b;
      BspPlane *prev = 0;

      do {
         if (p->plane) { // if (!seen(p->plane, b))
            SplitNodeByPlane(t, p->plane);
            t = t->inside;
         }
         p = GetPhNext(p, z);
      } while (p != z->poly);

      add_area(t);
      if (t != b) postorder(t);
   }
   postorder(b);
}

BspPlane hold[128];

void cross_prod(BspVertex *cp, BspVertex *p1, BspVertex *p2, BspVertex *p3)
{
   BspVertex t1,t2;

   t1.x = p2->x - p1->x;
   t1.y = p2->y - p1->y;
   t1.z = p2->z - p1->z;

   t2.x = p3->x - p2->x;
   t2.y = p3->y - p2->y;
   t2.z = p3->z - p2->z;

   cp->x = t1.y * t2.z - t1.z * t2.y;
   cp->y = t1.z * t2.x - t1.x * t2.z;
   cp->z = t1.x * t2.y - t1.y * t2.x;
}

static void normalize(BspVertex *v)
{
   Real len = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
   v->x /= len;
   v->y /= len;
   v->z /= len;
}

extern int VertexCode(BspVertex *v, BspPlane *p);
static int check_plane(PortalPolygon *poly, BspPlane *plane)
{
   PortalPolyEdge *first, *x;

   x = first = poly->edge;

   do {
      if (VertexCode(&x->data->start, plane) != COPLANAR)
         return 1;
      if (VertexCode(&x->data->end  , plane) != COPLANAR)
         return 1;
      x = x->poly_next;
   } while (x != first);

   return 0;
}

void write_bad_brush_pair(int a, int b)
{
   FILE *f = fopen("merge000.log","a");
   fprintf(f, "brush pair %d %d\n", a, b);
   fclose(f);
}

#define PLANE_ZERO   0.001

extern int cur_brush;

static void compute_plane(PortalPolygon *p, BspPlane *plane)
{
   // find three different points
   BspVertex *a,*b,*c;
   BspPlane initial;
   double dist;

   a = &p->edge->data->start;
   b = &p->edge->data->end;
   c = &p->edge->poly_next->data->start;
   if (!VERTEX_COMPARE(*a,*c)) {
      // b->a..c->d
      a = b;
      b = c;
      c = &p->edge->poly_next->data->end;
   } else if (!VERTEX_COMPARE(*b,*c)) {
      // a->b..c->d
      c = &p->edge->poly_next->data->end;
   } else if (!VERTEX_COMPARE(*a, p->edge->poly_next->data->end)) {
      // b->a..d->c
      a = b;
      b = &p->edge->data->start;
   } else if (!VERTEX_COMPARE(*b, p->edge->poly_next->data->end)) {
      // a->b..d->c
      // do nothing
   } else {
      Error(1, "csg@compute_plane: input brush polygon incorrectly wound.\n");
   }
   cross_prod((BspVertex *) plane, c, b, a);
   normalize((BspVertex *) plane);

   initial = *plane;
   // fix up the normal first, so it's good when we compute the constant
#if 1
   find_matched_normal(plane);
#else
   if  (fabs(plane->a) < PLANE_ZERO) plane->a = 0;
   else if (fabs(plane->a) > VEC_DOT_ONE) plane->a = (plane->a < 0) ? -1 : 1;
   if  (fabs(plane->b) < PLANE_ZERO) plane->b = 0;
   else if (fabs(plane->b) > VEC_DOT_ONE) plane->b = (plane->b < 0) ? -1 : 1;
   if  (fabs(plane->c) < PLANE_ZERO) plane->c = 0;
   else if (fabs(plane->c) > VEC_DOT_ONE) plane->c = (plane->c < 0) ? -1 : 1;
#endif
   plane->d = -(a->x * plane->a + a->y * plane->b + a->z * plane->c);
   dist = plane->d;

//mprintf("Plane: %g %g %g %g\n", plane->a, plane->b, plane->c, plane->d);
   find_matched_plane(plane);

   if (check_plane(p, plane)) {
      extern int plane_brush(BspPlane *);
      extern int normal_brush(BspPlane *);
      int z = plane_brush(plane);
      if (z == cur_brush)
         z = normal_brush(plane);
      write_bad_brush_pair(cur_brush, z);

#ifdef WARN_ON
      dump_plane(plane);
#endif
      plane->d = dist;   // restore plane constant
      if (check_plane(p, plane)) {
         *plane = initial;
         plane->d = -(a->x * plane->a + a->y * plane->b + a->z * plane->c);
#ifdef WARN_ON
         dump_plane(plane);
#endif
         Warning(("Very un-merged above planes\n"));
      } else {
#ifdef WARN_ON
         dump_plane(plane);
#endif
         Warning(("Un-merged above planes\n"));
      }
   }
   p->plane = plane;
         
   CheckPolygonPlane(p, " (initial brush)");
}

static void compute_planes(PortalPolyhedron *z)
{
   int n=0;
   PortalPolygon *p = z->poly;
   do {
      compute_plane(p, &hold[n++]);
      p = GetPhNext(p, z);
   } while (p != z->poly);
}

static void PutVolumeInTree(void *tree, void *z)
{
   active = 0;
   InsertPortalPolyhedronInNode((BspNode *) tree, z);
   if (active)
      Error(1, "Active list non-empty after inserting polyhedron.\n");
}

extern int brush_count, csg_clip_count;
extern void store_brush_bbox(int brushid, PortalPolyhedron *ph);
extern void store_brush_planes(int brushid, PortalPolyhedron *ph);
extern void quiet_portalize_mem_reset(void);

void cid_insert_brush(int brushid, void *geom)
{
   cur_brush = brushid;
   compute_planes(geom);
#ifdef DBG_ON
   CheckPolyhedron(geom, "(input brush polyhedron)");
#endif
   if (CB_MEDIUM(brushid) == CB_MEDIUM_HOTREGION)
      ++csg_clip_count;
   store_brush_bbox(brushid, geom);
   store_brush_planes(brushid, geom);
   ++brush_count;
   PutVolumeInTree(tree, geom);
   quiet_portalize_mem_reset();
}

void cid_register_brush_planes(PortalPolyhedron *geom)
{
   compute_planes(geom);
}
