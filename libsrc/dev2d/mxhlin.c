/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxhlin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:45:54 $
 *
 * Mode X horizontal line routine.
 *
 * This file is part of the dev2d library.
 *
 */

#include <string.h>
#include <grd.h>
#include <mxlatch.h>

void modex_uhline_fill (uchar c, int x, int y, int x1)
{
   uchar *p;                /* pointer start of scanline */
   uchar m;                 /* write mask to use */
   int lb;                  /* left byte of span*/
   int rb;                  /* right byte of span */
   uchar ws;

   x+=grd_bm.align;
   x1+=grd_bm.align;

   /* get the address of scanline y, and index for left & right bytes. */
   p = grd_bm.bits + y*grd_bm.row;
   lb = x>>2;
   rb = x1>>2;

   /* if the whole span is in one byte, the pixels actually set are the
      intersection of the left and right spans. */

   wlatch_start(ws);
   if (lb == rb) {
      m = modex_lmask[x&3] & modex_rmask[x1&3];
      modex_set_wlatch (m);
      p[lb] = c;
      return;
   }

   /* draw left edge. */
   m = modex_lmask[x&3];
   modex_set_wlatch (m);
   p[lb++] = c;

   /* draw right edge. */
   m = modex_rmask[x1&3];
   modex_set_wlatch (m);
   p[rb] = c;

   /* only do this if there are actually 4-pixel wide spans between the
      left and right edges. */
   if (lb < rb) {
      modex_set_wlatch (0xf);
      memset (p+lb, c, rb-lb);
   }
   wlatch_restore(ws);
}

void modex_norm_uhline(int x, int y, int x1)
{
   uchar c = grd_gc.fcolor;
   modex_uhline_fill(c, x, y, x1);
}

void modex_clut_uhline(int x, int y, int x1)
{
   uchar c = ((uchar *)grd_gc.fill_parm)[grd_gc.fcolor];
   modex_uhline_fill(c, x, y, x1);
}

void modex_solid_uhline(int x, int y, int x1)
{
   uchar c = grd_gc.fill_parm;
   modex_uhline_fill(c, x, y, x1);
}
