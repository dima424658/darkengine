/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8pix.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:35:48 $
 * 
 * Routines for drawing pixels into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 * $Log: fl8pix.c $
 * Revision 1.1  1996/04/10  15:35:48  KEVIN
 * Initial revision
 * 
 */

#include <pixftype.h>
#include <grd.h>
extern void (*flat8_upix8_func[])();
extern void (*flat8_upix16_func[])();

int flat8_ugpix8(int x, int y)
{
   uchar *p = grd_bm.bits + x + y*grd_bm.row;
   return (*p);
}

#pragma off(unreferenced)
gdgpix_func *flat8_ugpix8_expose(int x, int y)
{
   return flat8_ugpix8;
}

gdupix_func *flat8_upix8_expose(int c, int x, int y)
{
   return (gdupix_func *)flat8_upix8_func[grd_gc.fill_type];
}

gdupix_func *flat8_upix16_expose(int c, int x, int y)
{
   return (gdupix_func *)flat8_upix16_func[grd_gc.fill_type];
}
#pragma on(unreferenced)

void flat8_upix8(int c, int x, int y)
{
   (flat8_upix8_expose(c, x, y))(c, x, y);
}

void flat8_upix16(int c, int x, int y)
{
   (flat8_upix16_expose(c, x, y))(c, x, y);
}

