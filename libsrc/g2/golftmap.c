// $Header: x:/prj/tech/libsrc/g2/RCS/golftmap.c 1.5 1998/04/28 14:28:48 KEVIN Exp $

#include <g2d.h>
#include <trirast.h>
#include <plyshell.h>
#include <genrast.h>

void golf_tmap_inner_loop(g2s_raster *r, g2s_poly_params *tp)
{
   fix v_mask = fix_make(tp->bm->h - 1, 0);
   fix u_mask = fix_make(tp->bm->w - 1, 0);
   uchar *bits = tp->bm->bits;
   uchar v_shift = 16 - tp->bm->wlog;
   fix err_mask = (g2d_error_dither) ? (FIX_UNIT-1) : 0;
   fix err_i = 0, err_h = 0, err_d = 0;
   ushort *p_dest = (ushort *)r->p;

   do {
      uint offset,i;
      uchar c, cl, ch, *clut;
      ushort c16, *clut16;

      offset = ((r->v&v_mask)>>v_shift) + ((r->u&u_mask)>>16);
      c = bits[offset];

      //dryness is 8->16
      i = c>>6;
      c &= 0x3f;
      clut = g2d_dryness_table_list[i] +
         (fix_int((r->d+err_d)&g2d_dryness_mask_list[i])<<7); 
      cl = clut[2*c], ch = clut[2*c+1];

      //lighting is 16->16
      clut16 = (ushort *)(grd_light_table + (fix_int(r->i+err_i)<<10));
      c16 = clut16[cl] + clut16[256+ch];

      //haze is 16->16
      clut16 = (ushort *)(g2d_haze_table + (fix_int(r->h+err_h)<<10));
      c16 = clut16[c16&0xff] + clut16[256+(c16>>8)];
      p_dest[r->x++] = c16;

      r->u += tp->dux;
      r->v += tp->dvx;
      r->i += tp->dix;
      r->h += tp->dhx;
      r->d += tp->ddx;
      err_i = (err_i+r->i)&err_mask;
      err_h = (err_h+r->h)&err_mask;
      err_d = (err_d+r->d)&err_mask;
   } while (--(r->n));
}

void g2_lit_haze_dry_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2s_poly_params tp;

   tp.flags = PPF_IUVHD;
   tp.canvas_row = grd_bm.row;

   tp.bm = bm;
   tp.i_scale = grd_light_table_size;
   tp.u_scale = bm->w;
   tp.v_scale = bm->h;
   tp.h_scale = g2d_haze_table_size;
   tp.d_scale = g2d_dryness_table_size;
   tp.inner_loop = golf_tmap_inner_loop;
   tp.raster_func = gen_raster_loop;
   tp.right_edge_func = gen_right_edge;
   tp.left_edge_func = gen_left_edge;
   tp.grad_func = gen_triangle_gradients;

   g2_trifan(&tp, n, vpl);
}

void golf_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2_lit_haze_dry_umap(bm, n, vpl);
}
