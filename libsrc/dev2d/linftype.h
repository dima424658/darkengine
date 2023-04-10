/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/linftype.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:59:01 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __LINFTYPE_H
#define __LINFTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (gdulin_func)(int x, int y, int c1);
typedef gdulin_func *gdulin_func_p;
typedef gdulin_func_p (gdulin_expose_func)(int x, int y, int c1);

#ifdef __cplusplus
};
#endif
#endif
