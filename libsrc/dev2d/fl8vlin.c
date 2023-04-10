/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8vlin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:36:53 $
 *
 * Declarations for globals.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>

void flat8_uvline_fill(uchar c, int x, int y, int y1)
{
   uchar *p, *p_last;
   int row=grd_bm.row;
   p = grd_bm.bits + x;
   p_last = p + y1*row;
   for (p+=y*row; p<=p_last; p+=row)
      *p = c;
}

void flat8_norm_uvline(int x, int y, int y1)
{
   uchar c = grd_gc.fcolor;
   flat8_uvline_fill(c, x, y, y1);
}

void flat8_clut_uvline(int x, int y, int y1)
{
   uchar c = ((uchar *)grd_gc.fill_parm)[grd_gc.fcolor];
   flat8_uvline_fill(c, x, y, y1);
}

void flat8_solid_uvline(int x, int y, int y1)
{
   uchar c = grd_gc.fill_parm;
   flat8_uvline_fill(c, x, y, y1);
}

void flat8_xor_uvline(int x, int y, int y1)
{
   uchar *p, *p_last;
   int row=grd_bm.row;
   uchar c = grd_gc.fcolor;

   p = grd_bm.bits +x;
   p_last = p + y1*row;
   for (p+=y*row; p<=p_last; p+=row)
      *p = *p^c;
}

void flat8_tluc_uvline(int x, int y, int y1)
{
   uchar *p, *p_last;
   int row;
   uchar c;
   uchar *clut;

   c = grd_gc.fcolor;
   clut = tluc8tab[c];
   if (clut==NULL)
      flat8_uvline_fill(c, x, y, y1);

   row = grd_bm.row;
   p = grd_bm.bits + x;
   p_last = p + row*y1;
   for (p+=row*y; p<=p_last; p+=row)
      *p = clut[*p];
}
