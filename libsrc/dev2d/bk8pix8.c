/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8pix8.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:51:47 $
 * 
 * Routines for drawing pixels into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 * $Log: bk8pix8.c $
 * Revision 1.1  1996/04/10  15:51:47  KEVIN
 * Initial revision
 * 
 */

#include <bank.h>

/* draws an unclipped pixel of the given color at (x, y) on the canvas. */
void bank8_norm_upix8(int color, int x, int y)
{
   long p;
   int save_bank=gdd_bank;

   p = ((long )grd_bm.bits) + grd_bm.row*y + x;
   gd_set_bank(p>>16);
   *(gd_bank_p(p)) =
      color;
   gd_restore_bank(save_bank);
}

void bank8_clut_upix8 (int color, int x, int y)
{
   long p;
   int save_bank=gdd_bank;

   p = ((long )grd_bm.bits) + grd_bm.row*y + x;
   gd_set_bank(p>>16);
   *(gd_bank_p(p)) =
      ((uchar *)grd_gc.fill_parm)[color];
   gd_restore_bank(save_bank);
}

void bank8_xor_upix8 (int color, int x, int y)
{
   uchar *bank_p;
   long p;
   int save_bank=gdd_bank;

   p = ((long )grd_bm.bits) + grd_bm.row*y + x;
   gd_set_bank(p>>16);
   bank_p = gd_bank_p(p);
   *bank_p = color ^ *bank_p;
   gd_restore_bank(save_bank);
}

void bank8_tluc_upix8 (int color, int x, int y)
{
   uchar *clut=tluc8tab[color];
   uchar *bank_p;
   long p;
   int save_bank=gdd_bank;

   p = ((long )grd_bm.bits) + grd_bm.row*y + x;
   gd_set_bank(p>>16);
   bank_p = gd_bank_p(p);

   if (clut!=NULL) {
      *bank_p = clut[*bank_p];
   } else {
      *bank_p = color;
   }
   gd_restore_bank(save_bank);
}

#pragma off (unreferenced)
void bank8_solid_upix8 (int color, int x, int y)
{
   long p;
   int save_bank=gdd_bank;

   p = ((long )grd_bm.bits) + grd_bm.row*y + x;
   gd_set_bank(p>>16);
   *(gd_bank_p(p)) =
      (uchar )grd_gc.fill_parm;
   gd_restore_bank(save_bank);
}
#pragma on (unreferenced)
