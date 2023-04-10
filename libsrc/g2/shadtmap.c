// $Header: x:/prj/tech/libsrc/g2/RCS/shadtmap.c 1.7 1998/04/28 14:29:37 KEVIN Exp $

#include <g2d.h>
#include <trirast.h>
#include <plyshell.h>
#include <genrast.h>

int g2d_ambient_light = 16;
static ushort *shadow_clut;

void shadow_tmap_inner_loop(g2s_raster *r, g2s_poly_params *tp)
{
   fix v_mask = fix_make(tp->bm->h - 1, 0);
   fix u_mask = fix_make(tp->bm->w - 1, 0);
   uchar *bits = tp->bm->bits;
   uchar v_shift = 16 - tp->bm->wlog;
   ushort *p_dest = (ushort *)r->p;

   do {
      uint offset;
      uchar c;
      ushort c16;

      offset = ((r->v&v_mask)>>v_shift) + ((r->u&u_mask)>>16);
      c = bits[offset];
      if (c!=0) {
         c16 = p_dest[r->x];

         //lighting is 16->16
         c16 = shadow_clut[c16&0xff] + shadow_clut[256+(c16>>8)];

         p_dest[r->x] = c16;
      }

      r->x++;
      r->u += tp->dux;
      r->v += tp->dvx;
   } while (--(r->n));
}

void shadow_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2s_poly_params tp;

   tp.flags = PPF_UV;
   tp.canvas_row = grd_bm.row;

   tp.bm = bm;
   tp.u_scale = bm->w;
   tp.v_scale = bm->h;
   tp.inner_loop = shadow_tmap_inner_loop;
   tp.raster_func = gen_raster_loop;
   tp.right_edge_func = gen_right_edge;
   tp.left_edge_func = gen_left_edge;
   tp.grad_func = gen_triangle_gradients;
   shadow_clut = (ushort *)(grd_light_table + (g2d_ambient_light<<10));
   g2_trifan(&tp, n, vpl);
}

