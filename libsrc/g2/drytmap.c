// $Header: x:/prj/tech/libsrc/g2/RCS/drytmap.c 1.4 1998/04/28 14:31:15 KEVIN Exp $

#include <g2d.h>
#include <trirast.h>
#include <plyshell.h>
#include <genrast.h>

void dry_tmap_inner_loop(g2s_raster *r, g2s_poly_params *tp)
{
   fix v_mask = fix_make(tp->bm->h - 1, 0);
   fix u_mask = fix_make(tp->bm->w - 1, 0);
   uchar *bits = tp->bm->bits;
   uchar v_shift = 16 - tp->bm->wlog;
   fix err_mask = (g2d_error_dither) ? (FIX_UNIT-1) : 0;
   fix err_d = 0;
   ushort *p_dest = (ushort *)r->p;

   do {
      uint offset,i;
      uchar c;
      ushort *clut16;

      offset = ((r->v&v_mask)>>v_shift) + ((r->u&u_mask)>>16);
      c = bits[offset];

      //dryness is 8->16
      i = c>>6;
      c &= 0x3f;
      clut16 = (ushort *)(g2d_dryness_table_list[i] +
         (fix_int((r->d+err_d)&g2d_dryness_mask_list[i])<<7));
      p_dest[r->x++]  = clut16[c];

      r->u += tp->dux;
      r->v += tp->dvx;
      r->d += tp->ddx;
      err_d = (err_d+r->d)&err_mask;
   } while (--(r->n));
}


void g2_dry_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2s_poly_params tp;

   tp.flags = PPF_UVD;
   tp.canvas_row = grd_bm.row;

   tp.bm = bm;
   tp.d_scale = g2d_dryness_table_size;
   tp.u_scale = bm->w;
   tp.v_scale = bm->h;
   tp.inner_loop = dry_tmap_inner_loop;
   tp.raster_func = gen_raster_loop;
   tp.right_edge_func = gen_right_edge;
   tp.left_edge_func = gen_left_edge;
   tp.grad_func = gen_triangle_gradients;

   g2_trifan(&tp, n, vpl);
}
