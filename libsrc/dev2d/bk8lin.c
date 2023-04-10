/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8lin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:48:43 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <linftype.h>
#include <grd.h>
extern void (*bank8_uhline_func[])();
extern void (*bank8_uvline_func[])();

#pragma off(unreferenced)
gdulin_func *bank8_uhline_expose(int x, int y, int x1)
{
   return (gdulin_func *)bank8_uhline_func[grd_gc.fill_type];
}

gdulin_func *bank8_uvline_expose(int x, int y, int y1)
{
   return (gdulin_func *)bank8_uvline_func[grd_gc.fill_type];
}
#pragma on(unreferenced)

void bank8_uhline(int x, int y, int x1)
{
   (bank8_uhline_expose(x, y, x1))(x, y, x1);
}

void bank8_uvline(int x, int y, int y1)
{
   (bank8_uvline_expose(x, y, y1))(x, y, y1);
}
