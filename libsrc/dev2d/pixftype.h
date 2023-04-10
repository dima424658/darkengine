/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/pixftype.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:59:50 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __PIXFTYPE_H
#define __PIXFTYPE_H
#include <fix.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (gdupix_func)(int c, int x, int y);
typedef gdupix_func *gdupix_func_p;
typedef gdupix_func_p (gdupix_expose_func)(int c, int x, int y);

typedef int (gdgpix_func)(int x, int y);
typedef gdgpix_func *gdgpix_func_p;
typedef gdgpix_func_p (gdgpix_expose_func)(int x, int y);

typedef int (gdlpix_func)(fix i, int c);
typedef gdlpix_func *gdlpix_func_p;
typedef gdlpix_func_p (gdlpix_expose_func)(fix i, int c);

#ifdef __cplusplus
};
#endif
#endif
