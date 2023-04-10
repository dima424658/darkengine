/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/fl8hlin.c $
 * $Revision: 1.2 $
 * $Author: JAEMZ $
 * $Date: 1997/05/07 11:39:12 $
 *
 * Declarations for globals.
 *
 * This file is part of the dev2d library.
 *
 */

#include <string.h>
#include <grd.h>

void flat8_uhline_fill(uchar c, int x, int y, int x1)
{
   uchar *p;
   p = grd_bm.bits + grd_bm.row*y + x;

   if (x1>=x)
      memset(p, c, x1-x+1);
}

void flat8_norm_uhline(int x, int y, int x1)
{
   uchar c = grd_gc.fcolor;
   flat8_uhline_fill(c, x, y, x1);
}

void flat8_clut_uhline(int x, int y, int x1)
{
   uchar c = ((uchar *)grd_gc.fill_parm)[grd_gc.fcolor];
   flat8_uhline_fill(c, x, y, x1);
}

void flat8_solid_uhline(int x, int y, int x1)
{
   uchar c = grd_gc.fill_parm;
   flat8_uhline_fill(c, x, y, x1);
}

void flat8_xor_uhline(int x, int y, int x1)
{
   uchar *p, *p_last;
   uchar c = grd_gc.fcolor;

   p = grd_bm.bits + grd_bm.row*y;
   p_last = p + x1;
   for (p+=x; p<=p_last; p++)
      *p = *p^c;
}

void flat8_tluc_uhline(int x, int y, int x1)
{
   uchar *p, *p_last;
   uchar c;
   uchar *clut;

   c = grd_gc.fcolor;
   clut = tluc8tab[c];
   if (clut==NULL) {
      flat8_uhline_fill(c, x, y, x1);
      return;
   }

   p = grd_bm.bits + grd_bm.row*y;
   p_last = p + x1;
   for (p+=x; p<=p_last; p++)
      *p = clut[*p];
}
