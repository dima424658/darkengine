// $Header: x:/prj/tech/libsrc/g2/RCS/shadhack.c 1.6 1998/04/28 14:29:32 KEVIN Exp $

#include <g2d.h>
#include <trirast.h>
#include <plyshell.h>
#include <tmapd.h>
#include <dev2d.h>
#include <genrast.h>

static void golf_shadow_hack_il(g2s_raster *r, g2s_poly_params *tp)
{
   uint offset_max;
   fix u,v,du,dv;
   int row, left, right, x, x_last;
   ushort *dst;
   uchar *src;

   if (r->n <= 0)
      return;

   if ((r->y < grd_clip.top) || (r->y >= grd_clip.bot))
      return;

   src = tp->bm->bits;
   row = tp->bm->row;
   offset_max = (uint )(row * tp->bm->h);
   left = grd_clip.left;
   right = grd_clip.right;
   dst = (ushort *)(grd_bm.bits + r->y * grd_bm.row);

   x = r->x;
   u = r->u;
   v = r->v;
   du = tp->dux;
   dv = tp->dvx;
   x_last = x + r->n - 1;
   goto loop_start;
   do {
      uint offset;
      uchar c;
      ushort c16;
      ushort *clut;

      u += du;
      v += dv;
      x++;
loop_start:
      if ((x < left) || (x >= right))
         continue;

      offset = (uint )(fix_int(u) + fix_int(v)*row);
      if (offset >= offset_max)
         offset = 0;
      c = src[offset];
      if (c==0)
         continue;

      clut = (ushort *)(tluc8tab[c]);
      c16 = dst[x];
      dst[x] = clut[c16&0xff] + clut[256 + (c16>>8)];
   } while (x<x_last);
}

void golf_shadow_hack(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2s_poly_params tp;

   tp.flags = PPF_UV;
   tp.dy = 1;

   tp.bm = bm;
   tp.u_scale = bm->w;
   tp.v_scale = bm->h;
   tp.inner_loop = golf_shadow_hack_il;
   tp.raster_func = gen_raster_loop;
   tp.right_edge_func = gen_right_edge;
   tp.left_edge_func = gen_left_edge;
   tp.grad_func = gen_triangle_gradients;

   g2_trifan(&tp, n, vpl);
}
