/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/lftype.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:03:47 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the g2 library.
 *
 */

#ifndef __LFTYPE_H
#define __LFTYPE_H

#include <dev2d.h>
#include <plytyp.h>

typedef void (g2ul_func)(grs_vertex *v0, grs_vertex *v1);
typedef g2ul_func *g2ul_func_p;
typedef g2ul_func_p (g2ul_expose_func)(grs_vertex *v0, grs_vertex *v1);

typedef int (g2l_func)(grs_vertex *v0, grs_vertex *v1);
typedef g2l_func *g2l_func_p;
typedef g2l_func_p (g2l_expose_func)(grs_vertex *v0, grs_vertex *v1);

enum {
   G2C_CLIP_LINE,
   G2C_CLIP_SLINE,
   G2C_CLIP_CLINE,
   G2C_CLIP_LINE_TYPES
};
#endif

