/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/genlin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:38:39 $
 *
 * Global stuff.
 *
 * This file is part of the dev2d library.
 */


#include <gdpix.h>

void gen_uhline(int x, int y, int x1)
{
   uchar c=grd_gc.fcolor;
   gdupix_func *upix8_func;

   upix8_func = gd_upix8_expose(c, x, y);
   for (;x<=x1; x++)
      upix8_func(c, x, y);
}

void gen_uvline(int x, int y, int y1)
{
   uchar c=grd_gc.fcolor;
   gdupix_func *upix8_func;

   upix8_func = gd_upix8_expose(c, x, y);
   for (;y<=y1; y++)
      upix8_func(c, x, y);
}
