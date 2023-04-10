/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxvlin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:47:20 $
 *
 * Declarations for globals.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>
#include <mxlatch.h>

void modex_uvline_fill(uchar c, int x, int y, int y1)
{
   uchar *p, *p_last;
   int row=grd_bm.row;
   uchar ws;

   x += grd_bm.align;
   p = grd_bm.bits + (x>>2);
   p_last = p + y1*row;

   wlatch_start(ws);
   modex_set_wlatch(1<<(x&3));
   for (p+=y*row; p<=p_last; p+=row)
      *p = c;
   wlatch_restore(ws);
}

void modex_norm_uvline(int x, int y, int y1)
{
   uchar c = grd_gc.fcolor;
   modex_uvline_fill(c, x, y, y1);
}

void modex_clut_uvline(int x, int y, int y1)
{
   uchar c = ((uchar *)grd_gc.fill_parm)[grd_gc.fcolor];
   modex_uvline_fill(c, x, y, y1);
}

void modex_solid_uvline(int x, int y, int y1)
{
   uchar c = grd_gc.fill_parm;
   modex_uvline_fill(c, x, y, y1);
}

void modex_xor_uvline(int x, int y, int y1)
{
   uchar *p, *p_last;
   int row=grd_bm.row;
   uchar c = grd_gc.fcolor;
   uchar ws;

   x += grd_bm.align;
   p = grd_bm.bits + (x>>2);
   p_last = p + y1*row;

   wlatch_start(ws);
   modex_set_wlatch(1<<(x&3));
   for (p+=y*row; p<=p_last; p+=row)
      *p = *p^c;
   wlatch_restore(ws);
}

void modex_tluc_uvline(int x, int y, int y1)
{
   uchar *p, *p_last;
   int row;
   uchar c;
   uchar *clut;
   uchar ws;

   c = grd_gc.fcolor;
   clut = tluc8tab[c];
   if (clut==NULL)
      modex_uvline_fill(c, x, y, y1);

   row = grd_bm.row;
   x += grd_bm.align;
   p = grd_bm.bits + (x>>2);
   p_last = p + row*y1;

   wlatch_start(ws);
   modex_set_wlatch(1<<(x&3));
   for (p+=row*y; p<=p_last; p+=row)
      *p = clut[*p];
   wlatch_restore(ws);
}
