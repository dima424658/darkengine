/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/grmalloc.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:40:10 $
 *
 * Declarations for internal memory allocation routines.
 *
 * This file is part of the dev2d library.
 */

#ifndef __GRMALLOC_H
#define __GRMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*grs_voidvoid)(void *);
extern void gr_set_malloc (void *(*malloc_func)(int bytes));
extern void gr_set_free (grs_voidvoid);
extern void *(*gr_malloc)(int n);
extern void (*gr_free)(void *p);

#ifdef __cplusplus
};
#endif
#endif /* !__GRMALLOC_H */
