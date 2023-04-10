/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/fl8il.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/12/20 11:00:38 $
 *
 * inner loops.
 *
 * This file is part of the 2d library.
 *
 */

#include <tmapd.h>

void flat8_flat8_norm_trans_il(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   fix du, dv;
   int row;
   uchar *bits;
   uchar *p, *p_last;
   uint offset_max;

   p = ti->p_dest + x;
   p_last = ti->p_dest + xf;
   ti->p_dest += grd_bm.row;
   if (p>=p_last)
      return;

   bits = ti->bm->bits;
   row = ti->bm->row;
   offset_max = ti->bm->h * row - 1;
   du = ti->dux;
   dv = ti->dvx;

   do {
      uint offset = fix_int(v)*row + fix_int(u);
      uchar c;
      if (offset>offset_max)
         offset = 0;
      c=bits[offset];
      if (c)
         *p = c;
      u += du;
      v += dv;
      p++;
   } while (p<p_last);
}

void flat8_flat8_clut_trans_il(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   fix du, dv;
   int row;
   uchar *bits;
   uchar *p, *p_last;
   uchar *clut;
   uint offset_max;

   p = ti->p_dest + x;
   p_last = ti->p_dest + xf;
   ti->p_dest += grd_bm.row;
   if (p>=p_last)
      return;

   clut = (uchar *)grd_gc.fill_parm;
   bits = ti->bm->bits;
   row = ti->bm->row;
   offset_max = ti->bm->h * row - 1;
   du = ti->dux;
   dv = ti->dvx;

   do {
      uint offset = fix_int(v)*row + fix_int(u);
      uchar c;
      if (offset>offset_max)
         offset = 0;
      c=bits[offset];
      if (c)
         *p = clut[c];
      u += du;
      v += dv;
      p++;
   } while (p<p_last);
}

void flat8_flat8_tluc_trans_il(int x, int xf, fix u, fix v)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   fix du, dv;
   int row;
   uchar *bits;
   uchar *p, *p_last;
   uint offset_max;

   p = ti->p_dest + x;
   p_last = ti->p_dest + xf;
   ti->p_dest += grd_bm.row;
   if (p>=p_last)
      return;

   bits = ti->bm->bits;
   row = ti->bm->row;
   offset_max = ti->bm->h * row - 1;
   du = ti->dux;
   dv = ti->dvx;

   do {
      uint offset = fix_int(v)*row + fix_int(u);
      uchar c;
      if (offset>offset_max)
         offset = 0;
      c=bits[offset];
      if (c) {
         uchar *clut=tluc8tab[c];
         if (clut==NULL)
            *p = c;
         else
            *p = clut[*p];
      }
      u += du;
      v += dv;
      p++;
   } while (p<p_last);
}
