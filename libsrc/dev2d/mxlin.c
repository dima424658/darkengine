/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxlin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:46:40 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <linftype.h>
#include <grd.h>
extern void (*modex_uhline_func[])();
extern void (*modex_uvline_func[])();

#pragma off(unreferenced)
gdulin_func *modex_uhline_expose(int x, int y, int x1)
{
   return (gdulin_func *)modex_uhline_func[grd_gc.fill_type];
}

gdulin_func *modex_uvline_expose(int x, int y, int y1)
{
   return (gdulin_func *)modex_uvline_func[grd_gc.fill_type];
}
#pragma on(unreferenced)

void modex_uhline(int x, int y, int x1)
{
   (modex_uhline_expose(x, y, x1))(x, y, x1);
}

void modex_uvline(int x, int y, int y1)
{
   (modex_uvline_expose(x, y, y1))(x, y, y1);
}
