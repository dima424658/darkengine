/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/plytyp.h $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1997/10/01 13:37:00 $
 *
 * Polygon-related structures.
 *
 * This file is part of the g2 library.
 *
 */

#ifndef __PLYTYP
#define __PLYTYP
#include <fix.h>

/* format for vertex buffers. */
typedef struct grs_vertex {
   fix x, y;                  /* screen coordinates */
   fix u, v, w;               /* texture parameters/rgb */
   fix i;                     /* intensity */
} grs_vertex;
#endif /* !__PLYTYP */
