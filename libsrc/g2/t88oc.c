// $Header: x:/prj/tech/libsrc/g2/RCS/t88oc.c 1.4 1998/04/28 14:31:40 KEVIN Exp $
#include <trirast.h>
#include <plyshell.h>
#include <genrast.h>
#include <tmapd.h>
#include <g2tm.h>

static int total_pixels, total_visible;
static int row;
static uint offset_max;
static uchar *src;

static void trans_8to8_check_occlusion_inner_loop(g2s_raster *r, g2s_poly_params *tp)
{
   fix u,v;
   int x;
   uchar *dst;

   dst = r->p + r->x + r->n;
   x = -r->n;
   u = r->u;
   v = r->v;

   total_pixels += r->n;

   goto loop_start;
   do {
      uint offset;
      uchar c;

      u += g2d_pp.dux;
      v += g2d_pp.dvx;
loop_start:
      offset = (uint )(fix_int(u) + fix_int(v)*row);
      if (offset >= offset_max)
         offset = 0;
      c = src[offset];
      if (c==0) {
         total_pixels--;
         continue;
      }

      if (c == dst[x])
         total_visible++;
   } while (++x);
}

fix trans_8to8_check_occlusion_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   total_pixels = total_visible = 0;
   g2d_pp.flags = PPF_UV;
   g2d_pp.canvas_row = grd_bm.row;
   g2d_pp.bm = bm;
   g2d_pp.u_scale = bm->w;
   g2d_pp.v_scale = bm->h;
   g2d_pp.inner_loop = trans_8to8_check_occlusion_inner_loop;
   g2d_pp.raster_func = gen_raster_loop;
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.grad_func = gen_triangle_gradients;

   row = bm->row;
   src = bm->bits;
   offset_max = (uint )(row * bm->h);

   g2_umap(n, vpl);

   if (total_pixels <= 0)
      return 0;

   return fix_div(total_visible, total_pixels);
}

