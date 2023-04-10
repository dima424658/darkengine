/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8pix8.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:36:13 $
 * 
 * Routines for drawing pixels into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 * $Log: fl8pix8.c $
 * Revision 1.1  1996/04/10  15:36:13  KEVIN
 * Initial revision
 * 
 */

#include <grd.h>

/* draws an unclipped pixel of the given color at (x, y) on the canvas. */
void flat8_norm_upix8(int color, int x, int y)
{
   uchar *p;

   p = grd_bm.bits + grd_bm.row*y + x;
   *p = color;
}

void flat8_clut_upix8 (int color, int x, int y)
{
   uchar *p;

   p = grd_bm.bits + grd_bm.row*y + x;
   *p = ((uchar *)grd_gc.fill_parm)[color];
}

void flat8_xor_upix8 (int color, int x, int y)
{
   uchar *p;

   p = grd_bm.bits + grd_bm.row*y + x;
   *p = color ^ *p;
}

void flat8_tluc_upix8 (int color, int x, int y)
{
   uchar *p;
   uchar *clut=tluc8tab[color];

   p = grd_bm.bits + grd_bm.row*y + x;
   if (clut!=NULL) {
      *p = clut[*p];
   } else {
      *p = color;
   }
}

#pragma off (unreferenced)
void flat8_solid_upix8 (int color, int x, int y)
{
   uchar *p;

   p = grd_bm.bits + grd_bm.row*y + x;
   *p = (uchar )grd_gc.fill_parm;
}
#pragma on (unreferenced)
