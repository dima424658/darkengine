/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8vlin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:52:06 $
 *
 * Declarations for globals.
 *
 * This file is part of the dev2d library.
 *
 */

#include <bank.h>

void bank8_uvline_fill(uchar c, int x, int y, int y1)
{
   long p;
   int row=grd_bm.row;
   int save_bank=gdd_bank;
   uchar *base = grd_cap->vbase;

   p = ((long )grd_bm.bits) + y*row + x;
   gd_set_bank(p>>16);
   p &= 0xffff;
   while (y++<=y1) {
      if (p>0x10000) {
         p-=0x10000;
         gd_inc_bank();
      }
      base[p] = c;
      p += row;
   }
   gd_restore_bank(save_bank);
}

void bank8_norm_uvline(int x, int y, int y1)
{
   uchar c = grd_gc.fcolor;
   bank8_uvline_fill(c, x, y, y1);
}

void bank8_clut_uvline(int x, int y, int y1)
{
   uchar c = ((uchar *)grd_gc.fill_parm)[grd_gc.fcolor];
   bank8_uvline_fill(c, x, y, y1);
}

void bank8_solid_uvline(int x, int y, int y1)
{
   uchar c = grd_gc.fill_parm;
   bank8_uvline_fill(c, x, y, y1);
}

void bank8_xor_uvline(int x, int y, int y1)
{
   long p;
   int row = grd_bm.row;
   uchar c = grd_gc.fcolor;
   int save_bank = gdd_bank;
   uchar *base = grd_cap->vbase;

   p = ((long )grd_bm.bits) + x + y*row;
   gd_set_bank(p>>16);
   p &= 0xffff;
   while (y++<=y1) {
      if (p>0x10000) {
         p-=0x10000;
         gd_inc_bank();
      }
      base[p] = base[p]^c;
      p += row;
   }
   gd_restore_bank(save_bank);
}

void bank8_tluc_uvline(int x, int y, int y1)
{
   long p;
   int row;
   uchar c;
   uchar *clut, *base;
   int save_bank;

   c = grd_gc.fcolor;
   clut = tluc8tab[c];
   if (clut==NULL) {
      bank8_uvline_fill(c, x, y, y1);
      return;
   }

   save_bank = gdd_bank;
   base = grd_cap->vbase;
   row = grd_bm.row;
   p = ((long )grd_bm.bits) + x + y*row;
   gd_set_bank(p>>16);
   p &= 0xffff;
   while (y++<=y1) {
      if (p>0x10000) {
         p-=0x10000;
         gd_inc_bank();
      }
      base[p] = clut[base[p]];
      p += row;
   }
   gd_restore_bank(save_bank);
}
