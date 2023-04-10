/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/genlil.c $
 * $Revision: 1.6 $
 * $Author: KEVIN $
 * $Date: 1996/12/20 11:01:04 $
 * 
 * This file is part of the g2 library.
 *
 */

#include <g2d.h>
#include <tmapd.h>

void gen_flat8_lit_il(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   int y;
   fix i, du, dv, di, err, err_mask;
   int row;
   uchar *bits;
   gdupix_func *pix_func;
   gdlpix_func *lpix8_func;
   uint offset_max;

   y = ti->y;
   if (x>=xf)
      return;

   bits = ti->bm->bits;
   row = ti->bm->row;
   offset_max = ti->bm->h * row - 1;
   du = ti->dux;
   dv = ti->dvx;
   di = ti->dix;
   i = ti->i;
   err = 0;
   err_mask = (g2d_error_dither) ? (FIX_UNIT-1) : 0;
   pix_func = ti->pix_func;
   lpix8_func = ti->lpix_func;

   if (ti->bm->flags&BMF_TRANS) {
      do {
         uint offset = fix_int(v)*row + fix_int(u);
         uchar c;
         if (offset>offset_max) offset = 0;
         c=bits[offset];
         if (c)
            pix_func(lpix8_func(i+err, c), x, y);
         u += du;
         v += dv;
         i += di;
         err = (err+i)&err_mask;
         x++;
      } while (x<xf);
   } else {
      do {
         uint offset = fix_int(v)*row + fix_int(u);
         uchar c;
         if (offset>offset_max) offset = 0;
         c=bits[offset];
         pix_func(lpix8_func(i+err, c), x, y);
         u += du;
         v += dv;
         i += di;
         err = (err+i)&err_mask;
         x++;
      } while (x<xf);
   }
}

void gen_flat8_lit_il_wrap(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   int y;
   fix i, du, dv, di, err, err_mask;
   fix u_mask, v_mask;
   int row;
   uchar *bits;
   gdupix_func *pix_func;
   gdlpix_func *lpix8_func;
   grs_bitmap *bm;

   y = ti->y;
   if (x>=xf)
      return;

   bm = ti->bm;
   y = ti->y;
   bits = bm->bits;
   row = bm->row;
   u_mask = fix_make(bm->w-1,0);
   v_mask = fix_make(bm->h-1,0)<<bm->wlog;
   v <<= bm->wlog;
   du = ti->dux;
   dv = ti->dvx<<bm->wlog;
   di = ti->dix;
   i = ti->i;
   pix_func = ti->pix_func;
   lpix8_func = ti->lpix_func;
   err = 0;
   err_mask = (g2d_error_dither) ? (FIX_UNIT-1) : 0;

   if (ti->bm->flags&BMF_TRANS) {
      do {
         uint offset = ((ulong )((v&v_mask) + (u&u_mask)))>>16;
         uchar c = bits[offset];
         if (c)
            pix_func(lpix8_func(i+err, c), x, y);
         u += du;
         v += dv;
         i += di;
         err = (err+i)&err_mask;
         x++;
      } while (x<xf);
   } else {
      do {
         uint offset = ((ulong )((v&v_mask) + (u&u_mask)))>>16;
         pix_func(lpix8_func(i+err, bits[offset]), x, y);
         u += du;
         v += dv;
         i += di;
         err = (err+i)&err_mask;
         x++;
      } while (x<xf);
   }
}

void gen_flat16_lit_il(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   int y;
   fix i, du, dv, di, err, err_mask;
   int row;
   ushort *bits;
   gdupix_func *pix_func;
   gdlpix_func *lpix16_func;
   uint offset_max;

   y = ti->y;
   if (x>=xf)
      return;

   bits = (ushort *)ti->bm->bits;
   row = ti->bm->row;
   offset_max = ti->bm->h * row - 1;
   du = ti->dux;
   dv = ti->dvx;
   di = ti->dix;
   i = ti->i;
   pix_func = ti->pix_func;
   lpix16_func = ti->lpix_func;
   err = 0;
   err_mask = (g2d_error_dither) ? (FIX_UNIT-1) : 0;

   if (ti->bm->flags&BMF_TRANS) {
      do {
         uint offset = fix_int(v)*row + fix_int(u);
         ushort c;
         if (offset>offset_max) offset = 0;
         c=bits[offset];
         if (c)
            pix_func(lpix16_func(i+err, c), x, y);
         u += du;
         v += dv;
         i += di;
         err = (err+i)&err_mask;
         x++;
      } while (x<xf);
   } else {
      do {
         uint offset = fix_int(v)*row + fix_int(u);
         ushort c;
         if (offset>offset_max) offset = 0;
         c=bits[offset];
         pix_func(lpix16_func(i+err, c), x, y);
         u += du;
         v += dv;
         i += di;
         err = (err+i)&err_mask;
         x++;
      } while (x<xf);
   }
}

void gen_flat16_lit_il_wrap(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   int y;
   fix i, du, dv, di, err, err_mask;
   fix u_mask, v_mask;
   int row;
   ushort *bits;
   gdupix_func *pix_func;
   gdlpix_func *lpix16_func;
   grs_bitmap *bm;

   y = ti->y;
   if (x>=xf)
      return;

   bm = ti->bm;
   y = ti->y;
   bits = (ushort *)bm->bits;
   row = bm->row;
   u_mask = fix_make(bm->w-1,0);
   v_mask = fix_make(bm->h-1,0)<<bm->wlog;
   v <<= bm->wlog;
   du = ti->dux;
   dv = ti->dvx<<bm->wlog;
   di = ti->dix;
   i = ti->i;
   pix_func = ti->pix_func;
   lpix16_func = ti->lpix_func;
   err = 0;
   err_mask = (g2d_error_dither) ? (FIX_UNIT-1) : 0;

   if (ti->bm->flags&BMF_TRANS) {
      do {
         uint offset = ((ulong )((v&v_mask) + (u&u_mask)))>>16;
         ushort c = bits[offset];
         if (c)
            pix_func(lpix16_func(i+err, c), x, y);
         u += du;
         v += dv;
         i += di;
         err = (err+i)&err_mask;
         x++;
      } while (x<xf);
   } else {
      do {
         uint offset = ((ulong )((v&v_mask) + (u&u_mask)))>>16;
         pix_func(lpix16_func(i+err, bits[offset]), x, y);
         u += du;
         v += dv;
         i += di;
         err = (err+i)&err_mask;
         x++;
      } while (x<xf);
   }
}
