/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8pix.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:48:54 $
 * 
 * Routines for drawing pixels into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 * $Log: bk8pix.c $
 * Revision 1.1  1996/04/10  15:48:54  KEVIN
 * Initial revision
 * 
 */

#include <pixftype.h>
#include <grd.h>
extern void (*bank8_upix8_func[])();

#pragma off(unreferenced)
gdupix_func *bank8_upix8_expose(int c, int x, int y)
{
   return (gdupix_func *)bank8_upix8_func[grd_gc.fill_type];
}
#pragma on(unreferenced)

void bank8_upix8(int c, int x, int y)
{
   (bank8_upix8_expose(c, x, y))(c, x, y);
}


