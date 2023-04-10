/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/f16lin.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1998/04/02 11:46:55 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <gdpix.h>
#include <linftype.h>
#include <grd.h>

extern void (*flat16_uhline_func[])();
extern void (*flat16_uvline_func[])();

#pragma off(unreferenced)
gdulin_func *flat16_uhline_expose(int x, int y, int x1)
{
   return (gdulin_func *)flat16_uhline_func[grd_gc.fill_type];
}

gdulin_func *flat16_uvline_expose(int x, int y, int y1)
{
   return (gdulin_func *)flat16_uvline_func[grd_gc.fill_type];
}
#pragma on(unreferenced)

void flat16_uhline(int x, int y, int x1)
{
   (flat16_uhline_expose(x, y, x1))(x, y, x1);
}

void flat16_uvline(int x, int y, int y1)
{
   (flat16_uvline_expose(x, y, y1))(x, y, y1);
}
