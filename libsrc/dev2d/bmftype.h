/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bmftype.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:50:36 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __BMFTYPE_H
#define __BMFTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <grs.h>
typedef void (gdubm_func)(grs_bitmap *bm, int x, int y);
typedef gdubm_func *gdubm_func_p;
typedef gdubm_func_p (gdubm_expose_func)(grs_bitmap *bm, int x, int y);

typedef int (gdbm_func)(grs_bitmap *bm, int x, int y, int code, gdubm_func *ubm_func);

#ifdef __cplusplus
};
#endif
#endif
