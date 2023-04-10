/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/genil.c $
 * $Revision: 1.6 $
 * $Author: KEVIN $
 * $Date: 1996/12/20 11:01:03 $
 * 
 * This file is part of the g2 library.
 *
 */

#include <tmapd.h>

void gen_flat8_il(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   int y;
   fix du, dv;
   int row;
   uchar *bits;
   gdupix_func *pix8_func;
   uint offset_max;

   y = ti->y;
   if (x>=xf)
      return;

   bits = ti->bm->bits;
   row = ti->bm->row;
   offset_max = ti->bm->h * row - 1;
   du = ti->dux;
   dv = ti->dvx;
   pix8_func = ti->pix_func;

   if (ti->bm->flags&BMF_TRANS) {
      do {
         uint offset = fix_int(v)*row + fix_int(u);
         uchar c;
         if (offset>offset_max) offset = 0;
         c=bits[offset];
         if (c)
            pix8_func(c, x, y);
         u += du;
         v += dv;
         x++;
      } while (x<xf);
   } else {
      do {
         uint offset = fix_int(v)*row + fix_int(u);
         uchar c;
         if (offset>offset_max) offset = 0;
         c=bits[offset];
         pix8_func(c, x, y);
         u += du;
         v += dv;
         x++;
      } while (x<xf);
   }
}

void gen_flat8_il_wrap(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   int y;
   fix du, dv;
   fix u_mask, v_mask;
   uchar *bits;
   gdupix_func *pix8_func;
   grs_bitmap *bm;

   if (x>=xf)
      return;

   bm = ti->bm;
   y = ti->y;
   bits = bm->bits;
   u_mask = fix_make(bm->w-1,0);
   v_mask = fix_make(bm->h-1,0)<<bm->wlog;
   v <<= bm->wlog;
   du = ti->dux;
   dv = ti->dvx<<bm->wlog;
   pix8_func = ti->pix_func;

   if (ti->bm->flags&BMF_TRANS) {
      do {
         uint offset = ((ulong )((v&v_mask) + (u&u_mask)))>>16;
         uchar c=bits[offset];
         if (c)
            pix8_func(c, x, y);
         u += du;
         v += dv;
         x++;
      } while (x<xf);
   } else {
      do {
         uint offset = ((ulong )((v&v_mask) + (u&u_mask)))>>16;
         pix8_func(bits[offset], x, y);
         u += du;
         v += dv;
         x++;
      } while (x<xf);
   }
}

void gen_flat16_il(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   int y;
   fix du, dv;
   int row;
   ushort *bits;
   gdupix_func *pix16_func;
   uint offset_max;

   y = ti->y;
   if (x>=xf)
      return;

   bits = (ushort *)ti->bm->bits;
   row = ti->bm->row>>1; // since bits are (ushort *) and row is num bytes
   offset_max = ti->bm->h * row - 1;
   du = ti->dux;
   dv = ti->dvx;
   pix16_func = ti->pix_func;

   if (ti->bm->flags&BMF_TRANS) {
      do {
         uint offset = fix_int(v)*row + fix_int(u);
         ushort c;
         if (offset>offset_max) offset = 0;
         c=bits[offset];
         if (c!=grd_chroma_key)
            pix16_func(c, x, y);
         u += du;
         v += dv;
         x++;
      } while (x<xf);
   } else {
      do {
         uint offset = fix_int(v)*row + fix_int(u);
         ushort c;
         if (offset>offset_max) offset = 0;
         c=bits[offset];
         pix16_func(c, x, y);
         u += du;
         v += dv;
         x++;
      } while (x<xf);
   }
}

void gen_flat16_il_wrap(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   int y;
   fix du, dv;
   fix u_mask, v_mask;
   ushort *bits;
   gdupix_func *pix16_func;
   grs_bitmap *bm;

   if (x>=xf)
      return;

   bm = ti->bm;
   y = ti->y;
   bits = (ushort *)bm->bits;
   u_mask = fix_make(bm->w-1,0);
   v_mask = fix_make(bm->h-1,0)<<bm->wlog;
   v <<= bm->wlog;
   du = ti->dux;
   dv = ti->dvx<<bm->wlog;
   pix16_func = ti->pix_func;

   if (ti->bm->flags&BMF_TRANS) {
      do {
         uint offset = ((ulong )((v&v_mask) + (u&u_mask)))>>16;
         ushort c=bits[offset];
         if (c!=grd_chroma_key)
            pix16_func(c, x, y);
         u += du;
         v += dv;
         x++;
      } while (x<xf);
   } else {
      do {
         uint offset = ((ulong )((v&v_mask) + (u&u_mask)))>>16;
         pix16_func(bits[offset], x, y);
         u += du;
         v += dv;
         x++;
      } while (x<xf);
   }
}
