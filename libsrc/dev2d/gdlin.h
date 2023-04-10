/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/gdlin.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:37:26 $
 *
 * Global stuff.
 *
 * This file is part of the dev2d library.
 */

#ifndef __GDLIN_H
#define __GDLIN_H

#include <linftype.h>
#include <grd.h>
#include <icanvas.h>

#ifdef __cplusplus
extern "C" {
#endif

#define gd_uhline \
   ((gdulin_func *)(gdd_canvas_table[GDC_UHLINE]))

#define gd_uvline \
   ((gdulin_func *)(gdd_canvas_table[GDC_UVLINE]))

#define gd_uhline_expose \
   ((gdulin_expose_func *)(gdd_canvas_table[GDC_UHLINE_EXPOSE]))

#define gd_uvline_expose \
   ((gdulin_expose_func *)(gdd_canvas_table[GDC_UVLINE_EXPOSE]))

extern int gd_hline(int x, int y, int x1);
extern int gd_hline_opt(int x, int y, int x1, gdulin_func *uhlin_func);
extern int gd_vline(int x, int y, int y1);
extern int gd_vline_opt(int x, int y, int y1, gdulin_func *uvlin_func);

#ifdef __cplusplus
};
#endif
#endif

