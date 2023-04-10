/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgfind.c,v 1.3 2000/02/19 12:55:00 toml Exp $

// find_brface_from_poly

#include <lg.h>
#include <matrixs.h>

#include <csg.h>
#include <bspdata.h>
#include <mprintf.h>
#include <csgbrush.h>
#include <csgutil.h>
#include <csgbbox.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

int num_brush_faces[MAX_FACE_BRUSHES];
BspPlane brush_faces[MAX_FACE_BRUSHES][MAX_FACES];

#define BBOX_EPSILON   0.1

extern int csg_num_brushes;

void store_brush_planes(int brushid, PortalPolyhedron *ph)
{
   int n = 0;
   PortalPolygon *p = ph->poly;
   n=0;

   do {
      brush_faces[brushid][n] = *p->plane;
      p = GetPhNext(p, ph);
      ++n;
   } while (p != ph->poly);

   num_brush_faces[brushid] = n;
}

bool bbox_inside(mxs_vector *s0, mxs_vector *e0, mxs_vector *s1, mxs_vector *e1)
{
   if (s0->x < s1->x-BBOX_EPSILON) return FALSE;
   if (s0->y < s1->y-BBOX_EPSILON) return FALSE;
   if (s0->z < s1->z-BBOX_EPSILON) return FALSE;

   if (e0->x > e1->x+BBOX_EPSILON) return FALSE;
   if (e0->y > e1->y+BBOX_EPSILON) return FALSE;
   if (e0->z > e1->z+BBOX_EPSILON) return FALSE;

   return TRUE;
}
   
mxs_vector brush_min[MAX_FACE_BRUSHES];
mxs_vector brush_max[MAX_FACE_BRUSHES];

void store_brush_bbox(int brushid, PortalPolyhedron *ph)
{
   BspVertex mn,mx;
   compute_ph_bbox(ph, &mn, &mx);

   brush_min[brushid].x = mn.x;
   brush_min[brushid].y = mn.y;
   brush_min[brushid].z = mn.z;

   brush_max[brushid].x = mx.x;
   brush_max[brushid].y = mx.y;
   brush_max[brushid].z = mx.z;
}

extern int PortalPolygonPlaneCompare(PortalPolygon *p, BspPlane *plane);

// determine what the most recent brush is which shares a boundary
// with this -- basically, figure out which brush contains it and
// shares a boundary with it... this can fail if the polygon doesn't
// match any brush

static int PortalPolyInBrush(PortalPolygon *poly, int brushid)
{
   int i,n = num_brush_faces[brushid], r;

   r = -1;
   for (i=0; i < n; ++i) {
      switch (PortalPolygonPlaneCompare(poly, &brush_faces[brushid][i])) {
         case IN_FRONT: break;
         case BEHIND: return -1;
         case CROSS: return -1;
         case COPLANAR:
            r = i;
            break;
      }
   }
   return r;
}

static int PortalPolyMatchBrush(PortalPolygon *poly, int brushid)
{
   int i,n = num_brush_faces[brushid];

   for (i=0; i < n; ++i) {
      if (PortalPolygonPlaneCompare(poly, &brush_faces[brushid][i])==COPLANAR)
         return i;
   }
   return -1;
}

bool error_find;
int find_brface_from_poly(PortalPolygon *poly)
{
   int i, best_tm=-1, x,y=-1;
   mxs_vector mn, mx;
   BspVertex bmin, bmax;

   compute_poly_bbox(poly, &bmin, &bmax);

   mn.x = bmin.x;
   mn.y = bmin.y;
   mn.z = bmin.z;
   mx.x = bmax.x;
   mx.y = bmax.y;
   mx.z = bmax.z;

   for (i=0; i < csg_num_brushes; ++i) {
      if (CSG_BRUSH(i) && CB_TIMESTAMP(i) > best_tm &&
         bbox_inside(&mn, &mx, &brush_min[i], &brush_max[i])) {
         x = PortalPolyInBrush(poly, i);
         if (x >= 0) { // && check if we have a texture on this face
            best_tm = CB_TIMESTAMP(i);
            y = x + i*256;
         }
      }
   }

   if (y < 0) {
      mprintf("Oops, polygon spans more than one brush.\n");
      // ok, we might have a polygon which spans two brushes.
      for (i=0; i < csg_num_brushes; ++i) {
         if (CSG_BRUSH(i) && CB_TIMESTAMP(i) > best_tm) {
            x = PortalPolyMatchBrush(poly, i);
            if (x >= 0) {
               best_tm = CB_TIMESTAMP(i);
               y = x + i*256;
            }
         }
      }
      // if currently doing error processing,
      // don't abort if none
      if (!error_find && y < 0)
         Error(1, "find_brface_from_poly: Unable to find csg face\n");
   }
   return y;
}


static FILE *brush_list;

// @TODO
char brushlist_file[] = "crash000.log";

void write_brush_start(void)
{
   brush_list = fopen(brushlist_file, "w");
   if (!brush_list)
      Warning(("Could not write crash log file.\n"));
}

void write_polyhedron_brushes(PortalPolyhedron *ph)
{
   PortalPolygon *p = ph->poly;
   error_find = TRUE;
   do {
      int z = find_brface_from_poly(p);
      if (z >= 0) {
         fprintf(brush_list, "Brush %d (face %d)\n",
             z >> 8, z & 255);
      } else
         fprintf(brush_list, "No brush matched.\n");
      p = GetPhNext(p, ph);
   } while (p != ph->poly);
}

void write_plane_brush(BspPlane *clip)
{
}

void write_brush_end(void)
{
   fclose(brush_list);
}
