/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/src/portal/portpick.c,v 1.8 2000/02/19 13:18:41 toml Exp $
//
//  PORTAL
//
//  dynamic portal/cell-based renderer

#include <string.h>
#include <math.h>

#include <lg.h>
#include <r3d.h>
#include <mprintf.h>

#include <portal_.h>
#include <portclip.h>
#include <portdraw.h>
#include <pt.h>
#include <pt_clut.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#pragma off(unreferenced)
bool pick_surface(PortalPolygonCore *poly, int voff, void *clip, int x, int y)
{
   r3s_phandle vlist[32], *final;
   int i, j, n = poly->num_vertices;
   bool inside = FALSE;

   // prepare the vertex list

   for (i=0; i < n; ++i)
      vlist[i] = &cur_ph[r_vertex_list[voff+i]];

   n = r3_clip_polygon(n, vlist, &final);
   if (n <= 2) return FALSE;

   // now test whether the point x,y is inside the 
   // polygon.  This test routine is sort of overkill,
   // since it works for any odd-even wound concave polygon.

   x = fix_make(x,0);
   y = fix_make(y,0);

   j = n-1;
   for (i=0; i < n; ++i) {
      // check that our y appears between the other two 
      if ((y < final[i]->grp.sy) ^ (y < final[j]->grp.sy)) {
           // find where this line intersects with the horizontal line at y
         fix dx = final[i]->grp.sx - final[j]->grp.sx;
         fix dy = final[i]->grp.sy - final[j]->grp.sy;
         fix sx = fix_mul_div(y - final[j]->grp.sy, dx, dy) + final[j]->grp.sx;
         if (x > sx) inside = !inside;
      }
      j = i;
   }
   return inside;
}
#pragma on(unreferenced)

int pick_region(PortalCell *r, int x, int y)
{
   int i, n = r->num_render_polys;
   int voff=0;
   int retval = -1;
   PortalPolygonCore *poly = r->poly_list;

   if (!n) return -1;

     // copy common data into globals for efficient communicating
     // someday we should inline the function "draw_surface" and
     // then get rid of these globals

   cur_ph = POINTS(r);

   r_vertex_list = r->vertex_list;

   r_clip = CLIP_DATA(r);

   r3_set_clip_flags(0);

      // now test all the polygons

   r3_start_block();

   for (i=0; i < n; ++i) {
      if (check_surface_visible(r, poly, voff))
         if (pick_surface(poly, voff, CLIP_DATA(r), x, y))
            retval = i;
      voff += poly->num_vertices;
      ++poly;
   }

   r3_end_block();
   return retval;
}
