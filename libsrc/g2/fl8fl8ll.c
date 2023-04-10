/* $Header: x:/prj/tech/libsrc/g2/RCS/fl8fl8ll.c 1.4 1996/09/23 10:41:47 KEVIN Exp $
 *
 * 8->8 lit inner loop.
 *
 * This file is part of the g2 library.
 *
 */

#include <dev2d.h>
#include <g2d.h>
#include <tmapd.h>
#include <tmapfcn.h>
#include <dbg.h>

extern void flat8_flat8_lit_opaque_p2_wrap_il();
extern void flat8_flat8_lit_opaque_p2_wrap_dither_il();

extern uchar *g2d_ltab_dither0[];
extern uchar *g2d_ltab_dither1[];
extern ulong g2d_o2wi_mask_u[];
extern ulong g2d_o2wi_last_mask_u;
extern ulong g2d_o2wi_mask_v[];
extern ulong g2d_o2wi_last_mask_v;
extern uchar g2d_o2wi_shift_v[];
extern uchar g2d_o2wi_last_shift_v;
extern uchar *g2d_o2wi_p_src[];
extern uchar *g2d_o2wi_last_p_src;

static uchar *last_ltab=(uchar *)0xdeadbeef;

#define flat8_flat8_opaque_wrap_lit_ulmap flat8_flat8_opaque_lit_ulmap
void flat8_flat8_opaque_wrap_lit_ulmap(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   ulong mask_u, mask_v;
   uchar shift_v;

   Assrt(bm->row==1<<bm->wlog);
   Assrt(((bm->w-1)&bm->w)==0);
   Assrt(((bm->h-1)&bm->h)==0);

   ti->bm = bm;
   ti->drow = grd_bm.row;

   if (bm->bits != g2d_o2wi_last_p_src)
      g2d_o2wi_p_src[-1] = g2d_o2wi_last_p_src = bm->bits;
   shift_v = 16-bm->wlog;
   mask_v = ((bm->h-1)<<16);
   mask_u = bm->w-1;
   if (shift_v != g2d_o2wi_last_shift_v)
      g2d_o2wi_shift_v[-1] = g2d_o2wi_last_shift_v = shift_v;
   if (mask_v != g2d_o2wi_last_mask_v)
      g2d_o2wi_mask_v[-1] = g2d_o2wi_last_mask_v = mask_v;
   if (mask_u != g2d_o2wi_last_mask_u)
      g2d_o2wi_mask_u[-1] = g2d_o2wi_last_mask_u = mask_u;

   ti->ltab = grd_light_table;
   if (g2d_error_dither) {
      if (last_ltab != ti->ltab) {
         last_ltab = ti->ltab;
         g2d_ltab_dither0[-1] = last_ltab;
         g2d_ltab_dither1[-1] = last_ltab;
      }
      ti->il_func = flat8_flat8_lit_opaque_p2_wrap_dither_il;
   } else {
      ti->il_func = flat8_flat8_lit_opaque_p2_wrap_il;
   }
   do_lmap(n, vpl, lit_lmap_shell);
}
