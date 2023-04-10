/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/f16pix8.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:45:01 $
 * 
 * Routines for drawing pixels into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 * $Log: f16pix8.c $
 * Revision 1.1  1996/04/10  15:45:01  KEVIN
 * Initial revision
 * 
 */

#include <grd.h>

/* draws an unclipped pixel of the given color at (x, y) on the canvas. */
void flat16_norm_upix8(int c8, int x, int y)
{
   ushort *p;

   p = (ushort *)(grd_bm.bits + grd_bm.row*y + 2*x);
   *p = ((ushort *)pixpal)[c8];
}

void flat16_clut_upix8 (int c8, int x, int y)
{
   ushort *p;
   ushort *clut16;
   int c16;

   p = (ushort *)(grd_bm.bits + grd_bm.row*y + 2*x);
   c16 = ((ushort *)pixpal)[c8];
   clut16 = (ushort *)(grd_gc.fill_parm);
   *p = clut16[c16&255] + clut16[256+(c16>>8)];
}

void flat16_xor_upix8 (int c8, int x, int y)
{
   ushort *p;
   ushort c16;

   p = (ushort *)(grd_bm.bits + grd_bm.row*y + 2*x);
   c16 = ((ushort *)pixpal)[c8];
   *p = c16 ^ *p;
}

void flat16_tluc_upix8 (int c8, int x, int y)
{
   ushort *p;
   ushort *clut16=(ushort *)(tluc8tab[c8]);
   int c16;

   p = (ushort *)(grd_bm.bits + grd_bm.row*y + 2*x);
   if (clut16!=NULL) {
      c16 = *p;
      *p = clut16[c16&255]+clut16[256+(c16>>8)];
   } else {
      *p = ((ushort *)pixpal)[c8];
   }
}

#pragma off (unreferenced)
void flat16_solid_upix8 (int c8, int x, int y)
{
   ushort *p;

   p = (ushort *)(grd_bm.bits + grd_bm.row*y + 2*x);
   *p = (ushort )grd_gc.fill_parm;
}
#pragma on (unreferenced)
