/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/gdrect.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:37:46 $
 *
 * Global stuff.
 *
 * This file is part of the dev2d library.
 */

#ifndef __GDRECT_H
#define __GDRECT_H
#include <context.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void gd_urect(int x, int y, int x1, int y1);
extern int gd_rect(int x, int y, int x1, int y1);
extern void gd_ubox(int x, int y, int x1, int y1);
extern int gd_box(int x, int y, int x1, int y1);
extern void gd_clear(int c);

#ifdef __cplusplus
};
#endif
#endif
