/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/f16pix.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1997/04/01 14:52:05 $
 * 
 * Routines for drawing pixels into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 * $Log: f16pix.c $
 * Revision 1.2  1997/04/01  14:52:05  KEVIN
 * Added gpix16 funcs.
 * 
 * Revision 1.1  1996/04/10  15:44:16  KEVIN
 * Initial revision
 * 
 */

#include <pixftype.h>
#include <grd.h>
extern void (*flat16_upix8_func[])();
extern void (*flat16_upix16_func[])();

int flat16_ugpix8(int x, int y)
{
   ushort *p = (ushort *)(grd_bm.bits + 2*x + y*grd_bm.row);
   return grd_ipal[(*p)];
}

#pragma off(unreferenced)
gdgpix_func *flat16_ugpix8_expose(int x, int y)
{
   return flat16_ugpix8;
}

int flat16_ugpix16(int x, int y)
{
   return *((ushort *)(grd_bm.bits + 2*x + y*grd_bm.row));
}

#pragma off(unreferenced)
gdgpix_func *flat16_ugpix16_expose(int x, int y)
{
   return flat16_ugpix16;
}

gdupix_func *flat16_upix8_expose(int c, int x, int y)
{
   return (gdupix_func *)flat16_upix8_func[grd_gc.fill_type];
}

gdupix_func *flat16_upix16_expose(int c, int x, int y)
{
   return (gdupix_func *)flat16_upix16_func[grd_gc.fill_type];
}
#pragma on(unreferenced)

void flat16_upix8(int c, int x, int y)
{
   (flat16_upix8_expose(c, x, y))(c, x, y);
}

void flat16_upix16(int c, int x, int y)
{
   (flat16_upix16_expose(c, x, y))(c, x, y);
}

