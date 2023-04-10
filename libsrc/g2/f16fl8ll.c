/* $Header: x:/prj/tech/libsrc/g2/RCS/f16fl8ll.c 1.5 1996/10/16 16:07:23 TOML Exp $
 *
 * 8->16 lit inner loop.
 *
 * This file is part of the g2 library.
 *
 */

#include <dev2d.h>
#include <g2d.h>
#include <tmapd.h>
#include <tmapfcn.h>
#include <dbg.h>

extern ulong g2d_o2wi_mask_u[];
extern ulong g2d_o2wi_last_mask_u;
extern ulong g2d_o2wi_mask_v[];
extern ulong g2d_o2wi_last_mask_v;
extern uchar g2d_o2wi_shift_v[];
extern uchar g2d_o2wi_last_shift_v;
extern uchar *g2d_o2wi_p_src[];
extern uchar *g2d_o2wi_last_p_src;

extern void flat8_flat8_opaque_p2_wrap_il_chain_from_c(int n, fix u, fix v);

void flat16_flat8_lit_opaque_p2_wrap_il(int left, int right, fix u, fix v)
{
   int n=left-right;
   fix i,di,err,err_mask;
   g2s_tmap_info *ti=&g2d_tmap_info;
   ushort *d=((ushort *)ti->p_dest)+left;
   ti->p_dest += ti->drow;

   if (n>=0)
      return;
   flat8_flat8_opaque_p2_wrap_il_chain_from_c(n, u, v);

   i = g2d_tmap_info.i;
   di = g2d_tmap_info.dix;
   err = 0;
   err_mask = (g2d_error_dither) ? (FIX_UNIT-1) : 0;
   do {
      int c=g2d_tmap_buffer[G2C_TMAP_BUFFER_SIZE+n];
      ushort *clut = ((ushort *)grd_light_table)+
                     2*((fix_int(i+err))<<8);
      err = (err + i)&err_mask;
      i += di;
      c = ((ushort *)pixpal)[c];
      *d++ = clut[c&255] + clut[256+(c>>8)];
   } while (++n!=0);
}

void flat16_flat8_opaque_lit_ulmap(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   ulong mask_u, mask_v;
   uchar shift_v;

   Assrt(bm->row==1<<bm->wlog);
   Assrt(((bm->w-1)&bm->w)==0);
   Assrt(((bm->h-1)&bm->h)==0);

   pixpal = (void *)(grd_pal16_list[bm->align]);
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

   ti->il_func = flat16_flat8_lit_opaque_p2_wrap_il;
   do_lmap(n, vpl, lit_lmap_shell);
}
