/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/gridsnap.c,v 1.5 2000/02/19 13:10:53 toml Exp $

// Grid snapping code

#include <r3d.h>
#include <gridsnap.h>
#include <math.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static mxs_real quantize_point(mxs_real pt, mxs_real scale)
{
   // We want to compute a*scale+b = pt
   // where 0 <= b < scale, and return a*scale.

   // One way to do this is with modf, which
   // computes (a,b) itself.  Then we could
   // either return a*scale or pt-b.  The latter
   // will introduce rounding errors and make
   // points not quite identical.  Alternately,
   // can we be sure that a is integral, and not
   // 4.9999999 or the like?  Unclear what modf() promises.

   // If we keep our grid scales powers of two,
   // this will help a lot.

   // anyway, we also have the problem that modf
   // deals incorrectly with the negative case,
   // forming negative values of b.  So we just
   // use an explicit floor instead.

   // the rounding problems aren't as bad as they
   // sound because we also actually bias by scale/2
   // and we don't really care about consistency of
   // points right at the boundary.

   return floor((pt + scale/2) / scale) * scale;
}

void gedit_vertex_snap(Grid *g, mxs_vector *dest, mxs_vector *src)
{
   if (g->orientation.tx || g->orientation.ty || g->orientation.tz) {
      // rotated case; this code would work in the non-rotated
      // case but why be slow?

      // besides, this way we can test non-rotated first
   } else {
      // non-rotated case
      dest->x = quantize_point(src->x, g->line_spacing);
      dest->y = quantize_point(src->y, g->line_spacing);
      dest->z = quantize_point(src->z, g->line_spacing);
   }
}

// we render the 3d grid in the current r3_color.
// We assume we only need to draw the grid perpendicular
// to prime_axis.  In a 2d view, prime_axis is the depth axis,
// and thus drawing one grid layer draws them all.
//
// In a 3d view, prime_axis is determined by the user,
// and allows the user to place a visible grid in 3d.
// However, in this case the grid won't necessarily extend
// everywhere you can see, since in a 3d view you can see
// to infinity along more than one axis.
//
// We use "height" for the world location of the grid along
// the prime axis; this is only visible in a 3d view

static void render_line(mxs_vector *a, mxs_vector *b)
{
   r3s_point pts[2];

   r3_transform_point(pts, a);
   r3_transform_point(pts+1, b);

   r3_draw_line(pts, pts+1);
}

void gedit_render_grid(Grid *g, int prime_axis, mxs_real height,
                       mxs_vector *start, mxs_vector *end)
{
   mxs_vector origin;
   mxs_vector s,e;
   mxs_real temp, x, limit;
   mxs_vector pt1,pt2;
   int a0, a1;

   origin.x = origin.y = origin.z = 0;

   r3_start_object_angles(&origin, &g->orientation, R3_DEFANG);
   r3_start_block();

   r3_transform_w2o(&s, start);
   r3_transform_w2o(&e, end);

   // swap axes around so they're predictable

   if (s.x > e.x) { temp = e.x; e.x = s.x; s.x = temp; }
   if (s.y > e.y) { temp = e.y; e.y = s.y; s.y = temp; }
   if (s.z > e.z) { temp = e.z; e.z = s.z; s.z = temp; }

   // gridize those points
   
   s.x = quantize_point(s.x, g->line_spacing) - g->line_spacing;
   s.y = quantize_point(s.y, g->line_spacing) - g->line_spacing;
   s.z = quantize_point(s.z, g->line_spacing) - g->line_spacing;

   e.x = quantize_point(e.x, g->line_spacing) + g->line_spacing;
   e.y = quantize_point(e.y, g->line_spacing) + g->line_spacing;
   e.z = quantize_point(e.z, g->line_spacing) + g->line_spacing;

   // determine which axes to iterate through
   a0 = (prime_axis + 1) % 3;
   a1 = (prime_axis + 2) % 3;

   pt1.el[prime_axis] = height;
   pt2.el[prime_axis] = height;

     // draw lines parallel to a0

   pt1.el[a1] = s.el[a1];
   pt2.el[a1] = e.el[a1];

   limit = e.el[a0] + g->line_spacing/2;

   if ((limit - s.el[a0])/g->line_spacing < 100) {
      for (x=s.el[a0]; x <= limit; x += g->line_spacing) {
         pt1.el[a0] = x;     
         pt2.el[a0] = x;     
         render_line(&pt1, &pt2);
      }
   }

     // draw lines parallel to a1

   pt1.el[a0] = s.el[a0];
   pt2.el[a0] = e.el[a0];

   limit = e.el[a1] + g->line_spacing/2;

   if ((limit - s.el[a1])/g->line_spacing < 100) {
      for (x=s.el[a1]; x <= limit; x += g->line_spacing) {
         pt1.el[a1] = x;
         pt2.el[a1] = x;
         render_line(&pt1, &pt2);
      }
   }
 
   r3_end_block();
   r3_end_object();
}
