/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/fl8lin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:26:08 $
 *
 * Routines for drawing fixed-point lines onto a flat 8 canvas.
 *
 * This file is part of the g2 library.
 *
 */

#include <dev2d.h>
#include <mathmac.h>
#include <plytyp.h>

/* same for norm, solid and clut */

#undef  flat8_pixel_fill
#define flat8_pixel_fill(_p, _c) \
do { \
   *_p = _c; \
} while (0)

void flat8_uline (uchar c, grs_vertex *v0, grs_vertex *v1)
{
   #include <fl8lin.h>
}

/* norm */
void flat8_uline_norm (grs_vertex *v0, grs_vertex *v1)
{
   flat8_uline(grd_gc.fcolor, v0, v1);
}

/* solid */

void flat8_uline_solid (grs_vertex *v0, grs_vertex *v1)
{
   flat8_uline (grd_gc.fill_parm, v0, v1);
}

/* clut */

void flat8_uline_clut (grs_vertex *v0, grs_vertex *v1)
{
   uchar c = (uchar )(((uchar *)grd_gc.fill_parm)[grd_gc.fcolor]);
   flat8_uline (c, v0, v1);
}

/* xor */

#undef  flat8_pixel_fill
#define flat8_pixel_fill(_p, _c) \
do { \
   *_p = _c ^ *_p; \
} while (0)

void flat8_uline_xor (grs_vertex *v0, grs_vertex *v1)
{
   uchar c=grd_gc.fcolor;
   #include <fl8lin.h>
}

/* blend */

#undef  flat8_pixel_fill
#define flat8_pixel_fill(_p, _c) \
do { \
   *_p = clut[*_p]; \
} while (0)

void flat8_uline_blend (grs_vertex *v0, grs_vertex *v1)
{
   uchar c = grd_gc.fcolor;
   uchar *clut = tluc8tab[c];
   if (clut == NULL) {
      flat8_uline (c, v0, v1);
   } else {
      #include <fl8lin.h>
   }
}



