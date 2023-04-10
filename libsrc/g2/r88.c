// $Header: x:/prj/tech/libsrc/g2/RCS/r88.c 1.2 1998/04/28 14:31:38 KEVIN Exp $
#include <dev2d.h>
#include <genrast.h>
#include <tmapd.h>

extern g2il_func gen_flat8_il;

static uchar *bits;
static BOOL opaque;
static fix u_mask, v_mask;
static uchar v_shift;

#pragma off(unreferenced)
void translucent_8to8_inner_loop(g2s_raster *r, g2s_poly_params *tp)
{
   uchar *p_dst = r->p + r->x + r->n;
   int n = -r->n;
   fix u = r->u;
   fix v = r->v;

   do {
      uint offset = ((v&v_mask)>>v_shift) + ((u>>16)&u_mask);
      uchar c = bits[offset];
      if (opaque||(c!=0)) {
         uchar *clut = tluc8tab[c];
         if (clut != NULL)
            p_dst[n] = clut[p_dst[n]];
         else
            p_dst[n] = c;
      }
      u += g2d_pp.dux;
      v += g2d_pp.dvx;
   } while (++n);
}


void translucent_8to8_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   g2d_pp.left_edge_func = gen_left_edge;
   g2d_pp.right_edge_func = gen_right_edge;
   g2d_pp.raster_func = gen_raster_loop;

   if (POW2(bm)) {
      g2d_pp.grad_func = uv_triangle_gradients;
      g2d_pp.canvas_row = grd_bm.row;
      g2d_pp.inner_loop = translucent_8to8_inner_loop;
      bits = bm->bits;
      opaque = !(bm->flags&BMF_TRANS);
      u_mask = bm->w-1;
      v_mask = (bm->h-1)<<16;
      v_shift = 16 - bm->wlog;
   } else {
      g2d_pp.grad_func = gen_triangle_gradients;
      g2d_pp.inner_loop = gen_flat8_il;
      g2d_pp.pix_func = gd_upix8_expose(0,0,0);
      g2d_pp.dy = 1;
   }
}
#pragma on(unreferenced)
