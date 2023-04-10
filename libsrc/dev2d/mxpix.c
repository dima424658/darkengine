/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxpix.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:46:50 $
 * 
 * Routines for drawing pixels into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 * $Log: mxpix.c $
 * Revision 1.1  1996/04/10  15:46:50  KEVIN
 * Initial revision
 * 
 */

#include <pixftype.h>
#include <grd.h>
extern void (*modex_upix8_func[])();

#pragma off(unreferenced)
gdupix_func *modex_upix8_expose(int c, int x, int y)
{
   return (gdupix_func *)modex_upix8_func[grd_gc.fill_type];
}
#pragma on(unreferenced)

void modex_upix8(int c, int x, int y)
{
   (modex_upix8_expose(c, x, y))(c, x, y);
}


