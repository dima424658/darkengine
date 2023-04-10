/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/wrcell.c,v 1.8 2000/01/20 23:20:34 PATMAC Exp $

#include <wr.h>
#include <wrbsp.h>
#include <wrdbrend.h>

#ifndef SHIP
#include <mprintf.h>
#include <report.h>
#include <math.h>
#endif

#include <wrcell.h>
#include <wrlimit.h>

#include <dbmem.h>

//////////////////////////
// LOOK: Storage for the world rep

PortalCell *wr_cell[MAX_REGIONS];
int wr_num_cells;

#ifndef SHIP

int count_bits(unsigned int k)
{
   int n=0;
   while (k) { 
      if (k & 1) ++n;
      k >>= 1;
   }
   return n;
}

void wr_stats(char **rpt_str)
{
   char buf[800], *p=buf; // way more than enough

   int i,j;

   ulong count_poly=0, count_portal=0, count_point=0, count_plane=0, count_vlist=0;
   //// stats
   ulong total;      // all storage used by world rep

   ulong cells;      // storage due to cells
   ulong points;     // storage due to points
   ulong polys;      // storage due to polys
   ulong portals;    // storage due to portals
   ulong planes;     // storage due to planes

   ulong lm_base;    // storage due to lightmaps (one per polygon)
   ulong lm_anim;    // storage due to animating lightmaps

   ulong bsp;

   total = cells = points = polys = portals = planes = 0;
   lm_base = lm_anim = bsp = 0;

     //// easy to collect data

   bsp = g_wrBspTreeSize * sizeof(wrBspNode);
   cells = wr_num_cells * sizeof(PortalCell);

     //// others

   for (i=0; i < wr_num_cells; ++i) {
      PortalCell *p = WR_CELL(i);

      count_portal += p->num_polys-p->num_render_polys;
      count_poly   += p->num_render_polys;
      count_point  += p->num_vertices;
      count_vlist  += p->num_vlist;
      count_plane  += p->num_planes;

      points += p->num_vertices * sizeof(Vertex);
      points += p->num_vlist    * sizeof(uchar);  // indirection

      polys  += p->num_render_polys *
                 (sizeof(PortalPolygonCore) + sizeof(PortalPolygonRenderInfo));

      portals += (p->num_polys - p->num_render_polys) *
                 (sizeof(PortalPolygonCore));

      planes += p->num_planes * sizeof(PortalPlane);

      for (j=0; j < p->num_render_polys; ++j) {
         PortalLightMap *lm = &p->light_list[j];
         int sz = lm->w * lm->h * sizeof(LightmapEntry);
         lm_base += sizeof(PortalLightMap) + sz;
         lm_anim += count_bits(lm->anim_light_bitmask) * sz;
      }
   }

   total = cells + points + polys + portals + planes + lm_base + lm_anim + bsp;

   rsprintf(&p,"%d polys, %d cells, %d portals, %d points, %d vlist, %d planes\n", count_poly, wr_num_cells, count_portal, count_point, count_vlist, count_plane);
   rsprintf(&p,"Total WR size: %6dK\n", total >> 10);
   rsprintf(&p,"Geometry:      %6dK\n", (cells+points+polys+portals+planes) >> 10);
   rsprintf(&p,"    Cells:     %6dK\n", cells >> 10);
   rsprintf(&p,"    Points:    %6dK\n", points >> 10);
   rsprintf(&p,"    Polygons:  %6dK\n", (polys+portals) >> 10);
   rsprintf(&p,"    Planes:    %6dK\n", planes >> 10);
   rsprintf(&p,"Lightmaps:     %6dK\n", (lm_base + lm_anim) >> 10);
   rsprintf(&p,"    Main lm:   %6dK\n", lm_base >> 10);
   rsprintf(&p,"    Anim lm:   %6dK\n", lm_anim >> 10);
   rsprintf(&p,"BSP:           %6dK\n", bsp >> 10);

   if (rpt_str)
      rsprintf(rpt_str,buf);
   else
      mprint(buf);
}

void wr_mono_stats(void)
{
   wr_stats(NULL);
}

#define SLIVER_3D 0.05
#define SLIVER_2D 0.03
#define SLIVER_1D 0.02

// small in all 3d
static BOOL cell_small_3d(PortalCell *p, float tolerance)
{
   return (p->sphere_radius < tolerance);
}

// small in 2d (effectively a line)
static BOOL cell_small_2d(PortalCell *p, float tolerance)
{
   mxs_vector *sliver_normal = NULL;
   int j,k;
   float max_dist;
   for (j=0; j < p->num_planes; ++j) {
      mxs_vector *norm = &p->plane_list[j].normal;
      float d = p->plane_list[j].plane_constant;
      max_dist = 0;
      for (k=0; k < p->num_vertices; ++k) {
         mxs_vector *v = &p->vpool[k];
         float dist = v->x*norm->x + v->y*norm->y + v->z*norm->z + d;
         if (fabs(dist) > max_dist) {
            max_dist = fabs(dist);
            if (max_dist > tolerance)
               break;
         }
      }
      if (max_dist < tolerance) {
         if (sliver_normal == NULL) {
            sliver_normal = norm;
         } else {
            float dot = mx_dot_vec(norm, sliver_normal);
            if (fabs(dot) < 0.9)
               return TRUE;
         }
      }
   }
   return FALSE;
}

// small in 1d (effectively a polygon)
static BOOL cell_small_1d(PortalCell *p, float tolerance)
{
   float max_dist;
   int j,k;
   for (j=0; j < p->num_planes; ++j) {
      mxs_vector *norm = &p->plane_list[j].normal;
      float d = p->plane_list[j].plane_constant;
      max_dist = 0;
      for (k=0; k < p->num_vertices; ++k) {
         mxs_vector *v = &p->vpool[k];
         float dist = v->x*norm->x + v->y*norm->y + v->z*norm->z + d;
         if (fabs(dist) > max_dist) {
            max_dist = fabs(dist);
            if (max_dist > tolerance)
               break;
         }
      }
      if (max_dist < tolerance)
         return TRUE;
   }
   return FALSE;
}

static void check_cells(int check_num, char *title)
{
   int i;
   BOOL any_printed = FALSE;

   for (i=0; i < wr_num_cells; ++i) {
      PortalCell *p = WR_CELL(i);
      if (cell_small_3d(p, SLIVER_3D)) {
         if (check_num == 3) goto display;
         continue;
      }
      if (cell_small_2d(p, SLIVER_2D)) {
         if (check_num == 2) goto display;
         continue;
      }
      if (cell_small_1d(p, SLIVER_2D)) {
         if (check_num == 1) goto display;
         continue;
      }
      continue;
     display:
      if (!any_printed) {
         mprintf("%s: ", title);
         any_printed = TRUE;
      }
      mprintf("%d ",i);
   }
   if (any_printed)
      mprintf("\n");
}

void wr_check_cells(void)
{
   check_cells(3, "Tiny cells");
   check_cells(2, "Sliver cells");
   check_cells(1, "Flat cells");
}

#endif
