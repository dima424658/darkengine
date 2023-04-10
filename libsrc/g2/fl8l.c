
/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/fl8l.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:27:43 $
 *
 * flat8 canvas routines for drawing fixed-point lines.
 *
 * This file is part of the g2 library.
 *
 */

#include <lftype.h>
extern g2ul_func *flat8_uline_func[];

#pragma off(unreferenced)
g2ul_func *g2_flat8_uline_expose (grs_vertex *v0, grs_vertex *v1)
{
   return flat8_uline_func[grd_gc.fill_type];
}
#pragma on(unreferenced)

void g2_flat8_uline (grs_vertex *v0, grs_vertex *v1)
{
   (g2_flat8_uline_expose(v0, v1))(v0, v1);
}
