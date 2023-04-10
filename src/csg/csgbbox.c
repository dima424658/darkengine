/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


#include <lg.h>
#include <csg.h>
#include <bspdata.h>
#include <csgutil.h>
#include <csgbbox.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

void compute_ph_bounds_init(BspVertex *minv, BspVertex *maxv)
{
   minv->x = minv->y = minv->z = 1e20;
   maxv->x = maxv->y = maxv->z = -1e20;
}

void compute_ph_bounds_more(PortalPolygon *poly, BspVertex *minv, BspVertex *maxv)
{
   PortalPolyEdge *edge = poly->edge, *first = edge;
   do {
      if (edge->data->start.x < minv->x) minv->x = edge->data->start.x;
      if (edge->data->  end.x < minv->x) minv->x = edge->data->end.x;
      if (edge->data->start.x > maxv->x) maxv->x = edge->data->start.x;
      if (edge->data->  end.x > maxv->x) maxv->x = edge->data->end.x;

      if (edge->data->start.y < minv->y) minv->y = edge->data->start.y;
      if (edge->data->  end.y < minv->y) minv->y = edge->data->end.y;
      if (edge->data->start.y > maxv->y) maxv->y = edge->data->start.y;
      if (edge->data->  end.y > maxv->y) maxv->y = edge->data->end.y;

      if (edge->data->start.z < minv->z) minv->z = edge->data->start.z;
      if (edge->data->  end.z < minv->z) minv->z = edge->data->end.z;
      if (edge->data->start.z > maxv->z) maxv->z = edge->data->start.z;
      if (edge->data->  end.z > maxv->z) maxv->z = edge->data->end.z;

      edge = edge->poly_next;
   } while (edge != first);
}

void compute_poly_bbox(PortalPolygon *poly, BspVertex *minv, BspVertex *maxv)
{
   compute_ph_bounds_init(minv, maxv);
   compute_ph_bounds_more(poly, minv, maxv);
}

void compute_ph_bbox(PortalPolyhedron *ph, BspVertex *minv, BspVertex *maxv)
{
   PortalPolygon *p = ph->poly;
   compute_ph_bounds_init(minv, maxv);
   do {
      compute_ph_bounds_more(p, minv, maxv);
      p = GetPhNext(p, ph);
   } while (p != ph->poly);
}
