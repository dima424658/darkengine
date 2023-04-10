/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8hlin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:48:23 $
 *
 * Declarations for globals.
 *
 * This file is part of the dev2d library.
 *
 */

#include <string.h>
#include <bank.h>

void bank8_uhline_fill(uchar c, int x, int y, int x1)
{
   long p, p_last;
   uchar *bank_p;
   int save_bank;

   if (x1<x)
      return;

   p = ((long )grd_bm.bits) + grd_bm.row*y;
   p_last = p + x1;
   p += x;
   save_bank = gdd_bank;
   gd_set_bank(p>>16);
   bank_p = gd_bank_p(p);
   if ((p>>16)==(p_last>>16)) {
      memset(bank_p, c, x1-x+1);
   } else {
      memset(bank_p, c, 0x10000 - (p&0xffff));
      gd_inc_bank();
      memset(gd_bank_p(0), c, (p_last&0xffff)+1);
   }
   gd_restore_bank(save_bank);
}

void bank8_norm_uhline(int x, int y, int x1)
{
   uchar c = grd_gc.fcolor;
   bank8_uhline_fill(c, x, y, x1);
}

void bank8_clut_uhline(int x, int y, int x1)
{
   uchar c = ((uchar *)grd_gc.fill_parm)[grd_gc.fcolor];
   bank8_uhline_fill(c, x, y, x1);
}

void bank8_solid_uhline(int x, int y, int x1)
{
   uchar c = grd_gc.fill_parm;
   bank8_uhline_fill(c, x, y, x1);
}

void bank8_xor_uhline(int x, int y, int x1)
{
   long p, p_last;
   uchar *bp, *bp_last;
   uchar c;
   int save_bank;

   if (x1<x)
      return;

   c=grd_gc.fcolor;
   p = ((long )grd_bm.bits) + grd_bm.row*y;
   p_last = p + x1;
   p += x;
   save_bank = gdd_bank;
   gd_set_bank(p>>16);
   bp = gd_bank_p(p);
   if ((p>>16)==(p_last>>16)) {
      bp_last = gd_bank_p(p_last);
      for (; bp<=bp_last; bp++)
         *bp = *bp^c;
   } else {
      bp_last = gd_bank_p(0xffff);
      for (; bp<=bp_last; bp++)
         *bp = *bp^c;
      bp_last = gd_bank_p(p_last);
      gd_inc_bank();
      for (bp=gd_bank_p(0); bp<=bp_last; bp++)
         *bp = *bp^c;
   }
   gd_restore_bank(save_bank);
}

void bank8_tluc_uhline(int x, int y, int x1)
{
   uchar *clut;
   long p, p_last;
   uchar *bp, *bp_last;
   uchar c;
   int save_bank;

   if (x1<x)
      return;

   c = grd_gc.fcolor;
   clut = tluc8tab[c];
   if (clut==NULL) {
      bank8_uhline_fill(c, x, y, x1);
      return;
   }

   p = ((long )grd_bm.bits) + grd_bm.row*y;
   p_last = p + x1;
   p += x;
   save_bank = gdd_bank;
   gd_set_bank(p>>16);
   bp = gd_bank_p(p);
   if ((p>>16)==(p_last>>16)) {
      uchar *bp_last = gd_bank_p(p_last);
      for (; bp<=bp_last; bp++)
         *bp = clut[*bp];
   } else {
      bp_last = gd_bank_p(0xffff);
      for (; bp<=bp_last; bp++)
         *bp = clut[*bp];
      bp_last = gd_bank_p(p_last);
      gd_inc_bank();
      for (bp=gd_bank_p(0); bp<=bp_last; bp++)
         *bp = clut[*bp];
   }
   gd_restore_bank(save_bank);
}
