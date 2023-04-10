/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxpix8.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:47:04 $
 * 
 * Routines for drawing pixels into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 * $Log: mxpix8.c $
 * Revision 1.1  1996/04/10  15:47:04  KEVIN
 * Initial revision
 * 
 */

#include <grd.h>
#include <mxlatch.h>

/* draws an unclipped pixel of the given color at (x, y) on the canvas. */
void modex_norm_upix8(int color, int x, int y)
{
   uchar *p;
   uchar ws;

   wlatch_start(ws);

   x += grd_bm.align;
   p = grd_bm.bits + grd_bm.row*y + (x>>2);
   modex_set_wlatch(1<<(x&3));
   *p = color;
   wlatch_restore(ws);
}

void modex_clut_upix8 (int color, int x, int y)
{
   uchar *p;
   uchar ws;

   wlatch_start(ws);

   x += grd_bm.align;
   p = grd_bm.bits + grd_bm.row*y + (x>>2);
   modex_set_wlatch(1<<(x&3));
   *p = ((uchar *)grd_gc.fill_parm)[color];
   wlatch_restore(ws);
}

void modex_xor_upix8 (int color, int x, int y)
{
   uchar *p;
   uchar ws,rs;

   wlatch_start(ws);
   rlatch_start(rs);

   x += grd_bm.align;
   p = grd_bm.bits + grd_bm.row*y + (x>>2);
   modex_set_rlatch(x&3);
   modex_set_wlatch(1<<(x&3));
   *p = color ^ *p;
   wlatch_restore(ws);
   rlatch_restore(rs);
}

void modex_tluc_upix8 (int color, int x, int y)
{
   uchar *clut=tluc8tab[color];
   uchar *p;
   uchar ws,rs;

   wlatch_start(ws);
   rlatch_start(rs);

   x += grd_bm.align;
   p = grd_bm.bits + grd_bm.row*y + (x>>2);
   modex_set_rlatch(x&3);
   modex_set_wlatch(1<<(x&3));
   if (clut!=NULL) {
      *p = clut[*p];
   } else {
      *p = color;
   }
   wlatch_restore(ws);
   rlatch_restore(rs);
}

#pragma off (unreferenced)
void modex_solid_upix8 (int color, int x, int y)
{
   uchar *p;
   uchar ws;

   wlatch_start(ws);

   x += grd_bm.align;
   p = grd_bm.bits + grd_bm.row*y + (x>>2);
   modex_set_wlatch(1<<(x&3));
   *p = (uchar )grd_gc.fill_parm;
   wlatch_restore(ws);
}
#pragma on (unreferenced)
